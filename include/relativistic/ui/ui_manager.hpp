#pragma once

#include "relativistic/io/user_settings.hpp"
#include "relativistic/io/screenshot_exporter.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/ui/telemetry_window.hpp"
#include "relativistic/ui/spectrograph_window.hpp"
#include "relativistic/ui/control_panel_window.hpp"
#include "relativistic/ui/secondary_view_window.hpp"
#include "relativistic/ui/viewport_primary_window.hpp"
#include "relativistic/ui/scenario_selector_window.hpp"
#include "relativistic/ui/performance_settings_window.hpp"
#include "relativistic/ui/performance_analysis_window.hpp"
#include "relativistic/ui/visual_diagnostics_window.hpp"
#include "relativistic/ui/body_manager_window.hpp"
#include "relativistic/ui/interactive_camera_controller.hpp"
#include "relativistic/ui/keybind_settings_window.hpp"
#include "relativistic/ui/hud_manager_window.hpp"
#include "relativistic/ui/input_actions.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <stdexcept>
#include <algorithm>
#include <optional>

namespace Relativistic::UI {

enum class UiLayoutPreset : uint32_t {
	MultiWindowDetached = 0,
	DockedWorkspace = 1,
	ViewportFocused = 2,
	DeepAnalysis = 3
};

class UiManager {
private:
	GLFWwindow* main_window_{nullptr};
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	IO::UserSettings& user_settings_;
	InteractiveCameraController camera_controller_;
	KeybindSettingsWindow keybind_window_;
	HudManagerWindow hud_manager_window_;
	ActionEdgeTracker global_action_tracker_{};

	std::unique_ptr<ViewportPrimaryWindow> viewport_window_;
	std::unique_ptr<ScenarioSelectorWindow> scenario_window_;
	TelemetryWindow telemetry_window_;
	SpectrographWindow spectrograph_window_;
	ControlPanelWindow control_panel_window_;
	PerformanceSettingsWindow performance_window_;
	PerformanceAnalysisWindow performance_analysis_window_;
	VisualDiagnosticsWindow diagnostics_window_;
	BodyManagerWindow body_manager_window_;
	std::vector<SecondaryViewWindow> secondary_views_;

	bool show_viewport_{true};
	bool multi_window_mode_{true};
	bool pending_layout_reset_{false};
	UiLayoutPreset current_layout_{UiLayoutPreset::MultiWindowDetached};

	std::chrono::steady_clock::time_point last_frame_time_;

public:
	explicit UiManager(Orchestrator::SimulationOrchestrator<1024>& orchestrator, IO::UserSettings& user_settings)
		: orchestrator_(orchestrator),
		  user_settings_(user_settings),
		  camera_controller_(orchestrator),
		  keybind_window_(camera_controller_.config()),
		  hud_manager_window_(user_settings_.hud_layout),
		  control_panel_window_(orchestrator, camera_controller_, user_settings_.hud_layout, user_settings_.schematic_view, hud_manager_window_.open_state(), keybind_window_.open_state()),
		  performance_window_(orchestrator),
		  performance_analysis_window_(orchestrator),
		  diagnostics_window_(orchestrator),
		  body_manager_window_(orchestrator) {}

	~UiManager() {
		shutdown();
	}

