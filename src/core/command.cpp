﻿#include "aris/core/command.hpp"

#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <functional>
#include <locale>

#include "aris/core/log.hpp"
#include "aris/core/reflection.hpp"

namespace aris::core
{
	struct ParamBase::Imp { bool is_taken_{ false }; };
	auto ParamBase::command()const->const Command & {
		if (auto c = dynamic_cast<const Command *>(father()))
			return *c;
		else if (auto p = dynamic_cast<const ParamBase *>(father()))
			return p->command();
		else
			THROW_FILE_LINE("failed to find father command, please check the command tree");
	};
	ParamBase::~ParamBase() = default;
	ParamBase::ParamBase(const std::string &name) :ObjectPool(name), imp_(new Imp) {}
	ARIS_DEFINE_BIG_FOUR_CPP(ParamBase);

	struct Param::Imp
	{
		std::string default_value_{ "" };
		char abbreviation_{ 0 };

		Imp(const std::string &default_param = std::string(""), char abbrev = 0) :default_value_(default_param), abbreviation_(abbrev) {}
	};
	auto Param::saveXml(aris::core::XmlElement &xml_ele) const->void
	{
		ParamBase::saveXml(xml_ele);
		char abbrev[2]{ imp_->abbreviation_,0 };
		if (imp_->abbreviation_)xml_ele.SetAttribute("abbreviation", abbrev);
		if (!imp_->default_value_.empty())xml_ele.SetAttribute("default", imp_->default_value_.c_str());
	}
	auto Param::loadXml(const aris::core::XmlElement &xml_ele)->void
	{
		ParamBase::loadXml(xml_ele);
		imp_->abbreviation_ = attributeChar(xml_ele, "abbreviation", imp_->abbreviation_);
		imp_->default_value_ = attributeString(xml_ele, "default", imp_->default_value_);
	}
	auto Param::abbreviation()const->char { return imp_->abbreviation_; }
	auto Param::setAbbreviation(char abbreviation)->void { imp_->abbreviation_ = abbreviation; }
	auto Param::defaultValue()const->const std::string & { return imp_->default_value_; }
	auto Param::setDefaultValue(const std::string & default_value)->void { imp_->default_value_ = default_value; }
	Param::~Param() = default;
	Param::Param(const std::string &name, const std::string &default_param, char abbrev) :ParamBase(name), imp_(new Imp(default_param, abbrev)) {}
	ARIS_DEFINE_BIG_FOUR_CPP(Param);

	GroupParam::~GroupParam() = default;
	GroupParam::GroupParam(const std::string &name) :ParamBase(name) {}
	ARIS_DEFINE_BIG_FOUR_CPP(GroupParam);

	struct UniqueParam::Imp
	{
		std::string default_value_{ "" };
		Imp(const std::string &default_param = std::string("")) :default_value_(default_param) {}
	};
	auto UniqueParam::saveXml(aris::core::XmlElement &xml_ele) const->void
	{
		ParamBase::saveXml(xml_ele);
		if (!imp_->default_value_.empty())xml_ele.SetAttribute("default", imp_->default_value_.c_str());
	}
	auto UniqueParam::loadXml(const aris::core::XmlElement &xml_ele)->void
	{
		ParamBase::loadXml(xml_ele);
		imp_->default_value_ = attributeString(xml_ele, "default", imp_->default_value_);
	}
	auto UniqueParam::defaultValue()const->const std::string & { return imp_->default_value_; }
	auto UniqueParam::setDefaultValue(const std::string & default_value)->void { imp_->default_value_ = default_value; }
	UniqueParam::~UniqueParam() = default;
	UniqueParam::UniqueParam(const std::string &name, const std::string &default_param) :ParamBase(name), imp_(new Imp(default_param)) {}
	ARIS_DEFINE_BIG_FOUR_CPP(UniqueParam);

	struct Command::Imp
	{
		bool is_taken_;
		std::string default_value_{ "" };
		std::map<std::string, Param*> param_map_;
		std::map<char, std::string> abbreviation_map_;

		Imp(const std::string &default_param = std::string("")) :default_value_(default_param) {}

