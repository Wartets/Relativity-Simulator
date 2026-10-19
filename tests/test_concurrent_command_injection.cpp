#include "support/test_harness.hpp"
#include "relativistic/orchestrator/command.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <thread>

using namespace Relativistic::Orchestrator;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"concurrent_command_injection",
		"Verifies the lock-free command path under load: one hundred thousand commands of eight different kinds are injected by a producer thread while a simulation thread drains the queue and advances the scheduler, and every command is processed with the scheduler making progress and the injected parameters staying in their injected ranges."
	);

	constexpr size_t total_commands = 100000;
	constexpr size_t queue_capacity = 32768;

	suite.section("Concurrent producer and simulation worker");
	auto orchestrator = std::make_unique<SimulationOrchestrator<queue_capacity>>();
	orchestrator->scheduler().set_tick_rate(1000.0);

	std::atomic<bool> producer_done{false};
	std::atomic<uint64_t> ticks_executed{0};

	std::thread simulation_worker([&orchestrator, &producer_done, &ticks_executed]() {
		auto last_time = std::chrono::steady_clock::now();
		while (orchestrator->is_running() && (!producer_done.load(std::memory_order_acquire) || orchestrator->total_commands_processed() < total_commands)) {
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
		for (size_t i = 0; i < total_commands; ++i) {
			Command command{};
			switch (i % 8) {
				case 0:
					command = Command::make_warp(1.0 + static_cast<double>(i % 10) * 0.1);
					break;
				case 1:
					command = Command::make_set_param(ParameterType::Mass, 1.0 + static_cast<double>(i % 100) * 0.01);
					break;
				case 2:
					command = Command::make_set_param(ParameterType::Spin, static_cast<double>(i % 100) * 0.009);
					break;
				case 3:
					command = Command::make_set_param(ParameterType::Charge, static_cast<double>(i % 50) * 0.01);
					break;
				case 4:
					command = Command::make_step(1);
					break;
				case 5:
					command = Command::make_set_tickrate(500.0 + static_cast<double>(i % 500));
					break;
				case 6:
					command = Command::make_resume();
					break;
				default:
					command = Command::make_status();
					break;
			}
			while (!orchestrator->enqueue_command(command)) {
				std::this_thread::yield();
			}
		}
		producer_done.store(true, std::memory_order_release);
	});

	producer.join();

	bool timed_out = false;
	const auto timeout_start = std::chrono::steady_clock::now();
	while (orchestrator->total_commands_processed() < total_commands) {
		const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - timeout_start).count();
		if (elapsed >= 10) {
			timed_out = true;
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	orchestrator->stop();
	simulation_worker.join();

	suite.check(!timed_out, "the simulation worker drains the queue within ten seconds of the end of production");
	suite.check(orchestrator->total_commands_processed() == total_commands, "every injected command is processed exactly once", "processed=" + std::to_string(orchestrator->total_commands_processed()));
	suite.check(orchestrator->scheduler().current_tick() > 0, "the scheduler advances while commands are being injected");
	suite.check_finite(orchestrator->parameters().mass, "the injected mass stays finite");
	suite.check(orchestrator->parameters().mass >= 1.0 && orchestrator->parameters().mass < 2.0, "the final mass equals one of the injected values in [1, 2)");
	suite.measure("ticks_executed", static_cast<double>(ticks_executed.load()), "ticks");
	suite.measure("commands_processed", static_cast<double>(orchestrator->total_commands_processed()), "commands");
	return suite.finish();
}