	void initialize() {
		if (!glfwInit()) {
			throw std::runtime_error("Failed to initialize GLFW");
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

		main_window_ = glfwCreateWindow(1920, 1080, "Relativistic Engine - Primary Simulation Host", nullptr, nullptr);
		if (!main_window_) {
			glfwTerminate();
			throw std::runtime_error("Failed to create GLFW window");
		}

		glfwMakeContextCurrent(main_window_);
		glfwSwapInterval(1);

		glfwSetWindowUserPointer(main_window_, this);
		glfwSetScrollCallback(main_window_, [](GLFWwindow* win, double, double yoffset) {
			auto* self = static_cast<UiManager*>(glfwGetWindowUserPointer(win));
			if (self && self->viewport_window_ && self->viewport_window_->is_hovered()) {
				if (self->camera_controller_.config().keybinds.is_active(InputAction::ZoomModifier, win)) {
					self->viewport_window_->handle_zoom_scroll(yoffset);
				} else {
					self->camera_controller_.handle_scroll(yoffset);
				}
			}
		});

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImPlot::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 6.0f;
		style.ChildRounding = 4.0f;
		style.FrameRounding = 4.0f;
		style.PopupRounding = 4.0f;
		style.ScrollbarRounding = 4.0f;
		style.GrabRounding = 4.0f;
		style.TabRounding = 4.0f;
		style.WindowMenuButtonPosition = ImGuiDir_Right;
		style.Colors[ImGuiCol_WindowBg].w = 0.96f;
		style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.04f, 0.04f, 0.07f, 1.0f);

		ImGui_ImplGlfw_InitForOpenGL(main_window_, true);
		ImGui_ImplOpenGL3_Init("#version 330");

		viewport_window_ = std::make_unique<ViewportPrimaryWindow>(orchestrator_, camera_controller_, user_settings_.hud_layout, user_settings_.schematic_view);
		viewport_window_->set_screenshot_callback([this]() { trigger_screenshot_capture(); });
		viewport_window_->set_fullscreen_toggle_callback([this]() { multi_window_mode_ = !multi_window_mode_; });
		scenario_window_ = std::make_unique<ScenarioSelectorWindow>(orchestrator_, &camera_controller_);
		performance_window_.attach_render_pipeline(viewport_window_->pipeline_ref());
		performance_window_.attach_performance_analysis_window(performance_analysis_window_.open_state());
		performance_analysis_window_.attach_render_pipeline(viewport_window_->pipeline_ref());
		last_frame_time_ = std::chrono::steady_clock::now();

		telemetry_window_.open_state() = false;
		spectrograph_window_.open_state() = false;
		diagnostics_window_.open_state() = false;
		performance_analysis_window_.open_state() = false;
		control_panel_window_.open_state() = true;
		performance_window_.open_state() = true;
		scenario_window_->open_state() = true;

		camera_controller_.config() = user_settings_.camera_controls;
		keybind_window_.attach_hud_layout(user_settings_.hud_layout);
		multi_window_mode_ = user_settings_.multi_window_mode;
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_camera_mode(user_settings_.default_camera_mode)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_performance_preset(user_settings_.default_performance_preset)));