		static auto take(Object* param)->void
		{
			if (auto p = dynamic_cast<Param*>(param))
			{
				if (p->ParamBase::imp_->is_taken_)
					THROW_FILE_LINE("parse command error: command \"" + p->command().name() + "\"'s param \"" + p->name() + "\" has been set more than once");
				p->ParamBase::imp_->is_taken_ = true;
				take(param->father());
			}
			else if (auto g = dynamic_cast<GroupParam*>(param))
			{
				if (!g->ParamBase::imp_->is_taken_)
				{
					g->ParamBase::imp_->is_taken_ = true;
					take(param->father());
				}
			}
			else if (auto u = dynamic_cast<UniqueParam*>(param))
			{
				if (u->ParamBase::imp_->is_taken_)
					THROW_FILE_LINE("parse command error: command \"" + u->command().name() + "\"'s UNIQUE param \"" + u->name() + "\" has been set more than once");
				u->ParamBase::imp_->is_taken_ = true;
				take(param->father());
			}
			else if (auto c = dynamic_cast<Command*>(param))
			{
				if (c->imp_->is_taken_)
					THROW_FILE_LINE("invalid param: some params of command \"" + c->name() + "\" has been set more than once");
				c->imp_->is_taken_ = true;
			}
			else
			{
				THROW_FILE_LINE("wrong type when cmd parse in take");
			}
		}
		static auto reset(Object* param)->void
		{
			if (auto p = dynamic_cast<Param*>(param))
			{
				p->ParamBase::imp_->is_taken_ = false;
			}
			else if (auto g = dynamic_cast<GroupParam*>(param))
			{
				g->ParamBase::imp_->is_taken_ = false;
				for (auto &child : *g)reset(&child);
			}
			else if (auto u = dynamic_cast<UniqueParam*>(param))
			{
				u->ParamBase::imp_->is_taken_ = false;
				for (auto &child : *u)reset(&child);
			}
			else if (auto c = dynamic_cast<Command*>(param))
			{
				c->imp_->is_taken_ = false;
				for (auto &child : *c)reset(&child);
			}
			else
			{
				THROW_FILE_LINE("wrong type when cmd parse in reset");
			}
		}
		static auto addDefaultParam(Object* param, std::map<std::string_view, std::string_view> &param_map_out)->void
		{
			if (auto p = dynamic_cast<Param*>(param))
			{
				if (!p->ParamBase::imp_->is_taken_) { param_map_out.insert(std::make_pair(std::string_view(p->name()), std::string_view(p->imp_->default_value_))); }
			}
			else if (auto g = dynamic_cast<GroupParam*>(param))
			{
				for (auto &child : *g) { addDefaultParam(&child, param_map_out); }
			}
			else if (auto u = dynamic_cast<UniqueParam*>(param))
			{
				if (u->size() == 0)return;

				auto default_param_iter = std::find_if(u->begin(), u->end(), [](ParamBase &param)->bool { return param.imp_->is_taken_; });
				auto default_param_ptr = u->imp_->default_value_ == "" ? nullptr : &*u->findByName(u->imp_->default_value_);
				auto default_param = default_param_iter == u->end() ? default_param_ptr : &*default_param_iter;
				default_param = u->size() == 1 ? &u->front() : default_param;

				if (!default_param)THROW_FILE_LINE("failed to find default param in command \"" + u->command().name() + "\" param \"" + u->name() + "\"");

				addDefaultParam(default_param, param_map_out);
			}
			else if (auto c = dynamic_cast<Command*>(param))
			{
				if (c->size() == 0)return;

				auto default_param_iter = std::find_if(c->begin(), c->end(), [](ParamBase &param)->bool { return param.imp_->is_taken_; });
				auto default_param_ptr = c->imp_->default_value_ == "" ? nullptr : &*c->findByName(c->imp_->default_value_);
				auto default_param = default_param_iter == c->end() ? default_param_ptr : &*default_param_iter;
				default_param = c->size() == 1 ? &c->front() : default_param;

				if (!default_param)THROW_FILE_LINE("failed to find default param in command \"" + c->name() + "\"");

				addDefaultParam(default_param, param_map_out);
			}
			else
			{
				THROW_FILE_LINE("wrong type when cmd parse in addDefaultParam");
			}
		}
		static auto add_param_map_and_check_default(Command *cmd, ParamBase &param)->void
		{
			if (auto p = dynamic_cast<Param*>(&param))
			{
				if (cmd->imp_->param_map_.find(param.name()) != cmd->imp_->param_map_.end())
					THROW_FILE_LINE("failed to add param \"" + param.name() + "\" to cmd \"" + cmd->name() + "\", because this param already exists");
				if (cmd->imp_->abbreviation_map_.find(p->abbreviation()) != cmd->imp_->abbreviation_map_.end() && p->abbreviation() != 0)
					THROW_FILE_LINE("failed to add param \"" + param.name() + "\" to cmd \"" + cmd->name() + "\", because its abbreviation already exists");

				cmd->imp_->param_map_.insert(std::make_pair(param.name(), p));
				cmd->imp_->abbreviation_map_.insert(std::make_pair(p->abbreviation(), param.name()));
			}
			else if (auto u = dynamic_cast<UniqueParam*>(&param))
			{
				if ((u->imp_->default_value_ != "") && (u->findByName(u->imp_->default_value_) == u->end()))
				{
					THROW_FILE_LINE("Unique param \"" + u->name() + "\" has invalid default param name");
				}
				for (auto &sub_param : param) add_param_map_and_check_default(cmd, sub_param);
			}
			else if (auto g = dynamic_cast<GroupParam*>(&param))
			{
				for (auto &sub_param : param) add_param_map_and_check_default(cmd, sub_param);
			}
		}
	};
	auto Command::saveXml(aris::core::XmlElement &xml_ele) const->void
	{
		ObjectPool<ParamBase>::saveXml(xml_ele);
		if (!imp_->default_value_.empty())xml_ele.SetAttribute("default", imp_->default_value_.c_str());
	}
	auto Command::loadXml(const aris::core::XmlElement &xml_ele)->void
	{
		ObjectPool<ParamBase>::loadXml(xml_ele);
		imp_->default_value_ = attributeString(xml_ele, "default", imp_->default_value_);
	}
	auto Command::defaultValue()const->const std::string & { return imp_->default_value_; }
	auto Command::setDefaultValue(const std::string & default_value)->void { imp_->default_value_ = default_value; }
	auto Command::findParam(const std::string &param_name)->Param*
	{
		std::function<Param*(const std::string &, ParamBase &)> find_func = [&](const std::string &param_name, ParamBase &param)->Param*
		{
			if (auto p = dynamic_cast<Param*>(&param))
				return p->name() == param_name ? p : nullptr;
			
			for(auto &p: param)
			{
				if (auto result = find_func(param_name, p)) return result;
			}

			return nullptr;
		};

		for (auto &param : *this)
		{
			auto result = find_func(param_name, param);
			if (result) return result;
		}

		return nullptr;
	}
	Command::~Command() = default;
	Command::Command(const std::string &name, const std::string &default_param) :ObjectPool(name), imp_(new Imp(default_param)){}
	ARIS_DEFINE_BIG_FOUR_CPP(Command);

