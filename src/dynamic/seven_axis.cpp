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
#include <array>

#include "aris/dynamic/model.hpp"
#include "aris/dynamic/model_solver.hpp"
#include "aris/dynamic/seven_axis.hpp"

namespace aris::dynamic
{
	auto createModelSevenAxis(const SevenAxisParam &param)->std::unique_ptr<aris::dynamic::Model>
	{
		std::unique_ptr<aris::dynamic::Model> model = std::make_unique<aris::dynamic::Model>("model");

		// 设置重力 //
		const double gravity[6]{ 0.0,0.0,-9.8,0.0,0.0,0.0 };
		model->environment().setGravity(gravity);

		// 添加变量 //
		model->calculator().addVariable("PI", "Matrix", aris::core::Matrix(PI));

		// add part //
		const double default_iv[10]{ 1,0,0,0,0,0,0,0,0,0 };
		auto &p1 = model->partPool().add<Part>("L1", param.iv_vec.size() == 7 ? param.iv_vec[0].data() : default_iv);
		auto &p2 = model->partPool().add<Part>("L2", param.iv_vec.size() == 7 ? param.iv_vec[1].data() : default_iv);
		auto &p3 = model->partPool().add<Part>("L3", param.iv_vec.size() == 7 ? param.iv_vec[2].data() : default_iv);
		auto &p4 = model->partPool().add<Part>("L4", param.iv_vec.size() == 7 ? param.iv_vec[3].data() : default_iv);
		auto &p5 = model->partPool().add<Part>("L5", param.iv_vec.size() == 7 ? param.iv_vec[4].data() : default_iv);
		auto &p6 = model->partPool().add<Part>("L6", param.iv_vec.size() == 7 ? param.iv_vec[5].data() : default_iv);
		auto &p7 = model->partPool().add<Part>("L7", param.iv_vec.size() == 7 ? param.iv_vec[6].data() : default_iv);

		// add joint //
		const double j1_pos[3]{ 0.0, 0.0, param.d1 };
		const double j2_pos[3]{ 0.0, 0.0, param.d1 };
		const double j3_pos[3]{ 0.0, 0.0, param.d1 };
		const double j4_pos[3]{ 0.0, 0.0, param.d1 + param.d3 };
		const double j5_pos[3]{ 0.0, 0.0, param.d1 + param.d3 + param.d5 };
		const double j6_pos[3]{ 0.0, 0.0, param.d1 + param.d3 + param.d5 };
		const double j7_pos[3]{ 0.0, 0.0, param.d1 + param.d3 + param.d5 };

		const double j1_axis[3]{ 0.0, 0.0, 1.0 };
		const double j2_axis[3]{ 0.0, 1.0, 0.0 };
		const double j3_axis[3]{ 0.0, 0.0, 1.0 };
		const double j4_axis[3]{ 0.0, 1.0, 0.0 };
		const double j5_axis[3]{ 0.0, 0.0, 1.0 };
		const double j6_axis[3]{ 0.0, 1.0, 0.0 };
		const double j7_axis[3]{ 0.0, 0.0, 1.0 };

		auto &j1 = model->addRevoluteJoint(p1, model->ground(), j1_pos, j1_axis);
		auto &j2 = model->addRevoluteJoint(p2, p1, j2_pos, j2_axis);
		auto &j3 = model->addRevoluteJoint(p3, p2, j3_pos, j3_axis);
		auto &j4 = model->addRevoluteJoint(p4, p3, j4_pos, j4_axis);
		auto &j5 = model->addRevoluteJoint(p5, p4, j5_pos, j5_axis);
		auto &j6 = model->addRevoluteJoint(p6, p5, j6_pos, j6_axis);
		auto &j7 = model->addRevoluteJoint(p7, p6, j7_pos, j7_axis);

		// add actuation //
		auto &m1 = model->addMotion(j1);
		auto &m2 = model->addMotion(j2);
		auto &m3 = model->addMotion(j3);
		auto &m4 = model->addMotion(j4);
		auto &m5 = model->addMotion(j5);
		auto &m6 = model->addMotion(j6);
		auto &m7 = model->addMotion(j7);

		const double default_mot_frc[3]{0.0, 0.0, 0.0};

		m1.setFrcCoe(param.mot_frc_vec.size() == 7 ? param.mot_frc_vec[0].data() : default_mot_frc);
		m2.setFrcCoe(param.mot_frc_vec.size() == 7 ? param.mot_frc_vec[1].data() : default_mot_frc);
		m3.setFrcCoe(param.mot_frc_vec.size() == 7 ? param.mot_frc_vec[2].data() : default_mot_frc);
		m4.setFrcCoe(param.mot_frc_vec.size() == 7 ? param.mot_frc_vec[3].data() : default_mot_frc);
		m5.setFrcCoe(param.mot_frc_vec.size() == 7 ? param.mot_frc_vec[4].data() : default_mot_frc);
		m6.setFrcCoe(param.mot_frc_vec.size() == 7 ? param.mot_frc_vec[5].data() : default_mot_frc);
		m7.setFrcCoe(param.mot_frc_vec.size() == 7 ? param.mot_frc_vec[6].data() : default_mot_frc);

		// add ee general motion //
		const double axis_7_pe[]{ 0.0, 0.0, param.d1 + param.d3 + param.d5, 0.0, 0.0 ,0.0 };
		double axis_7_pm[16];
		double ee_i_pm[16], ee_i_wrt_axis_7_pm[16];
		double ee_j_pm[16]{ 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };

		s_pe2pm(axis_7_pe, axis_7_pm, "321");
		s_pe2pm(param.tool0_pe, ee_i_wrt_axis_7_pm, param.tool0_pe_type.empty() ? "321" : param.tool0_pe_type.c_str());
		s_pm2pm(axis_7_pm, ee_i_wrt_axis_7_pm, ee_i_pm);

		auto &makI = p7.markerPool().add<Marker>("ee_makI", ee_i_pm);
		auto &makJ = model->ground().markerPool().add<Marker>("ee_makJ", ee_j_pm);
		auto &ee = model->generalMotionPool().add<aris::dynamic::GeneralMotion>("ee", &makI, &makJ, false);

		// change robot pose wrt ground //
		double robot_pm[16];
		s_pe2pm(param.base2ref_pe, robot_pm, param.base2ref_pe_type.empty() ? "321" : param.base2ref_pe_type.c_str());

		p1.setPm(s_pm_dot_pm(robot_pm, *p1.pm()));
		p2.setPm(s_pm_dot_pm(robot_pm, *p2.pm()));
		p3.setPm(s_pm_dot_pm(robot_pm, *p3.pm()));
		p4.setPm(s_pm_dot_pm(robot_pm, *p4.pm()));
		p5.setPm(s_pm_dot_pm(robot_pm, *p5.pm()));
		p6.setPm(s_pm_dot_pm(robot_pm, *p6.pm()));
		p7.setPm(s_pm_dot_pm(robot_pm, *p7.pm()));
		j1.makJ().setPrtPm(s_pm_dot_pm(robot_pm, *j1.makJ().prtPm()));
		ee.makJ().setPrtPm(s_pm_dot_pm(robot_pm, *ee.makJ().prtPm()));

		// add solver
		auto &inverse_kinematic = model->solverPool().add<aris::dynamic::SevenAxisInverseKinematicSolver>();
		auto &forward_kinematic = model->solverPool().add<ForwardKinematicSolver>();
		auto &inverse_dynamic = model->solverPool().add<aris::dynamic::InverseDynamicSolver>();
		auto &forward_dynamic = model->solverPool().add<aris::dynamic::ForwardDynamicSolver>();

		inverse_kinematic.allocateMemory();
		forward_kinematic.allocateMemory();
		inverse_dynamic.allocateMemory();
		forward_dynamic.allocateMemory();

		inverse_kinematic.setWhichRoot(8);
		inverse_kinematic.setAxisAngle(0.0);

		// make topology correct // 
		for (auto &m : model->motionPool())m.activate(true);
		for (auto &gm : model->generalMotionPool())gm.activate(false);
		for (auto &f : model->forcePool())f.activate(false);

		return model;
	}
	