		apply_multi_window_layout_preset(static_cast<UiLayoutPreset>(user_settings_.last_window_layout));
	}

	void add_secondary_view(const std::string& name) {
		secondary_views_.emplace_back(name);
	}

	void trigger_screenshot_capture() noexcept {
		if (viewport_window_) {
			viewport_window_->request_screenshot(
				user_settings_.screenshot_output_directory,
				user_settings_.screenshot_filename_pattern,
				static_cast<IO::ScreenshotFormat>(user_settings_.screenshot_format)
			);
		}
	}

	void export_runtime_settings() const noexcept {
		user_settings_.camera_controls = camera_controller_.config();
		user_settings_.multi_window_mode = multi_window_mode_;
		user_settings_.last_window_layout = static_cast<uint32_t>(current_layout_);
		user_settings_.default_camera_mode = orchestrator_.parameters().camera_mode;
		user_settings_.default_performance_preset = orchestrator_.parameters().performance_preset;
	}

	void apply_multi_window_layout_preset(UiLayoutPreset preset) noexcept {
		current_layout_ = preset;
		pending_layout_reset_ = true;
	}

	void show_all_panels() noexcept {
		show_viewport_ = true;
		if (scenario_window_) scenario_window_->open_state() = true;
		control_panel_window_.open_state() = true;
		performance_window_.open_state() = true;
		body_manager_window_.open_state() = true;
		diagnostics_window_.open_state() = true;
		telemetry_window_.open_state() = true;
		spectrograph_window_.open_state() = true;
		performance_analysis_window_.open_state() = true;
	}

	void render_frame() {
		const auto now = std::chrono::steady_clock::now();
		const double dt = std::chrono::duration<double>(now - last_frame_time_).count();
		last_frame_time_ = now;

		glfwPollEvents();
		process_global_hotkeys();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		render_main_menu_bar();

		if (!multi_window_mode_) {
			ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(0U, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
			static_cast<void>(dockspace_id);
		}

		if (pending_layout_reset_) {
			dispatch_layout_reconfiguration();
			pending_layout_reset_ = false;
		}

		show_viewport_ = true;
		if (viewport_window_) {
			viewport_window_->render(main_window_, dt, multi_window_mode_);
		}

		if (scenario_window_ && scenario_window_->open_state()) {
			scenario_window_->render();
		}

		if (control_panel_window_.open_state()) {
			control_panel_window_.render();
		}

		if (telemetry_window_.open_state()) {
			telemetry_window_.render(orchestrator_);
		}

		if (spectrograph_window_.open_state()) {
			spectrograph_window_.render(orchestrator_);
		}

		if (performance_window_.open_state()) {
			performance_window_.render();
		}

		if (performance_analysis_window_.open_state()) {
			performance_analysis_window_.render();
		}

		if (diagnostics_window_.open_state()) {
			diagnostics_window_.render();
		}

		if (body_manager_window_.open_state()) {
			body_manager_window_.render();
		}

		if (keybind_window_.open_state()) {
			keybind_window_.render(main_window_);
		}

		for (auto& view : secondary_views_) {
			view.render();
		}

		ImGui::Render();

		int display_w = 0, display_h = 0;
		glfwGetFramebufferSize(main_window_, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.04f, 0.04f, 0.06f, 1.0f);
		glClear(static_cast<unsigned int>(GL_COLOR_BUFFER_BIT));

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		glfwSwapBuffers(main_window_);
	}

	[[nodiscard]] bool should_close() const noexcept {
		return glfwWindowShouldClose(main_window_);
	}

	void shutdown() noexcept {
		if (main_window_) {
			viewport_window_.reset();
			scenario_window_.reset();

			ImGuiIO& io = ImGui::GetIO();
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
				ImGui::DestroyPlatformWindows();
			}
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImPlot::DestroyContext();
			ImGui::DestroyContext();
			glfwDestroyWindow(main_window_);
			glfwTerminate();
			main_window_ = nullptr;
		}
	}

