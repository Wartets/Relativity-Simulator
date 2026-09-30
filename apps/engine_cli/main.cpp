#include "relativistic/orchestrator/command.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/repl.hpp"
#include "relativistic/ui/ui_manager.hpp"
#include "relativistic/io/user_settings.hpp"
#include "relativistic/core/system_console.hpp"
#include "relativistic/core/engine_log.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <exception>
#include <cstdlib>
#include <cstdio>
#include <ctime>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}

namespace {

LONG WINAPI relativistic_crash_handler(EXCEPTION_POINTERS* info) noexcept {
	if (info == nullptr || info->ExceptionRecord == nullptr) {
		return EXCEPTION_CONTINUE_SEARCH;
	}
	const DWORD code = info->ExceptionRecord->ExceptionCode;
	if (code != EXCEPTION_ACCESS_VIOLATION &&
	    code != EXCEPTION_STACK_OVERFLOW &&
	    code != EXCEPTION_ILLEGAL_INSTRUCTION &&
	    code != EXCEPTION_ARRAY_BOUNDS_EXCEEDED &&
	    code != EXCEPTION_INT_DIVIDE_BY_ZERO) {
		return EXCEPTION_CONTINUE_SEARCH;
	}
	FILE* crash_file = std::fopen("crash_report.log", "a");
	if (crash_file != nullptr) {
		const std::time_t now = std::time(nullptr);
		std::fprintf(
			crash_file,
			"[%ld] Native exception code=0x%08lX address=%p\n",
			static_cast<long>(now),
			static_cast<unsigned long>(code),
			info->ExceptionRecord->ExceptionAddress
		);
		std::fflush(crash_file);
		std::fclose(crash_file);
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

}
#endif

int main(int argc, char* argv[]) {
	std::setvbuf(stdout, nullptr, _IONBF, 0);
	std::setvbuf(stderr, nullptr, _IONBF, 0);
#if defined(_WIN32)
	AddVectoredExceptionHandler(1, relativistic_crash_handler);
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif

	std::set_terminate([]() noexcept {
		try {
			if (const auto current_exception_ptr = std::current_exception()) {
				std::rethrow_exception(current_exception_ptr);
			}
		} catch (const std::exception& ex) {
			Relativistic::Core::log_error(std::string("Unhandled exception reached top level, engine is terminating: ") + ex.what());
		} catch (...) {
			Relativistic::Core::log_error("Unhandled non-standard exception reached top level, engine is terminating.");
		}
		std::abort();
	});

	using namespace Relativistic::Orchestrator;

	bool headless = false;
	for (int i = 1; i < argc; ++i) {
		if (std::string_view(argv[i]) == "--headless" || std::string_view(argv[i]) == "-h") {
			headless = true;
		}
	}

	auto orchestrator = std::make_unique<SimulationOrchestrator<1024>>();
	orchestrator->profiler().load_from_disk();
	MasterTerminalRepl<1024> repl(*orchestrator);

	std::cout << "Relativistic Engine - Master Terminal Control Loop\n";
	std::cout << "Type 'help' for available commands, 'quit' to exit.\n\n";

	std::jthread sim_thread([&orchestrator](std::stop_token stop_token) {
		auto last_time = std::chrono::steady_clock::now();
		while (!stop_token.stop_requested() && orchestrator->is_running()) {
			try {
				const auto current_time = std::chrono::steady_clock::now();
				const auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - last_time).count();
				last_time = current_time;

				orchestrator->scheduler().add_real_time_nanoseconds(elapsed_ns);
				orchestrator->process_incoming_commands();

				if (!orchestrator->parameters().schematic_mode_enabled || orchestrator->parameters().schematic_allow_simulation) {
					uint32_t ticks_this_iteration = 0;
					while (orchestrator->scheduler().can_advance_tick() && ticks_this_iteration < 200U) {
						if (orchestrator->scheduler().advance_tick()) {
							const double tick_dt = orchestrator->scheduler().tick_dt() * orchestrator->scheduler().warp_factor();
							orchestrator->advance_simulation(tick_dt);
						}
						++ticks_this_iteration;
					}
				}
			} catch (const std::exception& ex) {
				Relativistic::Core::log_error(std::string("Simulation thread caught an exception and will continue: ") + ex.what());
			} catch (...) {
				Relativistic::Core::log_error("Simulation thread caught an unknown exception and will continue.");
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
			try {
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
			} catch (const std::exception& ex) {
				Relativistic::Core::log_error(std::string("Headless command loop caught an exception and will continue: ") + ex.what());
			} catch (...) {
				Relativistic::Core::log_error("Headless command loop caught an unknown exception and will continue.");
			}
		}
	} else {
		Relativistic::IO::UserSettings user_settings = Relativistic::IO::UserSettings::load_or_default();
		Relativistic::IO::UserSettings::mark_session_started();
		Relativistic::Core::SystemConsole::set_visible(user_settings.show_system_console);

		Relativistic::UI::UiManager ui_manager(*orchestrator, user_settings);
		ui_manager.initialize();

		while (orchestrator->is_running() && !ui_manager.should_close()) {
			try {
				ui_manager.render_frame();
			} catch (const std::exception& ex) {
				Relativistic::Core::log_error(std::string("UI frame render failed and was skipped: ") + ex.what());
			} catch (...) {
				Relativistic::Core::log_error("UI frame render failed with an unknown error and was skipped.");
			}
		}
		orchestrator->stop();

		ui_manager.export_runtime_settings();
		user_settings.save();
		Relativistic::IO::UserSettings::mark_session_ended_cleanly();
	}

	orchestrator->profiler().save_to_disk();
	orchestrator->stop();
	sim_thread.request_stop();
	return 0;
}
