#include "relativistic/orchestrator/command.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/repl.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
	using namespace Relativistic::Orchestrator;

	bool headless = false;
	for (int i = 1; i < argc; ++i) {
		if (std::string_view(argv[i]) == "--headless" || std::string_view(argv[i]) == "-h") {
			headless = true;
		}
	}

	auto orchestrator = std::make_unique<SimulationOrchestrator<16384>>();
	MasterTerminalRepl<16384> repl(*orchestrator);

	std::cout << "Relativistic Engine - Master Terminal REPL v0.3.0\n";
	std::cout << "Type 'help' for commands list, 'quit' to exit.\n\n";

	std::jthread sim_thread([&orchestrator](std::stop_token stop_token) {
		auto last_time = std::chrono::steady_clock::now();
		while (!stop_token.stop_requested() && orchestrator->is_running()) {
			const auto current_time = std::chrono::steady_clock::now();
			const auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - last_time).count();
			last_time = current_time;

			orchestrator->scheduler().add_real_time_nanoseconds(elapsed_ns);
			orchestrator->process_incoming_commands();

			while (orchestrator->scheduler().can_advance_tick()) {
				orchestrator->scheduler().advance_tick();
			}

			std::this_thread::sleep_for(std::chrono::microseconds(500));
		}
	});

	if (headless) {
		std::cout << "Running in headless mode. Press Ctrl+C or send 'shutdown' to exit.\n";
		while (orchestrator->is_running()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	} else {
		std::string line;
		while (orchestrator->is_running()) {
			repl.print_prompt();
			if (!std::getline(std::cin, line)) {
				break;
			}
			if (line.empty()) {
				continue;
			}
			if (line == "help") {
				repl.print_help();
				continue;
			}
			if (line == "status") {
				repl.print_status();
				continue;
			}

			CommandResult result{};
			const bool ok = repl.execute_line(line, &result);
			if (!ok) {
				std::cout << "Error: " << result.message << "\n";
			} else if (result.message[0] != '\0') {
				std::cout << "OK: " << result.message << "\n";
			}
		}
	}

	orchestrator->stop();
	sim_thread.request_stop();
	return 0;
}