private:
	[[nodiscard]] std::string key_hint(InputAction action) const noexcept {
		const auto& b = camera_controller_.config().keybinds.get(action);
		if (b.primary_key == GLFW_KEY_UNKNOWN && b.secondary_key == GLFW_KEY_UNKNOWN) {
			return "";
		}
		return format_key_binding(b);
	}

	void process_global_hotkeys() noexcept {
		ImGuiIO& io = ImGui::GetIO();
		if (io.WantCaptureKeyboard) return;

		const auto& keybinds = camera_controller_.config().keybinds;

		if (global_action_tracker_.just_pressed(keybinds, InputAction::ToggleControlPanel, main_window_)) {
			control_panel_window_.open_state() = !control_panel_window_.open_state();
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::LayoutMultiWindow, main_window_)) {
			apply_multi_window_layout_preset(UiLayoutPreset::MultiWindowDetached);
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::LayoutDocked, main_window_)) {
			apply_multi_window_layout_preset(UiLayoutPreset::DockedWorkspace);
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::LayoutViewportFocus, main_window_)) {
			apply_multi_window_layout_preset(UiLayoutPreset::ViewportFocused);
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::TogglePausePlay, main_window_)) {
			if (orchestrator_.scheduler().is_paused()) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_resume()));
			} else {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_pause()));
			}
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::SingleStepTick, main_window_)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_step(1)));
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::ResetClock, main_window_)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_reset()));
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::ToggleBodyManager, main_window_)) {
			body_manager_window_.open_state() = !body_manager_window_.open_state();
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::CycleCameraMode, main_window_)) {
			const uint32_t next_mode = (orchestrator_.parameters().camera_mode + 1) % 4;
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_camera_mode(next_mode)));
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::ToggleTelemetryWindow, main_window_)) {
			telemetry_window_.open_state() = !telemetry_window_.open_state();
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::TogglePerformanceWindow, main_window_)) {
			performance_window_.open_state() = !performance_window_.open_state();
			diagnostics_window_.open_state() = !diagnostics_window_.open_state();
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::CaptureScreenshot, main_window_)) {
			trigger_screenshot_capture();
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::ToggleHudManager, main_window_)) {
			user_settings_.hud_layout.master_enabled = !user_settings_.hud_layout.master_enabled;
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::ToggleKeybindSettings, main_window_)) {
			keybind_window_.open_state() = !keybind_window_.open_state();
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::ToggleScenarioWindow, main_window_)) {
			if (scenario_window_) scenario_window_->open_state() = !scenario_window_->open_state();
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::ToggleDiagnosticsWindow, main_window_)) {
			diagnostics_window_.open_state() = !diagnostics_window_.open_state();
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::ToggleSpectrographWindow, main_window_)) {
			spectrograph_window_.open_state() = !spectrograph_window_.open_state();
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::TogglePerformanceAnalysisWindow, main_window_)) {
			performance_analysis_window_.open_state() = !performance_analysis_window_.open_state();
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::StartStopBenchmarkCapture, main_window_)) {
			auto& profiler = orchestrator_.profiler();
			if (profiler.is_capturing()) {
				profiler.cancel_capture();
			} else {
				profiler.start_capture("Quick Capture", 10.0, std::nullopt, ImGui::GetTime());
			}
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::QuickSaveBenchmarkRun, main_window_)) {
			auto& profiler = orchestrator_.profiler();
			if (!profiler.is_capturing()) {
				profiler.start_capture("Quick Save", std::nullopt, size_t{60}, ImGui::GetTime());
			}
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::ToggleFullscreenViewport, main_window_)) {
			multi_window_mode_ = !multi_window_mode_;
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::ToggleGpuCompute, main_window_)) {
			const bool next_state = !orchestrator_.parameters().use_gpu_compute;
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::UseGpuCompute, next_state ? 1.0 : 0.0)));
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::ToggleSpaceSkipping, main_window_)) {
			const bool next_state = !orchestrator_.parameters().space_skipping_enabled;
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SpaceSkippingEnabled, next_state ? 1.0 : 0.0)));
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::ToggleLodSystem, main_window_)) {
			const bool next_state = !orchestrator_.parameters().lod_enabled;
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::LodEnabled, next_state ? 1.0 : 0.0)));
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::ToggleWorkDistributionTiling, main_window_)) {
			const bool next_state = (orchestrator_.parameters().visual_overlays_flags & Render::RenderFlags::USE_TILED_DISTRIBUTION) == 0U;
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::WorkDistributionMode, next_state ? 1.0 : 0.0)));
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::CycleStepController, main_window_)) {
			const uint32_t next_mode = (orchestrator_.parameters().step_controller_mode + 1) % 3;
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::StepControllerMode, static_cast<double>(next_mode))));
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::ResetToDefaultPerformance, main_window_)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_performance_preset(2)));
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::IncreaseExposure, main_window_)) {
			const double next_exposure = std::clamp(orchestrator_.parameters().camera_exposure + 0.25, -6.0, 6.0);
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::CameraExposure, next_exposure)));
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::DecreaseExposure, main_window_)) {
			const double next_exposure = std::clamp(orchestrator_.parameters().camera_exposure - 0.25, -6.0, 6.0);
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::CameraExposure, next_exposure)));
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::IncreaseTimeWarp, main_window_)) {
			const double next_warp = orchestrator_.scheduler().warp_factor() * 1.5;
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_warp(next_warp)));
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::DecreaseTimeWarp, main_window_)) {
			const double next_warp = std::max(orchestrator_.scheduler().warp_factor() / 1.5, 0.05);
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_warp(next_warp)));
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::QuickSaveScenario, main_window_)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_save_scenario("scenarios/quicksave.yaml")));
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::QuickLoadScenario, main_window_)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_load_scenario("scenarios/quicksave.yaml")));
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::CycleProjectionMode, main_window_)) {
			const uint32_t next_mode = (orchestrator_.parameters().projection_mode + 1) % 8;
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::ProjectionMode, static_cast<double>(next_mode))));
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::CycleTonemapper, main_window_)) {
			const uint32_t next_mode = (orchestrator_.parameters().tonemapping_mode + 1) % 4;
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::TonemappingMode, static_cast<double>(next_mode))));
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::CycleSkyboxStyle, main_window_)) {
			const uint32_t current_style = orchestrator_.parameters().visual_overlays_flags & Render::RenderFlags::SKYBOX_MODE_MASK;
			const uint32_t next_style = (current_style + 1) % 6;
			const uint32_t next_flags = (orchestrator_.parameters().visual_overlays_flags & ~Render::RenderFlags::SKYBOX_MODE_MASK) | next_style;
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::VisualOverlays, static_cast<double>(next_flags))));
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::CycleMetric, main_window_)) {
			static constexpr const char* kMetricCycle[] = {
				"Flat Minkowski", "Schwarzschild Black Hole", "Kerr Rotating Black Hole",
				"Reissner-Nordstrom Charged", "Kerr-Newman Charged Rotating",
				"Schwarzschild-de Sitter (Lambda)", "FLRW Cosmological Expansion",
				"Morris-Thorne Traversable Wormhole", "Alcubierre Warp Drive Bubble", "BSSN 3+1 Numerical Grid"
			};
			constexpr int metric_count = static_cast<int>(sizeof(kMetricCycle) / sizeof(kMetricCycle[0]));
			int current_idx = 0;
			for (int i = 0; i < metric_count; ++i) {
				if (orchestrator_.active_metric_name() == kMetricCycle[i]) {
					current_idx = i;
					break;
				}
			}
			const int next_idx = (current_idx + 1) % metric_count;
			orchestrator_.set_active_metric_name(kMetricCycle[next_idx]);
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_metric(kMetricCycle[next_idx])));
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::CycleIntegrator, main_window_)) {
			static constexpr const char* kIntegratorCycle[] = {
				"Dormand-Prince RK45 (Adaptive)", "Cash-Karp 5(4) (Adaptive)", "Vernier 9(8) High-Order",
				"Symplectic Gauss-Legendre 4th", "Symplectic Gauss-Legendre 6th", "Hermite 4th-Order (Aarseth)"
			};
			constexpr int integrator_count = static_cast<int>(sizeof(kIntegratorCycle) / sizeof(kIntegratorCycle[0]));
			int current_idx = 0;
			for (int i = 0; i < integrator_count; ++i) {
				if (orchestrator_.active_integrator_name() == kIntegratorCycle[i]) {
					current_idx = i;
					break;
				}
			}
			const int next_idx = (current_idx + 1) % integrator_count;
			orchestrator_.set_active_integrator_name(kIntegratorCycle[next_idx]);
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_integrator(kIntegratorCycle[next_idx])));
		}
	}

	void dispatch_layout_reconfiguration() noexcept {
		const ImGuiViewport* main_vp = ImGui::GetMainViewport();
		const float screen_w = main_vp->WorkSize.x;
		const float screen_h = main_vp->WorkSize.y;
		const float offset_x = main_vp->WorkPos.x;
		const float offset_y = main_vp->WorkPos.y;

		if (current_layout_ == UiLayoutPreset::MultiWindowDetached) {
			multi_window_mode_ = true;

			const float left_col_w = std::clamp(screen_w * 0.18f, 280.0f, 360.0f);
			const float right_col_w = std::clamp(screen_w * 0.23f, 380.0f, 480.0f);
			const float center_w = screen_w - left_col_w - right_col_w - 40.0f;
			const float top_h = std::clamp(screen_h * 0.68f, 450.0f, 780.0f);
			const float bottom_h = screen_h - top_h - 45.0f;

			ImGui::SetWindowPos("Scenario Manager & Presets", ImVec2(offset_x + 15.0f, offset_y + 30.0f));
			ImGui::SetWindowSize("Scenario Manager & Presets", ImVec2(left_col_w, top_h * 0.50f));

			ImGui::SetWindowPos("Celestial Body & N-Body Manager", ImVec2(offset_x + 15.0f, offset_y + 30.0f + top_h * 0.50f + 8.0f));
			ImGui::SetWindowSize("Celestial Body & N-Body Manager", ImVec2(left_col_w, top_h * 0.50f - 16.0f));

			ImGui::SetWindowPos("Master Simulation Controls", ImVec2(offset_x + screen_w - right_col_w - 15.0f, offset_y + 30.0f));
			ImGui::SetWindowSize("Master Simulation Controls", ImVec2(right_col_w, top_h));

			ImGui::SetWindowPos("Telemetry & Invariants", ImVec2(offset_x + 15.0f, offset_y + top_h + 24.0f));
			ImGui::SetWindowSize("Telemetry & Invariants", ImVec2(left_col_w, bottom_h + 10.0f));

			ImGui::SetWindowPos("Radiative Transfer & Spectrograph Monitor", ImVec2(offset_x + left_col_w + 25.0f, offset_y + top_h + 24.0f));
			ImGui::SetWindowSize("Radiative Transfer & Spectrograph Monitor", ImVec2(center_w * 0.5f - 8.0f, bottom_h + 10.0f));

			ImGui::SetWindowPos("Performance & Engine Optimization", ImVec2(offset_x + left_col_w + 25.0f + center_w * 0.5f, offset_y + top_h + 24.0f));
			ImGui::SetWindowSize("Performance & Engine Optimization", ImVec2(center_w * 0.5f - 8.0f, bottom_h + 10.0f));

			ImGui::SetWindowPos("Curvature Diagnostics & Tensor Inspector", ImVec2(offset_x + screen_w - right_col_w - 15.0f, offset_y + top_h + 24.0f));
			ImGui::SetWindowSize("Curvature Diagnostics & Tensor Inspector", ImVec2(right_col_w, bottom_h + 10.0f));
		} else if (current_layout_ == UiLayoutPreset::ViewportFocused) {
			if (scenario_window_) scenario_window_->open_state() = false;
			telemetry_window_.open_state() = false;
			spectrograph_window_.open_state() = false;
			performance_window_.open_state() = false;
			diagnostics_window_.open_state() = false;
		} else {
			multi_window_mode_ = false;
			show_viewport_ = true;
			if (scenario_window_) scenario_window_->open_state() = true;
			telemetry_window_.open_state() = true;
			spectrograph_window_.open_state() = true;
			control_panel_window_.open_state() = true;
			performance_window_.open_state() = true;
			diagnostics_window_.open_state() = true;
		}
	}

	void render_main_menu_bar() noexcept {
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Save Snapshot Scenario...")) {
					static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_save_scenario("scenarios/snapshot.yaml")));
				}
				if (ImGui::MenuItem("Export FITS Spectral Cube")) {
					static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_trigger_export("fits")));
				}
				if (ImGui::MenuItem("Export HDF5 Trajectories")) {
					static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_trigger_export("hdf5")));
				}
				if (ImGui::MenuItem("Export VTK Horizons")) {
					static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_trigger_export("vtk")));
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Exit", "Alt+F4")) {
					orchestrator_.stop();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Simulation")) {
				if (ImGui::MenuItem("Pause / Resume", "P / F5")) {
					if (orchestrator_.scheduler().is_paused()) {
						static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_resume()));
					} else {
						static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_pause()));
					}
				}
				if (ImGui::MenuItem("Single Step Tick", "F6")) {
					static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_step(1)));
				}
				if (ImGui::MenuItem("Reset Clock & Orbit", "F7")) {
					static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_reset()));
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Cycle Camera Navigation Mode", "F9")) {
					const uint32_t next_mode = (orchestrator_.parameters().camera_mode + 1) % 4;
					static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_camera_mode(next_mode)));
				}
				if (ImGui::MenuItem("Snap Camera to Equatorial (r=50)")) {
					camera_controller_.snap_to_equatorial_front(50.0);
				}
				if (ImGui::MenuItem("Snap Camera to ISCO Orbit")) {
					camera_controller_.snap_to_isco();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Window Layouts")) {
				if (ImGui::MenuItem("Multi-Window Detached (Default)", "F2", current_layout_ == UiLayoutPreset::MultiWindowDetached)) {
					apply_multi_window_layout_preset(UiLayoutPreset::MultiWindowDetached);
				}
				if (ImGui::MenuItem("Docked Workspace Container", "F3", current_layout_ == UiLayoutPreset::DockedWorkspace)) {
					apply_multi_window_layout_preset(UiLayoutPreset::DockedWorkspace);
				}
				if (ImGui::MenuItem("Viewport Fullscreen Focus", "F4", current_layout_ == UiLayoutPreset::ViewportFocused)) {
					apply_multi_window_layout_preset(UiLayoutPreset::ViewportFocused);
				}
				ImGui::Separator();
				ImGui::Checkbox("Multi-Window Viewport Separation", &multi_window_mode_);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View Windows")) {
				ImGui::TextDisabled("Primary Relativistic Viewport is always visible");
				ImGui::Separator();
				if (scenario_window_) {
					ImGui::MenuItem("Scenario Manager & Presets", key_hint(InputAction::ToggleScenarioWindow).c_str(), &scenario_window_->open_state());
				}
				ImGui::MenuItem("Master Simulation Controls", key_hint(InputAction::ToggleControlPanel).c_str(), &control_panel_window_.open_state());
				ImGui::MenuItem("Performance & Engine Optimization", key_hint(InputAction::TogglePerformanceWindow).c_str(), &performance_window_.open_state());
				ImGui::MenuItem("Celestial Body & N-Body Manager", key_hint(InputAction::ToggleBodyManager).c_str(), &body_manager_window_.open_state());
				ImGui::MenuItem("Curvature Diagnostics & Tensor Inspector", key_hint(InputAction::ToggleDiagnosticsWindow).c_str(), &diagnostics_window_.open_state());
				ImGui::MenuItem("Telemetry & Invariants", key_hint(InputAction::ToggleTelemetryWindow).c_str(), &telemetry_window_.open_state());
				ImGui::MenuItem("Radiative Transfer & Spectrograph Monitor", key_hint(InputAction::ToggleSpectrographWindow).c_str(), &spectrograph_window_.open_state());
				ImGui::MenuItem("Keybind Settings", key_hint(InputAction::ToggleKeybindSettings).c_str(), &keybind_window_.open_state());
				ImGui::MenuItem("Performance Analysis & Profiling", key_hint(InputAction::TogglePerformanceAnalysisWindow).c_str(), &performance_analysis_window_.open_state());
				ImGui::MenuItem("HUD Manager", nullptr, &hud_manager_window_.open_state());
				ImGui::Separator();
				if (ImGui::MenuItem("Show All Panels")) {
					show_all_panels();
				}
				if (ImGui::MenuItem("Focus Viewport Only")) {
					apply_multi_window_layout_preset(UiLayoutPreset::ViewportFocused);
				}
				if (ImGui::MenuItem("Force Viewport Refresh")) {
					if (viewport_window_) viewport_window_->request_rerender();
				}
				if (!secondary_views_.empty()) {
					ImGui::Separator();
					ImGui::TextDisabled("Secondary Observer Viewports");
					for (auto& view : secondary_views_) {
						ImGui::MenuItem(view.name().c_str(), nullptr, &view.open_state());
					}
				}
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
	}
};

}
