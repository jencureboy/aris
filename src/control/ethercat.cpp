﻿#include <mutex>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <atomic>
#include <memory>
#include <typeinfo>
#include <thread>
#include <chrono>
#include <future>
#include <iomanip>

#include "aris/core/core.hpp"

#include "aris/control/rt_timer.hpp"
#include "aris/control/ethercat_kernel.hpp"
#include "aris/control/ethercat.hpp"

namespace aris::control
{
	struct PdoEntry::Imp
	{ 
		std::any ec_handle_; 
		std::uint16_t index_;
		std::uint8_t subindex_;
		aris::Size bit_size_;
	};
	auto PdoEntry::saveXml(aris::core::XmlElement &xml_ele) const->void 
	{ 
		Object::saveXml(xml_ele);

		std::stringstream s;
		s << "0x" << std::setfill('0') << std::setw(sizeof(std::int16_t) * 2) << std::hex << static_cast<std::uint32_t>(index());
		xml_ele.SetAttribute("index", s.str().c_str());

		s = std::stringstream();
		s << "0x" << std::setfill('0') << std::setw(sizeof(std::int8_t) * 2) << std::hex << static_cast<std::uint32_t>(subindex());
		xml_ele.SetAttribute("subindex", s.str().c_str());

		xml_ele.SetAttribute("size", static_cast<std::int32_t>(bitSize()));
	}
	auto PdoEntry::loadXml(const aris::core::XmlElement &xml_ele)->void 
	{ 
		imp_->index_ = attributeUint16(xml_ele, "index");
		imp_->subindex_ = attributeUint8(xml_ele, "subindex");
		imp_->bit_size_ = attributeUint32(xml_ele, "size");
		
		Object::loadXml(xml_ele);
	}
	auto PdoEntry::ecHandle()->std::any& { return imp_->ec_handle_; }
	auto PdoEntry::index()const->std::uint16_t { return imp_->index_; }
	auto PdoEntry::subindex()const->std::uint8_t { return imp_->subindex_; }
	auto PdoEntry::bitSize()const->aris::Size { return imp_->bit_size_; }
	PdoEntry::~PdoEntry() = default;
	PdoEntry::PdoEntry(const std::string &name, std::uint16_t index, std::uint8_t sub_index, aris::Size bit_size) :Object(name) 
	{
		imp_->index_ = index;
		imp_->subindex_ = sub_index;
		imp_->bit_size_ = bit_size;
	}
	ARIS_DEFINE_BIG_FOUR_CPP(PdoEntry)

	struct Pdo::Imp
	{
		std::any handle_;
		std::uint16_t index_;
	};
	auto Pdo::saveXml(aris::core::XmlElement &xml_ele) const->void
	{
		aris::core::Object::saveXml(xml_ele);

		std::stringstream s;
		s << "0x" << std::setfill('0') << std::setw(sizeof(std::int16_t) * 2) << std::hex << static_cast<std::uint32_t>(index());
		xml_ele.SetAttribute("index", s.str().c_str());
	}
	auto Pdo::loadXml(const aris::core::XmlElement &xml_ele)->void
	{
		imp_->index_ = attributeUint16(xml_ele, "index");

		aris::core::Object::loadXml(xml_ele);
	}
	auto Pdo::ecHandle()->std::any& { return imp_->handle_; }
	auto Pdo::index()const->std::uint16_t { return imp_->index_; }
	Pdo::~Pdo() = default;
	Pdo::Pdo(const std::string &name, std::uint16_t index) :aris::core::ObjectPool<PdoEntry>(name), imp_(new Imp){ imp_->index_ = index; }
	ARIS_DEFINE_BIG_FOUR_CPP(Pdo)

	struct SyncManager::Imp { std::any handle_; bool is_tx_; };
	auto SyncManager::saveXml(aris::core::XmlElement &xml_ele) const->void
	{
		aris::core::ObjectPool<Pdo>::saveXml(xml_ele);
		xml_ele.SetAttribute("is_tx", tx());
	}
	auto SyncManager::loadXml(const aris::core::XmlElement &xml_ele)->void
	{
		imp_->is_tx_ = attributeBool(xml_ele, "is_tx");
		aris::core::ObjectPool<Pdo>::loadXml(xml_ele);
	}
	auto SyncManager::tx()const->bool { return imp_->is_tx_; }
	auto SyncManager::rx()const->bool { return !imp_->is_tx_; }
	SyncManager::~SyncManager()=default;
	SyncManager::SyncManager(const std::string &name, bool is_tx):ObjectPool(name), imp_(new Imp) {	imp_->is_tx_ = is_tx;}
	ARIS_DEFINE_BIG_FOUR_CPP(SyncManager)

	struct EthercatSlave::Imp
	{
	public:
		std::any ec_handle_;

