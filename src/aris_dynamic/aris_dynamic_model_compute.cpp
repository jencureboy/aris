﻿#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <limits>
#include <sstream>
#include <regex>
#include <limits>
#include <type_traits>

#include "aris_dynamic_model.h"


namespace aris
{
	namespace dynamic
	{
		struct Solver::Imp
		{
			Size max_iter_count_, iter_count_;
			double max_error_, error_;

			Imp(Size max_iter_count, double max_error) :max_iter_count_(max_iter_count), max_error_(max_error) {};
		};
		auto Solver::saveXml(aris::core::XmlElement &xml_ele) const->void
		{
			Element::saveXml(xml_ele);
			xml_ele.SetAttribute("max_iter_count", static_cast<std::int64_t>(imp_->max_iter_count_));
			xml_ele.SetAttribute("max_error", imp_->max_error_);
		}
		auto Solver::loadXml(const aris::core::XmlElement &xml_ele)->void 
		{
			imp_->max_iter_count_ = attributeInt32(xml_ele, "max_iter_count", 100);
			imp_->max_error_ = attributeDouble(xml_ele, "max_error", 1e-10);
			Element::loadXml(xml_ele);
			
		}
		auto Solver::init()->void
		{
			// make all part cm correct //
			for (auto &c : model().jointPool())s_tf_n(c.dim(), *c.makI().prtPm(), c.locCmI(), const_cast<double*>(c.prtCmI()));
			for (auto &c : model().motionPool())s_tf_n(c.dim(), *c.makI().prtPm(), c.locCmI(), const_cast<double*>(c.prtCmI()));
			for (auto &c : model().generalMotionPool())s_tf_n(c.dim(), *c.makI().prtPm(), c.locCmI(), const_cast<double*>(c.prtCmI()));

			allocateMemory();

		}
		auto Solver::error()const->double { return imp_->error_; }
		auto Solver::setError(double error)->void { imp_->error_ = error; }
		auto Solver::maxError()const->double { return imp_->max_error_; }
		auto Solver::setMaxError(double max_error)->void { imp_->max_error_ = max_error; }
		auto Solver::iterCount()const->Size { return imp_->iter_count_; }
		auto Solver::setIterCount(Size iter_count)->void { imp_->iter_count_ = iter_count; }
		auto Solver::maxIterCount()const->Size { return imp_->max_iter_count_; }
		auto Solver::setMaxIterCount(Size max_count)->void { imp_->max_iter_count_ = max_count; }
		Solver::~Solver() = default;
		Solver::Solver(const std::string &name, Size max_iter_count, double max_error) : Element(name), imp_(new Imp(max_iter_count, max_error)) {}
		Solver::Solver(const Solver&) = default;
		Solver::Solver(Solver&&) = default;
		Solver& Solver::operator=(const Solver&) = default;
		Solver& Solver::operator=(Solver&&) = default;

		struct Calibrator::Imp {};
		Calibrator::~Calibrator() = default;
		Calibrator::Calibrator(const std::string &name) : Element(name), imp_(new Imp) {}
		Calibrator::Calibrator(const Calibrator&) = default;
		Calibrator::Calibrator(Calibrator&&) = default;
		Calibrator& Calibrator::operator=(const Calibrator&) = default;
		Calibrator& Calibrator::operator=(Calibrator&&) = default;

		struct SimResult::TimeResult::Imp { std::deque<double> time_; };
		auto SimResult::TimeResult::saveXml(aris::core::XmlElement &xml_ele)const->void
		{
			Element::saveXml(xml_ele);

			std::stringstream ss;
			ss << std::setprecision(15);
			ss.str().reserve((25 * 1 + 1)*imp_->time_.size());

			for (auto &t : imp_->time_)ss << t << std::endl;

			xml_ele.SetText(ss.str().c_str());
		}
		auto SimResult::TimeResult::loadXml(const aris::core::XmlElement &xml_ele)->void
		{
			// 以下导入数据 //
			std::stringstream ss(std::string(xml_ele.GetText()));
			for (double t; ss >> t, !ss.eof(); imp_->time_.push_back(t));

			Element::loadXml(xml_ele);
		}
		auto SimResult::TimeResult::record()->void { imp_->time_.push_back(model().time()); }
		auto SimResult::TimeResult::restore(Size pos)->void { model().setTime(imp_->time_.at(pos)); }
		SimResult::TimeResult::~TimeResult() = default;
		SimResult::TimeResult::TimeResult(const std::string &name) : Element(name), imp_(new Imp) {}
		SimResult::TimeResult::TimeResult(const SimResult::TimeResult&) = default;
		SimResult::TimeResult::TimeResult(SimResult::TimeResult&&) = default;
		SimResult::TimeResult& SimResult::TimeResult::operator=(const TimeResult&) = default;
		SimResult::TimeResult& SimResult::TimeResult::operator=(TimeResult&&) = default;

		struct SimResult::PartResult::Imp
		{
			Part *part_;
			std::deque<std::array<double, 6> > pe_;
			std::deque<std::array<double, 6> > vs_;
			std::deque<std::array<double, 6> > as_;

			Imp(Part* part) :part_(part) {};
		};
		auto SimResult::PartResult::saveXml(aris::core::XmlElement &xml_ele)const->void
		{
			Element::saveXml(xml_ele);

			xml_ele.SetAttribute("part", part().name().c_str());
			std::stringstream ss;
			ss << std::setprecision(15);
			ss.str().reserve((25 * 18 + 1)*imp_->pe_.size());

			for (auto pe = imp_->pe_.begin(), vs = imp_->vs_.begin(), as = imp_->as_.begin(); pe < imp_->pe_.end(); ++pe, ++vs, ++as)
			{
				for (auto e : *pe) ss << e << " ";
				for (auto e : *vs)ss << e << " ";
				for (auto e : *as)ss << e << " ";
				ss << std::endl;
			}

			xml_ele.SetText(ss.str().c_str());
		}
		auto SimResult::PartResult::loadXml(const aris::core::XmlElement &xml_ele)->void
		{
			// 以下寻找对应的part //
			if (model().findByName("part_pool") == model().children().end())
				throw std::runtime_error("you must insert \"part_pool\" node before insert " + type() + " \"" + name() + "\"");

			auto &part_pool = static_cast<aris::core::ObjectPool<Part, Element>&>(*model().findByName("part_pool"));

			if (!xml_ele.Attribute("part"))throw std::runtime_error(std::string("xml element \"") + name() + "\" must have Attribute \"part\"");
			auto p = part_pool.findByName(xml_ele.Attribute("part"));
			if (p == part_pool.end())	throw std::runtime_error(std::string("can't find part for PartResult \"") + this->name() + "\"");

			imp_->part_ = &*p;

			// 以下导入数据 //
			std::stringstream ss(std::string(xml_ele.GetText()));
			std::array<double, 6> pe, vs, as;
			for (Size i{ 0 }; !ss.eof(); ++i)
			{
				if (i < 6) ss >> pe[i];
				else if (i < 12) ss >> vs[i - 6];
				else if (i < 18) ss >> as[i - 12];

				if (i == 6)imp_->pe_.push_back(pe);
				if (i == 12)imp_->vs_.push_back(vs);
				if (i == 18) { imp_->as_.push_back(as); i = -1; }
			}

			Element::loadXml(xml_ele);
		}
		auto SimResult::PartResult::part()->Part& { return *imp_->part_; }
		auto SimResult::PartResult::record()->void
		{
			std::array<double, 6> result;
			s_pm2pe(*part().pm(), result.data());
			imp_->pe_.push_back(result);
			std::copy(static_cast<const double*>(part().vs()), static_cast<const double*>(part().vs()) + 6, result.data());
			imp_->vs_.push_back(result);
			std::copy(static_cast<const double*>(part().as()), static_cast<const double*>(part().as()) + 6, result.data());
			imp_->as_.push_back(result);
		}
		auto SimResult::PartResult::restore(Size pos)->void
		{
			part().setPe(imp_->pe_.at(pos).data());
			part().setVs(imp_->vs_.at(pos).data());
			part().setAs(imp_->as_.at(pos).data());
		}
		SimResult::PartResult::~PartResult() = default;
		SimResult::PartResult::PartResult(const std::string &name, Part *part) : Element(name), imp_(new Imp(part)) {}
		SimResult::PartResult::PartResult(const SimResult::PartResult&) = default;
		SimResult::PartResult::PartResult(SimResult::PartResult&&) = default;
		SimResult::PartResult& SimResult::PartResult::operator=(const PartResult&) = default;
		SimResult::PartResult& SimResult::PartResult::operator=(PartResult&&) = default;

		struct SimResult::ConstraintResult::Imp
		{
			Constraint *constraint_;
			std::deque<std::array<double, 6> > cf_;

			Imp(Constraint* constraint) :constraint_(constraint) {};
		};
		auto SimResult::ConstraintResult::saveXml(aris::core::XmlElement &xml_ele)const->void
		{
			Element::saveXml(xml_ele);

			xml_ele.SetAttribute("constraint", constraint().name().c_str());

			std::stringstream ss;
			ss << std::setprecision(15);
			ss.str().reserve((25 * 6 + 1)*imp_->cf_.size());
			for (auto &cf : imp_->cf_)
			{
				for (Size i(-1); ++i < constraint().dim();) ss << cf[i] << " ";
				ss << std::endl;
			}

			xml_ele.SetText(ss.str().c_str());
		}
		auto SimResult::ConstraintResult::loadXml(const aris::core::XmlElement &xml_ele)->void
		{
			// 以下寻找对应的constraint //
			if (!xml_ele.Attribute("constraint"))throw std::runtime_error(std::string("xml element \"") + name() + "\" must have Attribute \"constraint\"");
			if (!imp_->constraint_ && model().findByName("joint_pool") != model().children().end())
			{
				auto &pool = static_cast<aris::core::ObjectPool<Joint, Element>&>(*model().findByName("joint_pool"));
				auto c = pool.findByName(xml_ele.Attribute("constraint"));
				if (c != pool.end())imp_->constraint_ = &*c;
			}
			if (!imp_->constraint_ && model().findByName("motion_pool") != model().children().end())
			{
				auto &pool = static_cast<aris::core::ObjectPool<Motion, Element>&>(*model().findByName("motion_pool"));
				auto c = pool.findByName(xml_ele.Attribute("constraint"));
				if (c != pool.end())imp_->constraint_ = &*c;
			}
			if (!imp_->constraint_ && model().findByName("general_motion_pool") != model().children().end())
			{
				auto &pool = static_cast<aris::core::ObjectPool<GeneralMotion, Element>&>(*model().findByName("general_motion_pool"));
				auto c = pool.findByName(xml_ele.Attribute("constraint"));
				if (c != pool.end())imp_->constraint_ = &*c;
			}
			if (!imp_->constraint_)throw std::runtime_error(std::string("can't find constraint for ConstraintResult \"") + this->name() + "\"");

			// 以下读取数据 //
			std::stringstream ss(std::string(xml_ele.GetText()));
			std::array<double, 6> cf{ 0,0,0,0,0,0 };
			for (Size i{ 0 }; !ss.eof(); ss >> cf[i++])
			{
				if (i == constraint().dim())
				{
					i = 0;
					imp_->cf_.push_back(cf);
				}
			}

			Element::loadXml(xml_ele);
		}
		auto SimResult::ConstraintResult::constraint()->Constraint& { return *imp_->constraint_; }
		auto SimResult::ConstraintResult::record()->void
		{
			std::array<double, 6> result{ 0,0,0,0,0,0 };
			std::copy(constraint().cf(), constraint().cf() + constraint().dim(), result.data());
			imp_->cf_.push_back(result);
		}
		auto SimResult::ConstraintResult::restore(Size pos)->void { constraint().setCf(imp_->cf_.at(pos).data()); }
		SimResult::ConstraintResult::~ConstraintResult() = default;
		SimResult::ConstraintResult::ConstraintResult(const std::string &name, Constraint *constraint) : Element(name), imp_(new Imp(constraint)) {}
		SimResult::ConstraintResult::ConstraintResult(const SimResult::ConstraintResult&) = default;
		SimResult::ConstraintResult::ConstraintResult(SimResult::ConstraintResult&&) = default;
		SimResult::ConstraintResult& SimResult::ConstraintResult::operator=(const ConstraintResult&) = default;
		SimResult::ConstraintResult& SimResult::ConstraintResult::operator=(ConstraintResult&&) = default;

