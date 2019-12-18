﻿#ifndef ARIS_PLAN_FUNCTION_H_
#define ARIS_PLAN_FUNCTION_H_


#include <aris/core/basic_type.hpp>

#include <string>
#include <map>
#include <aris/core/core.hpp>

namespace aris::plan
{
	auto inline acc_up(int n, int i)noexcept->double { return (-1.0 / 2 / n / n / n * i*i*i + 3.0 / 2.0 / n / n * i*i); }
	auto inline acc_down(int n, int i)noexcept->double { return (-1.0*i*i*i / 2.0 / n / n / n + 3.0 * i*i / 2.0 / n / n); }
	auto inline dec_up(int n, int i)noexcept->double { return 1.0 - (-1.0 / 2.0 / n / n / n * (n - i)*(n - i)*(n - i) + 3.0 / 2.0 / n / n * (n - i)*(n - i)); }
	auto inline dec_down(int n, int i)noexcept->double { return 1.0 - (-1.0*(n - i)*(n - i)*(n - i) / 2.0 / n / n / n + 3.0 * (n - i)*(n - i) / 2.0 / n / n); }

	auto inline acc_even(int n, int i)noexcept->double { return 1.0 / n / n * i * i; }
	auto inline dec_even(int n, int i)noexcept->double { return 1.0 - 1.0 / n / n * (n - i)*(n - i); }
	auto inline even(int n, int i)noexcept->double { return 1.0 / n * i; }

	auto inline s_p2p(int n, int i, double begin_pos, double end_pos)noexcept->double
	{
		double a = 4.0 * (end_pos - begin_pos) / n / n;
		return i <= n / 2.0 ? 0.5*a*i*i + begin_pos : end_pos - 0.5*a*(n - i)*(n - i);
	}
	auto inline s_v2v(int n, int i, double begin_vel, double end_vel)noexcept->double
	{
		double s = static_cast<double>(i) / n;
		double m = 1.0 - s;

		return (s*s*s - s * s)*end_vel*n + (m*m - m * m*m)*begin_vel*n;
	}
	auto inline s_interp(int n, int i, double begin_pos, double end_pos, double begin_vel, double end_vel)noexcept->double
	{
		double s = static_cast<double>(i) / n;

		double a, b, c, d;

		c = begin_vel * n;
		d = begin_pos;
		a = end_vel * n - 2.0 * end_pos + c + 2.0 * d;
		b = end_pos - c - d - a;

		return a * s*s*s + b * s*s + c * s + d;
	}

	auto moveAbsolute(double i, double begin_pos, double end_pos, double vel, double acc, double dec, double &current_pos, double &current_vel, double &current_acc, Size& total_count)->void;

	// pa : pos actual value
	// va : vel actual value
	// aa : acc actual value
	// pt : pos target value
	// vt : vel target value
	// at : acc target value
	// vm : vel max permitted value, always positive
	// am : acc max permitted value, always positive
	// dm : dec max permitted value, always positive
	// dt : time inteval, set to 1e-3
	// zero_check : set to 1e-10
	// pc : next planned pos
	// vc : next planned vel
	// ac : next planned acc
	// total_count : tbd, not finished yet
	auto moveAbsolute2(double pa, double va, double aa, double pt, double vt, double at, double vm, double am, double dm, double dt, double zero_check, double &pc, double &vc, double &ac, Size& total_count)->int;

	
	class LanguageParser : public aris::core::Object
	{
	public:
		auto setProgram(const std::string& program)->void;
		auto parseLanguage()->void;
		auto varPool()->const std::vector<std::string>&;
		auto gotoMain()->void;
		auto gotoLine(int line)->void;
		auto forward(bool is_this_cmd_successful = true)->void;
		auto currentCmd()const->const std::string&;
		auto currentLine()const->int;
		auto isCurrentLineKeyWord()const->bool;
		auto isCurrentLineFunction()const->bool;
		auto isEnd()const->bool;

		virtual ~LanguageParser();
		explicit LanguageParser(const std::string &name = "language_parser");
		ARIS_REGISTER_TYPE(LanguageParser);
		ARIS_DECLARE_BIG_FOUR(LanguageParser);

	private:
		struct Imp;
		aris::core::ImpPtr<Imp> imp_;
	};
}


#endif