		std::uint32_t vendor_id_, product_code_, revision_num_, dc_assign_activate_;
		std::int32_t sync0_shift_ns_;
		aris::core::ObjectPool<SyncManager> *sm_pool_;
		std::map<std::uint16_t, std::map<std::uint8_t, PdoEntry* > > pdo_map_;
	};
	auto EthercatSlave::saveXml(aris::core::XmlElement &xml_ele) const->void
	{
		Slave::saveXml(xml_ele);

		std::stringstream s;
		s << "0x" << std::setfill('0') << std::setw(sizeof(vendorID()) * 2) << std::hex << vendorID();
		xml_ele.SetAttribute("vendor_id", s.str().c_str());

		s = std::stringstream();
		s << "0x" << std::setfill('0') << std::setw(sizeof(productCode()) * 2) << std::hex << productCode();
		xml_ele.SetAttribute("product_code", s.str().c_str());

		s = std::stringstream();
		s << "0x" << std::setfill('0') << std::setw(sizeof(revisionNum()) * 2) << std::hex << revisionNum();
		xml_ele.SetAttribute("revision_num", s.str().c_str());

		s = std::stringstream();
		s << "0x" << std::setfill('0') << std::setw(sizeof(dcAssignActivate()) * 2) << std::hex << dcAssignActivate();
		xml_ele.SetAttribute("dc_assign_activate", s.str().c_str());

		xml_ele.SetAttribute("sync0_shift_ns", imp_->sync0_shift_ns_);
	}
	auto EthercatSlave::loadXml(const aris::core::XmlElement &xml_ele)->void
	{
		imp_->vendor_id_ = attributeUint32(xml_ele, "vendor_id");
		imp_->product_code_ = attributeUint32(xml_ele, "product_code");
		imp_->revision_num_ = attributeUint32(xml_ele, "revision_num");
		imp_->dc_assign_activate_ = attributeUint32(xml_ele, "dc_assign_activate");
		imp_->sync0_shift_ns_ = attributeInt32(xml_ele, "sync0_shift_ns", 650'000);

		Slave::loadXml(xml_ele);
		imp_->sm_pool_ = findOrInsertType<aris::core::ObjectPool<SyncManager> >();
	}
	auto EthercatSlave::ecHandle()->std::any& { return imp_->ec_handle_; }
	auto EthercatSlave::smPool()->aris::core::ObjectPool<SyncManager>& { return *imp_->sm_pool_; }
	auto EthercatSlave::vendorID()const->std::uint32_t { return imp_->vendor_id_; }
	auto EthercatSlave::setVendorID(std::uint32_t vendor_id)->void { imp_->vendor_id_ = vendor_id; }
	auto EthercatSlave::productCode()const->std::uint32_t { return imp_->product_code_; }
	auto EthercatSlave::setProductCode(std::uint32_t product_code)->void { imp_->product_code_ = product_code; }
	auto EthercatSlave::revisionNum()const->std::uint32_t { return imp_->revision_num_; }
	auto EthercatSlave::setRevisionNum(std::uint32_t revision_num)->void { imp_->revision_num_ = revision_num; }
	auto EthercatSlave::dcAssignActivate()const->std::uint32_t { return imp_->dc_assign_activate_; }
	auto EthercatSlave::setDcAssignActivate(std::uint32_t dc_assign_activate)->void { imp_->dc_assign_activate_ = dc_assign_activate; }
	auto EthercatSlave::sync0ShiftNs()const->std::int32_t { return imp_->sync0_shift_ns_; }
	auto EthercatSlave::setSync0ShiftNs(std::int32_t sync0_shift_ns)->void { imp_->sync0_shift_ns_ = sync0_shift_ns; }
	auto EthercatSlave::scanInfoForCurrentSlave()->void
	{
		aris::control::EthercatMaster mst;
		mst.scan();
		if (mst.slavePool().size() <= this->phyId()) THROW_FILE_LINE("ec scan failed");

		this->setProductCode(dynamic_cast<EthercatSlave&>(mst.slavePool().at(this->phyId())).productCode());
		this->setRevisionNum(dynamic_cast<EthercatSlave&>(mst.slavePool().at(this->phyId())).revisionNum());
		this->setVendorID(dynamic_cast<EthercatSlave&>(mst.slavePool().at(this->phyId())).vendorID());
		this->setDcAssignActivate(dynamic_cast<EthercatSlave&>(mst.slavePool().at(this->phyId())).dcAssignActivate());
	}
	auto EthercatSlave::scanPdoForCurrentSlave()->void
	{
		aris::control::EthercatMaster mst;
		mst.scan();
		if (mst.slavePool().size() <= this->phyId()) THROW_FILE_LINE("ec scan failed");

		this->smPool() = dynamic_cast<EthercatSlave&>(mst.slavePool().at(this->phyId())).smPool();
	}
	auto EthercatSlave::readPdo(std::uint16_t index, std::uint8_t subindex, void *value, aris::Size bit_size)const->void
	{
		if (auto found_pdo = imp_->pdo_map_.find(index); found_pdo == imp_->pdo_map_.end())
		{
			THROW_FILE_LINE("pdo not found:" + std::to_string(index));
		}
		else
		{
			if (auto found_entry = found_pdo->second.find(subindex); found_entry == found_pdo->second.end())
			{
				THROW_FILE_LINE("pdo entry not found:" + std::to_string(index) + ":" + std::to_string(subindex));
			}
			else
			{
				if (found_entry->second->bitSize() != bit_size)THROW_FILE_LINE("pdo entry size not equal:\"" + std::to_string(index) + ":" + std::to_string(subindex));
				aris_ecrt_pdo_read(found_entry->second, value);
			}
		}
	}
	auto EthercatSlave::writePdo(std::uint16_t index, std::uint8_t subindex, const void *value, aris::Size bit_size)->void
	{
		if (auto found_pdo = imp_->pdo_map_.find(index); found_pdo == imp_->pdo_map_.end())
		{
			THROW_FILE_LINE("pdo not found:" + std::to_string(index));
		}
		else
		{
			if (auto found_entry = found_pdo->second.find(subindex); found_entry == found_pdo->second.end())
			{
				THROW_FILE_LINE("pdo entry not found:" + std::to_string(index) + ":" + std::to_string(subindex));
			}
			else
			{
				if (found_entry->second->bitSize() != bit_size)THROW_FILE_LINE("pdo entry size not equal:\"" + std::to_string(index) + ":" + std::to_string(subindex));
				aris_ecrt_pdo_write(found_entry->second, value);
			}
		}
	}
	auto EthercatSlave::readSdo(std::uint16_t index, std::uint8_t subindex, void *value, aris::Size byte_size)->void
	{
		std::size_t result_size;
		std::uint32_t abort_code;
		aris_ecrt_sdo_read(ancestor<EthercatMaster>()->ecHandle(), phyId(), index, subindex, reinterpret_cast<std::uint8_t*>(value), byte_size, &result_size, &abort_code);
	}
	auto EthercatSlave::writeSdo(std::uint16_t index, std::uint8_t subindex, const void *value, aris::Size byte_size)->void
	{
		std::uint32_t abort_code;
		aris_ecrt_sdo_write(ancestor<EthercatMaster>()->ecHandle(), phyId(), index, subindex, const_cast<std::uint8_t*>(reinterpret_cast<const std::uint8_t*>(value)), byte_size, &abort_code);
	}
	EthercatSlave::~EthercatSlave() = default;
	EthercatSlave::EthercatSlave(const std::string &name, std::uint16_t phy_id, std::uint32_t vid, std::uint32_t p_code, std::uint32_t r_num, std::uint32_t dc, std::int32_t sync0_shift_ns) :Slave(name, phy_id), imp_(new Imp)
	{
		aris::core::Object::registerTypeGlobal<aris::core::ObjectPool<SyncManager> >();
		
		imp_->sm_pool_ = &add<aris::core::ObjectPool<SyncManager> >("sm_pool");
		imp_->vendor_id_ = vid;
		imp_->product_code_ = p_code;
		imp_->revision_num_ = r_num;
		imp_->dc_assign_activate_ = dc;
		imp_->sync0_shift_ns_ = sync0_shift_ns;
	}
	EthercatSlave::EthercatSlave(const EthercatSlave &other) :Slave(other), imp_(other.imp_)
	{
		imp_->sm_pool_ = findType<aris::core::ObjectPool<SyncManager> >();
	};
	EthercatSlave& EthercatSlave::operator=(const EthercatSlave &other)
	{
		Slave::operator=(other);
		imp_ = other.imp_;
		imp_->sm_pool_ = findType<aris::core::ObjectPool<SyncManager> >();
		return *this;
	}

	struct EthercatMaster::Imp 
	{ 
		std::any ec_handle_;
		aris::core::ChildRefPool<EthercatSlave, aris::core::ObjectPool<Slave>> slave_pool_{nullptr};

		std::vector<std::filesystem::path> esi_dirs_;
		std::map<std::uint32_t, std::tuple<std::string, std::map<std::uint32_t, std::tuple<std::string, std::map<int, std::filesystem::path>>>>> vendor_device_revision_map_;
	};
	auto EthercatMaster::saveXml(aris::core::XmlElement &xml_ele) const->void
	{
		Master::saveXml(xml_ele);

		std::stringstream s;
		for (auto &dir : imp_->esi_dirs_)s << dir.string() << "|";
		xml_ele.SetAttribute("esi_dirs", s.str().c_str());
	}
	auto EthercatMaster::loadXml(const aris::core::XmlElement &xml_ele)->void
	{
		Master::loadXml(xml_ele);

		imp_->esi_dirs_.clear();
		if (auto str = xml_ele.Attribute("esi_dirs")) 
		{
			std::string esi_str(str);
			std::istringstream f(esi_str);
			for (std::string s; std::getline(f, s, '|');)imp_->esi_dirs_.push_back(s);
		}
	}
	auto EthercatMaster::scan()->void { aris_ecrt_scan(this); }
	auto EthercatMaster::scanInfoForCurrentSlaves()->void
	{
		for (auto &slave : slavePool())
		{
			slave.scanInfoForCurrentSlave();
		}
	}
	auto EthercatMaster::scanPdoForCurrentSlaves()->void
	{
		for (auto &slave : slavePool())
		{
			slave.scanPdoForCurrentSlave();
		}
	}
	auto EthercatMaster::init()->void
	{
		// make pdo map for each slave //
		for (auto &sla : slavePool())
		{
			// make PDO map //
			sla.imp_->pdo_map_.clear();
			for (auto &sm : sla.smPool())
			{
				for (auto &pdo : sm)
				{
					for (auto &entry : pdo)
					{
						if (entry.index())
						{
							if (sla.imp_->pdo_map_.find(entry.index()) != sla.imp_->pdo_map_.end())
							{
								sla.imp_->pdo_map_.at(entry.index()).insert(std::make_pair(entry.subindex(), &entry));
							}
							else
							{
								std::map<std::uint8_t, PdoEntry*> subindex_map;
								subindex_map.insert(std::make_pair(entry.subindex(), &entry));
								sla.imp_->pdo_map_.insert(std::make_pair(entry.index(), subindex_map));
							}
						}
					}
				}
			}
		}

		aris_ecrt_master_request(this);
	}
	auto EthercatMaster::release()->void { aris_ecrt_master_stop(this); }
	auto EthercatMaster::send()->void { aris_ecrt_master_send(this); }
	auto EthercatMaster::recv()->void { aris_ecrt_master_recv(this); }
	auto EthercatMaster::slavePool()->aris::core::ChildRefPool<EthercatSlave, aris::core::ObjectPool<Slave>>&
	{
		imp_->slave_pool_ = aris::core::ChildRefPool<EthercatSlave, aris::core::ObjectPool<Slave>>(&Master::slavePool());
		return imp_->slave_pool_;
	}
	auto EthercatMaster::getLinkState(MasterLinkState *ms, SlaveLinkState *ss)->void { aris_ecrt_master_link_state(this, ms, ss); }
	auto EthercatMaster::ecHandle()->std::any& { return imp_->ec_handle_; }
	auto EthercatMaster::setEsiDirs(std::vector<std::filesystem::path> esi_dirs)->void{	imp_->esi_dirs_ = esi_dirs; }
	auto EthercatMaster::updateDeviceList()->void
	{
		imp_->vendor_device_revision_map_.clear();

		auto getUInt32 = [](std::string str)->std::uint32_t
		{
			std::replace(str.begin(), str.end(), '#', '0');
			return std::stoul(str, 0, 16);
		};

		for (const auto &dir : imp_->esi_dirs_)
		{
			std::error_code err_code;
			if (!std::filesystem::is_directory(dir, err_code)) THROW_FILE_LINE("Esi directory not found:" + dir.string());
			
			for (auto &p : std::filesystem::directory_iterator(dir))
			{
				// not regular file//
				if (!p.is_regular_file())continue;

				// not xml file //
				aris::core::XmlDocument esi_doc;
				if (esi_doc.LoadFile(p.path().string().c_str()) != 0)THROW_FILE_LINE(std::string("could not open file:") + p.path().string());

				// vendor info not found //
				if ((!esi_doc.RootElement()) || (!esi_doc.RootElement()->FirstChildElement("Vendor")))continue;
				auto vendor_name = std::string(esi_doc.RootElement()->FirstChildElement("Vendor")->FirstChildElement("Name")->GetText());
				auto vendor_id = getUInt32(esi_doc.RootElement()->FirstChildElement("Vendor")->FirstChildElement("Id")->GetText());
				std::get<0>(imp_->vendor_device_revision_map_[vendor_id]) = vendor_name;

				// devices not found //
				if (auto description = esi_doc.RootElement()->FirstChildElement("Descriptions"); (!description) || (!description->FirstChildElement("Devices")))continue;
				auto esi_devices = esi_doc.RootElement()->FirstChildElement("Descriptions")->FirstChildElement("Devices");
				for (auto device = esi_devices->FirstChildElement(); device; device = device->NextSiblingElement())
				{
					// invisible //
					if (auto visible = device->Attribute("Invisible"); visible && (std::string("true") == visible || std::string("1") == visible)) continue;

					// product code not found //
					if (auto type = device->FirstChildElement("Type"); (!type) || (!type->Attribute("ProductCode")))continue;
					auto product_code = getUInt32(device->FirstChildElement("Type")->Attribute("ProductCode"));
					auto revision_no = device->FirstChildElement("Type")->Attribute("RevisionNo") ? getUInt32(device->FirstChildElement("Type")->Attribute("RevisionNo")) : 0x00;

					// set to map //
					std::get<0>(std::get<1>(imp_->vendor_device_revision_map_[vendor_id])[product_code]) = device->FirstChildElement("Type")->GetText();
					std::get<1>(std::get<1>(imp_->vendor_device_revision_map_[vendor_id])[product_code])[revision_no] = p.path();
				}
			}
		}
	}
	auto EthercatMaster::getDeviceList()->std::string
	{
		aris::core::XmlDocument xml_doc;
		auto root_xml_ele = xml_doc.NewElement(name().c_str());
		xml_doc.InsertEndChild(root_xml_ele);
		
		for (auto &vendor : imp_->vendor_device_revision_map_)
		{
			auto vendor_ele = (aris::core::XmlElement*)root_xml_ele->InsertEndChild(xml_doc.NewElement("Vendor"));
			
			std::stringstream s;
			s << "0x" << std::setfill('0') << std::setw(sizeof(std::int32_t) * 2) << std::hex << static_cast<std::uint32_t>(vendor.first);
			vendor_ele->SetAttribute("Id", s.str().c_str());
			vendor_ele->SetAttribute("Name", std::get<0>(vendor.second).c_str());

			for (auto &device : std::get<1>(vendor.second))
			{
				for (auto &revision : std::get<1>(device.second))
				{
					auto device_ele = (aris::core::XmlElement*)vendor_ele->InsertEndChild(xml_doc.NewElement("Device"));
					
					device_ele->SetAttribute("Name", std::get<0>(device.second).c_str());

					std::stringstream s;
					s << "0x" << std::setfill('0') << std::setw(sizeof(std::int32_t) * 2) << std::hex << static_cast<std::uint32_t>(device.first);
					device_ele->SetAttribute("ProductCode", s.str().c_str());

					s = std::stringstream();
					s << "0x" << std::setfill('0') << std::setw(sizeof(std::int32_t) * 2) << std::hex << static_cast<std::uint32_t>(revision.first);
					device_ele->SetAttribute("RevisionNo", s.str().c_str());
				}
			}
		}

		tinyxml2::XMLPrinter printer;
		xml_doc.Print(&printer);
		return std::string(printer.CStr());
	}
	auto EthercatMaster::getPdoList(int vendor_id, int product_code, int revision_no)->std::string
	{
		auto getUInt32 = [](std::string str)->std::uint32_t
		{
			std::replace(str.begin(), str.end(), '#', '0');
			return std::stol(str, 0, 16);
		};

		auto &slave_path = std::get<1>(std::get<1>(imp_->vendor_device_revision_map_.at(vendor_id)).at(product_code)).at(revision_no);

		EthercatSlave slave;

		aris::core::XmlDocument esi_doc;
		if (esi_doc.LoadFile(slave_path.string().c_str()) != 0)THROW_FILE_LINE(std::string("could not open file:") + slave_path.string());
		auto esi_devices = esi_doc.RootElement()->FirstChildElement("Descriptions")->FirstChildElement("Devices");
		for (auto device = esi_devices->FirstChildElement(); device; device = device->NextSiblingElement())
		{
			auto product_code_ = getUInt32(device->FirstChildElement("Type")->Attribute("ProductCode"));
			auto revision_no_ = device->FirstChildElement("Type")->Attribute("RevisionNo") ? getUInt32(device->FirstChildElement("Type")->Attribute("RevisionNo")) : 0x00;

			if (product_code != product_code_ || revision_no != revision_no_) continue;

			slave.smPool().clear();
			slave.setName(device->FirstChildElement("Name")->GetText());
			slave.setVendorID(vendor_id);
			slave.setProductCode(product_code);
			slave.setRevisionNum(revision_no);

			auto last_sm_id = 0;
			for (auto ele = device->FirstChildElement(); ele; ele = ele->NextSiblingElement())
			{
				if (std::string(ele->Name()) == "Sm" && std::string(ele->GetText()) == "Inputs")
				{
					slave.smPool().add<SyncManager>("", true);
				}
				else if (std::string(ele->Name()) == "Sm" && std::string(ele->GetText()) == "MBoxIn")
				{
					slave.smPool().add<SyncManager>("", true);
				}
				else if (std::string(ele->Name()) == "Sm" && std::string(ele->GetText()) == "Outputs")
				{
					slave.smPool().add<SyncManager>("", false);
				}
				else if (std::string(ele->Name()) == "Sm" && std::string(ele->GetText()) == "MBoxOut")
				{
					slave.smPool().add<SyncManager>("", false);
				}
				else if (std::string(ele->Name()) == "RxPdo" || std::string(ele->Name()) == "TxPdo")
				{
					auto sm_id = ele->Attribute("Sm") ? Object::attributeInt32(*ele, "Sm") : last_sm_id;
					last_sm_id = sm_id;

					auto pdo_name = ele->FirstChildElement("Name") ? ele->FirstChildElement("Name")->GetText() : "";
					auto pdo_index = ele->FirstChildElement("Index") ? getUInt32(ele->FirstChildElement("Index")->GetText()) : std::uint32_t(0);

					auto &pdo = slave.smPool()[sm_id].add<Pdo>(pdo_name, pdo_index);
					for (auto entry_ele = ele->FirstChildElement("Entry"); entry_ele; entry_ele = entry_ele->NextSiblingElement())
					{
						auto index = getUInt32(entry_ele->FirstChildElement("Index")->GetText());
						auto sub_index = entry_ele->FirstChildElement("SubIndex") ? getUInt32(entry_ele->FirstChildElement("SubIndex")->GetText()) : 0;
						auto bit_length = std::stol(entry_ele->FirstChildElement("BitLen")->GetText());
						auto name = (entry_ele->FirstChildElement("Name") && entry_ele->FirstChildElement("Name")->GetText()) ? entry_ele->FirstChildElement("Name")->GetText() : "";
						pdo.add<PdoEntry>(name, index, sub_index, bit_length);
					}
				}
			}

			return slave.xmlString();
		}

		THROW_FILE_LINE("device not found");
	}
	EthercatMaster::~EthercatMaster() = default;
	EthercatMaster::EthercatMaster(const std::string &name) :Master(name), imp_(new Imp){}

	class EthercatMotor::Imp
	{
	public:
		std::uint16_t control_word;
		std::uint8_t mode_of_operation;
		double target_pos_{ 0 }, target_vel_{ 0 }, target_toq_{ 0 }, offset_vel_{ 0 }, offset_toq_{ 0 };
		
		int waiting_count_left{ 0 }; // enable 在用
		
		// home 在用 //
		int home_count{ 0 };
		bool need_clear{ true };
	};
	auto EthercatMotor::saveXml(aris::core::XmlElement &xml_ele) const->void
	{
		EthercatSlave::saveXml(xml_ele);
		Motor::saveXml(xml_ele);
	}
	auto EthercatMotor::loadXml(const aris::core::XmlElement &xml_ele)->void
	{
		Motor::loadXml(xml_ele);
		EthercatSlave::loadXml(xml_ele);
	}
	auto EthercatMotor::controlWord()const->std::uint16_t { return imp_->control_word; }
	auto EthercatMotor::modeOfOperation()const->std::uint8_t { return imp_->mode_of_operation; }
	auto EthercatMotor::targetPos()const->double { return imp_->target_pos_; }
	auto EthercatMotor::targetVel()const->double { return imp_->target_vel_; }
	auto EthercatMotor::targetToq()const->double { return imp_->target_toq_; }
	auto EthercatMotor::offsetVel()const->double { return imp_->offset_vel_; }
	auto EthercatMotor::offsetCur()const->double { return imp_->offset_toq_; }
	auto EthercatMotor::setControlWord(std::uint16_t control_word)->void
	{
		imp_->control_word = control_word;
		writePdo(0x6040, 0x00, control_word);
	}
	auto EthercatMotor::setModeOfOperation(std::uint8_t mode)->void
	{
		imp_->mode_of_operation = mode;
		writePdo(0x6060, 0x00, mode);
	}
	auto EthercatMotor::setTargetPos(double pos)->void
	{
		imp_->target_pos_ = pos;
		writePdo(0x607A, 0x00, static_cast<std::int32_t>((pos + posOffset()) * posFactor()));
	}
	auto EthercatMotor::setTargetVel(double vel)->void
	{
		imp_->target_vel_ = vel;
		writePdo(0x60FF, 0x00, static_cast<std::int32_t>(vel * posFactor()));
	}
	auto EthercatMotor::setTargetToq(double toq)->void
	{
		imp_->target_toq_ = toq;
		writePdo(0x6071, 0x00, static_cast<std::int16_t>(toq));
	}
	auto EthercatMotor::setOffsetVel(double vel)->void
	{
		imp_->offset_vel_ = vel;
		writePdo(0x60B1, 0x00, static_cast<std::int32_t>(vel * posFactor()));
	}
	auto EthercatMotor::setOffsetToq(double cur)->void
	{
		imp_->offset_toq_ = cur;
		writePdo(0x60B2, 0x00, static_cast<std::int16_t>(cur));
	}
	auto EthercatMotor::statusWord()const->std::uint16_t
	{
		std::uint16_t status_word;
		readPdo(0x6041, 0x00, status_word);
		return status_word;
	}
	auto EthercatMotor::modeOfDisplay()const->std::uint8_t
	{
		std::uint8_t mode;
		readPdo(0x6061, 0x00, mode);
		return mode;
	}
	auto EthercatMotor::actualPos()const->double
	{
		std::int32_t pos_count{ 0 };
		readPdo(0x6064, 0x00, pos_count);
		return static_cast<double>(pos_count) / posFactor() - posOffset();
	}
	auto EthercatMotor::actualVel()const->double
	{
		std::int32_t vel_count{ 0 };
		readPdo(0x606C, 0x00, vel_count);
		return static_cast<double>(vel_count) / posFactor();
	}
	auto EthercatMotor::actualToq()const->double
	{
		std::int16_t cur_count{ 0 };
		readPdo(0x6077, 0x00, cur_count);
		return static_cast<double>(cur_count);
	}
	auto EthercatMotor::actualCur()const->double
	{
		std::int16_t cur_count{ 0 };
		readPdo(0x6078, 0x00, cur_count);
		return static_cast<double>(cur_count);
	}
	auto EthercatMotor::disable()->int
	{
		// control word
		// 0x06    0b xxxx xxxx 0xxx 0110    A: transition 2,6,8         Shutdown
		// 0x07    0b xxxx xxxx 0xxx 0111    B: transition 3             Switch ON
		// 0x0F    0b xxxx xxxx 0xxx 1111    C: transition 3             Switch ON
		// 0x00    0b xxxx xxxx 0xxx 0000    D: transition 7,9,10,12     Disable Voltage
		// 0x02    0b xxxx xxxx 0xxx 0000    E: transition 7,10,11       Quick Stop
		// 0x07    0b xxxx xxxx 0xxx 0111    F: transition 5             Disable Operation
		// 0x0F    0b xxxx xxxx 0xxx 1111    G: transition 4,16          Enable Operation
		// 0x80    0b xxxx xxxx 1xxx xxxx    H: transition 15            Fault Reset
		//
		// status word
		// 0x00    0b xxxx xxxx x0xx 0000    A: not ready to switch on     
		// 0x40    0b xxxx xxxx x1xx 0000    B: switch on disabled         
		// 0x21    0b xxxx xxxx x01x 0001    C: ready to switch on         
		// 0x23    0b xxxx xxxx x01x 0011    D: switch on                  
		// 0x27    0b xxxx xxxx x01x 0111    E: operation enabled          
		// 0x07    0b xxxx xxxx x00x 0111    F: quick stop active
		// 0x0F    0b xxxx xxxx x0xx 1111    G: fault reaction active
		// 0x08    0b xxxx xxxx x0xx 1000    H: fault
		//
		// 0x6F    0b 0000 0000 0110 1111
		// 0x4F    0b 0000 0000 0100 1111
		// disable change state to A/B/C/E to D

		std::uint16_t status_word;
		readPdo(0x6041, 0x00, status_word);

		// check status A, now transition 1 automatically
		if ((status_word & 0x4F) == 0x00)
		{
			// this just set the initial control word...
			writePdo(0x6040, 0x00, std::uint16_t(0x00));
			return 1;
		}
		// check status B, now keep and return
		else if ((status_word & 0x4F) == 0x40)
		{
			// transition 2 //
			writePdo(0x6040, 0x00, std::uint16_t(0x00));
			return 0;
		}
		// check status C, now transition 7
		else if ((status_word & 0x6F) == 0x21)
		{
			// transition 3 //
			writePdo(0x6040, 0x00, std::uint16_t(0x00));
			return 2;
		}
		// check status D, now transition 10
		else if ((status_word & 0x6F) == 0x23)
		{
			writePdo(0x6040, 0x00, std::uint16_t(0x00));
			return 3;
		}
		// check status E, now transition 9
		else if ((status_word & 0x6F) == 0x27)
		{
			// transition 5 //
			writePdo(0x6040, 0x00, std::uint16_t(0x00));
			return 4;
		}
		// check status F, now transition 12
		else if ((status_word & 0x6F) == 0x07)
		{
			writePdo(0x6040, 0x00, std::uint16_t(0x00));
			return 5;
		}
		// check status G, now transition 14
		else if ((status_word & 0x4F) == 0x0F)
		{
			writePdo(0x6040, 0x00, std::uint16_t(0x00));
			return 6;
		}
		// check status H, now transition 15
		else if ((status_word & 0x4F) == 0x08)
		{
			// transition 4 //
			writePdo(0x6040, 0x00, std::uint16_t(0x80));
			return 7;
		}
		// unknown status
		else
		{
			return -1;
		}
	}
	auto EthercatMotor::enable()->int
	{
		// control word
		// 0x06    0b xxxx xxxx 0xxx 0110    A: transition 2,6,8       Shutdown
		// 0x07    0b xxxx xxxx 0xxx 0111    B: transition 3           Switch ON
		// 0x0F    0b xxxx xxxx 0xxx 1111    C: transition 3           Switch ON
		// 0x00    0b xxxx xxxx 0xxx 0000    D: transition 7,9,10,12   Disable Voltage
		// 0x02    0b xxxx xxxx 0xxx 0000    E: transition 7,10,11     Quick Stop
		// 0x07    0b xxxx xxxx 0xxx 0111    F: transition 5           Disable Operation
		// 0x0F    0b xxxx xxxx 0xxx 1111    G: transition 4,16        Enable Operation
		// 0x80    0b xxxx xxxx 1xxx xxxx    H: transition 15          Fault Reset
		// 
		// status word
		// 0x00    0b xxxx xxxx x0xx 0000    A: not ready to switch on     
		// 0x40    0b xxxx xxxx x1xx 0000    B: switch on disabled         
		// 0x21    0b xxxx xxxx x01x 0001    C: ready to switch on         
		// 0x23    0b xxxx xxxx x01x 0011    D: switch on                  
		// 0x27    0b xxxx xxxx x01x 0111    E: operation enabled          
		// 0x07    0b xxxx xxxx x00x 0111    F: quick stop active
		// 0x0F    0b xxxx xxxx x0xx 1111    G: fault reaction active
		// 0x08    0b xxxx xxxx x0xx 1000    H: fault
		// 
		// 0x6F    0b 0000 0000 0110 1111
		// 0x4F    0b 0000 0000 0100 1111
		// enable change state to A/B/C/D/F/G/H to E
		std::uint16_t status_word;
		readPdo(0x6041, 0x00, status_word);

		// check status A
		if ((status_word & 0x4F) == 0x00)
		{
			return 1;
		}
		// check status B, now transition 2
		else if ((status_word & 0x4F) == 0x40)
		{
			// transition 2 //
			writePdo(0x6040, 0x00, std::uint16_t(0x06));
			return 2;
		}
		// check status C, now transition 3
		else if ((status_word & 0x6F) == 0x21)
		{
			// transition 3 //
			writePdo(0x6040, 0x00, std::uint16_t(0x07));
			return 3;
		}
		// check status D, now transition 4
		else if ((status_word & 0x6F) == 0x23)
		{
			// transition 4 //
			writePdo(0x6040, 0x00, std::uint16_t(0x0F));
			imp_->waiting_count_left = 20;

			// check mode to set correct pos, vel or cur //
			switch (modeOfDisplay())
			{
			case 0x08: setTargetPos(actualPos()); break;
			case 0x09: setTargetVel(0.0); break;
			case 0x10: setTargetToq(0.0); break;
			default: setTargetPos(actualPos()); setTargetVel(0.0); setTargetToq(0.0);
			}

			return 4;
		}
		// check status E, now keep status
		else if ((status_word & 0x6F) == 0x27)
		{
			// check if need wait //
			if (--imp_->waiting_count_left > 0) return 5;
			// now return normal
			else return 0;
		}
		// check status F, now transition 12
		else if ((status_word & 0x6F) == 0x07)
		{
			writePdo(0x6040, 0x00, std::uint16_t(0x00));
			return 6;
		}
		// check status G, now transition 14
		else if ((status_word & 0x4F) == 0x0F)
		{
			writePdo(0x6040, 0x00, std::uint16_t(0x00));
			return 7;
		}
		// check status H, now transition 15
		else if ((status_word & 0x4F) == 0x08)
		{
			// transition 4 //
			writePdo(0x6040, 0x00, std::uint16_t(0x80));
			return 8;
		}
		// unknown status
		else
		{
			return -1;
		}
	}
	auto EthercatMotor::home()->int
	{
		// control word
		// 0x06    0b xxxx xxxx 0xxx 0110    A: transition 2,6,8       Shutdown
		// 0x07    0b xxxx xxxx 0xxx 0111    B: transition 3           Switch ON
		// 0x0F    0b xxxx xxxx 0xxx 1111    C: transition 3           Switch ON
		// 0x00    0b xxxx xxxx 0xxx 0000    D: transition 7,9,10,12   Disable Voltage
		// 0x02    0b xxxx xxxx 0xxx 0000    E: transition 7,10,11     Quick Stop
		// 0x07    0b xxxx xxxx 0xxx 0111    F: transition 5           Disable Operation
		// 0x0F    0b xxxx xxxx 0xxx 1111    G: transition 4,16        Enable Operation
		// 0x80    0b xxxx xxxx 1xxx xxxx    H: transition 15          Fault Reset
		// 
		// status word
		// 0x00    0b xxxx xxxx x0xx 0000    A: not ready to switch on     
		// 0x40    0b xxxx xxxx x1xx 0000    B: switch on disabled         
		// 0x21    0b xxxx xxxx x01x 0001    C: ready to switch on         
		// 0x23    0b xxxx xxxx x01x 0011    D: switch on                  
		// 0x27    0b xxxx xxxx x01x 0111    E: operation enabled          
		// 0x07    0b xxxx xxxx x00x 0111    F: quick stop active
		// 0x0F    0b xxxx xxxx x0xx 1111    G: fault reaction active
		// 0x08    0b xxxx xxxx x0xx 1000    H: fault
		// 
		// 0x6F    0b 0000 0000 0110 1111
		// 0x4F    0b 0000 0000 0100 1111
		// enable change state to A/B/C/D to E

		
		// 查看此前是否已经使能（status ：operation enabled） //
		if (imp_->need_clear && ((statusWord() & 0x6F) != 0x27))
		{
			imp_->home_count = 0;
			imp_->need_clear = true;
			return -1;
		}
		// 更改mode //
		else if (modeOfDisplay() != 0x06)
		{
			setModeOfOperation(0x06);
			return 2;
		}
		// 将home已经到达的标志位去掉 //
		else if (imp_->need_clear)
		{
			if (statusWord() & 0x3400)
			{
				if (++imp_->home_count % 20 < 10)
				{
					setControlWord(0x1F);
				}
				else
				{
					setControlWord(0x0F);
				}
			}
			else
			{
				setControlWord(0x0F);
				imp_->need_clear = false;
			}

			return 3;
		}
		// 开始执行home //
		else if ((statusWord() & 0x3400) == 0x0000)
		{
			setControlWord(0x1F);
			return 4;
		}
		// home attained //
		else if ((statusWord() & 0x3400) == 0x1400)
		{
			setControlWord(0x0F);
			imp_->home_count = 0;
			imp_->need_clear = true;
			return 0;
		}
		// home error //
		else if (statusWord() & 0x2000)
		{
			imp_->home_count = 0;
			imp_->need_clear = true;
			return -2;
		}
		// homing ... //
		else
		{
			return -3;
		}
	}
	auto EthercatMotor::mode(std::uint8_t md)->int
	{
		setModeOfOperation(md);
		return md == modeOfDisplay() ? 0 : 1;
	}
	EthercatMotor::EthercatMotor(const std::string &name, std::uint16_t phy_id, std::uint32_t vendor_id, std::uint32_t product_code, std::uint32_t revision_num, std::uint32_t dc_assign_activate
		, double max_pos, double min_pos, double max_vel, double min_vel, double max_acc, double min_acc, double max_pos_following_error, double max_vel_following_error, double pos_factor, double pos_offset, double home_pos)
		: EthercatSlave(name, phy_id, vendor_id, product_code, revision_num, dc_assign_activate)
		, Motor(name, phy_id, max_pos, min_pos, max_vel, min_vel, max_acc, min_acc, max_pos_following_error, max_vel_following_error, pos_factor, pos_offset, home_pos)
		, Slave(name, phy_id), imp_(new Imp)
	{
	}
	EthercatMotor::EthercatMotor(const EthercatMotor &other) = default;
	EthercatMotor& EthercatMotor::operator=(const EthercatMotor &other) = default;

	struct EthercatController::Imp
	{
		MotionPool motion_pool_{nullptr};
	};
	auto EthercatController::motionPool()->MotionPool& { return imp_->motion_pool_; }
	auto EthercatController::init()->void
	{ 
		EthercatMaster::init(); 
		Controller::init();

		imp_->motion_pool_ = MotionPool(&slavePool());
		motionPool().update();
	}
	EthercatController::~EthercatController() = default;
	EthercatController::EthercatController(const std::string &name) :imp_(new Imp), EthercatMaster(name), Controller(name), Master(name){}
}
