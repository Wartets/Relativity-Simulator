#include "relativistic/orchestrator/command.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/repl.hpp"
#include "relativistic/ui/ui_manager.hpp"
#include "relativistic/io/user_settings.hpp"
#include "relativistic/core/system_console.hpp"
#include "relativistic/core/engine_log.hpp"
#include "relativistic/core/diagnostics/diagnostics_session.hpp"
#include "relativistic/startup/application_info.hpp"
#include "relativistic/startup/platform_bootstrap.hpp"
#include "relativistic/startup/startup_progress.hpp"
#include "relativistic/startup/startup_window.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <exception>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <algorithm>
#include <cctype>
#include <memory>
#include <string_view>

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

#endif

namespace {

using SimulationHost = Relativistic::Orchestrator::SimulationOrchestrator<1024>;
using TerminalRepl = Relativistic::Orchestrator::MasterTerminalRepl<1024>;

struct CommandLineOptions {
	bool headless{false};
	bool skip_splash{false};
	bool show_help{false};
	bool show_version{false};
};

CommandLineOptions parse_command_line(int argc, char* argv[]) {
	CommandLineOptions options;
	for (int i = 1; i < argc; ++i) {
		const std::string_view argument(argv[i]);
		if (argument == "--headless" || argument == "-h") {
			options.headless = true;
		} else if (argument == "--no-splash") {
			options.skip_splash = true;
		} else if (argument == "--help" || argument == "-?") {
			options.show_help = true;
		} else if (argument == "--version") {
			options.show_version = true;
		}
	}
	return options;
}

void print_usage() {
	std::cout << "Usage: " << Relativistic::Startup::ApplicationInfo::kExecutableName << " [options]\n"
	          << "  --headless, -h   Run the simulation without a window and control it from the terminal\n"
	          << "  --no-splash      Do not display the startup window\n"
	          << "  --version        Print the application version\n"
	          << "  --help, -?       Print this message\n";
}

void print_version() {
	std::cout << Relativistic::Startup::ApplicationInfo::kProductName << " " << Relativistic::Startup::ApplicationInfo::kVersion << "\n";
}

void print_banner() {
	std::cout << "Relativistic Engine - Master Terminal Control Loop\n";
	std::cout << "Type 'help' for available commands, 'quit' to exit.\n\n";
}

std::string trim_console_line(const std::string& text) {
	const size_t first = text.find_first_not_of(" \t\r\n");
	if (first == std::string::npos) {
		return {};
	}
	const size_t last = text.find_last_not_of(" \t\r\n");
	return text.substr(first, last - first + 1);
}

std::string lowercase_console_line(std::string text) {
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return text;
}

void process_console_line(TerminalRepl& repl, const std::string& raw_line) {
	const std::string line = trim_console_line(raw_line);
	if (line.empty()) {
		return;
	}
	const std::string lowered = lowercase_console_line(line);
	if (lowered == "help" || lowered == "?") {
		repl.print_help();
		return;
	}
	if (lowered == "status") {
		repl.print_status();
		return;
	}
	Relativistic::Orchestrator::CommandResult result{};
	const bool ok = repl.execute_line(line, &result);
	if (!ok) {
		std::cout << "Error: " << result.message << "\n";
	} else if (result.message[0] != '\0') {
		std::cout << "OK: " << result.message << "\n";
	}
}

std::jthread start_simulation_thread(SimulationHost& orchestrator) {
	return std::jthread([&orchestrator](std::stop_token stop_token) {
		Relativistic::Core::Diagnostics::set_thread_name("simulation");
		auto last_time = std::chrono::steady_clock::now();
		while (!stop_token.stop_requested() && orchestrator.is_running()) {
			try {
				const auto current_time = std::chrono::steady_clock::now();
				const auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - last_time).count();
				last_time = current_time;

				orchestrator.scheduler().add_real_time_nanoseconds(elapsed_ns);
				Relativistic::Core::Diagnostics::set_activity("simulation: processing commands");
				orchestrator.process_incoming_commands();

				if (!orchestrator.parameters().schematic_mode_enabled || orchestrator.parameters().schematic_allow_simulation) {
					Relativistic::Core::Diagnostics::set_activity("simulation: advancing physics");
					uint32_t ticks_this_iteration = 0;
					while (orchestrator.scheduler().can_advance_tick() && ticks_this_iteration < 200U) {
						if (orchestrator.scheduler().advance_tick()) {
							const double tick_dt = orchestrator.scheduler().tick_dt() * orchestrator.scheduler().warp_factor();
							orchestrator.advance_simulation(tick_dt);
						}
						++ticks_this_iteration;
					}
				}
			} catch (const std::exception& ex) {
				Relativistic::Core::log_error(std::string("Simulation thread caught an exception and will continue: ") + ex.what());
			} catch (...) {
				Relativistic::Core::log_error("Simulation thread caught an unknown exception and will continue.");
			}

			if (orchestrator.scheduler().is_paused()) {
				Relativistic::Core::Diagnostics::set_activity("simulation: idle (paused)");
				std::this_thread::sleep_for(std::chrono::milliseconds(2));
			} else {
				std::this_thread::sleep_for(std::chrono::microseconds(500));
			}
		}
	});
}

