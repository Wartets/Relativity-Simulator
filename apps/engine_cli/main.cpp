#include "relativistic/orchestrator/command.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/repl.hpp"
#include "relativistic/ui/ui_manager.hpp"
#include "relativistic/io/user_settings.hpp"
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

	auto orchestrator = std::make_unique<SimulationOrchestrator<1024>>();
	MasterTerminalRepl<1024> repl(*orchestrator);

	std::cout << "Relativistic Engine - Master Terminal Control Loop\n";
	std::cout << "Type 'help' for available commands, 'quit' to exit.\n\n";

	std::jthread sim_thread([&orchestrator](std::stop_token stop_token) {
		auto last_time = std::chrono::steady_clock::now();
		while (!stop_token.stop_requested() && orchestrator->is_running()) {
			const auto current_time = std::chrono::steady_clock::now();
			const auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - last_time).count();
			last_time = current_time;

			orchestrator->scheduler().add_real_time_nanoseconds(elapsed_ns);
			orchestrator->process_incoming_commands();

			if (!orchestrator->parameters().schematic_mode_enabled) {
				while (orchestrator->scheduler().can_advance_tick()) {
					orchestrator->scheduler().advance_tick();
				}
			}

			if (orchestrator->scheduler().is_paused()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(2));
			} else {
				std::this_thread::sleep_for(std::chrono::microseconds(500));
			}
		}
	});

	if (headless) {
		std::cout << "Running in headless mode. Press Ctrl+C or send 'shutdown' to exit.\n";
		std::string line;
		while (orchestrator->is_running()) {
			repl.print_prompt();
			if (!std::getline(std::cin, line)) {
				break;
			}
			if (line.empty()) continue;
			if (line == "help") { repl.print_help(); continue; }
			if (line == "status") { repl.print_status(); continue; }

			CommandResult result{};
			const bool ok = repl.execute_line(line, &result);
			if (!ok) std::cout << "Error: " << result.message << "\n";
			else if (result.message[0] != '\0') std::cout << "OK: " << result.message << "\n";
		}
	} else {
		Relativistic::IO::UserSettings user_settings = Relativistic::IO::UserSettings::load_or_default();
		Relativistic::IO::UserSettings::mark_session_started();

		Relativistic::UI::UiManager ui_manager(*orchestrator, user_settings);
		ui_manager.initialize();

		for (int i = 1; i <= 2; ++i) {
			ui_manager.add_secondary_view("Secondary Observer Camera " + std::to_string(i));
		}

		while (orchestrator->is_running() && !ui_manager.should_close()) {
			ui_manager.render_frame();
		}
		orchestrator->stop();

		ui_manager.export_runtime_settings();
		user_settings.save();
		Relativistic::IO::UserSettings::mark_session_ended_cleanly();
	}

	orchestrator->stop();
	sim_thread.request_stop();
	return 0;
}
