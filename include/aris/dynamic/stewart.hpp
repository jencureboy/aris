﻿#ifndef ARIS_DYNAMIC_STEWART_H_
#define ARIS_DYNAMIC_STEWART_H_

#include <aris/dynamic/model_solver.hpp>

namespace aris::dynamic
{
	/// @defgroup dynamic_model_group 动力学建模模块
	/// @{
	///

	auto createModelStewart()->std::unique_ptr<aris::dynamic::Model>;

	class StewartInverseKinematicSolver :public aris::dynamic::InverseKinematicSolver
	{
	public:
		auto virtual allocateMemory()->void override;
		auto virtual kinPos()->int override;
		auto virtual setPmEE(const double *ee_pm, const double *ext_axes)->void
		{
			model().generalMotionPool()[0].setMpm(ee_pm);
			if (ext_axes)
			{
				for (int i = 6; i < model().motionPool().size(); ++i)
				{
					model().motionPool()[i].setMp(ext_axes[i - 6]);
				}
			}
		}

		virtual ~StewartInverseKinematicSolver() = default;
		explicit StewartInverseKinematicSolver(const std::string &name = "stewart_inverse_solver");
		ARIS_REGISTER_TYPE(StewartInverseKinematicSolver);
		ARIS_DECLARE_BIG_FOUR(StewartInverseKinematicSolver);

	private:
		struct Imp;
		aris::core::ImpPtr<Imp> imp_;
	};
	///
	/// @}
}

#endif
