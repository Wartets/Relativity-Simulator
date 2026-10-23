#pragma once

#include "relativistic/core/diagnostics/crash_reporter.hpp"
#include "relativistic/core/diagnostics/diagnostics_paths.hpp"
#include "relativistic/core/diagnostics/diagnostics_registry.hpp"
#include "relativistic/core/diagnostics/system_report.hpp"
#include "relativistic/core/engine_log.hpp"
#include "relativistic/startup/platform_bootstrap.hpp"
#include "relativistic/startup/startup_progress.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>

namespace Relativistic::Core::Diagnostics {

struct SessionDescriptor {
	std::string_view product_name{};
	std::string_view product_version{};
	int argument_count{0};
	char** arguments{nullptr};
};

class DiagnosticsSession {
private:
	static inline DiagnosticsPaths paths_{};
	static inline std::time_t start_time_{0};
	static inline bool active_{false};

	static void record_log_event(std::string_view line) noexcept {
		record_event(line);
	}

	[[nodiscard]] static std::string compiler_description() {
#if defined(_MSC_VER)
		return "MSVC " + std::to_string(_MSC_FULL_VER);
#elif defined(__clang__)
		return std::string("Clang ") + __clang_version__;
#elif defined(__GNUC__)
		return std::string("GCC ") + __VERSION__;
#else
		return "unknown compiler";
#endif
	}

	[[nodiscard]] static const char* architecture_name() noexcept {
#if defined(__x86_64__) || defined(_M_X64)
		return "x86-64";
#elif defined(__aarch64__) || defined(_M_ARM64)
		return "arm64";
#elif defined(__i386__) || defined(_M_IX86)
		return "x86";
#else
		return "unknown architecture";
#endif
	}

	[[nodiscard]] static const char* platform_name() noexcept {
#if defined(_WIN32)
		return "Windows";
#elif defined(__linux__)
		return "Linux";
#elif defined(__APPLE__)
		return "macOS";
#else
		return "unknown platform";
#endif
	}

	[[nodiscard]] static std::string build_description() {
#if defined(NDEBUG)
		const char* configuration = "release";
#else
		const char* configuration = "debug";
#endif
		std::string text;
		append_format(text, "%s | %s | %s | %s | C++ standard macro %ld | built %s %s", compiler_description().c_str(), platform_name(), architecture_name(), configuration, static_cast<long>(__cplusplus), __DATE__, __TIME__);
		return text;
	}

	[[nodiscard]] static std::string join_arguments(const SessionDescriptor& descriptor) {
		std::string joined;
		for (int index = 0; index < descriptor.argument_count && descriptor.arguments != nullptr; ++index) {
			if (descriptor.arguments[index] == nullptr) {
				continue;
			}
			if (!joined.empty()) {
				joined.push_back(' ');
			}
			joined.append(descriptor.arguments[index]);
		}
		return joined;
	}

	[[nodiscard]] static CrashConfiguration make_configuration(const SessionDescriptor& descriptor) {
		std::error_code ec;
		CrashConfiguration configuration;
		configuration.product_name = std::string(descriptor.product_name);
		configuration.product_version = std::string(descriptor.product_version);
		configuration.session_id = paths_.session_id;
		configuration.build_description = build_description();
		if (descriptor.arguments != nullptr && descriptor.argument_count > 0 && descriptor.arguments[0] != nullptr) {
			configuration.executable_path = std::filesystem::absolute(std::filesystem::path(descriptor.arguments[0]), ec).generic_string();
		}
		configuration.working_directory = std::filesystem::current_path(ec).generic_string();
		configuration.command_line = join_arguments(descriptor);
		configuration.crashes_directory = paths_.crashes_directory.generic_string();
		configuration.session_log_path = paths_.session_log_file.generic_string();
		configuration.settings_path = std::filesystem::absolute(std::filesystem::path("config") / "user_settings.cfg", ec).generic_string();
		configuration.marker_path = paths_.crash_marker_file.generic_string();
		return configuration;
	}

	static void announce_previous_crash() {
		if (paths_.crash_marker_file.empty()) {
			return;
		}
		std::error_code ec;
		if (!std::filesystem::exists(paths_.crash_marker_file, ec)) {
			return;
		}
		std::string folder;
		{
			std::ifstream marker(paths_.crash_marker_file);
			std::getline(marker, folder);
		}
		log_warning("The previous session terminated with a crash. Its report folder was: " + folder);
		const std::filesystem::path acknowledged = paths_.root_directory / "last_crash_acknowledged.txt";
		std::filesystem::remove(acknowledged, ec);
		std::filesystem::rename(paths_.crash_marker_file, acknowledged, ec);
	}

