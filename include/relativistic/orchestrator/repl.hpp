#pragma once

#include "relativistic/orchestrator/command.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include <iostream>
#include <string_view>
#include <string>

namespace Relativistic::Orchestrator {

template <size_t QueueCapacity = 16384>
class MasterTerminalRepl {
private:
	SimulationOrchestrator<QueueCapacity>& orchestrator_;

public:
	explicit constexpr MasterTerminalRepl(SimulationOrchestrator<QueueCapacity>& orchestrator) noexcept
		: orchestrator_(orchestrator) {}

	bool execute_line(std::string_view line, CommandResult* res = nullptr) noexcept {
		CommandResult local_res{};
		const auto cmd_opt = CommandParser::parse(line, &local_res);
		if (!cmd_opt.has_value()) {
			if (res != nullptr) {
				*res = local_res;
			}
			return false;
		}

		const bool pushed = orchestrator_.enqueue_command(*cmd_opt);
		if (!pushed) {
			if (res != nullptr) {
				res->success = false;
				std::strncpy(res->message, "Command queue full", sizeof(res->message) - 1);
			}
			return false;
		}

		if (res != nullptr) {
			*res = local_res;
		}
		return true;
	}

	void print_prompt(std::ostream& out = std::cout) const {
		out << "relativistic> " << std::flush;
	}

	void print_status(std::ostream& out = std::cout) const {
		const auto snap = orchestrator_.scheduler().snapshot();
		const auto& p = orchestrator_.parameters();
		out << "=== Simulation Status ===\n"
			<< "Tick Index:       " << snap.tick_index << "\n"
			<< "Logical Time:     " << snap.logical_time << " s\n"
			<< "Tick Rate:        " << snap.tick_rate_hz << " Hz (dt = " << snap.tick_dt << " s)\n"
			<< "Warp Factor:      " << snap.warp_factor << "x\n"
			<< "Paused:           " << (snap.is_paused ? "YES" : "NO") << "\n"
			<< "Remaining Steps:  " << snap.remaining_steps << "\n"
			<< "Mass:             " << p.mass << "\n"
			<< "Spin:             " << p.spin << "\n"
			<< "Charge:           " << p.charge << "\n"
			<< "Cosmo Lambda:     " << p.cosmological_lambda << "\n"
			<< "Wormhole Throat:  " << p.wormhole_throat << "\n"
			<< "Warp Velocity:    " << p.warp_velocity << "\n"
			<< "=========================\n";
	}

	void print_help(std::ostream& out = std::cout) const {
		out << "Available commands:\n"
			<< "  pause                  - Pause simulation\n"
			<< "  resume                 - Resume simulation\n"
			<< "  step [N]               - Advance N ticks (default 1) and pause\n"
			<< "  warp <factor>          - Set time warp factor (> 0)\n"
			<< "  tickrate <Hz>          - Set scheduler tick rate (10 to 1000 Hz)\n"
			<< "  set <param> <value>    - Modify physical parameter (mass, spin, charge, lambda, throat, warp_velocity, tickrate, <custom>)\n"
			<< "  reset                  - Reset simulation clock and parameters\n"
			<< "  status                 - Display current simulation state\n"
			<< "  quit / exit / shutdown - Terminate simulation\n";
	}
};

}