		struct SimResult::Imp
		{
			TimeResult *time_result_;
			aris::core::ObjectPool<PartResult, Element> *part_result_pool_;
			aris::core::ObjectPool<ConstraintResult, Element> *constraint_result_pool_;
		};
		auto SimResult::loadXml(const aris::core::XmlElement &xml_ele)->void
		{
			Element::loadXml(xml_ele);

			imp_->time_result_ = findOrInsert<TimeResult>("time_result");
			imp_->constraint_result_pool_ = findOrInsert<aris::core::ObjectPool<SimResult::ConstraintResult, Element> >("constraint_result_pool");
			imp_->part_result_pool_ = findOrInsert<aris::core::ObjectPool<SimResult::PartResult, Element> >("part_result_pool");

		}
		auto SimResult::timeResult()->TimeResult& { return *imp_->time_result_; }
		auto SimResult::partResultPool()->aris::core::ObjectPool<SimResult::PartResult, Element>& { return *imp_->part_result_pool_; }
		auto SimResult::constraintResultPool()->aris::core::ObjectPool<SimResult::ConstraintResult, Element>& { return *imp_->constraint_result_pool_; }
		auto SimResult::allocateMemory()->void
		{
			partResultPool().clear();
			for (auto &p : model().partPool())partResultPool().add<PartResult>(p.name() + "_result", &p);
			constraintResultPool().clear();
			for (auto &c : model().jointPool())constraintResultPool().add<ConstraintResult>(c.name() + "_result", &c);
			for (auto &c : model().motionPool())constraintResultPool().add<ConstraintResult>(c.name() + "_result", &c);
			for (auto &c : model().generalMotionPool())constraintResultPool().add<ConstraintResult>(c.name() + "_result", &c);
		}
		auto SimResult::record()->void
		{
			timeResult().record();
			for (auto &p : partResultPool())p.record();
			for (auto &p : constraintResultPool())p.record();
		}
		auto SimResult::restore(Size pos)->void
		{
			timeResult().restore(pos);
			for (auto &r : partResultPool())r.restore(pos);
			for (auto &r : constraintResultPool())r.restore(pos);
		}
		auto SimResult::size()const->Size { return timeResult().imp_->time_.size() == 0 ? 0 : timeResult().imp_->time_.size() - 1; }
		auto SimResult::clear()->void
		{
			timeResult().imp_->time_.clear();
			for (auto &r : partResultPool())
			{
				r.imp_->pe_.clear();
				r.imp_->vs_.clear();
				r.imp_->as_.clear();
			}
			for (auto &r : constraintResultPool())r.imp_->cf_.clear();
		}
		SimResult::~SimResult() = default;
		SimResult::SimResult(const std::string &name) : Element(name), imp_(new Imp())
		{
			imp_->time_result_ = &add<TimeResult>("time_result");
			imp_->part_result_pool_ = &add<aris::core::ObjectPool<SimResult::PartResult, Element> >("part_result_pool");
			imp_->constraint_result_pool_ = &add<aris::core::ObjectPool<SimResult::ConstraintResult, Element> >("constraint_result_pool");
		}
		SimResult::SimResult(const SimResult&other) : Element(other), imp_(other.imp_)
		{
			imp_->time_result_ = findType<TimeResult >("time_result");
			imp_->constraint_result_pool_ = findType<aris::core::ObjectPool<SimResult::ConstraintResult, Element> >("constraint_result_pool");
			imp_->part_result_pool_ = findType<aris::core::ObjectPool<SimResult::PartResult, Element> >("part_result_pool");
		}
		SimResult::SimResult(SimResult&&other) : Element(std::move(other)), imp_(std::move(other.imp_))
		{
			imp_->time_result_ = findType<TimeResult >("time_result");
			imp_->constraint_result_pool_ = findType<aris::core::ObjectPool<SimResult::ConstraintResult, Element> >("constraint_result_pool");
			imp_->part_result_pool_ = findType<aris::core::ObjectPool<SimResult::PartResult, Element> >("part_result_pool");
		}
		SimResult& SimResult::operator=(const SimResult&other)
		{
			Element::operator=(other);
			imp_ = other.imp_;
			imp_->time_result_ = findType<TimeResult >("time_result");
			imp_->constraint_result_pool_ = findType<aris::core::ObjectPool<SimResult::ConstraintResult, Element> >("constraint_result_pool");
			imp_->part_result_pool_ = findType<aris::core::ObjectPool<SimResult::PartResult, Element> >("part_result_pool");
			return *this;
		}
		SimResult& SimResult::operator=(SimResult&&other)
		{
			Element::operator=(std::move(other));
			imp_ = other.imp_;
			imp_->time_result_ = findType<TimeResult >("time_result");
			imp_->constraint_result_pool_ = findType<aris::core::ObjectPool<SimResult::ConstraintResult, Element> >("constraint_result_pool");
			imp_->part_result_pool_ = findType<aris::core::ObjectPool<SimResult::PartResult, Element> >("part_result_pool");
			return *this;
		}

		struct Simulator::Imp { };
		auto Simulator::simulate(const PlanFunction &plan, void *param, std::uint32_t param_size, SimResult &result)->void
		{
			result.allocateMemory();
			// 记录初始位置 //
			result.record();
			// 记录轨迹中的位置 //
			for (PlanParam plan_param{ &model(), std::uint32_t(1), param, param_size }; plan(plan_param) != 0; ++plan_param.count_);
			// 记录结束位置 //
			result.record();
		}
		Simulator::~Simulator() = default;
		Simulator::Simulator(const std::string &name) : Element(name), imp_(new Imp) {}
		Simulator::Simulator(const Simulator&) = default;
		Simulator::Simulator(Simulator&&) = default;
		Simulator& Simulator::operator=(const Simulator&) = default;
		Simulator& Simulator::operator=(Simulator&&) = default;

		struct CombineSolver::Imp
		{
			Size p_size_, c_size_;
			std::vector<double> A_, x_, b_;
			std::vector<double> U_, t_;
			std::vector<Size> p_;