int run_headless() {
	Relativistic::Core::SystemConsole::attach(true);

	auto orchestrator = std::make_unique<SimulationHost>();
	orchestrator->profiler().load_from_disk();
	TerminalRepl repl(*orchestrator);

	print_banner();

	std::jthread sim_thread = start_simulation_thread(*orchestrator);

	std::cout << "Running in headless mode. Press Ctrl+C or send 'shutdown' to exit.\n";
	std::string line;
	while (orchestrator->is_running()) {
		try {
			repl.print_prompt();
			if (!std::getline(std::cin, line)) {
				break;
			}
			process_console_line(repl, line);
		} catch (const std::exception& ex) {
			Relativistic::Core::log_error(std::string("Headless command loop caught an exception and will continue: ") + ex.what());
		} catch (...) {
			Relativistic::Core::log_error("Headless command loop caught an unknown exception and will continue.");
		}
	}

	orchestrator->profiler().save_to_disk();
	orchestrator->stop();
	sim_thread.request_stop();
	sim_thread.join();
	Relativistic::Core::Diagnostics::DiagnosticsSession::end_clean();
	return 0;
}

int run_interactive(const CommandLineOptions& options) {
	using namespace Relativistic;
	namespace Stage = Startup::StartupStage;

	Startup::StartupProgress progress;
	Core::Diagnostics::DiagnosticsSession::attach_startup(progress);
	Core::Diagnostics::set_phase(Core::Diagnostics::AppPhase::Startup);
	Startup::set_active_progress(&progress);
	Startup::StartupWindow splash;
	if (!options.skip_splash) {
		static_cast<void>(splash.open(progress));
	}

	std::unique_ptr<SimulationHost> orchestrator;
	std::unique_ptr<IO::UserSettings> user_settings;
	std::jthread sim_thread;
	std::unique_ptr<TerminalRepl> repl;
	std::unique_ptr<UI::UiManager> ui_manager;
	bool session_marked = false;
	bool cancelled = false;
	std::string failure_message;

	try {
		progress.report(Stage::Runtime, 0.0f, "Preparing runtime", "Initializing process environment...");
		const unsigned int logical_processors = std::thread::hardware_concurrency();
		progress.report(
			Stage::Runtime, 0.6f, "Preparing runtime",
			(logical_processors > 0U) ? (std::to_string(logical_processors) + " logical processors available") : std::string("Detecting hardware concurrency...")
		);
		progress.throw_if_cancelled();

		progress.report(Stage::Settings, 0.0f, "Loading configuration", "Reading user settings from config/user_settings.cfg...");
		user_settings = std::make_unique<IO::UserSettings>(IO::UserSettings::load_or_default());
		progress.report(Stage::Settings, 0.5f, "Loading configuration", "User settings loaded (format version " + std::to_string(user_settings->format_version) + ")");
		if (IO::UserSettings::previous_session_crashed()) {
			Core::log_warning("The previous session did not terminate cleanly.");
			progress.report(Stage::Settings, 0.8f, "Loading configuration", "Previous session ended unexpectedly, validating restored state...");
		}
		IO::UserSettings::mark_session_started();
		session_marked = true;
		progress.throw_if_cancelled();

		progress.report(Stage::Simulation, 0.0f, "Starting simulation core", "Creating simulation orchestrator and command queues...");
		orchestrator = std::make_unique<SimulationHost>();
		progress.report(Stage::Simulation, 0.4f, "Starting simulation core", "Restoring performance profiler history...");
		orchestrator->profiler().load_from_disk();
		progress.report(Stage::Simulation, 0.7f, "Starting simulation core", std::to_string(orchestrator->profiler().saved_runs().size()) + " benchmark runs restored");
		progress.report(Stage::Simulation, 0.85f, "Starting simulation core", "Starting the simulation thread...");
		sim_thread = start_simulation_thread(*orchestrator);
		repl = std::make_unique<TerminalRepl>(*orchestrator);
		progress.report(Stage::Simulation, 1.0f, "Starting simulation core", "Constructing user interface modules...");
		ui_manager = std::make_unique<UI::UiManager>(*orchestrator, *user_settings);

		ui_manager->initialize();

		progress.report(Stage::Finalize, 0.0f, "Finalizing", "Verifying simulation state...");
		progress.throw_if_cancelled();
		if (user_settings->show_system_console) {
			progress.report(Stage::Finalize, 0.4f, "Finalizing", "Preparing the system console...");
			Core::SystemConsole::set_visible(true);
		}
		progress.report(Stage::Finalize, 0.8f, "Ready", "Opening the workspace...");
		progress.throw_if_cancelled();

		ui_manager->reveal_window();
		progress.complete();
		Core::Diagnostics::set_phase(Core::Diagnostics::AppPhase::Running);
		Core::log_info("Startup completed in " + std::to_string(Core::Diagnostics::uptime_seconds()) + " s, workspace ready.");
		Startup::set_active_progress(nullptr);
		splash.close();
	} catch (const Startup::StartupCancelled&) {
		cancelled = true;
	} catch (const std::exception& ex) {
		failure_message = ex.what();
	} catch (...) {
		failure_message = "Unknown startup failure.";
	}

	if (progress.cancel_requested()) {
		cancelled = true;
	}

	if (cancelled || !failure_message.empty()) {
		Startup::set_active_progress(nullptr);
		splash.close();
		ui_manager.reset();
		if (orchestrator) {
			orchestrator->stop();
		}
		if (sim_thread.joinable()) {
			sim_thread.request_stop();
			sim_thread.join();
		}
		if (session_marked) {
			IO::UserSettings::mark_session_ended_cleanly();
		}
		if (!cancelled) {
			Core::log_error("Startup failed: " + failure_message);
			Startup::PlatformBootstrap::show_fatal_error(Startup::ApplicationInfo::kProductName, "The application could not start.\n\n" + failure_message);
			return EXIT_FAILURE;
		}
		return 0;
	}

	std::thread console_thread([&repl, &orchestrator]() {
		Core::Diagnostics::set_thread_name("console");
		bool banner_printed = false;
		std::string console_line;
		while (orchestrator->is_running()) {
			if (!Core::SystemConsole::is_attached()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(250));
				continue;
			}
			if (!banner_printed) {
				print_banner();
				banner_printed = true;
			}
			repl->print_prompt();
			if (!std::getline(std::cin, console_line)) {
				break;
			}
			try {
				process_console_line(*repl, console_line);
			} catch (const std::exception& ex) {
				Core::log_error(std::string("Console command failed: ") + ex.what());
			} catch (...) {
				Core::log_error("Console command failed with an unknown error.");
			}
		}
	});
	console_thread.detach();

	while (orchestrator->is_running() && !ui_manager->should_close()) {
		try {
			Core::Diagnostics::count_frame();
			Core::Diagnostics::set_activity("ui: rendering frame");
			ui_manager->render_frame();
		} catch (const std::exception& ex) {
			Core::log_error(std::string("UI frame render failed and was skipped: ") + ex.what());
		} catch (...) {
			Core::log_error("UI frame render failed with an unknown error and was skipped.");
		}
	}
	orchestrator->stop();

	ui_manager->export_runtime_settings();
	user_settings->save();
	ui_manager.reset();
	IO::UserSettings::mark_session_ended_cleanly();

	orchestrator->profiler().save_to_disk();
	orchestrator->stop();
	sim_thread.request_stop();
	sim_thread.join();
	Core::Diagnostics::DiagnosticsSession::end_clean();
	std::fflush(stdout);
	std::fflush(stderr);
	std::_Exit(0);
}

}

int main(int argc, char* argv[]) {
	Relativistic::Startup::PlatformBootstrap::initialize();
#if defined(_WIN32)
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif

	static_cast<void>(Relativistic::Core::Diagnostics::DiagnosticsSession::begin(Relativistic::Core::Diagnostics::SessionDescriptor{
		.product_name = Relativistic::Startup::ApplicationInfo::kProductName,
		.product_version = Relativistic::Startup::ApplicationInfo::kVersion,
		.argument_count = argc,
		.arguments = argv
	}));

	const CommandLineOptions options = parse_command_line(argc, argv);

	if (options.show_help || options.show_version) {
		Relativistic::Core::SystemConsole::attach(true);
		if (options.show_version) {
			print_version();
		} else {
			print_usage();
		}
		std::cout.flush();
		return 0;
	}

	if (options.headless) {
		return run_headless();
	}
	return run_interactive(options);
}
