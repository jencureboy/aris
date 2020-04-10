﻿#include <string>
#include <iostream>
#include <map>
#include <fstream>
#include <algorithm>
#include <mutex>
#include <thread>
#include <future>

#include "aris/control/rt_timer.hpp"
#include "aris/control/master_slave.hpp"

#include "aris/core/core.hpp"

namespace aris::control
{
	struct Slave::Imp { std::uint16_t phy_id_, sla_id_; };
	auto Slave::saveXml(aris::core::XmlElement &xml_ele) const->void
	{
		Object::saveXml(xml_ele);
		xml_ele.SetAttribute("phy_id", std::to_string(phyId()).c_str());
	}
	auto Slave::loadXml(const aris::core::XmlElement &xml_ele)->void
	{
		Object::loadXml(xml_ele);
		imp_->phy_id_ = attributeUint16(xml_ele, "phy_id");
	}
	auto Slave::phyId()const->std::uint16_t { return imp_->phy_id_; }
	auto Slave::setPhyId(std::uint16_t phy_id)->void { imp_->phy_id_ = phy_id; }
	Slave::~Slave() = default;
	Slave::Slave(const std::string &name, std::uint16_t phy_id) :Object(name), imp_(new Imp) { imp_->phy_id_ = phy_id; }
	ARIS_DEFINE_BIG_FOUR_CPP(Slave);

	struct Master::Imp
	{
	public:
		enum { LOG_NEW_FILE = 1, LOG_NEW_FILE_RAW_NAME = 2 };
		static auto rt_task_func(void *master)->void
		{
			auto &mst = *reinterpret_cast<Master*>(master);
			auto add_time_to_stastics = [](std::int64_t time, RtStasticsData *data) 
			{
				data->avg_time_consumed += (double(time) - data->avg_time_consumed) / (data->total_count + 1);
				data->max_time_occur_count = time < data->max_time_consumed ? data->max_time_occur_count : data->total_count;
				data->max_time_consumed = std::max(time, data->max_time_consumed);
				data->min_time_occur_count = time > data->min_time_consumed ? data->min_time_occur_count : data->total_count;
				data->min_time_consumed = std::min(time, data->min_time_consumed);
				data->total_count++;
				data->overrun_count += time > 900000 ? 1 : 0;
			};

			aris_rt_task_set_periodic(mst.imp_->sample_period_ns_);

			while (mst.imp_->is_rt_thread_running_)
			{
				// rt timer //
				aris_rt_task_wait_period();
				
				// receive //
				mst.recv();

				// tragectory generator //
				if (mst.imp_->strategy_)mst.imp_->strategy_();

				// send
				mst.send();

				// flush lout
				mst.lout() << std::flush;
				if (!mst.imp_->lout_msg_.empty())
				{
					mst.imp_->lout_pipe_->sendMsg(mst.imp_->lout_msg_);
					mst.lout().reset();
				}

				// flush mout
				mst.mout() << std::flush;
				if (!mst.imp_->mout_msg_.empty())
				{
					mst.imp_->mout_pipe_->sendMsg(mst.imp_->mout_msg_);
					mst.mout().reset();
				}

				// record stastics //
				auto time = aris_rt_time_since_last_time();
				add_time_to_stastics(time, &mst.imp_->global_stastics_);
				if (mst.imp_->this_stastics_)add_time_to_stastics(time, mst.imp_->this_stastics_);
				if (mst.imp_->is_need_change_)mst.imp_->this_stastics_ = mst.imp_->next_stastics_;
			}
		}

		// slave //
		aris::core::ObjectPool<Slave> *slave_pool_;
		std::vector<Size> sla_vec_phy2abs_;

		// for mout and lout //
		aris::core::Pipe *mout_pipe_, *lout_pipe_;
		aris::core::MsgFix<MAX_MSG_SIZE> mout_msg_, lout_msg_;
		std::unique_ptr<aris::core::MsgStream> mout_msg_stream_, lout_msg_stream_;
		std::thread mout_thread_;

		// strategy //
		std::function<void()> strategy_{ nullptr };

		// running flag //
		std::atomic_bool is_rt_thread_running_{ false };
		std::atomic_bool is_mout_thread_running_{ false };

		int sample_period_ns_{ 1000000 };

		// rt stastics //
		Master::RtStasticsData global_stastics_{ 0,0,0,0x8fffffff,0,0,0 };
		Master::RtStasticsData* this_stastics_{ nullptr }, *next_stastics_{ nullptr };
		bool is_need_change_{ false };

		std::any rt_task_handle_;