			std::vector<PartBlock> part_block_pool_;
			std::vector<ConstraintBlock> constraint_block_pool_;
		};
		auto CombineSolver::allocateMemory()->void
		{
			// make active pool //
			imp_->part_block_pool_.clear();
			imp_->constraint_block_pool_.clear();

			imp_->part_block_pool_.push_back(PartBlock{ &model().ground(), 0 });
			for (auto &prt : model().partPool())if (prt.active() && (&prt != &model().ground()))imp_->part_block_pool_.push_back(PartBlock{ &prt, 0 });
			for (auto &jnt : model().jointPool())if (jnt.active())imp_->constraint_block_pool_.push_back(ConstraintBlock{ &jnt,0, nullptr, nullptr });
			for (auto &mot : model().motionPool())if (mot.active()) imp_->constraint_block_pool_.push_back(ConstraintBlock{ &mot,0, nullptr, nullptr });
			for (auto &gmt : model().generalMotionPool())if (gmt.active())imp_->constraint_block_pool_.push_back(ConstraintBlock{ &gmt,0, nullptr, nullptr });

			// compute memory size //
			imp_->p_size_ = 0;
			imp_->c_size_ = 6;

			for (auto &pb : activePartBlockPool())
			{
				pb.row_id_ = imp_->p_size_;
				imp_->p_size_ += 6;
			}
			for (auto &cb : activeConstraintBlockPool())
			{
				cb.col_id_ = imp_->c_size_;
				imp_->c_size_ += cb.constraint_->dim();

				auto i_ = std::find_if(activePartBlockPool().begin(), activePartBlockPool().end(), [&cb](PartBlock &pb) {return pb.part_ == &cb.constraint_->makI().fatherPart(); });
				auto j_ = std::find_if(activePartBlockPool().begin(), activePartBlockPool().end(), [&cb](PartBlock &pb) {return pb.part_ == &cb.constraint_->makJ().fatherPart(); });
				if (i_ == activePartBlockPool().end()) throw std::runtime_error("i part not found");
				if (j_ == activePartBlockPool().end()) throw std::runtime_error("j part not found");
				cb.pb_i_ = &*i_;
				cb.pb_j_ = &*j_;
			}

			// allocate memory //
			imp_->A_.clear();
			imp_->A_.resize(aSize() * aSize(), 0.0);
			imp_->b_.clear();
			imp_->b_.resize(aSize() * 1, 0.0);
			imp_->x_.clear();
			imp_->x_.resize(aSize() * 1, 0.0);
			for (Size i(-1); ++i < 6;) 
			{
				imp_->A_.data()[aris::dynamic::id(i, i, aSize())] = 1.0;
				imp_->A_.data()[aris::dynamic::id(i + pSize(), i, aSize())] = 1.0;
				imp_->A_.data()[aris::dynamic::id(i, i + pSize(), aSize())] = 1.0;
			}

			imp_->U_.clear();
			imp_->U_.resize(aSize() * aSize(), 0.0);
			imp_->t_.clear();
			imp_->t_.resize(aSize() * 1, 0.0);
			imp_->p_.clear();
			imp_->p_.resize(aSize() * 1, 0);
		}
		auto CombineSolver::activePartBlockPool()->std::vector<PartBlock>& { return imp_->part_block_pool_; }
		auto CombineSolver::activeConstraintBlockPool()->std::vector<ConstraintBlock>& { return imp_->constraint_block_pool_; }
		auto CombineSolver::cSize()->Size { return imp_->c_size_; }
		auto CombineSolver::pSize()->Size { return imp_->p_size_; }
		auto CombineSolver::A()->double * { return imp_->A_.data(); }
		auto CombineSolver::x()->double * { return imp_->x_.data(); }
		auto CombineSolver::b()->double * { return imp_->b_.data(); }
		auto CombineSolver::kinPos()->void
		{
			setIterCount(0);

			double pm[16]{ 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
			s_mc(4, 4, pm, const_cast<double *>(*model().ground().pm()));
			for (; iterCount() < maxIterCount(); setIterCount(iterCount() + 1))
			{
				// make cp //
				updCp();

				// check error //
				setError(s_norm(cSize(), cp()));
				if (error() < maxError()) return;

				auto e = error();

				// make C //
				updCm();

				// solve dp //
				s_householder_utp(cSize(), pSize(), cm(), ColMajor{ aSize() }, imp_->U_.data(), pSize(), imp_->t_.data(), 1, imp_->p_.data(), maxError());
				s_householder_utp_sov(cSize(), pSize(), 1, imp_->U_.data(), imp_->t_.data(), imp_->p_.data(), cp(), pp(), maxError());

				// upd part pos //
				updPartPos();
			}
		}
		auto CombineSolver::kinVel()->void
		{
			s_fill(6, 1, 0.0, const_cast<double *>(model().ground().vs()));
			
			// make cv //
			updCv();

			// make C //
			updCm();

			// solve compensate pv //
			s_householder_utp(cSize(), pSize(), cm(), ColMajor{ aSize() }, imp_->U_.data(), pSize(), imp_->t_.data(), 1, imp_->p_.data(), maxError());
			s_householder_utp_sov(cSize(), pSize(), 1, imp_->U_.data(), imp_->t_.data(), imp_->p_.data(), cv(), pv(), maxError());

			// upd part vel //
			updPartVel();
		}
		auto CombineSolver::dynAccAndFce()->void
		{
			s_fill(6, 1, 0.0, const_cast<double *>(model().ground().as()));
			
			updIm();
			updCm();
			updCmT();

			updCa();
			updPf();

			// solve x //
			s_householder_utp(aSize(), aSize(), A(), imp_->U_.data(), imp_->t_.data(), imp_->p_.data(), maxError());
			s_householder_utp_sov(aSize(), aSize(), 1, imp_->U_.data(), imp_->t_.data(), imp_->p_.data(), b(), x(), maxError());

			updConstraintFce();
			updPartAcc();

		}
		CombineSolver::~CombineSolver() = default;
		CombineSolver::CombineSolver(const std::string &name, Size max_iter_count, double max_error) :Solver(name, max_iter_count, max_error) {}
		CombineSolver::CombineSolver(const CombineSolver &other) = default;
		CombineSolver::CombineSolver(CombineSolver &&other) = default;
		CombineSolver& CombineSolver::operator=(const CombineSolver &other) = default;
		CombineSolver& CombineSolver::operator=(CombineSolver &&other) = default;

		auto GroundCombineSolver::updCm()->void 
		{
			for (auto &cb : activeConstraintBlockPool())
			{
				auto row_i = cb.pb_i_->row_id_;
				auto row_j = cb.pb_j_->row_id_;
				auto col = cb.col_id_ + pSize();

				cb.constraint_->cptGlbCm(A() + dynamic::id(row_i, col, aSize()), aSize(), A() + dynamic::id(row_j, col, aSize()), aSize());
			}
		}
		auto GroundCombineSolver::updCmT()->void
		{
			for (auto &cb : activeConstraintBlockPool())
			{
				auto row_i = cb.pb_i_->row_id_;
				auto row_j = cb.pb_j_->row_id_;
				auto col = cb.col_id_ + pSize();

				cb.constraint_->cptGlbCm(A() + dynamic::id(row_i, col, ColMajor{ aSize() }), ColMajor{ aSize() }, A() + dynamic::id(row_j, col, ColMajor{ aSize() }), ColMajor{ aSize() });
			}
		}
		auto GroundCombineSolver::updIm()->void { for (auto &pb : activePartBlockPool())pb.part_->cptGlbIm(A() + dynamic::id(pb.row_id_, pb.row_id_, aSize()), aSize()); }
		auto GroundCombineSolver::updCp()->void { for (auto &cb : activeConstraintBlockPool())cb.constraint_->cptCp(cp() + cb.col_id_); }
		auto GroundCombineSolver::updCv()->void { for (auto &cb : activeConstraintBlockPool())cb.constraint_->cptCv(cv() + cb.col_id_); }
		auto GroundCombineSolver::updCa()->void { for (auto &cb : activeConstraintBlockPool())cb.constraint_->cptCa(ca() + cb.col_id_); }
		auto GroundCombineSolver::updPv()->void { for (auto &pb : activePartBlockPool())pb.part_->getVs(pv() + pb.row_id_); }
		auto GroundCombineSolver::updPa()->void { for (auto &pb : activePartBlockPool())pb.part_->getAs(pa() + pb.row_id_); }
		auto GroundCombineSolver::updPf()->void { for (auto &pb : activePartBlockPool())pb.part_->cptGlbPf(pf() + pb.row_id_); }
		auto GroundCombineSolver::updConstraintFce()->void { for (auto &cb : activeConstraintBlockPool())cb.constraint_->setCf(cf() + dynamic::id(cb.col_id_, 0, 1));}
		auto GroundCombineSolver::updPartPos()->void 
		{
			for (auto pb : activePartBlockPool())
			{
				if (pb.part_ != &model().ground())
				{
					double pm[4][4];
					double pq[7];

					s_vc(6, x() + dynamic::id(pb.row_id_, 0, 1), pq);

					double theta = s_norm(3, pq + 3);
					pq[6] = std::cos(theta / 2);

					double factor = theta < 1e-4 ? 0.5 : std::sin(theta / 2) / theta;
					s_nv(3, factor, pq + 3);

					s_pq2pm(pq, *pm);

					double final_pm[4][4];
					s_pm2pm(*pm, *pb.part_->pm(), *final_pm);

					pb.part_->setPm(*final_pm);
				}
			}
		}
		auto GroundCombineSolver::updPartVel()->void { for (auto &pb : activePartBlockPool()) s_va(6, pv() + pb.row_id_, const_cast<double*>(pb.part_->vs())); }
		auto GroundCombineSolver::updPartAcc()->void { for (auto &pb : activePartBlockPool()) pb.part_->setAs(pa() + dynamic::id(pb.row_id_, 0, 1)); }
		GroundCombineSolver::~GroundCombineSolver() = default;
		GroundCombineSolver::GroundCombineSolver(const std::string &name, Size max_iter_count, double max_error) :CombineSolver(name, max_iter_count, max_error) {}
		GroundCombineSolver::GroundCombineSolver(const GroundCombineSolver &other) = default;
		GroundCombineSolver::GroundCombineSolver(GroundCombineSolver &&other) = default;
		GroundCombineSolver& GroundCombineSolver::operator=(const GroundCombineSolver &other) = default;
		GroundCombineSolver& GroundCombineSolver::operator=(GroundCombineSolver &&other) = default;

		struct DividedSolver::Imp
		{
			Size p_size_, c_size_;
			std::vector<double> im_, cm_, pp_, pv_, pa_, pf_, cp_, cv_, ca_, cf_;

			std::vector<PartBlock> part_block_pool_;
			std::vector<ConstraintBlock> constraint_block_pool_;

			BlockSize p_blk_size_, c_blk_size_;
			BlockData im_blk_, cm_blk_, pp_blk_, pv_blk_, pa_blk_, pf_blk_, cp_blk_, cv_blk_, ca_blk_, cf_blk_;
		};
		auto DividedSolver::allocateMemory()->void
		{
			// make active pool //
			imp_->part_block_pool_.clear();
			imp_->constraint_block_pool_.clear();

			for (auto &prt : model().partPool())if (prt.active())imp_->part_block_pool_.push_back(PartBlock{ &prt, 0, 0 });
			for (auto &jnt : model().jointPool())if (jnt.active())imp_->constraint_block_pool_.push_back(ConstraintBlock{ &jnt,0,0, nullptr, nullptr });
			for (auto &mot : model().motionPool())if (mot.active()) imp_->constraint_block_pool_.push_back(ConstraintBlock{ &mot,0,0, nullptr, nullptr });
			for (auto &gmt : model().generalMotionPool())if (gmt.active())imp_->constraint_block_pool_.push_back(ConstraintBlock{ &gmt,0,0, nullptr, nullptr });

			// compute memory size //
			imp_->p_size_ = 0;
			imp_->c_size_ = 6;
			imp_->p_blk_size_.clear();
			imp_->c_blk_size_.clear();
			imp_->c_blk_size_.resize(1, 6);

			for (auto &pb : activePartBlockPool())
			{
				pb.row_id_ = imp_->p_size_;
				pb.blk_row_id_ = imp_->p_blk_size_.size();
				imp_->p_size_ += 6;
				imp_->p_blk_size_.push_back(6);
			}
			for (auto &cb : activeConstraintBlockPool())
			{
				cb.col_id_ = imp_->c_size_;
				cb.blk_col_id_ = imp_->c_blk_size_.size();
				imp_->c_size_ += cb.constraint_->dim();
				imp_->c_blk_size_.push_back(cb.constraint_->dim());

				auto i_ = std::find_if(activePartBlockPool().begin(), activePartBlockPool().end(), [&cb](PartBlock &pb) {return pb.part_ == &cb.constraint_->makI().fatherPart(); });
				auto j_ = std::find_if(activePartBlockPool().begin(), activePartBlockPool().end(), [&cb](PartBlock &pb) {return pb.part_ == &cb.constraint_->makJ().fatherPart(); });
				if (i_ == activePartBlockPool().end()) throw std::runtime_error("i part not found");
				if (j_ == activePartBlockPool().end()) throw std::runtime_error("j part not found");
				cb.pb_i_ = &*i_;
				cb.pb_j_ = &*j_;
			}

			// allocate memory //
			imp_->im_.clear();
			imp_->im_.resize(imp_->p_size_ * imp_->p_size_, 0.0);
			imp_->cm_.clear();
			imp_->cm_.resize(imp_->p_size_ * imp_->c_size_, 0.0);
			imp_->cp_.clear();
			imp_->cp_.resize(imp_->c_size_ * 1, 0.0);
			imp_->cv_.clear();
			imp_->cv_.resize(imp_->c_size_ * 1, 0.0);
			imp_->ca_.clear();
			imp_->ca_.resize(imp_->c_size_ * 1, 0.0);
			imp_->cf_.clear();
			imp_->cf_.resize(imp_->c_size_ * 1, 0.0);
			imp_->pp_.clear();
			imp_->pp_.resize(imp_->p_size_ * 1, 0.0);
			imp_->pv_.clear();
			imp_->pv_.resize(imp_->p_size_ * 1, 0.0);
			imp_->pa_.clear();
			imp_->pa_.resize(imp_->p_size_ * 1, 0.0);
			imp_->pf_.clear();
			imp_->pf_.resize(imp_->p_size_ * 1, 0.0);

			imp_->im_blk_.clear();
			imp_->im_blk_.resize(imp_->p_blk_size_.size() * imp_->p_blk_size_.size());
			imp_->cm_blk_.clear();
			imp_->cm_blk_.resize(imp_->p_blk_size_.size() * imp_->c_blk_size_.size());
			imp_->cp_blk_.clear();
			imp_->cp_blk_.resize(imp_->c_blk_size_.size() * 1);
			imp_->cv_blk_.clear();
			imp_->cv_blk_.resize(imp_->c_blk_size_.size() * 1);
			imp_->ca_blk_.clear();
			imp_->ca_blk_.resize(imp_->c_blk_size_.size() * 1);
			imp_->cf_blk_.clear();
			imp_->cf_blk_.resize(imp_->c_blk_size_.size() * 1);
			imp_->pp_blk_.clear();
			imp_->pp_blk_.resize(imp_->p_blk_size_.size() * 1);
			imp_->pv_blk_.clear();
			imp_->pv_blk_.resize(imp_->p_blk_size_.size() * 1);
			imp_->pa_blk_.clear();
			imp_->pa_blk_.resize(imp_->p_blk_size_.size() * 1);
			imp_->pf_blk_.clear();
			imp_->pf_blk_.resize(imp_->p_blk_size_.size() * 1);
			
			s_blk_map(cBlkSize(), { 1 }, imp_->cp_.data(), imp_->cp_blk_);
			s_blk_map(cBlkSize(), { 1 }, imp_->cv_.data(), imp_->cv_blk_);
			s_blk_map(cBlkSize(), { 1 }, imp_->ca_.data(), imp_->ca_blk_);
			s_blk_map(cBlkSize(), { 1 }, imp_->cf_.data(), imp_->cf_blk_);
			s_blk_map(pBlkSize(), { 1 }, imp_->pp_.data(), imp_->pp_blk_);
			s_blk_map(pBlkSize(), { 1 }, imp_->pv_.data(), imp_->pv_blk_);
			s_blk_map(pBlkSize(), { 1 }, imp_->pa_.data(), imp_->pa_blk_);
			s_blk_map(pBlkSize(), { 1 }, imp_->pf_.data(), imp_->pf_blk_);
			s_blk_map(pBlkSize(), pBlkSize(), imp_->im_.data(), imp_->im_blk_);
			s_blk_map(pBlkSize(), cBlkSize(), imp_->cm_.data(), imp_->cm_blk_);

			for (auto &ele : imp_->im_blk_)ele.is_zero = true;
			for (auto &ele : imp_->cm_blk_)ele.is_zero = true;
			for (auto &pb : activePartBlockPool())imp_->im_blk_[dynamic::id(pb.blk_row_id_, pb.blk_row_id_, pBlkSize().size())].is_zero = false;
			for (auto &cb : activeConstraintBlockPool())
			{
				imp_->cm_blk_[dynamic::id(cb.pb_i_->blk_row_id_, cb.blk_col_id_, cBlkSize().size())].is_zero = false;
				imp_->cm_blk_[dynamic::id(cb.pb_j_->blk_row_id_, cb.blk_col_id_, cBlkSize().size())].is_zero = false;
			}

			auto ground_iter = std::find_if(imp_->part_block_pool_.begin(), imp_->part_block_pool_.end(), [&](PartBlock &pb) {return pb.part_ == &model().ground(); });

			imp_->cm_blk_[dynamic::id(ground_iter->blk_row_id_, 0, cBlkSize().size())].is_zero = false;
			for (Size i = 0; i < 6; ++i)imp_->cm_[aris::dynamic::id(ground_iter->row_id_ + i, i, cSize())] = 1.0;

		}
		auto DividedSolver::activePartBlockPool()->std::vector<PartBlock>& { return imp_->part_block_pool_; }
		auto DividedSolver::activeConstraintBlockPool()->std::vector<ConstraintBlock>& { return imp_->constraint_block_pool_; }
		auto DividedSolver::cSize()->Size { return imp_->c_size_; }
		auto DividedSolver::pSize()->Size { return imp_->p_size_; }
		auto DividedSolver::im()->double * { return imp_->im_.data(); }
		auto DividedSolver::cm()->double * { return imp_->cm_.data(); }
		auto DividedSolver::pp()->double * { return imp_->pp_.data(); }
		auto DividedSolver::pv()->double * { return imp_->pv_.data(); }
		auto DividedSolver::pa()->double * { return imp_->pa_.data(); }
		auto DividedSolver::pf()->double * { return imp_->pf_.data(); }
		auto DividedSolver::cp()->double * { return imp_->cp_.data(); }
		auto DividedSolver::cv()->double * { return imp_->cv_.data(); }
		auto DividedSolver::ca()->double * { return imp_->ca_.data(); }
		auto DividedSolver::cf()->double * { return imp_->cf_.data(); }
		auto DividedSolver::cBlkSize()->BlockSize& { return imp_->c_blk_size_; }
		auto DividedSolver::pBlkSize()->BlockSize& { return imp_->p_blk_size_; }
		auto DividedSolver::imBlk()->BlockData& { return imp_->im_blk_; }
		auto DividedSolver::cmBlk()->BlockData& { return imp_->cm_blk_; }
		auto DividedSolver::ppBlk()->BlockData& { return imp_->pp_blk_; }
		auto DividedSolver::pvBlk()->BlockData& { return imp_->pv_blk_; }
		auto DividedSolver::paBlk()->BlockData& { return imp_->pa_blk_; }
		auto DividedSolver::pfBlk()->BlockData& { return imp_->pf_blk_; }
		auto DividedSolver::cpBlk()->BlockData& { return imp_->cp_blk_; }
		auto DividedSolver::cvBlk()->BlockData& { return imp_->cv_blk_; }
		auto DividedSolver::caBlk()->BlockData& { return imp_->ca_blk_; }
		auto DividedSolver::cfBlk()->BlockData& { return imp_->cf_blk_; }
		DividedSolver::~DividedSolver() = default;
		DividedSolver::DividedSolver(const std::string &name, Size max_iter_count, double max_error) :Solver(name, max_iter_count, max_error) {}
		DividedSolver::DividedSolver(const DividedSolver &other) = default;
		DividedSolver::DividedSolver(DividedSolver &&other) = default;
		DividedSolver& DividedSolver::operator=(const DividedSolver &other) = default;
		DividedSolver& DividedSolver::operator=(DividedSolver &&other) = default;
		
		struct GroundDividedSolver::Imp {};
		auto GroundDividedSolver::updCp()->void { for (auto &cb : activeConstraintBlockPool())cb.constraint_->cptCp(cp() + dynamic::id(cb.col_id_, 0, 1)); }
		auto GroundDividedSolver::updCv()->void { for (auto &cb : activeConstraintBlockPool())cb.constraint_->cptCv(cv() + dynamic::id(cb.col_id_, 0, 1)); }
		auto GroundDividedSolver::updCa()->void { for (auto &cb : activeConstraintBlockPool())cb.constraint_->cptCa(ca() + dynamic::id(cb.col_id_, 0, 1)); }
		auto GroundDividedSolver::updPv()->void { for (auto &pb : activePartBlockPool())pb.part_->getVs(pv() + dynamic::id(pb.row_id_, 0, 1)); }
		auto GroundDividedSolver::updPa()->void { for (auto &pb : activePartBlockPool())pb.part_->getAs(pa() + dynamic::id(pb.row_id_, 0, 1)); }
		auto GroundDividedSolver::updPf()->void { for (auto &pb : activePartBlockPool())pb.part_->cptGlbPf(pf() + dynamic::id(pb.row_id_, 0, 1)); }
		auto GroundDividedSolver::updIm()->void { for (auto &pb : activePartBlockPool())pb.part_->cptGlbIm(im() + dynamic::id(pb.row_id_, pb.row_id_, pSize()), pSize()); }
		auto GroundDividedSolver::updCm()->void { for (auto &cb : activeConstraintBlockPool())cb.constraint_->cptGlbCm(cm() + dynamic::id(cb.pb_i_->row_id_, cb.col_id_, cSize()), cSize(), cm() + dynamic::id(cb.pb_j_->row_id_, cb.col_id_, cSize()), cSize()); }
		auto GroundDividedSolver::updConstraintFce()->void { for (auto &cb : activeConstraintBlockPool())cb.constraint_->setCf(cf() + dynamic::id(cb.col_id_, 0, 1)); }
		auto GroundDividedSolver::updPartPos()->void
		{
			double pm[16]{ 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
			s_mc(4, 4, pm, const_cast<double *>(*model().ground().pm()));
			
			for (auto pb : activePartBlockPool())
			{
				if (pb.part_ != &model().ground())
				{
					double pm[4][4];
					double pq[7];

					s_vc(6, pp() + dynamic::id(pb.row_id_, 0, 1), pq);

					double theta = s_norm(3, pq + 3);
					pq[6] = std::cos(theta / 2);

					double factor = theta < 1e-4 ? 0.5 : std::sin(theta / 2) / theta;
					s_nv(3, factor, pq + 3);

					s_pq2pm(pq, *pm);

					double final_pm[4][4];
					s_pm2pm(*pm, *pb.part_->pm(), *final_pm);

					pb.part_->setPm(*final_pm);
				}
			}
		}
		auto GroundDividedSolver::updPartVel()->void 
		{ 
			for (auto &pb : activePartBlockPool()) 
				if (pb.part_ != &model().ground()) 
					s_va(6, pv() + dynamic::id(pb.row_id_, 0, 1), const_cast<double*>(pb.part_->vs())); 
		}
		auto GroundDividedSolver::updPartAcc()->void 
		{ 
			for (auto &pb : activePartBlockPool())
				if (pb.part_ != &model().ground()) 
					pb.part_->setAs(pa() + dynamic::id(pb.row_id_, 0, 1)); 
		}
		GroundDividedSolver::~GroundDividedSolver() = default;
		GroundDividedSolver::GroundDividedSolver(const std::string &name, Size max_iter_count, double max_error) :DividedSolver(name, max_iter_count, max_error) {}
		GroundDividedSolver::GroundDividedSolver(const GroundDividedSolver &other) = default;
		GroundDividedSolver::GroundDividedSolver(GroundDividedSolver &&other) = default;
		GroundDividedSolver& GroundDividedSolver::operator=(const GroundDividedSolver &other) = default;
		GroundDividedSolver& GroundDividedSolver::operator=(GroundDividedSolver &&other) = default;

		struct PartDividedSolver::Imp {};
		auto PartDividedSolver::updCp()->void { for (auto &cb : activeConstraintBlockPool())cb.constraint_->cptCp(cp() + dynamic::id(cb.col_id_, 0, 1)); }
		auto PartDividedSolver::updCv()->void { for (auto &cb : activeConstraintBlockPool())cb.constraint_->cptCv(cv() + dynamic::id(cb.col_id_, 0, 1)); }
		auto PartDividedSolver::updCa()->void { for (auto &cb : activeConstraintBlockPool())cb.constraint_->cptCa(ca() + dynamic::id(cb.col_id_, 0, 1)); }
		auto PartDividedSolver::updPv()->void { for (auto &pb : activePartBlockPool())pb.part_->cptPrtVs(pv() + dynamic::id(pb.row_id_, 0, 1)); }
		auto PartDividedSolver::updPa()->void { for (auto &pb : activePartBlockPool())pb.part_->cptPrtAs(pa() + dynamic::id(pb.row_id_, 0, 1)); }
		auto PartDividedSolver::updPf()->void { for (auto &pb : activePartBlockPool())pb.part_->cptPrtPf(pf() + dynamic::id(pb.row_id_, 0, 1)); }
		auto PartDividedSolver::updIm()->void { for (auto &pb : activePartBlockPool())s_mc(6, 6, *pb.part_->prtIm(), 6, im() + dynamic::id(pb.row_id_, pb.row_id_, pSize()), pSize()); }
		auto PartDividedSolver::updCm()->void { for (auto &cb : activeConstraintBlockPool())cb.constraint_->cptPrtCm(cm() + dynamic::id(cb.pb_i_->row_id_, cb.col_id_, cSize()), cSize(), cm() + dynamic::id(cb.pb_j_->row_id_, cb.col_id_, cSize()), cSize()); }
		auto PartDividedSolver::updConstraintFce()->void { for (auto &cb : activeConstraintBlockPool())cb.constraint_->setCf(cf() + dynamic::id(cb.col_id_, 0, 1)); }
		auto PartDividedSolver::updPartPos()->void
		{
			double pm[16]{ 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
			s_mc(4, 4, pm, const_cast<double *>(*model().ground().pm()));

			for (auto pb : activePartBlockPool())
			{
				if (pb.part_ != &model().ground())
				{
					double pm[4][4];
					double pq[7];

					s_vc(6, pp() + dynamic::id(pb.row_id_, 0, 1), pq);
					s_tv(*pb.part_->pm(), pp() + dynamic::id(pb.row_id_, 0, 1), pq);

					double theta = s_norm(3, pq + 3);
					pq[6] = std::cos(theta / 2);

					double factor = theta < 1e-4 ? 0.5 : std::sin(theta / 2) / theta;
					s_nv(3, factor, pq + 3);

					s_pq2pm(pq, *pm);

					double final_pm[4][4];
					s_pm2pm(*pm, *pb.part_->pm(), *final_pm);

					pb.part_->setPm(*final_pm);
				}
			}
		}
		auto PartDividedSolver::updPartVel()->void { for (auto &pb : activePartBlockPool())s_tva(*pb.part_->pm(), pv() + dynamic::id(pb.row_id_, 0, 1), const_cast<double6&>(pb.part_->vs())); }
		auto PartDividedSolver::updPartAcc()->void { for (auto &pb : activePartBlockPool())s_tv(*pb.part_->pm(), pa() + dynamic::id(pb.row_id_, 0, 1), const_cast<double6&>(pb.part_->as())); }
		PartDividedSolver::~PartDividedSolver() = default;
		PartDividedSolver::PartDividedSolver(const std::string &name, Size max_iter_count, double max_error) :DividedSolver(name, max_iter_count, max_error) {}
		PartDividedSolver::PartDividedSolver(const PartDividedSolver &other) = default;
		PartDividedSolver::PartDividedSolver(PartDividedSolver &&other) = default;
		PartDividedSolver& PartDividedSolver::operator=(const PartDividedSolver &other) = default;
		PartDividedSolver& PartDividedSolver::operator=(PartDividedSolver &&other) = default;

		struct LltGroundDividedSolver::Imp
		{
			std::vector<double> cct_, ctc_, cct_llt_, cct_x_, cct_b_, ctc_llt_, ctc_x_, ctc_b_;
			BlockData cct_blk_, ctc_blk_;
			BlockData cct_llt_blk_, cct_x_blk_, cct_b_blk_, ctc_llt_blk_, ctc_x_blk_, ctc_b_blk_;
		};
		auto LltGroundDividedSolver::allocateMemory()->void
		{
			DividedSolver::allocateMemory();

			imp_->cct_.resize(pSize() * pSize());
			imp_->cct_llt_.resize(pSize() * pSize());
			imp_->cct_b_.resize(pSize() * 1);
			imp_->cct_x_.resize(pSize() * 1);

			imp_->cct_blk_.resize(pBlkSize().size() * pBlkSize().size());
			imp_->cct_llt_blk_.resize(pBlkSize().size() * pBlkSize().size());
			imp_->cct_b_blk_.resize(pBlkSize().size() * 1);
			imp_->cct_x_blk_.resize(pBlkSize().size() * 1);

			s_blk_map(pBlkSize(), pBlkSize(), imp_->cct_.data(), imp_->cct_blk_);
			s_blk_map(pBlkSize(), pBlkSize(), imp_->cct_llt_.data(), imp_->cct_llt_blk_);
			s_blk_map(pBlkSize(), { 1 }, imp_->cct_b_.data(), imp_->cct_b_blk_);
			s_blk_map(pBlkSize(), { 1 }, imp_->cct_x_.data(), imp_->cct_x_blk_);
		}
		auto LltGroundDividedSolver::kinPos()->void
		{
			setIterCount(0);

			double pm[16]{ 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
			s_mc(4, 4, pm, const_cast<double *>(*model().ground().pm()));
			for (; iterCount() < maxIterCount(); setIterCount(iterCount() + 1))
			{
				updCm();
				updCp();
				s_blk_mm(pBlkSize(), pBlkSize(), cBlkSize(), cmBlk(), BlockStride{ cBlkSize().size(),1,cSize(),1 }, cmBlk(), T(BlockStride{ cBlkSize().size(),1,cSize(),1 }), imp_->cct_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 });
				s_blk_mm(pBlkSize(), { 1 }, cBlkSize(), cmBlk(), cpBlk(), imp_->cct_b_blk_);

				setError(s_blk_norm_fro(cBlkSize(), { 1 }, cpBlk(), BlockStride{ 1,1,1,1 }));

				if (error() < maxError()) return;

				s_blk_llt(pBlkSize(), imp_->cct_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 });
				s_blk_sov_lm(pBlkSize(), { 1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_b_blk_, BlockStride{ 1,1,1,1 }, imp_->cct_x_blk_, BlockStride{ 1,1,1,1 });
				s_blk_sov_um(pBlkSize(), { 1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_x_blk_, BlockStride{ 1,1,1,1 }, ppBlk(), BlockStride{ 1,1,1,1 });

				updPartPos();
			}
		}
		auto LltGroundDividedSolver::kinVel()->void
		{
			s_fill(6, 1, 0.0, const_cast<double*>(model().ground().vs()));
			
			updCm();
			updCv();

			s_blk_mm(pBlkSize(), pBlkSize(), cBlkSize(), cmBlk(), BlockStride{ cBlkSize().size(),1,cSize(),1 }, cmBlk(), T(BlockStride{ cBlkSize().size(),1,cSize(),1 }), imp_->cct_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 });
			s_blk_mm(pBlkSize(), { 1 }, cBlkSize(), cmBlk(), cvBlk(), imp_->cct_b_blk_);

			s_blk_llt(pBlkSize(), imp_->cct_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 });
			s_blk_sov_lm(pBlkSize(), { 1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_b_blk_, BlockStride{ 1,1,1,1 }, imp_->cct_x_blk_, BlockStride{ 1,1,1,1 });
			s_blk_sov_um(pBlkSize(), { 1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_x_blk_, BlockStride{ 1,1,1,1 }, pvBlk(), BlockStride{ 1,1,1,1 });

			updPartVel();
		}
		auto LltGroundDividedSolver::kinAcc()->void
		{
			s_fill(6, 1, 0.0, const_cast<double *>(model().ground().as()));
			
			updCm();
			updCa();
			s_blk_mm(pBlkSize(), pBlkSize(), cBlkSize(), cmBlk(), BlockStride{ cBlkSize().size(),1,cSize(),1 }, cmBlk(), T(BlockStride{ cBlkSize().size(),1,cSize(),1 }), imp_->cct_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 });
			s_blk_mm(pBlkSize(), { 1 }, cBlkSize(), cmBlk(), caBlk(), imp_->cct_b_blk_);

			s_blk_llt(pBlkSize(), imp_->cct_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 });
			s_blk_sov_lm(pBlkSize(), { 1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_b_blk_, BlockStride{ 1,1,1,1 }, imp_->cct_x_blk_, BlockStride{ 1,1,1,1 });
			s_blk_sov_um(pBlkSize(), { 1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_x_blk_, BlockStride{ 1,1,1,1 }, paBlk(), BlockStride{ 1,1,1,1 });

			updPartAcc();
		}
		auto LltGroundDividedSolver::dynFce()->void
		{
			updIm();
			updCm();
			updPf();
			updPa();

			s_blk_mm(pBlkSize(), pBlkSize(), cBlkSize(), cmBlk(), BlockStride{ cBlkSize().size(),1,cSize(),1 }, cmBlk(), T(BlockStride{ cBlkSize().size(),1,cSize(),1 }), imp_->cct_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 });
			s_vc(pSize(), pf(), imp_->cct_b_.data());
			s_blk_mma(pBlkSize(), { 1 }, pBlkSize(), -1.0, imBlk(), BlockStride{ pBlkSize().size(),1,pSize(),1 }, paBlk(), BlockStride{ 1,1,1,1 }, imp_->cct_b_blk_, BlockStride{ 1,1,1,1 });

			s_blk_llt(pBlkSize(), imp_->cct_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 });
			s_blk_sov_lm(pBlkSize(), { 1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_b_blk_, BlockStride{ 1,1,1,1 }, imp_->cct_x_blk_, BlockStride{ 1,1,1,1 });
			s_blk_sov_um(pBlkSize(), { 1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_x_blk_, BlockStride{ 1,1,1,1 }, imp_->cct_b_blk_, BlockStride{ 1,1,1,1 });
			s_vc(pSize(), imp_->cct_b_.data(), imp_->cct_x_.data());

			s_blk_mm(cBlkSize(), { 1 }, pBlkSize(), cmBlk(), T(BlockStride{ cBlkSize().size(),1,cSize(),1 }), imp_->cct_x_blk_, BlockStride{ 1,1,1,1 }, cfBlk(), BlockStride{ 1,1,1,1 });

			updConstraintFce();
		}
		LltGroundDividedSolver::~LltGroundDividedSolver() = default;
		LltGroundDividedSolver::LltGroundDividedSolver(const std::string &name) :GroundDividedSolver(name) {}
		LltGroundDividedSolver::LltGroundDividedSolver(const LltGroundDividedSolver &other) = default;
		LltGroundDividedSolver::LltGroundDividedSolver(LltGroundDividedSolver &&other) = default;
		LltGroundDividedSolver& LltGroundDividedSolver::operator=(const LltGroundDividedSolver &other) = default;
		LltGroundDividedSolver& LltGroundDividedSolver::operator=(LltGroundDividedSolver &&other) = default;

		struct LltPartDividedSolver::Imp
		{
			std::vector<double> cct_, ctc_, cct_llt_, cct_x_, cct_b_, ctc_llt_, ctc_x_, ctc_b_;
			BlockData cct_blk_, ctc_blk_;
			BlockData cct_llt_blk_, cct_x_blk_, cct_b_blk_, ctc_llt_blk_, ctc_x_blk_, ctc_b_blk_;
		};
		auto LltPartDividedSolver::allocateMemory()->void
		{
			DividedSolver::allocateMemory();

			imp_->cct_.resize(pSize() * pSize());
			imp_->cct_llt_.resize(pSize() * pSize());
			imp_->cct_b_.resize(pSize() * 1);
			imp_->cct_x_.resize(pSize() * 1);

			imp_->cct_blk_.resize(pBlkSize().size() * pBlkSize().size());
			imp_->cct_llt_blk_.resize(pBlkSize().size() * pBlkSize().size());
			imp_->cct_b_blk_.resize(pBlkSize().size() * 1);
			imp_->cct_x_blk_.resize(pBlkSize().size() * 1);

			s_blk_map(pBlkSize(), pBlkSize(), imp_->cct_.data(), imp_->cct_blk_);
			s_blk_map(pBlkSize(), pBlkSize(), imp_->cct_llt_.data(), imp_->cct_llt_blk_);
			s_blk_map(pBlkSize(), { 1 }, imp_->cct_b_.data(), imp_->cct_b_blk_);
			s_blk_map(pBlkSize(), { 1 }, imp_->cct_x_.data(), imp_->cct_x_blk_);
		}
		auto LltPartDividedSolver::kinPos()->void
		{
			setIterCount(0);

			double pm[16]{ 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
			s_mc(4, 4, pm, const_cast<double *>(*model().ground().pm()));
			for (; iterCount() < maxIterCount(); setIterCount(iterCount() + 1))
			{
				updCm();
				updCp();
				s_blk_mm(pBlkSize(), pBlkSize(), cBlkSize(), cmBlk(), BlockStride{ cBlkSize().size(),1,cSize(),1 }, cmBlk(), T(BlockStride{ cBlkSize().size(),1,cSize(),1 }), imp_->cct_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 });
				s_blk_mm(pBlkSize(), { 1 }, cBlkSize(), cmBlk(), cpBlk(), imp_->cct_b_blk_);

				setError(s_blk_norm_fro(cBlkSize(), { 1 }, cpBlk(), BlockStride{ 1,1,1,1 }));

				if (error() < maxError()) return;

				s_blk_llt(pBlkSize(), imp_->cct_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 });
				s_blk_sov_lm(pBlkSize(), { 1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_b_blk_, BlockStride{ 1,1,1,1 }, imp_->cct_x_blk_, BlockStride{ 1,1,1,1 });
				s_blk_sov_um(pBlkSize(), { 1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_x_blk_, BlockStride{ 1,1,1,1 }, ppBlk(), BlockStride{ 1,1,1,1 });

				updPartPos();
			}
		}
		auto LltPartDividedSolver::kinVel()->void
		{
			s_fill(6, 1, 0.0, const_cast<double*>(model().ground().vs()));
			
			updCm();
			updCv();

			s_blk_mm(pBlkSize(), pBlkSize(), cBlkSize(), cmBlk(), BlockStride{ cBlkSize().size(),1,cSize(),1 }, cmBlk(), T(BlockStride{ cBlkSize().size(),1,cSize(),1 }), imp_->cct_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 });
			s_blk_mm(pBlkSize(), { 1 }, cBlkSize(), cmBlk(), cvBlk(), imp_->cct_b_blk_);

			s_blk_llt(pBlkSize(), imp_->cct_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 });
			s_blk_sov_lm(pBlkSize(), { 1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_b_blk_, BlockStride{ 1,1,1,1 }, imp_->cct_x_blk_, BlockStride{ 1,1,1,1 });
			s_blk_sov_um(pBlkSize(), { 1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_x_blk_, BlockStride{ 1,1,1,1 }, pvBlk(), BlockStride{ 1,1,1,1 });

			updPartVel();
		}
		auto LltPartDividedSolver::kinAcc()->void
		{
			s_fill(6, 1, 0.0, const_cast<double *>(model().ground().as()));
			
			updCm();
			updCa();
			s_blk_mm(pBlkSize(), pBlkSize(), cBlkSize(), cmBlk(), BlockStride{ cBlkSize().size(),1,cSize(),1 }, cmBlk(), T(BlockStride{ cBlkSize().size(),1,cSize(),1 }), imp_->cct_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 });
			s_blk_mm(pBlkSize(), { 1 }, cBlkSize(), cmBlk(), caBlk(), imp_->cct_b_blk_);

			s_blk_llt(pBlkSize(), imp_->cct_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 });
			s_blk_sov_lm(pBlkSize(), { 1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_b_blk_, BlockStride{ 1,1,1,1 }, imp_->cct_x_blk_, BlockStride{ 1,1,1,1 });
			s_blk_sov_um(pBlkSize(), { 1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_x_blk_, BlockStride{ 1,1,1,1 }, paBlk(), BlockStride{ 1,1,1,1 });

			updPartAcc();
		}
		auto LltPartDividedSolver::dynFce()->void
		{
			updIm();
			updCm();
			updPf();
			updPa();

			s_blk_mm(pBlkSize(), pBlkSize(), cBlkSize(), cmBlk(), BlockStride{ cBlkSize().size(),1,cSize(),1 }, cmBlk(), T(BlockStride{ cBlkSize().size(),1,cSize(),1 }), imp_->cct_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 });
			s_vc(pSize(), pf(), imp_->cct_b_.data());
			s_blk_mma(pBlkSize(), { 1 }, pBlkSize(), -1.0, imBlk(), BlockStride{ pBlkSize().size(),1,pSize(),1 }, paBlk(), BlockStride{ 1,1,1,1 }, imp_->cct_b_blk_, BlockStride{ 1,1,1,1 });

			s_blk_llt(pBlkSize(), imp_->cct_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 });
			s_blk_sov_lm(pBlkSize(), { 1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_b_blk_, BlockStride{ 1,1,1,1 }, imp_->cct_x_blk_, BlockStride{ 1,1,1,1 });
			s_blk_sov_um(pBlkSize(), { 1 }, imp_->cct_llt_blk_, BlockStride{ pBlkSize().size(),1,pSize(),1 }, imp_->cct_x_blk_, BlockStride{ 1,1,1,1 }, imp_->cct_b_blk_, BlockStride{ 1,1,1,1 });
			s_vc(pSize(), imp_->cct_b_.data(), imp_->cct_x_.data());

			s_blk_mm(cBlkSize(), { 1 }, pBlkSize(), cmBlk(), T(BlockStride{ cBlkSize().size(),1,cSize(),1 }), imp_->cct_x_blk_, BlockStride{ 1,1,1,1 }, cfBlk(), BlockStride{ 1,1,1,1 });

			updConstraintFce();
		}
		LltPartDividedSolver::~LltPartDividedSolver() = default;
		LltPartDividedSolver::LltPartDividedSolver(const std::string &name, Size max_iter_count, double max_error) :PartDividedSolver(name, max_iter_count, max_error) {}
		LltPartDividedSolver::LltPartDividedSolver(const LltPartDividedSolver &other) = default;
		LltPartDividedSolver::LltPartDividedSolver(LltPartDividedSolver &&other) = default;
		LltPartDividedSolver& LltPartDividedSolver::operator=(const LltPartDividedSolver &other) = default;
		LltPartDividedSolver& LltPartDividedSolver::operator=(LltPartDividedSolver &&other) = default;

		struct DiagSolver::Imp
		{
			std::vector<Relation> relation_pool_;
			std::vector<Part *> part_pool_;
			std::vector<Diag> diag_pool_;
			std::vector<Remainder> remainder_pool_;

			std::vector<double> A_;
			std::vector<double> x_;
			std::vector<double> b_;
			std::vector<double> U_, tau_;
			std::vector<Size> p_;

			Size rows, cols;
		};
		auto DiagSolver::allocateMemory()->void 
		{
			// make active part pool //
			activePartPool().clear();
			for (auto &p : model().partPool())if (p.active())activePartPool().push_back(&p);
			
			// make active constraint pool //
			std::vector<Constraint*> cp;
			for (auto &jnt : model().jointPool())if (jnt.active())cp.push_back(&jnt);
			for (auto &mot : model().motionPool())if (mot.active()) cp.push_back(&mot);
			for (auto &gmt : model().generalMotionPool())if (gmt.active())cp.push_back( &gmt);
			
			// make relation pool //
			relationPool().clear();
			for (auto c : cp)
			{
				auto ret = std::find_if(relationPool().begin(), relationPool().end(), [&c](Relation &relation)
				{
					const auto ri{ relation.prtI }, rj{ relation.prtJ }, ci{ &c->makI().fatherPart() }, cj{ &c->makJ().fatherPart() };
					return ((ri == ci) && (rj == cj)) || ((ri == cj) && (rj == ci));
				});

				if (ret == relationPool().end()) relationPool().push_back(Relation{ &c->makI().fatherPart(), &c->makJ().fatherPart(), c->dim(),{ { c, true } } });
				else
				{
					ret->dim += c->dim();
					ret->cst_pool_.push_back({ c, &c->makI().fatherPart() == ret->prtI });
				}
			}
			
			// adjust order //
			for (Size i = 0; i < std::min(activePartPool().size(), relationPool().size()); ++i)
			{
				// 先对part排序，找出下一个跟上一个part联系的part
				std::sort(activePartPool().begin() + i, activePartPool().end(), [i, this](Part* a, Part* b)
				{
					if (a == &model().ground()) return true;
					if (b == &model().ground()) return false;
					if (i == 0)return a->id() < b->id();
					if (b == relationPool()[i - 1].prtI) return false;
					if (b == relationPool()[i - 1].prtJ) return false;
					if (a == relationPool()[i - 1].prtI) return true;
					if (a == relationPool()[i - 1].prtJ) return true;
					return a->id() < b->id();
				});
				// 再插入连接新part的relation
				std::sort(relationPool().begin() + i, relationPool().end(), [i, this](Relation a, Relation b)
				{
					auto pend = activePartPool().begin() + i + 1;
					auto a_part_i = std::find_if(activePartPool().begin(), pend, [a](Part* p)->bool { return p == a.prtI; });
					auto a_part_j = std::find_if(activePartPool().begin(), pend, [a](Part* p)->bool { return p == a.prtJ; });
					auto b_part_i = std::find_if(activePartPool().begin(), pend, [b](Part* p)->bool { return p == b.prtI; });
					auto b_part_j = std::find_if(activePartPool().begin(), pend, [b](Part* p)->bool { return p == b.prtJ; });

					bool a_is_ok = (a_part_i == pend) != (a_part_j == pend);
					bool b_is_ok = (b_part_i == pend) != (b_part_j == pend);

					if (a_is_ok && !b_is_ok) return true;
					else if (!a_is_ok && b_is_ok) return false;
					else if (a.dim != b.dim)return a.dim > b.dim;
					else return false;
				});
			}
			
			// make diag pool //
			diagPool().clear();
			diagPool().resize(activePartPool().size());
			diagPool().at(0).is_I = true;
			diagPool().at(0).rel = nullptr;
			diagPool().at(0).part = &model().ground();
			diagPool().at(0).rd = &diagPool().at(0);
			std::fill_n(diagPool().at(0).b, 6, 0.0);
			std::fill_n(diagPool().at(0).x, 6, 0.0);
			std::fill_n(diagPool().at(0).cm, 36, 0.0);
			for (Size i{ 0 }; i < 6; ++i)diagPool().at(0).cm[i * 6 + i] = 1.0;
			for (Size i = 1; i < diagPool().size(); ++i)
			{
				diagPool().at(i).rel = &relationPool().at(i - 1);
				diagPool().at(i).is_I = relationPool().at(i - 1).prtI == activePartPool().at(i);
				diagPool().at(i).part = diagPool().at(i).is_I ? relationPool().at(i - 1).prtI : relationPool().at(i - 1).prtJ;
				auto add_part = diagPool().at(i).is_I ? diagPool().at(i).rel->prtJ : diagPool().at(i).rel->prtI;
				diagPool().at(i).rd = &*std::find_if(diagPool().begin(), diagPool().end(), [&](Diag &d) {return d.part == add_part; });
				
			}
			
			// make remainder pool //
			remainderPool().clear();
			remainderPool().resize(relationPool().size() - activePartPool().size() + 1);
			for (Size i = 0; i < remainderPool().size(); ++i) 
			{
				auto &r = remainderPool().at(i);

				r.rel = &relationPool().at(i + diagPool().size() - 1);
				r.cm_blk_series.clear();
				r.cm_blk_series.push_back(Remainder::Block());
				r.cm_blk_series.back().diag = &*std::find_if(diagPool().begin(), diagPool().end(), [&r](Diag&d) {return r.rel->prtI == d.part; });
				r.cm_blk_series.back().is_I = true;
				r.cm_blk_series.push_back(Remainder::Block());
				r.cm_blk_series.back().diag = &*std::find_if(diagPool().begin(), diagPool().end(), [&r](Diag&d) {return r.rel->prtJ == d.part; });
				r.cm_blk_series.back().is_I = false;

				for (auto rd = diagPool().rbegin(); rd < diagPool().rend(); ++rd)
				{
					auto &d = *rd;
					
					// 判断是不是地 //
					if (d.rel)
					{
						auto diag_part = d.is_I ? d.rel->prtI : d.rel->prtJ;
						auto add_part = d.is_I ? d.rel->prtJ : d.rel->prtI;

						// 判断当前remainder加法元素是否存在（不为0）
						auto diag_blk = std::find_if(r.cm_blk_series.begin(), r.cm_blk_series.end(), [&](Remainder::Block &blk) {return blk.diag->part == diag_part; });
						auto add_blk = std::find_if(r.cm_blk_series.begin(), r.cm_blk_series.end(), [&](Remainder::Block &blk) {return blk.diag->part == add_part; });
						if (diag_blk != r.cm_blk_series.end())
						{
							if (add_blk != r.cm_blk_series.end())
							{
								r.cm_blk_series.erase(add_blk);
							}
							else
							{
								Remainder::Block blk;
								blk.is_I = diag_blk->is_I;
								
								blk.diag = &*std::find_if(diagPool().begin(), diagPool().end(), [&](Diag&d) {return d.part == add_part; });
								
								r.cm_blk_series.push_back(blk);
							}
						}
					}
				}
			}
			
			// allocate memory //
			imp_->rows = 0;
			imp_->cols = 0;
			for (auto &d : diagPool()) 
			{
				d.rows = 0;
				if (d.rel && d.rel->dim < 6)
				{
					d.rows = imp_->rows;
					imp_->rows += 6 - d.rel->dim;
				}
			}
			for (auto &r : remainderPool()) imp_->cols += r.rel->dim ;

			imp_->A_.clear();
			imp_->A_.resize(imp_->rows*imp_->cols, 0.0);
			imp_->x_.clear();
			imp_->x_.resize(std::max(imp_->rows, imp_->cols) * 1, 0.0);
			imp_->b_.clear();
			imp_->b_.resize(std::max(imp_->rows, imp_->cols) * 1, 0.0);
			imp_->U_.clear();
			imp_->U_.resize(imp_->rows*imp_->cols, 0.0);
			imp_->tau_.clear();
			imp_->tau_.resize(std::max(imp_->rows, imp_->cols), 0.0);
			imp_->p_.clear();
			imp_->p_.resize(std::max(imp_->rows, imp_->cols), 0);
		}
		auto DiagSolver::kinPos()->void 
		{
			double pm[16]{ 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
			s_mc(4, 4, pm, const_cast<double *>(*model().ground().pm()));

			setIterCount(0);
			
			for (; iterCount() < maxIterCount(); setIterCount(iterCount() + 1))
			{
				updDiagCp();
				updRemainderCp();
				
				double error{ 0.0 };
				for (auto d = diagPool().begin() + 1; d < diagPool().end(); ++d)for (Size i{ 0 }; i < d->rel->dim; ++i)error = std::max(error, std::abs(d->b[i]));
				for (auto &r : remainderPool())for (Size i{ 0 }; i < r.rel->dim; ++i)error = std::max(error, std::abs(r.b[i]));
				if (error < maxError())return;

				updDiagCm();
				updRemainderCm();
				updA();
				updB();
				updX();

				for (auto &x : imp_->x_)
				{
					x = std::min(x, std::max(error, 1.0));
					x = std::max(x, std::min(-error, -1.0));
				}

				// 将x写入diag, 重新乘Q, 并反向做行变换
				for (auto d = diagPool().begin() + 1; d<diagPool().end(); ++d)
				{
					double tem[6];
					s_vc(6 - d->rel->dim, imp_->x_.data() + d->rows, d->x + d->rel->dim);
					///////////////////////////////////////
					s_mm(6, 1, 6, d->Q, d->x, tem);
					////
					//s_householder_ut_q_dot(6, d->rel->dim, 1, d->U, d->tau, d->x, tem);
					///////////////////////////////////////
					s_vc(6, tem, d->x);
					s_va(6, d->rd->x, d->x);
				}

				// 将速度更新为矩阵
				for (auto d = diagPool().begin() + 1; d<diagPool().end(); ++d)
				{
					double pm[4][4];
					double pq[7];

					s_vc(6, d->x, pq);

					double theta = s_norm(3, pq + 3);
					pq[6] = std::cos(theta / 2);

					double factor = theta < 1e-4 ? 0.5 : std::sin(theta / 2) / theta;
					s_nv(3, factor, pq + 3);

					s_pq2pm(pq, *pm);

					double final_pm[4][4];
					s_pm2pm(*pm, *d->part->pm(), *final_pm);

					d->part->setPm(*final_pm);
				}
			}
		}
		auto DiagSolver::kinVel()->void 
		{
			s_fill(6, 1, 0.0, const_cast<double *>(model().ground().vs()));
			// make A
			updDiagCm();
			updRemainderCm();
			updA();
			// make b
			updDiagCv();
			updRemainderCv();
			updB();
			// using qr to solve x
			updX();

			// 将x写入diag, 重新乘Q, 并反向做行变换
			for (auto d = diagPool().begin() + 1; d<diagPool().end(); ++d)
			{
				double tem[6];
				s_vc(6 - d->rel->dim, imp_->x_.data() + d->rows, d->x + d->rel->dim);
				///////////////////////////////////////
				s_mm(6, 1, 6, d->Q, d->x, tem);
				////
				//s_householder_ut_q_dot(6, d->rel->dim, 1, d->U, d->tau, d->x, tem);
				///////////////////////////////////////
				s_vc(6, tem, d->x);
				s_va(6, d->rd->x, d->x);
			}

			// 将x更新到杆件速度
			for (auto d = diagPool().begin() + 1; d<diagPool().end(); ++d)
			{
				s_va(6, d->x, const_cast<double*>(d->part->vs()));
			}
		}
		auto DiagSolver::kinAcc()->void 
		{
			s_fill(6, 1, 0.0, const_cast<double *>(model().ground().as()));
			
			// make A
			updDiagCm();
			updRemainderCm();
			updA();
			// make b
			updDiagCa();
			updRemainderCa();
			updB();
			// using qr to solve x
			updX();

			// 将x写入diag, 重新乘Q, 并反向做行变换
			for (auto d = diagPool().begin() + 1; d<diagPool().end(); ++d)
			{
				s_mc(6 - d->rel->dim, 1, imp_->x_.data() + d->rows, d->x + d->rel->dim);
				//////////////////////////////////
				s_mm(6, 1, 6, d->Q, d->x, const_cast<double *>(d->part->as()));
				//////
				//s_householder_ut_q_dot(6, d->rel->dim, 1, d->U, d->tau, d->x, const_cast<double *>(d->part->as()));
				//////////////////////////////////
				s_ma(6, 1, d->rd->part->as(), const_cast<double *>(d->part->as()));
			}
		}
		auto DiagSolver::dynFce()->void 
		{
			// make A
			updDiagCm();
			updRemainderCm();
			updA();
			// make b
			updDiagPf();
			updBf();
			// using qr to solve x
			updXf();
		}
		auto DiagSolver::updDiagCm()->void
		{
			// upd diag cm data //
			for (auto d = diagPool().begin() + 1; d < diagPool().end(); ++d)
			{
				Size pos{ 0 };
				for (auto &c : d->rel->cst_pool_)
				{
					double cm[36];
					double *cmI = d->is_I ? d->cm : cm;
					double *cmJ = d->is_I ? cm : d->cm;

					c.constraint->cptGlbCm(cmI + pos, d->rel->dim, cmJ + pos, d->rel->dim);
					pos += c.constraint->dim();
					
					// make ut and qr
					s_householder_ut(6, d->rel->dim, d->cm, d->U, d->tau);
					s_householder_ut2qr(6, d->rel->dim, d->U, d->tau, d->Q, d->R);
				}
			}
		}
		auto DiagSolver::updDiagCp()->void 
		{
			for (auto d = diagPool().begin() + 1; d < diagPool().end(); ++d)
			{
				Size pos{ 0 };
				for (auto &c : d->rel->cst_pool_)
				{
					c.constraint->cptCp(d->b + pos);
					pos += c.constraint->dim();
				}
			}
		}
		auto DiagSolver::updDiagCv()->void 
		{
			for (auto d = diagPool().begin() + 1; d < diagPool().end(); ++d)
			{
				Size pos{ 0 };
				for (auto &c : d->rel->cst_pool_)
				{
					c.constraint->cptCv(d->b + pos);
					pos += c.constraint->dim();
				}
			}
		}
		auto DiagSolver::updDiagCa()->void 
		{
			for (auto d = diagPool().begin() + 1; d < diagPool().end(); ++d)
			{
				Size pos{ 0 };
				for (auto &c : d->rel->cst_pool_)
				{
					c.constraint->cptCa(d->b + pos);
					pos += c.constraint->dim();
				}
			}
		}
		auto DiagSolver::updDiagPf()->void 
		{
			for (auto d = diagPool().begin() + 1; d < diagPool().end(); ++d)
			{
				double prt_as[6], prt_f[6];
				
				d->part->cptPrtPf(prt_f);
				d->part->cptPrtAs(prt_as);
				s_mms(6, 1, 6, *d->part->prtIm(), prt_as, prt_f);
				s_tf(*d->part->pm(), prt_f, d->b);
			}
		}
		auto DiagSolver::updRemainderCm()->void
		{
			// upd remainder data //
			for (auto &r : remainderPool())
			{
				Size pos{ 0 };
				for (auto &c : r.rel->cst_pool_)
				{
					c.constraint->cptGlbCm(r.cmI + pos, r.rel->dim, r.cmJ + pos, r.rel->dim);
					pos += c.constraint->dim();
				}
			}
		}
		auto DiagSolver::updRemainderCp()->void
		{
			// upd remainder data //
			for (auto &r : remainderPool())
			{
				Size pos{ 0 };
				for (auto &c : r.rel->cst_pool_)
				{
					c.constraint->cptCp(r.b + pos);
					pos += c.constraint->dim();
				}
			}
		}
		auto DiagSolver::updRemainderCv()->void
		{
			// upd remainder data //
			for (auto &r : remainderPool())
			{
				Size pos{ 0 };
				for (auto &c : r.rel->cst_pool_)
				{
					c.constraint->cptCv(r.b + pos);
					pos += c.constraint->dim();
				}
			}
		}
		auto DiagSolver::updRemainderCa()->void
		{
			// upd remainder data //
			for (auto &r : remainderPool())
			{
				Size pos{ 0 };
				for (auto &c : r.rel->cst_pool_)
				{
					c.constraint->cptCa(r.b + pos);
					pos += c.constraint->dim();
				}
			}
		}
		auto DiagSolver::updA()->void
		{
			Size cols{ 0 };
			for (auto &r : remainderPool())
			{
				for (auto &b : r.cm_blk_series)
				{
					/////////////////////////////
					s_mm(6 - b.diag->rel->dim, r.rel->dim, 6, b.diag->Q + dynamic::id(0, b.diag->rel->dim, 6), ColMajor{ 6 }, b.is_I ? r.cmI : r.cmJ, r.rel->dim, imp_->A_.data() + dynamic::id(b.diag->rows, cols, imp_->cols), imp_->cols);
					/////////////////////////////
					//double tem[36];
					//s_householder_ut_qt_dot(6, b.diag->rel->dim, r.rel->dim, b.diag->U, b.diag->tau, b.is_I ? r.cmI : r.cmJ, tem);
					//s_mc(6 - b.diag->rel->dim, r.rel->dim, tem + dynamic::id(b.diag->rel->dim, 0, r.rel->dim), r.rel->dim, imp_->A_.data() + dynamic::id(b.diag->rows, cols, imp_->cols), imp_->cols);
					/////////////////////////////
				}
				cols += r.rel->dim;
			}
		}
		auto DiagSolver::updB()->void
		{
			// 求解对角线上的未知数
			for (auto d = diagPool().begin() + 1; d<diagPool().end(); ++d)
			{
				s_fill(6, 1, 0.0, d->x);
				s_sov_lm(d->rel->dim, 1, d->U, ColMajor{ d->rel->dim }, d->b, 1, d->x, 1);
			}
			// 使用已求出的未知数，用以构建b
			Size cols{ 0 };
			for (auto &r : remainderPool())
			{
				for (auto &b : r.cm_blk_series)
				{
					double tem[6];
					auto cm = b.is_I ? r.cmJ : r.cmI;//这里是颠倒的，因为加到右侧需要乘-1.0
					/////////////////////////////////////////////
					s_mm(6, 1, b.diag->rel->dim, b.diag->Q, 6, b.diag->x, 1, tem, 1);
					//s_householder_ut_q_dot(6, b.diag->rel->dim, 1, b.diag->U, b.diag->tau, b.diag->x, tem);
					/////////////////////////////////////////////
					s_mma(r.rel->dim, 1, 6, cm, ColMajor{ r.rel->dim }, tem, 1, r.b, 1);
				}
				s_mc(r.rel->dim, 1, r.b, imp_->b_.data() + cols);
				cols += r.rel->dim;
			}
		}
		auto DiagSolver::updX()->void
		{
			auto &a = imp_;
			
			// 求解x
			//s_householder_ut(imp_->cols, imp_->rows, imp_->A_.data(), ColMajor{ imp_->cols }, imp_->U_.data(), ColMajor{ imp_->cols }, imp_->tau_.data(), 1);
			//s_householder_ut_sov(imp_->cols, imp_->rows, 1, imp_->U_.data(), ColMajor{ imp_->cols }, imp_->tau_.data(), 1, imp_->b_.data(), 1, imp_->x_.data(), 1);

			s_householder_utp(imp_->cols, imp_->rows, imp_->A_.data(), ColMajor{ imp_->cols }, imp_->U_.data(), ColMajor{ imp_->cols }, imp_->tau_.data(), 1, imp_->p_.data());
			s_householder_utp_sov(imp_->cols, imp_->rows, 1, imp_->U_.data(), ColMajor{ imp_->cols }, imp_->tau_.data(), 1, imp_->p_.data(), imp_->b_.data(), 1, imp_->x_.data(), 1);
		}
		auto DiagSolver::updBf()->void
		{
			for (auto d = diagPool().rbegin(); d<diagPool().rend() -1; ++d)
			{
				// 做行变换
				s_va(6, d->b, d->rd->b);

				// dot Q //
				double tem[6];
				//////////////////////////////////////
				s_mm(6, 1, 6, d->Q, ColMajor{ 6 }, d->b, 1, tem, 1);
				//s_householder_ut_qt_dot(6, d->rel->dim, 1, d->U, d->tau, d->b, tem);
				//////////////////////////////////////
				s_vc(6, tem, d->b);
				s_vc(6 - d->rel->dim, d->b + d->rel->dim, imp_->b_.data() + d->rows);
			}
		}
		auto DiagSolver::updXf()->void
		{
			//s_householder_ut(imp_->rows, imp_->cols, imp_->A_.data(), imp_->U_.data(), imp_->tau_.data(), maxError());
			//s_householder_ut_sov(imp_->rows, imp_->cols, 1, imp_->U_.data(), imp_->tau_.data(), imp_->b_.data(), imp_->x_.data(), maxError());

			s_householder_utp(imp_->rows, imp_->cols, imp_->A_.data(), imp_->U_.data(), imp_->tau_.data(), imp_->p_.data(), maxError());
			s_householder_utp_sov(imp_->rows, imp_->cols, 1, imp_->U_.data(), imp_->tau_.data(), imp_->p_.data(), imp_->b_.data(), imp_->x_.data(), maxError());

			// 将已经求出的x更新到remainder中，此后将已知数移到右侧
			Size cols{ 0 };
			for (auto &r : remainderPool())
			{
				Size pos{ 0 };
				for (auto &c : r.rel->cst_pool_)
				{
					c.constraint->setCf(imp_->x_.data() + cols + pos);
					pos += c.constraint->dim();
				}
				for (auto &b : r.cm_blk_series)
				{
					double tem[6];
					s_mm(6, 1, r.rel->dim, b.is_I ? r.cmJ : r.cmI, imp_->x_.data() + cols, tem);
					/////////////////////////////////////////
					s_mma(6, 1, 6, b.diag->Q, ColMajor{ 6 }, tem, 1, b.diag->b, 1);
					/////////////////////////////////////////
					//double tem2[6];
					//s_householder_ut_qt_dot(6, b.diag->rel->dim, 1, b.diag->U, b.diag->tau, tem, tem2);
					//s_ma(6, 1, tem2, b.diag->b);
					////////////////////////////////////////
				}
				
				cols += r.rel->dim;
			}

			for (auto d = diagPool().begin() + 1; d < diagPool().end(); ++d)
			{
				s_sov_um(d->rel->dim, 1, d->U, d->b, d->x);
				Size pos{ 0 };
				for (auto &c : d->rel->cst_pool_)
				{
					c.constraint->setCf(d->x + pos);
					pos += c.constraint->dim();
				}
			}

		}
		auto DiagSolver::relationPool()->std::vector<Relation>& { return imp_->relation_pool_; }
		auto DiagSolver::activePartPool()->std::vector<Part*>& { return imp_->part_pool_; }
		auto DiagSolver::diagPool()->std::vector<Diag>& { return imp_->diag_pool_; }
		auto DiagSolver::remainderPool()->std::vector<Remainder>& { return imp_->remainder_pool_; }
		auto DiagSolver::plotRelation()->void
		{
			std::size_t name_size{ 0 };
			for (auto prt : activePartPool())
			{
				name_size = std::max(prt->name().size(), name_size);
			}
			
			for (auto prt : activePartPool())
			{
				std::string s(name_size, ' ');
				s.replace(0, prt->name().size(), prt->name().data());

				std::cout << s << ":";
				
				if (prt == &model().ground())
				{
					std::cout << "  6x6 ";
				}
				else
					std::cout << "      ";



				for (auto &rel : relationPool())
				{
					std::cout << " ";
					if (rel.prtI == prt)
						std::cout << " 6x" << rel.dim;
					else if (rel.prtJ == prt)
						std::cout << "-6x" << rel.dim;
					else
						std::cout << "    ";

					std::cout << " ";
				}
				std::cout << std::endl;
			}
		}
		auto DiagSolver::plotDiag()->void
		{
			std::size_t name_size{ 0 };
			for (auto prt : activePartPool())
			{
				name_size = std::max(prt->name().size(), name_size);
			}
			
			for (auto prt : activePartPool())
			{
				std::string s(name_size, ' ');
				s.replace(0, prt->name().size(), prt->name().data());

				std::cout << s << ":";
				
				
				
				
				for (auto &d : diagPool())
				{
					if (d.rel == nullptr)
					{
						if (prt == &model().ground())
						{
							std::cout << "  6x6 ";
						}
						else
							std::cout << "      ";

						continue;
					}

					auto &rel = *d.rel;
					std::cout << " ";
					if (d.is_I && rel.prtI == prt)
					{
						std::cout << " 6x" << rel.dim;
					}
					else if (!d.is_I && rel.prtJ == prt)
					{
						std::cout << "-6x" << rel.dim;
					}
					else
						std::cout << "    ";
					std::cout << " ";

				}
				std::cout << std::endl;
			}
		}
		auto DiagSolver::plotRemainder()->void
		{
			std::size_t name_size{ 0 };
			for (auto prt : activePartPool())
			{
				name_size = std::max(prt->name().size(), name_size);
			}
			
			for (auto prt : activePartPool())
			{
				std::string s(name_size, ' ');
				s.replace(0, prt->name().size(), prt->name().data());

				std::cout << s << ":";

				for (auto &r : remainderPool())
				{
					std::cout << " ";

					bool found{ false };
					for (auto blk : r.cm_blk_series)
					{
						if (prt == blk.diag->part)
						{
							found = true;
							if (blk.is_I)
								std::cout << " 6x" << r.rel->dim;
							else
								std::cout << "-6x" << r.rel->dim;
						}
					}
					if (!found)std::cout << "    ";

					std::cout << " ";
				}
				std::cout << std::endl;
			}
		}
		DiagSolver::~DiagSolver() = default;
		DiagSolver::DiagSolver(const std::string &name, Size max_iter_count, double max_error) :Solver(name, max_iter_count, max_error){}
		DiagSolver::DiagSolver(const DiagSolver &other) = default;
		DiagSolver::DiagSolver(DiagSolver &&other) = default;
		DiagSolver& DiagSolver::operator=(const DiagSolver &other) = default;
		DiagSolver& DiagSolver::operator=(DiagSolver &&other) = default;

		struct SolverSimulator::Imp
		{
			Solver *solver_;

			Imp(Solver *solver) :solver_(solver) { };
		};
		auto SolverSimulator::saveXml(aris::core::XmlElement &xml_ele) const->void
		{
			Simulator::saveXml(xml_ele);
			xml_ele.SetAttribute("solver", solver().name().c_str());
		}
		auto SolverSimulator::loadXml(const aris::core::XmlElement &xml_ele)->void
		{
			Simulator::loadXml(xml_ele);
			
			if (model().findByName("solver_pool") == model().children().end())
				throw std::runtime_error("you must insert \"solver_pool\" node before insert " + type() + " \"" + name() + "\"");

			auto &solver_pool = static_cast<aris::core::ObjectPool<Solver, Element>&>(*model().findByName("solver_pool"));

			if (!xml_ele.Attribute("solver"))throw std::runtime_error(std::string("xml element \"") + name() + "\" must have Attribute \"solver\"");
			auto s = solver_pool.findByName(xml_ele.Attribute("solver"));
			if (s == solver_pool.end())	throw std::runtime_error(std::string("can't find solver for element \"") + this->name() + "\"");

			imp_->solver_ = &*s;
		}
		auto SolverSimulator::solver()->Solver& { return *imp_->solver_; }
		auto SolverSimulator::simulate(const PlanFunction &plan, void *param, std::uint32_t param_size, SimResult &result)->void
		{
			solver().allocateMemory();
			result.allocateMemory();
			// 记录初始位置 //
			result.record();
			// 记录轨迹中的位置 //
			for (PlanParam plan_param{ &model(), std::uint32_t(1), param, param_size }; plan(plan_param) != 0; ++plan_param.count_)
			{
				solver().kinPos();
				if (solver().iterCount() == solver().maxIterCount())throw std::runtime_error("simulate failed because kinPos() failed at " + std::to_string(plan_param.count_) + " count");
				solver().kinVel();
				solver().dynAccAndFce();
				result.record();
			}
			// 记录结束位置 //
			result.record();
			result.restore(0);
		}
		SolverSimulator::~SolverSimulator() = default;
		SolverSimulator::SolverSimulator(const std::string &name, Solver *solver) : Simulator(name), imp_(new Imp(solver)) {}
		SolverSimulator::SolverSimulator(const SolverSimulator&) = default;
		SolverSimulator::SolverSimulator(SolverSimulator&&) = default;
		SolverSimulator& SolverSimulator::operator=(const SolverSimulator&) = default;
		SolverSimulator& SolverSimulator::operator=(SolverSimulator&&) = default;

		struct AdamsSimulator::Imp{};
		auto AdamsSimulator::saveAdams(const std::string &filename, SimResult &result, Size pos)->void
		{
			std::string filename_ = filename;
			if (filename_.size() < 4 || filename_.substr(filename.size() - 4, 4) != ".cmd")
			{
				filename_ += ".cmd";
			}

			std::ofstream file;
			file.open(filename_, std::ios::out | std::ios::trunc);

			saveAdams(file, result, pos);

			file.close();
		}
		auto AdamsSimulator::saveAdams(std::ofstream &file, SimResult &result, Size pos)->void
		{
			// 生成akima曲线 //
			std::vector<double> time(result.size() + 1);
			std::vector<std::vector<double>> mot_akima(model().motionPool().size(), std::vector<double>(result.size() + 1));
			std::vector<std::vector<std::array<double, 6>>> gm_akima(model().generalMotionPool().size(), std::vector<std::array<double, 6>>(result.size() + 1));
			if (pos == -1)
			{
				if (result.size() < 4)throw std::runtime_error("failed to AdamsSimulator::saveAdams: because result size is smaller than 4\n");

				for (Size i(-1); ++i < result.size() + 1;)
				{
					result.restore(i);
					time.at(i) = model().time();
					for (Size j(-1); ++j < model().motionPool().size();)
					{
						model().motionPool().at(j).updMp();
						mot_akima.at(j).at(i) = model().motionPool().at(j).mp();
					}
					for (Size j(-1); ++j < model().generalMotionPool().size();)
					{
						model().generalMotionPool().at(j).updMpm();
						model().generalMotionPool().at(j).getMpe(gm_akima.at(j).at(i).data(), "123");
					}
				}
			}

			// 生成ADAMS模型
			result.restore(pos == -1 ? 0 : pos);
			file << "!----------------------------------- Environment -------------------------------!\r\n!\r\n!\r\n";
			file << "!-------------------------- Default Units for Model ---------------------------!\r\n"
				<< "!\r\n"
				<< "!\r\n"
				<< "defaults units  &\r\n"
				<< "    length = meter  &\r\n"
				<< "    angle = rad  &\r\n"
				<< "    force = newton  &\r\n"
				<< "    mass = kg  &\r\n"
				<< "    time = sec\r\n"
				<< "!\n"
				<< "defaults units  &\r\n"
				<< "    coordinate_system_type = cartesian  &\r\n"
				<< "    orientation_type = body313\r\n"
				<< "!\r\n"
				<< "!------------------------ Default Attributes for Model ------------------------!\r\n"
				<< "!\r\n"
				<< "!\r\n"
				<< "defaults attributes  &\r\n"
				<< "    inheritance = bottom_up  &\r\n"
				<< "    icon_visibility = off  &\r\n"
				<< "    grid_visibility = off  &\r\n"
				<< "    size_of_icons = 5.0E-002  &\r\n"
				<< "    spacing_for_grid = 1.0\r\n"
				<< "!\r\n"
				<< "!------------------------------ Adams/View Model ------------------------------!\r\n"
				<< "!\r\n"
				<< "!\r\n"
				<< "model create  &\r\n"
				<< "   model_name = " << this->model().name() << "\r\n"
				<< "!\r\n"
				<< "view erase\r\n"
				<< "!\r\n"
				<< "!---------------------------------- Accgrav -----------------------------------!\r\n"
				<< "!\r\n"
				<< "!\r\n"
				<< "force create body gravitational  &\r\n"
				<< "    gravity_field_name = gravity  &\r\n"
				<< "    x_component_gravity = " << model().environment().gravity()[0] << "  &\r\n"
				<< "    y_component_gravity = " << model().environment().gravity()[1] << "  &\r\n"
				<< "    z_component_gravity = " << model().environment().gravity()[2] << "\r\n"
				<< "!\r\n";
			for (auto &part : model().partPool())
			{
				if (&part == &model().ground())
				{
					file << "!----------------------------------- ground -----------------------------------!\r\n"
						<< "!\r\n"
						<< "!\r\n"
						<< "! ****** Ground Part ******\r\n"
						<< "!\r\n"
						<< "defaults model  &\r\n"
						<< "    part_name = ground\r\n"
						<< "!\r\n"
						<< "defaults coordinate_system  &\r\n"
						<< "    default_coordinate_system = ." << model().name() << ".ground\r\n"
						<< "!\r\n"
						<< "! ****** Markers for current part ******\r\n"
						<< "!\r\n";
				}
				else
				{
					double pe[6];
					s_pm2pe(*part.pm(), pe, "313");
					core::Matrix ori(1, 3, &pe[3]), loc(1, 3, &pe[0]);

					file << "!----------------------------------- " << part.name() << " -----------------------------------!\r\n"
						<< "!\r\n"
						<< "!\r\n"
						<< "defaults coordinate_system  &\r\n"
						<< "    default_coordinate_system = ." << model().name() << ".ground\r\n"
						<< "!\r\n"
						<< "part create rigid_body name_and_position  &\r\n"
						<< "    part_name = ." << model().name() << "." << part.name() << "  &\r\n"
						<< "    adams_id = " << adamsID(part) << "  &\r\n"
						<< "    location = (" << loc.toString() << ")  &\r\n"
						<< "    orientation = (" << ori.toString() << ")\r\n"
						<< "!\r\n"
						<< "defaults coordinate_system  &\r\n"
						<< "    default_coordinate_system = ." << model().name() << "." << part.name() << " \r\n"
						<< "!\r\n";


					double mass = part.prtIm()[0][0] == 0 ? 1 : part.prtIm()[0][0];
					std::fill_n(pe, 6, 0);
					pe[0] = part.prtIm()[1][5] / mass;
					pe[1] = -part.prtIm()[0][5] / mass;
					pe[2] = part.prtIm()[0][4] / mass;

					file << "! ****** cm and mass for current part ******\r\n"
						<< "marker create  &\r\n"
						<< "    marker_name = ." << model().name() << "." << part.name() << ".cm  &\r\n"
						<< "    adams_id = " << adamsID(part) + model().markerSize() << "  &\r\n"
						<< "    location = ({" << pe[0] << "," << pe[1] << "," << pe[2] << "})  &\r\n"
						<< "    orientation = (" << "{0,0,0}" << ")\r\n"
						<< "!\r\n";

					double pm[16];
					double im[6][6];

					pe[0] = -pe[0];
					pe[1] = -pe[1];
					pe[2] = -pe[2];

					s_pe2pm(pe, pm);
					s_im2im(pm, *part.prtIm(), *im);

					//！注意！//
					//Adams里对惯量矩阵的定义貌似和我自己的定义在Ixy,Ixz,Iyz上互为相反数。别问我为什么,我也不知道。
					file << "part create rigid_body mass_properties  &\r\n"
						<< "    part_name = ." << model().name() << "." << part.name() << "  &\r\n"
						<< "    mass = " << part.prtIm()[0][0] << "  &\r\n"
						<< "    center_of_mass_marker = ." << model().name() << "." << part.name() << ".cm  &\r\n"
						<< "    inertia_marker = ." << model().name() << "." << part.name() << ".cm  &\r\n"
						<< "    ixx = " << im[3][3] << "  &\r\n"
						<< "    iyy = " << im[4][4] << "  &\r\n"
						<< "    izz = " << im[5][5] << "  &\r\n"
						<< "    ixy = " << -im[4][3] << "  &\r\n"
						<< "    izx = " << -im[5][3] << "  &\r\n"
						<< "    iyz = " << -im[5][4] << "\r\n"
						<< "!\r\n";
				}

				//导入marker
				for (auto &marker : part.markerPool())
				{
					double pe[6];

					s_pm2pe(*marker.prtPm(), pe, "313");
					core::Matrix ori(1, 3, &pe[3]), loc(1, 3, &pe[0]);

					file << "marker create  &\r\n"
						<< "marker_name = ." << model().name() << "." << part.name() << "." << marker.name() << "  &\r\n"
						<< "adams_id = " << adamsID(marker) << "  &\r\n"
						<< "location = (" << loc.toString() << ")  &\r\n"
						<< "orientation = (" << ori.toString() << ")\r\n"
						<< "!\r\n";
				}
				for (auto &geometry : part.geometryPool())
				{
					if (ParasolidGeometry* geo = dynamic_cast<ParasolidGeometry*>(&geometry))
					{
						double pe[6];
						s_pm2pe(*geo->prtPm(), pe, "313");
						core::Matrix ori(1, 3, &pe[3]), loc(1, 3, &pe[0]);

						file << "file parasolid read &\r\n"
							<< "	file_name = \"" << geo->filePath() << "\" &\r\n"
							<< "	type = ASCII" << " &\r\n"
							<< "	part_name = " << part.name() << " &\r\n"
							<< "	location = (" << loc.toString() << ") &\r\n"
							<< "	orientation = (" << ori.toString() << ") &\r\n"
							<< "	relative_to = ." << model().name() << "." << part.name() << " \r\n"
							<< "\r\n";
					}
					else
					{
						throw std::runtime_error("unrecognized geometry type:" + geometry.type());
					}

				}
			}
			for (auto &joint : model().jointPool())
			{
				std::string type;
				if (dynamic_cast<RevoluteJoint*>(&joint))type = "revolute";
				else if (dynamic_cast<PrismaticJoint*>(&joint))type = "translational";
				else if (dynamic_cast<UniversalJoint*>(&joint))type = "universal";
				else if (dynamic_cast<SphericalJoint*>(&joint))type = "spherical";
				else throw std::runtime_error("unrecognized joint type:" + joint.type());

				file << "constraint create joint " << type << "  &\r\n"
					<< "    joint_name = ." << model().name() << "." << joint.name() << "  &\r\n"
					<< "    adams_id = " << adamsID(joint) << "  &\r\n"
					<< "    i_marker_name = ." << model().name() << "." << joint.makI().fatherPart().name() << "." << joint.makI().name() << "  &\r\n"
					<< "    j_marker_name = ." << model().name() << "." << joint.makJ().fatherPart().name() << "." << joint.makJ().name() << "  \r\n"
					<< "!\r\n";
			}
			for (auto &motion : model().motionPool())
			{
				std::string axis_names[6]{ "x","y","z","B1","B2","B3" };
				std::string axis_name = axis_names[motion.axis()];

				std::string akima = motion.name() + "_akima";
				std::string akima_func = "AKISPL(time,0," + akima + ")";
				std::string polynomial_func = static_cast<const std::stringstream &>(std::stringstream() << std::setprecision(16) << motion.mp() << " + " << motion.mv() << " * time + " << motion.ma()*0.5 << " * time * time").str();

				// 构建akima曲线 //
				if (pos == -1)
				{
					file << "data_element create spline &\r\n"
						<< "    spline_name = ." << model().name() + "." + motion.name() + "_akima &\r\n"
						<< "    adams_id = " << adamsID(motion) << "  &\r\n"
						<< "    units = m &\r\n"
						<< "    x = " << time.at(0);
					for (auto p = time.begin() + 1; p < time.end(); ++p)
					{
						file << "," << *p;
					}
					file << "    y = " << mot_akima.at(motion.id()).at(0);
					for (auto p = mot_akima.at(motion.id()).begin() + 1; p < mot_akima.at(motion.id()).end(); ++p)
					{
						file << "," << *p;
					}
					file << " \r\n!\r\n";
				}

				file << "constraint create motion_generator &\r\n"
					<< "    motion_name = ." << model().name() << "." << motion.name() << "  &\r\n"
					<< "    adams_id = " << adamsID(motion) << "  &\r\n"
					<< "    i_marker_name = ." << model().name() << "." << motion.makI().fatherPart().name() << "." << motion.makI().name() << "  &\r\n"
					<< "    j_marker_name = ." << model().name() << "." << motion.makJ().fatherPart().name() << "." << motion.makJ().name() << "  &\r\n"
					<< "    axis = " << axis_name << "  &\r\n"
					<< "    function = \"" << (pos == -1 ? akima_func : polynomial_func) << "\"  \r\n"
					<< "!\r\n";
			}
			for (auto &gm : model().generalMotionPool())
			{
				file << "ude create instance  &\r\n"
					<< "    instance_name = ." << model().name() << "." << gm.name() << "  &\r\n"
					<< "    definition_name = .MDI.Constraints.general_motion  &\r\n"
					<< "    location = 0.0, 0.0, 0.0  &\r\n"
					<< "    orientation = 0.0, 0.0, 0.0  \r\n"
					<< "!\r\n";

				file << "variable modify  &\r\n"
					<< "	variable_name = ." << model().name() << "." << gm.name() << ".i_marker  &\r\n"
					<< "	object_value = ." << model().name() << "." << gm.makI().fatherPart().name() << "." << gm.makI().name() << " \r\n"
					<< "!\r\n";

				file << "variable modify  &\r\n"
					<< "	variable_name = ." << model().name() << "." << gm.name() << ".j_marker  &\r\n"
					<< "	object_value = ." << model().name() << "." << gm.makJ().fatherPart().name() << "." << gm.makJ().name() << " \r\n"
					<< "!\r\n";

				std::string axis_names[6]{ "t1", "t2", "t3", "r1", "r2", "r3" };

				double pe123[6], ve123[6], ae123[6];
				gm.getMpe(pe123, "123");
				gm.getMve(ve123, "123");
				gm.getMae(ae123, "123");
				for (Size i = 0; i < 6; ++i)
				{
					std::string akima = gm.name() + "_" + axis_names[i] + "_akima";
					std::string akima_func = "AKISPL(time,0," + akima + ")";
					std::string polynomial_func = static_cast<const std::stringstream &>(std::stringstream() << std::setprecision(16) << pe123[i] << " + " << ve123[i] << " * time + " << ae123[i] * 0.5 << " * time * time").str();
					std::string func = pos == -1 ? akima_func : polynomial_func;

					// 构建akima曲线 //
					if (pos == -1)
					{
						file << "data_element create spline &\r\n"
							<< "    spline_name = ." << model().name() + "." + akima + " &\r\n"
							<< "    adams_id = " << model().motionPool().size() + adamsID(gm) * 6 + i << "  &\r\n"
							<< "    units = m &\r\n"
							<< "    x = " << time.at(0);
						for (auto p = time.begin() + 1; p < time.end(); ++p)
						{
							file << "," << *p;
						}
						file << "    y = " << gm_akima.at(gm.id()).at(0).at(i);
						for (auto p = gm_akima.at(gm.id()).begin() + 1; p < gm_akima.at(gm.id()).end(); ++p)
						{
							file << "," << p->at(i);
						}
						file << " \r\n!\r\n";
					}

					file << "variable modify  &\r\n"
						<< "	variable_name = ." << model().name() << "." << gm.name() << "." << axis_names[i] << "_type  &\r\n"
						<< "	integer_value = 1 \r\n"
						<< "!\r\n";

					file << "variable modify  &\r\n"
						<< "	variable_name = ." << model().name() << "." << gm.name() << "." << axis_names[i] << "_func  &\r\n"
						<< "	string_value = \"" + func + "\" \r\n"
						<< "!\r\n";

					file << "variable modify  &\r\n"
						<< "	variable_name = ." << model().name() << "." << gm.name() << "." << axis_names[i] << "_ic_disp  &\r\n"
						<< "	real_value = 0.0 \r\n"
						<< "!\r\n";

					file << "variable modify  &\r\n"
						<< "	variable_name = ." << model().name() << "." << gm.name() << "." << axis_names[i] << "_ic_velo  &\r\n"
						<< "	real_value = 0.0 \r\n"
						<< "!\r\n";
				}

				file << "ude modify instance  &\r\n"
					<< "	instance_name = ." << model().name() << "." << gm.name() << "\r\n"
					<< "!\r\n";
			}
			for (auto &force : model().forcePool())
			{
				if (dynamic_cast<SingleComponentForce *>(&force))
				{
					std::string type = "translational";

					file << "force create direct single_component_force  &\r\n"
						<< "    single_component_force_name = ." << model().name() << "." << force.name() << "  &\r\n"
						<< "    adams_id = " << adamsID(force) << "  &\r\n"
						<< "    type_of_freedom = " << type << "  &\r\n"
						<< "    i_marker_name = ." << model().name() << "." << force.makI().fatherPart().name() << "." << force.makI().name() << "  &\r\n"
						<< "    j_marker_name = ." << model().name() << "." << force.makJ().fatherPart().name() << "." << force.makJ().name() << "  &\r\n"
						<< "    action_only = off  &\r\n"
						<< "    function = \"" << dynamic_cast<SingleComponentForce&>(force).fce() << "\"  \r\n"
						<< "!\r\n";
				}

			}

			file << "!----------------------------------- Motify Active -------------------------------------!\r\n!\r\n!\r\n";
			for (auto &prt : model().partPool())
			{
				if ((&prt != &model().ground()) && (!prt.active()))
				{
					file << "part attributes  &\r\n"
						<< "    part_name = ." << model().name() << "." << prt.name() << "  &\r\n"
						<< "    active = off \r\n!\r\n";
				}
			}
			for (auto &jnt : model().jointPool())
			{
				if (!jnt.active())
				{
					file << "constraint attributes  &\r\n"
						<< "    constraint_name = ." << model().name() << "." << jnt.name() << "  &\r\n"
						<< "    active = off \r\n!\r\n";
				}
			}
			for (auto &mot : model().motionPool())
			{
				if (!mot.active())
				{
					file << "constraint attributes  &\r\n"
						<< "    constraint_name = ." << model().name() << "." << mot.name() << "  &\r\n"
						<< "    active = off \r\n!\r\n";
				}
			}
			for (auto &gm : model().generalMotionPool())
			{
				if (!gm.active())
				{
					file << "ude attributes  &\r\n"
						<< "    instance_name = ." << model().name() << "." << gm.name() << "  &\r\n"
						<< "    active = off \r\n!\r\n";
				}
			}
			for (auto &fce : model().forcePool())
			{
				if (!fce.active())
				{
					file << "force attributes  &\r\n"
						<< "    force_name = ." << model().name() << "." << fce.name() << "  &\r\n"
						<< "    active = off \r\n!\r\n";
				}
			}
		}
		auto AdamsSimulator::adamsID(const Marker &mak)const->Size
		{
			Size size{ 0 };

			for (auto &prt : model().partPool())
			{
				if (&prt == &mak.fatherPart()) break;
				size += prt.markerPool().size();
			}

			size += mak.id() + 1;

			return size;
		}
		auto AdamsSimulator::adamsID(const Part &prt)const->Size { return (&prt == &model().ground()) ? 1 : prt.id() + (model().ground().id() < prt.id() ? 1 : 2); }
		AdamsSimulator::~AdamsSimulator() = default;
		AdamsSimulator::AdamsSimulator(const std::string &name, Solver *solver) : SolverSimulator(name, solver) {}
		AdamsSimulator::AdamsSimulator(const AdamsSimulator&) = default;
		AdamsSimulator::AdamsSimulator(AdamsSimulator&&) = default;
		AdamsSimulator& AdamsSimulator::operator=(const AdamsSimulator&) = default;
		AdamsSimulator& AdamsSimulator::operator=(AdamsSimulator&&) = default;
	}
}