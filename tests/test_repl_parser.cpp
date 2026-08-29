#include "relativistic/orchestrator/command.hpp"
#include <cassert>
#include <cstring>
#include <cmath>

int main() {
	using namespace Relativistic::Orchestrator;

	{
		CommandResult res{};
		auto cmd = CommandParser::parse("pause", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::Pause);
		assert(res.success);

		cmd = CommandParser::parse("  PAUSE  ", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::Pause);
	}

	{
		CommandResult res{};
		auto cmd = CommandParser::parse("resume", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::Resume);

		cmd = CommandParser::parse("RESUME", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::Resume);
	}

	{
		CommandResult res{};
		auto cmd = CommandParser::parse("step", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::Step);
		assert(cmd->step_count == 1);

		cmd = CommandParser::parse("step 100", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::Step);
		assert(cmd->step_count == 100);

		cmd = CommandParser::parse("step -5", &res);
		assert(!cmd.has_value());
		assert(!res.success);

		cmd = CommandParser::parse("step abc", &res);
		assert(!cmd.has_value());
	}

	{
		CommandResult res{};
		auto cmd = CommandParser::parse("warp 2.5", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::Warp);
		assert(std::abs(cmd->numeric_value - 2.5) < 1e-12);

		cmd = CommandParser::parse("warp -1.0", &res);
		assert(!cmd.has_value());

		cmd = CommandParser::parse("warp", &res);
		assert(!cmd.has_value());
	}

	{
		CommandResult res{};
		auto cmd = CommandParser::parse("reset", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::Reset);
	}

	{
		CommandResult res{};
		auto cmd = CommandParser::parse("tickrate 120", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::SetTickRate);
		assert(std::abs(cmd->numeric_value - 120.0) < 1e-12);

		cmd = CommandParser::parse("tickrate 5", &res);
		assert(!cmd.has_value());

		cmd = CommandParser::parse("tickrate 1500", &res);
		assert(!cmd.has_value());
	}

	{
		CommandResult res{};
		auto cmd = CommandParser::parse("set mass 1.989e30", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::SetParam);
		assert(cmd->param_type == ParameterType::Mass);
		assert(std::abs(cmd->numeric_value - 1.989e30) < 1e20);

		cmd = CommandParser::parse("set spin 0.98", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::SetParam);
		assert(cmd->param_type == ParameterType::Spin);
		assert(std::abs(cmd->numeric_value - 0.98) < 1e-12);

		cmd = CommandParser::parse("set charge -0.5", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::SetParam);
		assert(cmd->param_type == ParameterType::Charge);
		assert(std::abs(cmd->numeric_value - (-0.5)) < 1e-12);

		cmd = CommandParser::parse("set lambda 1.1e-52", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::SetParam);
		assert(cmd->param_type == ParameterType::CosmologicalLambda);

		cmd = CommandParser::parse("set throat 2.5", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::SetParam);
		assert(cmd->param_type == ParameterType::WormholeThroat);

		cmd = CommandParser::parse("set warp_velocity 1.5", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::SetParam);
		assert(cmd->param_type == ParameterType::WarpVelocity);

		cmd = CommandParser::parse("set tickrate 240", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::SetParam);
		assert(cmd->param_type == ParameterType::TickRate);

		cmd = CommandParser::parse("set my_parameter 42.125", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::SetParam);
		assert(cmd->param_type == ParameterType::Custom);
		assert(std::strcmp(cmd->custom_param_name, "my_parameter") == 0);
		assert(std::abs(cmd->numeric_value - 42.125) < 1e-12);

		cmd = CommandParser::parse("set", &res);
		assert(!cmd.has_value());

		cmd = CommandParser::parse("set mass", &res);
		assert(!cmd.has_value());

		cmd = CommandParser::parse("set mass not_a_number", &res);
		assert(!cmd.has_value());
	}

	{
		CommandResult res{};
		auto cmd = CommandParser::parse("status", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::Status);

		cmd = CommandParser::parse("shutdown", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::Shutdown);

		cmd = CommandParser::parse("quit", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::Shutdown);

		cmd = CommandParser::parse("exit", &res);
		assert(cmd.has_value());
		assert(cmd->type == CommandType::Shutdown);
	}

	return 0;
}