	auto sevenAxisInverse(const SevenAxisParam &param, const double *ee_pm, double axis_angle, int which_root, double *input)->bool
	{
		// 七轴机器人构型：
		//
		//      EE
		//      |        z
		//     ---       y      ---
		//      |        z       |
		//                       d5
		//                       |
		//     ---       y      ---                                                            
		//                       |
		//                       d3
		//      |        z       |   
		//     ---       y      ---
		//      |        z    
		//     BASE
		//
		//  A 坐标系为前3轴的交点， z 轴和 1 轴平行， y 轴和 2 轴平行
		//  D 坐标系为 5 6 7 三根轴的交点，零位下与 A 坐标系方向一致
		//  
		//  
		// 
		auto d3 = param.d3;
		auto d5 = param.d5;

		double pm_A_in_Ground[16]{ 1,0,0,0,0,1,0,0,0,0,1,param.d1,0,0,0,1 };
		double pm_EE_in_D[16];
		s_pe2pm(param.tool0_pe, pm_EE_in_D, param.base2ref_pe_type.c_str());

		double E_in_A[16];
		s_inv_pm_dot_pm(pm_A_in_Ground, ee_pm, E_in_A);
		double D_in_A[16];
		s_pm_dot_inv_pm(E_in_A, pm_EE_in_D, D_in_A);

		double q[7]{ 0 };

		// 求q4
		double distance_D = std::sqrt(D_in_A[3] * D_in_A[3] + D_in_A[7] * D_in_A[7] + D_in_A[11] * D_in_A[11]);
		double theta_part3;// part 3的z轴和两点之间连线的夹角
		if (auto cq4 = (d3*d3 + d5 * d5 - distance_D * distance_D) / (2 * d3*d5); cq4 > 1.0 || cq4 < -1.0)
		{
			return false;
		}
		else
		{
			if (which_root & 0x01)
			{
				q[3] = -aris::PI + std::acos(cq4);
				theta_part3 = std::acos((d3*d3 - d5 * d5 + distance_D * distance_D) / (2 * d3*distance_D));
			}
			else
			{
				q[3] = aris::PI - std::acos(cq4);
				theta_part3 = -std::acos((d3*d3 - d5 * d5 + distance_D * distance_D) / (2 * d3*distance_D));
			}
		}

		// 求part 3的姿态，这里让它的y轴是地面z轴和两连接点的叉乘
		double rm_3_1[9]{1,0,0,0,1,0,0,0,1}, rm_3[9];

		double y_axis[3]{ -D_in_A[7],D_in_A[3],0.0 };
		double nm_y = s_norm(3, y_axis);
		if (nm_y > 1e-8)s_vc(3, 1.0 / nm_y, y_axis, 1, rm_3_1 + 1, 3);

		double z_axis[3]{ D_in_A[3] ,D_in_A[7] ,D_in_A[11] };
		s_vc(3, 1.0 / s_norm(3, z_axis), z_axis, 1, rm_3_1 + 2, 3);

		s_c3(rm_3_1 + 1, 3, rm_3_1 + 2, 3, rm_3_1, 3);

		// 乘以轴角
		double rm_zy[9];
		aris::dynamic::s_re2rm(std::array<double, 3>{axis_angle, theta_part3, 0.0}.data(), rm_zy, "321");
		s_mm(3, 3, 3, rm_3_1, rm_zy, rm_3);

		// 求part 4的姿态，它相对于part 3沿着转动后的y轴转了q[3]的角度
		double rmy[9], rm_4[9];
		aris::dynamic::s_rmy(q[3], rmy);
		s_mm(3, 3, 3, rm_3, rmy, rm_4);

		// 求123轴的位置
		s_rm2re(rm_3, q, "323");
		if (which_root & 0x02)
		{
			q[0] = q[0] > PI ? q[0] - PI : q[0] + PI;
			q[1] = 2 * PI - q[1];
			q[2] = q[2] > PI ? q[2] - PI : q[2] + PI;
		}

		// 求567轴的位置
		double rm_E_wrt_4[9];
		s_mm(3, 3, 3, rm_4, ColMajor(3), D_in_A, 4, rm_E_wrt_4, 3);
		s_rm2re(rm_E_wrt_4, q + 4, "323");
		if (which_root & 0x04)
		{
			q[4] = q[4] > PI ? q[4] - PI : q[4] + PI;
			q[5] = 2 * PI - q[5];
			q[6] = q[6] > PI ? q[6] - PI : q[6] + PI;
		}

		// 添加所有的偏移 //
		for (int i = 0; i < 7; ++i)
		{
			while (q[i] > PI) q[i] -= 2 * PI;
			while (q[i] < -PI) q[i] += 2 * PI;
		}

		s_vc(7, q, input);

		return true;
	}
	struct SevenAxisInverseKinematicSolver::Imp
	{
		int which_root_{ 0 };
		double axis_angle{ 0.0 };
		SevenAxisParam seven_axis_param;
		union
		{
			struct { Part* GR, *L1, *L2, *L3, *L4, *L5, *L6, *L7; };
			Part* parts[8];
		};
		union
		{
			struct { RevoluteJoint *R1, *R2, *R3, *R4, *R5, *R6, *R7; };
			RevoluteJoint* joints[7];
		};
		union
		{
			struct { Motion *M1, *M2, *M3, *M4, *M5, *M6, *M7; };
			Motion* motions[7];
		};
		GeneralMotion *ee;
	};
	auto SevenAxisInverseKinematicSolver::saveXml(aris::core::XmlElement &xml_ele) const->void
	{
		InverseKinematicSolver::saveXml(xml_ele);
		xml_ele.SetAttribute("which_root", imp_->which_root_);
	}
	auto SevenAxisInverseKinematicSolver::loadXml(const aris::core::XmlElement &xml_ele)->void
	{
		InverseKinematicSolver::loadXml(xml_ele);
		setWhichRoot(Object::attributeInt32(xml_ele, "which_root"));
	}
	auto SevenAxisInverseKinematicSolver::allocateMemory()->void
	{
		InverseKinematicSolver::allocateMemory();

		this->imp_->GR;
		imp_->GR = &ancestor<Model>()->partPool().at(0);
		imp_->L1 = &ancestor<Model>()->partPool().at(1);
		imp_->L2 = &ancestor<Model>()->partPool().at(2);
		imp_->L3 = &ancestor<Model>()->partPool().at(3);
		imp_->L4 = &ancestor<Model>()->partPool().at(4);
		imp_->L5 = &ancestor<Model>()->partPool().at(5);
		imp_->L6 = &ancestor<Model>()->partPool().at(6);
		imp_->L7 = &ancestor<Model>()->partPool().at(7);

		imp_->R1 = dynamic_cast<RevoluteJoint*>(&ancestor<Model>()->jointPool().at(0));
		imp_->R2 = dynamic_cast<RevoluteJoint*>(&ancestor<Model>()->jointPool().at(1));
		imp_->R3 = dynamic_cast<RevoluteJoint*>(&ancestor<Model>()->jointPool().at(2));
		imp_->R4 = dynamic_cast<RevoluteJoint*>(&ancestor<Model>()->jointPool().at(3));
		imp_->R5 = dynamic_cast<RevoluteJoint*>(&ancestor<Model>()->jointPool().at(4));
		imp_->R6 = dynamic_cast<RevoluteJoint*>(&ancestor<Model>()->jointPool().at(5));
		imp_->R7 = dynamic_cast<RevoluteJoint*>(&ancestor<Model>()->jointPool().at(6));

		imp_->M1 = &ancestor<Model>()->motionPool().at(0);
		imp_->M2 = &ancestor<Model>()->motionPool().at(1);
		imp_->M3 = &ancestor<Model>()->motionPool().at(2);
		imp_->M4 = &ancestor<Model>()->motionPool().at(3);
		imp_->M5 = &ancestor<Model>()->motionPool().at(4);
		imp_->M6 = &ancestor<Model>()->motionPool().at(5);
		imp_->M7 = &ancestor<Model>()->motionPool().at(6);

		imp_->ee = &ancestor<Model>()->generalMotionPool().at(0);


		auto &p = imp_->seven_axis_param;
		
		//  config seven axis param, tbd.....//
		imp_->seven_axis_param.d1 = imp_->R1->makJ().prtPm()[2][3];

		double diff_p[3];
		s_vc(3, &imp_->R4->makJ().prtPm()[0][3], 4, diff_p, 1);
		s_vs(3, &imp_->R3->makI().prtPm()[0][3], 4, diff_p, 1);
		imp_->seven_axis_param.d3 = s_norm(3, diff_p);

		s_vc(3, &imp_->R5->makJ().prtPm()[0][3], 4, diff_p, 1);
		s_vs(3, &imp_->R4->makI().prtPm()[0][3], 4, diff_p, 1);
		imp_->seven_axis_param.d5 = s_norm(3, diff_p);

		// config tool0 //
		const double axis_7_pe[]{ 0.0, 0.0, imp_->seven_axis_param.d1 + imp_->seven_axis_param.d3 + imp_->seven_axis_param.d5, 0.0, 0.0 ,0.0 };
		double axis_7_pm[16];
		double ee_i_pm[16], ee_i_wrt_axis_7_pm[16];
		double ee_j_pm[16]{ 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };

		s_vc(16, static_cast<const double*>(*imp_->ee->makI().prtPm()), ee_i_pm);
		s_pe2pm(axis_7_pe, axis_7_pm, "321");
		s_inv_pm2pm(axis_7_pm, ee_i_pm, ee_i_wrt_axis_7_pm);
		imp_->seven_axis_param.tool0_pe_type = "321";
		s_pm2pe(ee_i_wrt_axis_7_pm, imp_->seven_axis_param.tool0_pe, "321");
	}
	auto SevenAxisInverseKinematicSolver::kinPos()->int
	{
		if (imp_->which_root_ == 8)
		{
			int solution_num = 0;
			double diff_q[8][7];
			double diff_norm[8];

			for (int i = 0; i < 8; ++i)
			{
				if (sevenAxisInverse(imp_->seven_axis_param, *imp_->ee->mpm(), imp_->axis_angle, i, diff_q[solution_num]))
				{
					diff_norm[solution_num] = 0;
					for (int j = 0; j < 7; ++j)
					{
						diff_q[solution_num][j] -= imp_->motions[j]->mpInternal();

						while (diff_q[solution_num][j] > PI) diff_q[solution_num][j] -= 2 * PI;
						while (diff_q[solution_num][j] < -PI)diff_q[solution_num][j] += 2 * PI;

						diff_norm[solution_num] += std::abs(diff_q[solution_num][j]);
					}

					++solution_num;
				}
			}

			if (solution_num == 0) return -1;

			auto real_solution = std::min_element(diff_norm, diff_norm + solution_num) - diff_norm;

			for (aris::Size i = 0; i < 7; ++i)
			{
				if (&imp_->joints[i]->makI().fatherPart() == imp_->parts[i + 1])
				{
					double pm_prt_i[16], pm_mak_i[16], pm_rot[16];
					s_pe2pm(std::array<double, 6>{0, 0, 0, 0, 0, imp_->motions[i]->mpInternal() + diff_q[real_solution][i]}.data(), pm_rot);
					s_pm_dot_pm(*imp_->joints[i]->makJ().pm(), pm_rot, pm_mak_i);
					s_pm_dot_inv_pm(pm_mak_i, *imp_->joints[i]->makI().prtPm(), pm_prt_i);
					imp_->parts[i + 1]->setPm(pm_prt_i);
				}
				else
				{
					double pm_prt_j[16], pm_mak_j[16], pm_rot[16];
					s_pe2pm(std::array<double, 6>{0, 0, 0, 0, 0, -imp_->motions[i]->mpInternal() - diff_q[real_solution][i]}.data(), pm_rot);
					s_pm_dot_pm(*imp_->joints[i]->makI().pm(), pm_rot, pm_mak_j);
					s_pm_dot_inv_pm(pm_mak_j, *imp_->joints[i]->makJ().prtPm(), pm_prt_j);
					imp_->parts[i + 1]->setPm(pm_prt_j);
				}

				imp_->motions[i]->setMpInternal(imp_->motions[i]->mpInternal() + diff_q[real_solution][i]);
			}

			return 0;
		}
		else
		{
			if (double q[7]; sevenAxisInverse(imp_->seven_axis_param, *imp_->ee->mpm(), imp_->axis_angle, imp_->which_root_, q))
			{
				for (aris::Size i = 0; i < 7; ++i)
				{
					if (&imp_->joints[i]->makI().fatherPart() == imp_->parts[i + 1])
					{
						double pm_prt_i[16], pm_mak_i[16], pm_rot[16];
						s_pe2pm(std::array<double, 6>{0, 0, 0, 0, 0, q[i]}.data(), pm_rot);
						s_pm_dot_pm(*imp_->joints[i]->makJ().pm(), pm_rot, pm_mak_i);
						s_pm_dot_inv_pm(pm_mak_i, *imp_->joints[i]->makI().prtPm(), pm_prt_i);
						imp_->parts[i + 1]->setPm(pm_prt_i);
					}
					else
					{
						double pm_prt_j[16], pm_mak_j[16], pm_rot[16];
						s_pe2pm(std::array<double, 6>{0, 0, 0, 0, 0, -q[i]}.data(), pm_rot);
						s_pm_dot_pm(*imp_->joints[i]->makI().pm(), pm_rot, pm_mak_j);
						s_pm_dot_inv_pm(pm_mak_j, *imp_->joints[i]->makJ().prtPm(), pm_prt_j);
						imp_->parts[i + 1]->setPm(pm_prt_j);
					}

					double last_mp = imp_->motions[i]->mpInternal();
					imp_->motions[i]->updMp();
					while (imp_->motions[i]->mpInternal() - last_mp > PI)imp_->motions[i]->setMpInternal(imp_->motions[i]->mpInternal() - 2 * PI);
					while (imp_->motions[i]->mpInternal() - last_mp < -PI)imp_->motions[i]->setMpInternal(imp_->motions[i]->mpInternal() + 2 * PI);
				}

				return 0;
			}
			else return -2;
		}
	}
	auto SevenAxisInverseKinematicSolver::setWhichRoot(int root_of_0_to_7)->void { imp_->which_root_ = root_of_0_to_7; }
	auto SevenAxisInverseKinematicSolver::setAxisAngle(double axis_angle)->void { imp_->axis_angle = axis_angle; }
	SevenAxisInverseKinematicSolver::SevenAxisInverseKinematicSolver(const std::string &name) :InverseKinematicSolver(name, 1, 0.0), imp_(new Imp) {}
	ARIS_DEFINE_BIG_FOUR_CPP(SevenAxisInverseKinematicSolver);
}