		Imp() { mout_msg_stream_.reset(new aris::core::MsgStream(mout_msg_)); lout_msg_stream_.reset(new aris::core::MsgStream(lout_msg_)); }

		friend class Slave;
		friend class Master;
	};
	auto Master::saveXml(aris::core::XmlElement &xml_ele) const->void
	{
		Object::saveXml(xml_ele);
		xml_ele.SetAttribute("sample_period_ns", imp_->sample_period_ns_);
	}
	auto Master::loadXml(const aris::core::XmlElement &xml_ele)->void
	{
		Object::loadXml(xml_ele);

		// attribute //
		imp_->sample_period_ns_ = attributeInt32(xml_ele, "sample_period_ns", 1'000'000);

		// children //
		imp_->slave_pool_ = findByName("slave_pool") == children().end() ? &add<aris::core::ObjectPool<Slave, Object> >("slave_pool") : static_cast<aris::core::ObjectPool<Slave, Object> *>(&(*findByName("slave_pool")));
		imp_->mout_pipe_ = findOrInsert<aris::core::Pipe>("mout_pipe");
		imp_->lout_pipe_ = findOrInsert<aris::core::Pipe>("lout_pipe");
	}
	auto Master::init()->void
	{
		// make vec_phy2abs //
		imp_->sla_vec_phy2abs_.clear();
		for (auto &sla : slavePool())
		{
			imp_->sla_vec_phy2abs_.resize(std::max(static_cast<aris::Size>(sla.phyId() + 1), imp_->sla_vec_phy2abs_.size()), -1);
			if (imp_->sla_vec_phy2abs_.at(sla.phyId()) != -1) THROW_FILE_LINE("invalid Master::Slave phy id:\"" + std::to_string(sla.phyId()) + "\" of slave \"" + sla.name() + "\" already exists");
			imp_->sla_vec_phy2abs_.at(sla.phyId()) = sla.id();
		}
	}
	auto Master::start()->void
	{
		if (imp_->is_rt_thread_running_)THROW_FILE_LINE("master already running, so cannot start");
		imp_->is_rt_thread_running_ = true;

		struct RaiiCollector
		{
			Master *self_;
			auto reset()->void { self_ = nullptr; }
			RaiiCollector(Master *self) :self_(self) {}
			~RaiiCollector()
			{
				if (self_)
				{
					self_->imp_->is_rt_thread_running_ = false;
					aris_rt_task_join(self_->rtHandle());
					self_->imp_->is_mout_thread_running_ = false;
					if (self_->imp_->mout_thread_.joinable())self_->imp_->mout_thread_.join();
				}
			}
		};
		RaiiCollector raii_collector(this);

		// lock memory // 
		aris_mlockall();

		// create mout & lout thread //
		imp_->is_mout_thread_running_ = true;
		imp_->mout_thread_ = std::thread([this]()
		{
			// prepare lout //
			auto file_name = aris::core::logDirPath() / ("rt_log--" + aris::core::logFileTimeFormat(std::chrono::system_clock::now()) + "--");
			std::fstream file;
			file.open(file_name.string() + "0.txt", std::ios::out | std::ios::trunc);

			// start read mout and lout //
			aris::core::Msg msg;
			while (imp_->is_mout_thread_running_)
			{
				if (imp_->lout_pipe_->recvMsg(msg))
				{
					if (msg.msgID() == Imp::LOG_NEW_FILE)
					{
						file.close();
						file.open(file_name.string() + msg.toString() + ".txt", std::ios::out | std::ios::trunc);
					}
					else if (msg.msgID() == Imp::LOG_NEW_FILE_RAW_NAME)
					{
						auto raw_name = aris::core::logDirPath() / (msg.toString() + ".txt");
						file.close();
						file.open(raw_name, std::ios::out | std::ios::trunc);
					}
					else if (!msg.empty())
					{
						file << msg.toString();
					}
				}
				else if (imp_->mout_pipe_->recvMsg(msg))
				{
					if (!msg.empty())aris::core::cout() << msg.toString() << std::flush;
				}
				else
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
			}

			// 结束前最后一次接收，此时实时线程已经结束 //
			while (imp_->mout_pipe_->recvMsg(msg)) if (!msg.empty())std::cout << msg.toString() << std::endl;
			while (imp_->lout_pipe_->recvMsg(msg))
			{
				if (msg.msgID() == Imp::LOG_NEW_FILE)
				{
					file.close();
					file.open(file_name.string() + msg.toString() + ".txt", std::ios::out | std::ios::trunc);
				}
				else if (!msg.empty())
				{
					file << msg.toString();
				}
			}
		});

		// create and start rt task //
		imp_->rt_task_handle_ = aris_rt_task_create();
		if (!imp_->rt_task_handle_.has_value()) THROW_FILE_LINE("rt_task_create failed");
		if (aris_rt_task_start(rtHandle(), &Imp::rt_task_func, this))THROW_FILE_LINE("rt_task_start failed");

		raii_collector.reset();
	}
	auto Master::stop()->void
	{
		if (!imp_->is_rt_thread_running_)THROW_FILE_LINE("master is not running, so can't stop");
		
		// join rt task //
		imp_->is_rt_thread_running_ = false;
		if (aris_rt_task_join(rtHandle()))THROW_FILE_LINE("aris_rt_task_join failed");

		// join mout task //
		imp_->is_mout_thread_running_ = false;
		imp_->mout_thread_.join();

		// release child resources //
		release();
	}
	auto Master::setControlStrategy(std::function<void()> strategy)->void
	{
		if (imp_->is_rt_thread_running_)THROW_FILE_LINE("master already running, cannot set control strategy");
		imp_->strategy_ = strategy;
	}
	auto Master::rtHandle()->std::any& { return imp_->rt_task_handle_; }
	auto Master::logFile(const char *file_name)->void
	{
		// 将已有的log数据发送过去 //
		if (!imp_->lout_msg_.empty())
		{
			// 补充一个0作为结尾 //
			lout() << std::flush;
			imp_->lout_pipe_->sendMsg(imp_->lout_msg_);
			imp_->lout_msg_.resize(0);
		}

		// 发送切换文件的msg //
		imp_->lout_msg_.setMsgID(Imp::LOG_NEW_FILE);
		imp_->lout_msg_.copy(file_name);
		imp_->lout_pipe_->sendMsg(imp_->lout_msg_);

		// 将msg变更回去
		imp_->lout_msg_.setMsgID(0);
		imp_->lout_msg_.resize(0);
	}
	auto Master::logFileRawName(const char *file_name)->void
	{
		// 将已有的log数据发送过去 //
		if (!imp_->lout_msg_.empty())
		{
			// 补充一个0作为结尾 //
			lout() << std::flush;
			imp_->lout_pipe_->sendMsg(imp_->lout_msg_);
			imp_->lout_msg_.resize(0);
		}

		// 发送切换文件的msg //
		imp_->lout_msg_.setMsgID(Imp::LOG_NEW_FILE_RAW_NAME);
		imp_->lout_msg_.copy(file_name);
		imp_->lout_pipe_->sendMsg(imp_->lout_msg_);

		// 将msg变更回去
		imp_->lout_msg_.setMsgID(0);
		imp_->lout_msg_.resize(0);
	}
	auto Master::lout()->aris::core::MsgStream & { return *imp_->lout_msg_stream_; }
	auto Master::mout()->aris::core::MsgStream & { return *imp_->mout_msg_stream_; }
	auto Master::slaveAtPhy(aris::Size id)->Slave& { return slavePool().at(imp_->sla_vec_phy2abs_.at(id)); }
	auto Master::slavePool()->aris::core::ObjectPool<Slave, aris::core::Object>& { return *imp_->slave_pool_; }
	auto Master::resetRtStasticData(RtStasticsData *stastics, bool is_new_data_include_this_count)->void
	{
		if (stastics)*stastics = RtStasticsData{ 0,0,0,0x8fffffff,0,0,0 };
		
		if (is_new_data_include_this_count)
		{
			// make new stastics //
			imp_->this_stastics_ = stastics;
			imp_->is_need_change_ = !is_new_data_include_this_count;
		}
		else
		{
			// mark need to change when this count finished //
			imp_->next_stastics_ = stastics;
			imp_->is_need_change_ = !is_new_data_include_this_count;
		}
	}
	auto Master::setSamplePeriodNs(int period_ns)->void {	imp_->sample_period_ns_ = period_ns;}
	auto Master::samplePeriodNs()const ->int { return imp_->sample_period_ns_; }
	Master::~Master() = default;
	Master::Master(const std::string &name) :imp_(new Imp), Object(name)
	{
		this->registerType<aris::core::ObjectPool<Slave, aris::core::Object> >();
		
		imp_->slave_pool_ = &add<aris::core::ObjectPool<Slave> >("slave_pool");
		imp_->mout_pipe_ = &add<aris::core::Pipe>("mout_pipe");
		imp_->lout_pipe_ = &add<aris::core::Pipe>("lout_pipe");
	}
}