	static void publish_environment(const CrashConfiguration& configuration) {
		char brand[96];
		SystemReport::query_cpu_brand(brand, sizeof(brand));
		char stamp[48];
		format_wall_clock(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", start_time_);
		set_environment("product", configuration.product_name);
		set_environment("version", configuration.product_version);
		set_environment("session.id", configuration.session_id);
		set_environment("session.started", stamp);
		set_environment("build", configuration.build_description);
		set_environment("executable", configuration.executable_path);
		set_environment("executable.directory", Startup::PlatformBootstrap::executable_directory().generic_string());
		set_environment("working.directory", configuration.working_directory);
		set_environment("command.line", configuration.command_line);
		set_environment("logs.directory", paths_.root_directory.generic_string());
		set_environment("logs.relocated", paths_.relocated ? "yes" : "no");
		set_environment("cpu.brand", brand);
		set_environment("cpu.logical.processors", std::to_string(std::thread::hardware_concurrency()));
		static constexpr std::array<const char*, 12> kVariables{
			"VK_ICD_FILENAMES", "VK_INSTANCE_LAYERS", "VK_LAYER_PATH", "DISPLAY", "WAYLAND_DISPLAY", "XDG_SESSION_TYPE",
			"LANG", "PROCESSOR_ARCHITECTURE", "NUMBER_OF_PROCESSORS", "SESSIONNAME", "MESA_LOADER_DRIVER_OVERRIDE", "__GL_THREADED_OPTIMIZATIONS"
		};
		for (const char* name : kVariables) {
			if (const char* value = std::getenv(name); value != nullptr && value[0] != '\0') {
				set_environment(std::string("env.") + name, value);
			}
		}
	}

	static void write_header(const CrashConfiguration& configuration) {
		char brand[96];
		SystemReport::query_cpu_brand(brand, sizeof(brand));
		char stamp[48];
		format_wall_clock(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", start_time_);
		log_info("==== session start ====");
		log_info("Product: " + configuration.product_name + " " + configuration.product_version);
		log_info("Session id: " + configuration.session_id + " (started " + stamp + ")");
		log_info("Build: " + configuration.build_description);
		log_info("Executable: " + configuration.executable_path);
		log_info("Working directory: " + configuration.working_directory);
		log_info("Command line: " + configuration.command_line);
		log_info("Log directory: " + paths_.root_directory.generic_string() + (paths_.relocated ? " (relocated, the working directory is not writable)" : ""));
		log_info("Processor: " + std::string(brand) + ", " + std::to_string(std::thread::hardware_concurrency()) + " logical processors");
		if (paths_.root_directory.empty()) {
			log_warning("No writable log directory was found; session logs and crash reports cannot be stored on disk.");
		}
	}

public:
	static bool begin(const SessionDescriptor& descriptor) noexcept {
		try {
			initialize_registry();
			set_phase(AppPhase::Launching);
			set_thread_name("main");
			start_time_ = std::time(nullptr);
			paths_ = DiagnosticsLocator::resolve(start_time_);
			if (!paths_.session_log_file.empty() && !EngineLog::instance().open_session_file(paths_.session_log_file.string())) {
				log_warning("The session log file could not be opened: " + paths_.session_log_file.generic_string());
			}
			EngineLog::instance().set_event_sink(&DiagnosticsSession::record_log_event);
			announce_previous_crash();
			const CrashConfiguration configuration = make_configuration(descriptor);
			publish_environment(configuration);
			CrashReporter::configure(configuration);
			CrashReporter::install();
			write_header(configuration);
			active_ = true;
			return true;
		} catch (...) {
			return false;
		}
	}

	static void attach_startup(Startup::StartupProgress& progress) {
		progress.set_observer([](const Startup::StartupSnapshot& snapshot) {
			std::string text;
			append_format(text, "  progress %.1f%%\n  stage    %s\n  detail   %s\n  phase    %u\n  cancel   %s\n", static_cast<double>(snapshot.fraction) * 100.0, snapshot.stage.c_str(), snapshot.detail.c_str(), static_cast<unsigned>(snapshot.phase), snapshot.cancel_requested ? "requested" : "no");
			if (!snapshot.error.empty()) {
				append_format(text, "  error    %s\n", snapshot.error.c_str());
			}
			publish_startup_snapshot(text);
			std::string line;
			append_format(line, "Startup %5.1f%% [%s] %s", static_cast<double>(snapshot.fraction) * 100.0, snapshot.stage.c_str(), snapshot.detail.c_str());
			log_trace(line);
		});
	}

	static void end_clean() noexcept {
		if (!active_) {
			return;
		}
		try {
			set_phase(AppPhase::ShuttingDown);
			std::string text;
			append_format(text, "Session ended cleanly after %.1f s (%llu UI frames, %llu commands processed)", uptime_seconds(), static_cast<unsigned long long>(g_registry.frame_counter.load(std::memory_order_relaxed)), static_cast<unsigned long long>(g_registry.command_counter.load(std::memory_order_relaxed)));
			log_info(text);
			log_info("==== session end ====");
			EngineLog::instance().close_session_file();
		} catch (...) {
		}
		active_ = false;
	}

	[[nodiscard]] static const DiagnosticsPaths& paths() noexcept {
		return paths_;
	}
};

}
