#pragma once

#include "relativistic/core/spsc_queue.hpp"
#include "relativistic/orchestrator/command.hpp"
#include "relativistic/orchestrator/scheduler.hpp"
#include <array>
#include <atomic>
#include <cstring>
#include <string_view>
#include <optional>

namespace Relativistic::Orchestrator {

struct PhysicalParameters {
	double mass{1.0};
	double spin{0.0};
	double charge{0.0};
	double cosmological_lambda{0.0};
	double wormhole_throat{1.0};
	double warp_velocity{1.0};
};

struct CustomParameterEntry {
	char name[32]{};
	double value{0.0};
	bool active{false};
};

template <size_t QueueCapacity = 16384>
class SimulationOrchestrator {
private:
	Core::SpscQueue<Command, QueueCapacity> command_queue_;
	Core::SpscQueue<CommandResult, QueueCapacity> result_queue_;

	Scheduler<double> scheduler_;
	PhysicalParameters params_;
	std::array<CustomParameterEntry, 32> custom_params_{};

	std::atomic<bool> is_running_{true};
	std::atomic<uint64_t> commands_processed_{0};

public:
	constexpr SimulationOrchestrator() noexcept = default;

	[[nodiscard]] bool enqueue_command(const Command& cmd) noexcept {
		return command_queue_.try_push(cmd);
	}

	[[nodiscard]] bool enqueue_command(Command&& cmd) noexcept {
		return command_queue_.try_push(std::move(cmd));
	}

	[[nodiscard]] bool poll_result(CommandResult& res) noexcept {
		return result_queue_.try_pop(res);
	}

	void process_incoming_commands() noexcept {
		Command cmd;
		while (command_queue_.try_pop(cmd)) {
			CommandResult result{.success = true, .message = {}};
			apply_command(cmd, result);
			commands_processed_.fetch_add(1, std::memory_order_relaxed);
			static_cast<void>(result_queue_.try_push(result));
		}
	}

	void apply_command(const Command& cmd, CommandResult& res) noexcept {
		res.success = true;
		switch (cmd.type) {
			case CommandType::Pause:
				scheduler_.pause();
				std::strncpy(res.message, "Simulation paused", sizeof(res.message) - 1);
				break;
			case CommandType::Resume:
				scheduler_.resume();
				std::strncpy(res.message, "Simulation resumed", sizeof(res.message) - 1);
				break;
			case CommandType::Step:
				scheduler_.request_steps(cmd.step_count > 0 ? cmd.step_count : 1);
				std::strncpy(res.message, "Stepping simulation", sizeof(res.message) - 1);
				break;
			case CommandType::Warp:
				scheduler_.set_warp_factor(cmd.numeric_value);
				std::strncpy(res.message, "Warp factor updated", sizeof(res.message) - 1);
				break;
			case CommandType::Reset:
				scheduler_.reset();
				params_ = PhysicalParameters{};
				for (auto& entry : custom_params_) {
					entry.active = false;
				}
				std::strncpy(res.message, "Simulation reset", sizeof(res.message) - 1);
				break;
			case CommandType::SetTickRate:
				scheduler_.set_tick_rate(cmd.numeric_value);
				std::strncpy(res.message, "Tick rate updated", sizeof(res.message) - 1);
				break;
			case CommandType::SetParam:
				set_physical_param(cmd.param_type, cmd.numeric_value, cmd.custom_param_name);
				std::strncpy(res.message, "Parameter updated", sizeof(res.message) - 1);
				break;
			case CommandType::Status:
				std::strncpy(res.message, "Status reported", sizeof(res.message) - 1);
				break;
			case CommandType::Shutdown:
				is_running_.store(false, std::memory_order_release);
				std::strncpy(res.message, "Shutdown initiated", sizeof(res.message) - 1);
				break;
			case CommandType::None:
			default:
				res.success = false;
				std::strncpy(res.message, "Unknown command", sizeof(res.message) - 1);
				break;
		}
	}

	void set_physical_param(ParameterType param, double val, const char* custom_name = nullptr) noexcept {
		switch (param) {
			case ParameterType::Mass:
				params_.mass = val;
				break;
			case ParameterType::Spin:
				params_.spin = val;
				break;
			case ParameterType::Charge:
				params_.charge = val;
				break;
			case ParameterType::CosmologicalLambda:
				params_.cosmological_lambda = val;
				break;
			case ParameterType::WormholeThroat:
				params_.wormhole_throat = val;
				break;
			case ParameterType::WarpVelocity:
				params_.warp_velocity = val;
				break;
			case ParameterType::TickRate:
				scheduler_.set_tick_rate(val);
				break;
			case ParameterType::Custom:
				if (custom_name != nullptr && custom_name[0] != '\0') {
					for (auto& entry : custom_params_) {
						if (entry.active && std::strncmp(entry.name, custom_name, sizeof(entry.name)) == 0) {
							entry.value = val;
							return;
						}
					}
					for (auto& entry : custom_params_) {
						if (!entry.active) {
							entry.active = true;
							std::strncpy(entry.name, custom_name, sizeof(entry.name) - 1);
							entry.value = val;
							return;
						}
					}
				}
				break;
			default:
				break;
		}
	}

	[[nodiscard]] double get_custom_param(std::string_view name, double fallback = 0.0) const noexcept {
		for (const auto& entry : custom_params_) {
			if (entry.active && name == entry.name) {
				return entry.value;
			}
		}
		return fallback;
	}

	[[nodiscard]] constexpr Scheduler<double>& scheduler() noexcept {
		return scheduler_;
	}

	[[nodiscard]] constexpr const Scheduler<double>& scheduler() const noexcept {
		return scheduler_;
	}

	[[nodiscard]] constexpr const PhysicalParameters& parameters() const noexcept {
		return params_;
	}

	[[nodiscard]] constexpr PhysicalParameters& parameters() noexcept {
		return params_;
	}

	[[nodiscard]] bool is_running() const noexcept {
		return is_running_.load(std::memory_order_acquire);
	}

	void stop() noexcept {
		is_running_.store(false, std::memory_order_release);
	}

	[[nodiscard]] uint64_t total_commands_processed() const noexcept {
		return commands_processed_.load(std::memory_order_relaxed);
	}
};

}