	struct CommandParser::Imp { ObjectPool<Command>* command_pool_; };
	auto CommandParser::loadXml(const aris::core::XmlElement &xml_ele)->void
	{
		Object::loadXml(xml_ele);
		imp_->command_pool_ = findOrInsertType<aris::core::ObjectPool<Command>>();
	}
	auto CommandParser::commandPool()->ObjectPool<Command> & { return *imp_->command_pool_; }
	auto CommandParser::commandPool()const->const ObjectPool<Command> & { return *imp_->command_pool_; }
	auto CommandParser::init()->void
	{
		// make map and abbrev map //
		for (auto &c : commandPool())
		{
			c.imp_->param_map_.clear();
			c.imp_->abbreviation_map_.clear();
			if ((c.imp_->default_value_ != "") && (c.findByName(c.imp_->default_value_) == c.end())) THROW_FILE_LINE("Command \"" + c.name() + "\" has invalid default param name");
			for (auto &param : c) Command::Imp::add_param_map_and_check_default(&c, param);
		}
	}
	auto CommandParser::parse(std::string_view cmd_str)->std::tuple<std::string_view, std::map<std::string_view, std::string_view>>
	{
		auto cut_str = [](std::string_view &input, const char *c)->std::string_view
		{
			// 此时c中字符是或的关系 //
			auto point = input.find_first_of(c);
			auto ret = input.substr(0, point);
			input = point == std::string::npos ? std::string_view() : input.substr(point);
			return ret;
		};
		auto trim_left = [](std::string_view input)->std::string_view 
		{
			auto point = input.find_first_not_of(' ');
			return point == std::string::npos ? std::string_view() : input.substr(point, std::string::npos);
		};
		auto get_param_value = [&](aris::core::Param *param, std::string_view &cmd_str)->std::string_view
		{
			if (cmd_str.empty() || cmd_str[0] != '=')return param->defaultValue();
			
			int brace_num = 0;
			int i = 1;
			for (i = 1; i < cmd_str.size() && !(std::isspace(cmd_str[i]) && brace_num == 0); ++i)
			{
				switch (cmd_str[i])
				{
				case '{':
					++brace_num;
					break;
				case '}':
					--brace_num;
					if (brace_num < 0)THROW_FILE_LINE("brace not pair");
					break;
				default:
					break;
				}
			}

			if (brace_num)THROW_FILE_LINE("brace not pair");

			auto ret = cmd_str.substr(1, i - 1);// here is length
			cmd_str = trim_left(cmd_str.substr(i));
			return ret;
		};

		std::string_view cmd;
		std::map<std::string_view, std::string_view> param_map;

		if (cmd = cut_str(cmd_str, " "); cmd.empty())THROW_FILE_LINE("invalid command string: please at least contain a word");
		cmd_str = trim_left(cmd_str);

		auto command = imp_->command_pool_->findByName(std::string(cmd));
		if (command == imp_->command_pool_->end()) THROW_FILE_LINE("invalid command name: server does not have this command \"" + std::string(cmd) + "\"");

		Command::Imp::reset(&*command);
		for (; !cmd_str.empty();)
		{
			auto param_name_origin = cut_str(cmd_str, " =");
			cmd_str = trim_left(cmd_str);

			if (param_name_origin == "")THROW_FILE_LINE("invalid param: param should not start with '='");
			else if (param_name_origin == "-")THROW_FILE_LINE("invalid param: symbol \"-\" must be followed by an abbreviation of param");
			else if (param_name_origin == "--")THROW_FILE_LINE("invalid param: symbol \"--\" must be followed by a full name of param");
			else if (param_name_origin.size() > 2 && param_name_origin.data()[0] == '-' && param_name_origin.data()[1] != '-')THROW_FILE_LINE("invalid param: param start with single '-' must be an abbreviation");
			else if (param_name_origin.size() == 2 && param_name_origin.data()[0] == '-' && param_name_origin.data()[1] != '-')
			{
				char abbrev = param_name_origin.data()[1];

				if (command->imp_->abbreviation_map_.find(abbrev) == command->imp_->abbreviation_map_.end())
					THROW_FILE_LINE(std::string("invalid param: param \"") + abbrev + "\" is not a abbreviation of any valid param");

				auto param = command->imp_->param_map_.at(command->imp_->abbreviation_map_.at(abbrev));
				auto &param_name = command->imp_->abbreviation_map_.at(abbrev);
				auto param_value = get_param_value(param, cmd_str);

				param_map.insert(make_pair(std::string_view(param_name), std::string_view(param_value)));
				Command::Imp::take(param);
			}
			else if (param_name_origin.data()[0] == '-' && param_name_origin.data()[1] == '-')
			{
				auto param_name = param_name_origin.substr(2);

				if (command->imp_->param_map_.find(std::string(param_name)) == command->imp_->param_map_.end())
					THROW_FILE_LINE(std::string("invalid param: param \"") + std::string(param_name) + "\" is not a valid param");

				auto param = command->imp_->param_map_.at(std::string(param_name));
				auto param_value = get_param_value(param, cmd_str);

				param_map.insert(make_pair(std::string_view(param_name), std::string_view(param_value)));
				Command::Imp::take(param);
			}
			else
			{
				for (auto abbrev : param_name_origin)
				{
					if (command->imp_->abbreviation_map_.find(abbrev) == command->imp_->abbreviation_map_.end())
						THROW_FILE_LINE(std::string("invalid param: param \"") + abbrev + "\" is not a abbreviation of any valid param");

					auto param = command->imp_->param_map_.at(command->imp_->abbreviation_map_.at(abbrev));
					auto param_name = command->imp_->abbreviation_map_.at(abbrev);
					auto param_value = param->defaultValue();
					param_map.insert(make_pair(param_name, param_value));
					Command::Imp::take(param);
				}
			}
		}
		Command::Imp::addDefaultParam(&*command, param_map);

		return std::make_tuple(cmd, param_map);
	}
	CommandParser::~CommandParser() = default;
	CommandParser::CommandParser(const std::string &name) :Object(name)
	{
		this->registerType<aris::core::ObjectPool<Command> >();
		imp_->command_pool_ = &add<aris::core::ObjectPool<Command> >("command_pool");
	}
	ARIS_DEFINE_BIG_FOUR_CPP(CommandParser);


	ARIS_REGISTRATION
	{
		class_<Param>("Param")
			.inherit<Object>()
			.property("abbreviation", &Param::setAbbreviation, &Param::abbreviation)
			.property("default", &Param::setDefaultValue, &Param::defaultValue);

		class_<UniqueParam>("UniqueParam")
			.inherit<Object>()
			.asRefArray()
			.property("default", &UniqueParam::setDefaultValue, &UniqueParam::defaultValue);

		class_<GroupParam>("GroupParam")
			.inherit<Object>()
			.asRefArray();

		class_<Command>("Command")
			.inherit<Object>()
			.asRefArray();

		class_<ObjectPool<Command>>("CommandPool")
			.inherit<Object>()
			.asRefArray();

		typedef ObjectPool<Command>&(CommandParser::*CommandPoolFunc)();
		//CommandPoolFunc func = &CommandParser::commandPool;
		class_<CommandParser>("CommandParser")
			.inherit<Object>()
			.property<CommandPoolFunc>("command_pool", &CommandParser::commandPool);
	}




}