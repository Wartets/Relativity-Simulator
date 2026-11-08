#include "relativistic/startup/application_info.hpp"
#include "relativistic/startup/platform_bootstrap.hpp"
#include "relativistic/startup/startup_progress.hpp"
#include "relativistic/startup/startup_window.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <thread>
#include <vector>

namespace {

namespace Startup = Relativistic::Startup;

struct PreviewOptions {
	double speed{1.0};
	double hold_seconds{1.5};
	double compile_seconds{4.0};
	bool simulate_failure{false};
	bool show_help{false};
};

struct PreviewStep {
	Startup::StartupRange range;
	std::string_view stage;
	std::vector<std::string_view> details;
	double seconds;
};

constexpr size_t kRenderBackendStepIndex = 5;
constexpr double kFailureHoldSeconds = 6.0;

void print_usage() {
	std::cout << "Usage: startup_preview [options]\n"
	          << "  --speed <factor>      Playback speed multiplier (default 1.0)\n"
	          << "  --hold <seconds>      Time the completed window stays visible (default 1.5)\n"
	          << "  --compile <seconds>   Simulated driver compilation time (default 4.0)\n"
	          << "  --fail                End the sequence with a simulated startup failure\n"
	          << "  --help, -h            Print this message\n";
}

[[nodiscard]] double parse_positive(const char* text, double fallback) noexcept {
	char* end = nullptr;
	const double value = std::strtod(text, &end);
	return (end != text && value > 0.0) ? value : fallback;
}

[[nodiscard]] PreviewOptions parse_options(int argc, char** argv) {
	PreviewOptions options;
	for (int i = 1; i < argc; ++i) {
		const std::string_view argument(argv[i]);
		if (argument == "--help" || argument == "-h") {
			options.show_help = true;
		} else if (argument == "--fail") {
			options.simulate_failure = true;
		} else if (argument == "--speed" && i + 1 < argc) {
			options.speed = parse_positive(argv[++i], options.speed);
		} else if (argument == "--hold" && i + 1 < argc) {
			options.hold_seconds = parse_positive(argv[++i], options.hold_seconds);
		} else if (argument == "--compile" && i + 1 < argc) {
			options.compile_seconds = parse_positive(argv[++i], options.compile_seconds);
		}
	}
	return options;
}

[[nodiscard]] std::vector<PreviewStep> build_steps() {
	namespace Stage = Startup::StartupStage;
	return {
		{Stage::Runtime, "Preparing runtime", {"Initializing process environment...", "8 logical processors available"}, 0.5},
		{Stage::Settings, "Loading configuration", {"Reading user settings from config/user_settings.cfg...", "User settings loaded (format version 5)"}, 0.5},
		{Stage::Simulation, "Starting simulation core", {"Creating simulation orchestrator and command queues...", "Restoring performance profiler history...", "Starting the simulation thread..."}, 0.7},
		{Stage::WindowSystem, "Creating window system", {"Initializing GLFW...", "Creating the main window (hidden until startup completes)..."}, 0.5},
		{Stage::GraphicsContext, "Initializing graphics", {"OpenGL context ready", "Creating the Dear ImGui and ImPlot contexts...", "Initializing the GLFW and OpenGL 3.3 platform backends..."}, 0.6},
		{Stage::RenderBackend, "Starting render backend", {
			"Creating the Vulkan instance (API 1.2)...",
			"Enumerating Vulkan physical devices...",
			"Selected compute device: Preview Device",
			"Creating the logical device, compute queue and command pool...",
			"Locating the compute shader bytecode (geodesic_tracer_fp64.comp.spv)...",
			"Validating and loading the SPIR-V module (512 KiB)...",
			"Declaring descriptor bindings: camera uniform, pixel output, bodies, work counter and texture samplers...",
			"Creating the pipeline layout...",
			"Pipeline cache read from disk (1536 KiB), validating it against the device...",
			"Describing the compute stage (entry point main, workgroup 16x16)...",
			"Submitting the compute pipeline to the driver (cold start, the shader is compiled from scratch)..."
		}, 1.6},
		{Stage::Interface, "Building interface", {"Creating the capture studio...", "Creating the scenario manager...", "Restoring window layout and panel visibility...", "Configuring the physical constants engine (SI preset)..."}, 0.8},
		{Stage::Content, "Loading content", {"Resolving the startup scenario...", "Loading scenario scenarios/schwarzschild.yaml...", "Simulation state synchronized"}, 0.6},
		{Stage::Finalize, "Finalizing", {"Verifying simulation state...", "Opening the workspace..."}, 0.4}
	};
}

void pause_for(Startup::StartupProgress& progress, double seconds, double speed) {
	const auto end = std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(seconds / speed));
	while (std::chrono::steady_clock::now() < end) {
		progress.throw_if_cancelled();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void simulate_driver_compilation(Startup::StartupProgress& progress, const PreviewOptions& options) {
	const auto started = std::chrono::steady_clock::now();
	const double limit = options.compile_seconds / options.speed;
	for (;;) {
		const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
		if (elapsed >= limit) {
			break;
		}
		char text[128];
		std::snprintf(text, sizeof(text), "The driver is compiling the compute shader (%.1f s elapsed)...", elapsed);
		Startup::note(text);
		pause_for(progress, 0.25, 1.0);
	}
	Startup::note("Compute pipeline ready (simulated)...");
}

[[nodiscard]] bool run_sequence(Startup::StartupProgress& progress, const PreviewOptions& options) {
	const std::vector<PreviewStep> steps = build_steps();
	for (size_t index = 0; index < steps.size(); ++index) {
		const PreviewStep& step = steps[index];
		const float count = static_cast<float>(step.details.size());
		for (size_t detail = 0; detail < step.details.size(); ++detail) {
			progress.throw_if_cancelled();
			progress.report(step.range, static_cast<float>(detail) / count, step.stage, step.details[detail]);
			pause_for(progress, step.seconds / static_cast<double>(step.details.size()), options.speed);
		}
		if (index == kRenderBackendStepIndex) {
			simulate_driver_compilation(progress, options);
			if (options.simulate_failure) {
				progress.fail("Simulated failure: the compute pipeline could not be built on this device.");
				return false;
			}
		}
	}
	progress.complete();
	return true;
}

void hold_visible(Startup::StartupProgress& progress, double seconds) {
	const auto end = std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(seconds));
	while (std::chrono::steady_clock::now() < end && !progress.cancel_requested()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
}

}

int main(int argc, char** argv) {
	Startup::PlatformBootstrap::initialize();

	const PreviewOptions options = parse_options(argc, argv);
	if (options.show_help) {
		print_usage();
		return 0;
	}

	Startup::StartupProgress progress;
	Startup::set_active_progress(&progress);
	Startup::StartupWindow window;
	if (!window.open(progress)) {
		Startup::set_active_progress(nullptr);
		Startup::PlatformBootstrap::show_fatal_error(Startup::ApplicationInfo::kProductName, "The startup window is not available on this platform.");
		return EXIT_FAILURE;
	}

	bool completed = false;
	try {
		completed = run_sequence(progress, options);
	} catch (const Startup::StartupCancelled&) {
		completed = false;
		window.close();
		Startup::set_active_progress(nullptr);
		return 0;
	}

	hold_visible(progress, completed ? options.hold_seconds : kFailureHoldSeconds);
	Startup::set_active_progress(nullptr);
	window.close();
	return completed ? 0 : EXIT_FAILURE;
}
