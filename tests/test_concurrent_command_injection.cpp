#include "relativistic/orchestrator/command.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include <cassert>
#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>

int main() {
	using namespace Relativistic::Orchestrator;

	constexpr size_t TOTAL_COMMANDS = 100000;
	constexpr size_t QUEUE_CAPACITY = 32768;

	auto orchestrator = std::make_unique<SimulationOrchestrator<QUEUE_CAPACITY>>();
	orchestrator->scheduler().set_tick_rate(1000.0);

	std::atomic<bool> producer_done{false};
	std::atomic<uint64_t> ticks_executed{0};

	std::thread sim_worker([&orchestrator, &producer_done, &ticks_executed]() {
		auto last_time = std::chrono::steady_clock::now();
		while (orchestrator->is_running() && (!producer_done.load(std::memory_order_acquire) || orchestrator->total_commands_processed() < TOTAL_COMMANDS)) {
			const auto current_time = std::chrono::steady_clock::now();
			const auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - last_time).count();
			last_time = current_time;

			orchestrator->scheduler().add_real_time_nanoseconds(elapsed_ns);
			orchestrator->process_incoming_commands();

			while (orchestrator->scheduler().can_advance_tick()) {
				if (orchestrator->scheduler().advance_tick()) {
					ticks_executed.fetch_add(1, std::memory_order_relaxed);
				}
			}

			std::this_thread::yield();
		}
	});

	std::thread producer([&orchestrator, &producer_done]() {
		for (size_t i = 0; i < TOTAL_COMMANDS; ++i) {
			Command cmd{};
			const size_t type_idx = i % 8;
			switch (type_idx) {
				case 0:
					cmd = Command::make_warp(1.0 + static_cast<double>(i % 10) * 0.1);
					break;
				case 1:
					cmd = Command::make_set_param(ParameterType::Mass, 1.0 + static_cast<double>(i % 100) * 0.01);
					break;
				case 2:
					cmd = Command::make_set_param(ParameterType::Spin, static_cast<double>(i % 100) * 0.009);
					break;
				case 3:
					cmd = Command::make_set_param(ParameterType::Charge, static_cast<double>(i % 50) * 0.01);
					break;
				case 4:
					cmd = Command::make_step(1);
					break;
				case 5:
					cmd = Command::make_set_tickrate(500.0 + static_cast<double>(i % 500));
					break;
				case 6:
					cmd = Command::make_resume();
					break;
				case 7:
				default:
					cmd = Command::make_status();
					break;
			}

			while (!orchestrator->enqueue_command(cmd)) {
				std::this_thread::yield();
			}
		}
		producer_done.store(true, std::memory_order_release);
	});

	producer.join();

	const auto timeout_start = std::chrono::steady_clock::now();
	while (orchestrator->total_commands_processed() < TOTAL_COMMANDS) {
		const auto now = std::chrono::steady_clock::now();
		const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - timeout_start).count();
		assert(elapsed < 10);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	orchestrator->stop();
	sim_worker.join();

	assert(orchestrator->total_commands_processed() == TOTAL_COMMANDS);
	assert(orchestrator->scheduler().current_tick() > 0);
	assert(orchestrator->parameters().mass >= 1.0);

	return 0;
}
