#pragma once

#include "relativistic/io/user_settings.hpp"
#include "relativistic/io/screenshot_exporter.hpp"
#include "relativistic/io/screenshot_capture_settings.hpp"
#include "relativistic/io/video_capture_settings.hpp"
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
#include "relativistic/ui/constants_window.hpp"
#include "relativistic/ui/input_actions.hpp"
#include "relativistic/ui/log_console_window.hpp"
#include "relativistic/ui/secondary_viewport_manager.hpp"
#include "relativistic/core/system_console.hpp"

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
#include <cstring>

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
	ConstantsWindow constants_window_;
	LogConsoleWindow log_console_window_;
	std::unique_ptr<SecondaryViewportManager> secondary_viewport_manager_;
	bool secondary_viewport_manager_panel_open_{false};

	bool show_viewport_{true};
	bool multi_window_mode_{true};
	bool pending_layout_reset_{false};
	bool pending_screenshot_popup_open_{false};
	UiLayoutPreset current_layout_{UiLayoutPreset::MultiWindowDetached};

	std::chrono::steady_clock::time_point last_frame_time_;
	char screenshot_dir_buffer_[256]{};
	char screenshot_pattern_buffer_[128]{};
	bool screenshot_buffers_synced_{false};
	int screenshot_capture_mode_{0};
	IO::VideoSequenceSettings sequence_settings_{};
	int sequence_frame_count_{300};
	char screenshot_watermark_buffer_[128]{};
	bool screenshot_watermark_buffer_synced_{false};

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
		  body_manager_window_(orchestrator),
		  constants_window_(orchestrator),
		  secondary_viewport_manager_(std::make_unique<SecondaryViewportManager>(orchestrator)) {}

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
		viewport_window_->set_open_screenshot_settings_callback([this]() { pending_screenshot_popup_open_ = true; });
		scenario_window_ = std::make_unique<ScenarioSelectorWindow>(orchestrator_, &camera_controller_);
		performance_window_.attach_render_pipeline(viewport_window_->pipeline_ref());
		performance_window_.attach_performance_analysis_window(performance_analysis_window_.open_state());
		performance_analysis_window_.attach_render_pipeline(viewport_window_->pipeline_ref());
		last_frame_time_ = std::chrono::steady_clock::now();

		telemetry_window_.open_state() = user_settings_.window_telemetry_open;
		spectrograph_window_.open_state() = user_settings_.window_spectrograph_open;
		diagnostics_window_.open_state() = user_settings_.window_diagnostics_open;
		performance_analysis_window_.open_state() = user_settings_.window_performance_analysis_open;
		control_panel_window_.open_state() = user_settings_.window_control_panel_open;
		performance_window_.open_state() = user_settings_.window_performance_open;
		scenario_window_->open_state() = user_settings_.window_scenario_open;
		body_manager_window_.open_state() = user_settings_.window_body_manager_open;
		hud_manager_window_.open_state() = user_settings_.window_hud_manager_open;
		keybind_window_.open_state() = user_settings_.window_keybind_settings_open;
		constants_window_.open_state() = user_settings_.window_constants_open;
		log_console_window_.open_state() = user_settings_.window_log_console_open;
		log_console_window_.attach_system_console_flag(user_settings_.show_system_console);

		orchestrator_.constants_engine().apply_preset_by_index(user_settings_.constants_preset);
		if (user_settings_.constants_preset == 2) {
			orchestrator_.constants_engine().set_speed_of_light(user_settings_.constants_c);
			orchestrator_.constants_engine().set_gravitational_constant(user_settings_.constants_g);
			orchestrator_.constants_engine().set_planck_constant(user_settings_.constants_h);
			orchestrator_.constants_engine().set_boltzmann_constant(user_settings_.constants_kb);
			orchestrator_.constants_engine().set_avogadro_constant(user_settings_.constants_na);
			orchestrator_.constants_engine().set_coulomb_constant(user_settings_.constants_ke);
			orchestrator_.constants_engine().set_luminous_efficacy(user_settings_.constants_kcd);
		}

		camera_controller_.config() = user_settings_.camera_controls;
		keybind_window_.attach_hud_layout(user_settings_.hud_layout);
		multi_window_mode_ = user_settings_.multi_window_mode;
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_camera_mode(user_settings_.default_camera_mode)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_performance_preset(user_settings_.default_performance_preset)));

		apply_multi_window_layout_preset(static_cast<UiLayoutPreset>(user_settings_.last_window_layout));
	}

	void add_secondary_view(const std::string& name) {
		if (secondary_viewport_manager_) {
			secondary_viewport_manager_->add_view(name);
		}
	}

	void trigger_screenshot_capture() noexcept {
		if (viewport_window_) {
			viewport_window_->request_screenshot(
				user_settings_.screenshot_output_directory,
				user_settings_.screenshot_filename_pattern,
				static_cast<IO::ScreenshotFormat>(user_settings_.screenshot_format),
				user_settings_.screenshot_resolution_scale,
				static_cast<IO::ScreenshotOverwritePolicy>(user_settings_.screenshot_overwrite_policy),
				user_settings_.screenshot_watermark_enabled ? user_settings_.screenshot_watermark_text : std::string{}
			);
		}
	}

	void export_runtime_settings() noexcept {
		user_settings_.camera_controls = camera_controller_.config();
		user_settings_.multi_window_mode = multi_window_mode_;
		user_settings_.last_window_layout = static_cast<uint32_t>(current_layout_);
		user_settings_.default_camera_mode = orchestrator_.parameters().camera_mode;
		user_settings_.default_performance_preset = orchestrator_.parameters().performance_preset;
		user_settings_.window_control_panel_open = control_panel_window_.open_state();
		user_settings_.window_performance_open = performance_window_.open_state();
		user_settings_.window_scenario_open = scenario_window_ ? scenario_window_->open_state() : user_settings_.window_scenario_open;
		user_settings_.window_telemetry_open = telemetry_window_.open_state();
		user_settings_.window_spectrograph_open = spectrograph_window_.open_state();
		user_settings_.window_diagnostics_open = diagnostics_window_.open_state();
		user_settings_.window_body_manager_open = body_manager_window_.open_state();
		user_settings_.window_performance_analysis_open = performance_analysis_window_.open_state();
		user_settings_.window_hud_manager_open = hud_manager_window_.open_state();
		user_settings_.window_keybind_settings_open = keybind_window_.open_state();
		user_settings_.window_constants_open = constants_window_.open_state();
		user_settings_.window_log_console_open = log_console_window_.open_state();
		user_settings_.constants_preset = static_cast<uint32_t>(orchestrator_.constants_engine().active_preset());
		user_settings_.constants_c = orchestrator_.constants_engine().sim_speed_of_light();
		user_settings_.constants_g = orchestrator_.constants_engine().sim_gravitational_constant();
		user_settings_.constants_h = orchestrator_.constants_engine().sim_planck_constant();
		user_settings_.constants_kb = orchestrator_.constants_engine().sim_boltzmann_constant();
		user_settings_.constants_na = orchestrator_.constants_engine().sim_avogadro_constant();
		user_settings_.constants_ke = orchestrator_.constants_engine().sim_coulomb_constant();
		user_settings_.constants_kcd = orchestrator_.constants_engine().sim_luminous_efficacy();
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
		constants_window_.open_state() = true;
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
		render_screenshot_settings_popup();

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

		if (constants_window_.open_state()) {
			constants_window_.render();
		}

		if (log_console_window_.open_state()) {
			log_console_window_.render();
		}

		if (secondary_viewport_manager_) {
			secondary_viewport_manager_->render_all();
			secondary_viewport_manager_->render_management_panel(secondary_viewport_manager_panel_open_);
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
			pending_screenshot_popup_open_ = true;
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::ToggleHudManager, main_window_)) {
			user_settings_.hud_layout.master_enabled = !user_settings_.hud_layout.master_enabled;
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::ToggleKeybindSettings, main_window_)) {
			keybind_window_.open_state() = !keybind_window_.open_state();
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::ToggleConstantsWindow, main_window_)) {
			constants_window_.open_state() = !constants_window_.open_state();
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

	void render_screenshot_settings_popup() noexcept {
		if (!screenshot_buffers_synced_) {
			std::strncpy(screenshot_dir_buffer_, user_settings_.screenshot_output_directory.c_str(), sizeof(screenshot_dir_buffer_) - 1);
			std::strncpy(screenshot_pattern_buffer_, user_settings_.screenshot_filename_pattern.c_str(), sizeof(screenshot_pattern_buffer_) - 1);
			screenshot_buffers_synced_ = true;
		}
		if (!screenshot_watermark_buffer_synced_) {
			std::strncpy(screenshot_watermark_buffer_, user_settings_.screenshot_watermark_text.c_str(), sizeof(screenshot_watermark_buffer_) - 1);
			screenshot_watermark_buffer_synced_ = true;
		}

		if (pending_screenshot_popup_open_) {
			ImGui::OpenPopup("Capture Studio");
			pending_screenshot_popup_open_ = false;
		}

		ImGui::SetNextWindowSize(ImVec2(560.0f, 0.0f), ImGuiCond_FirstUseEver);
		if (ImGui::BeginPopupModal("Capture Studio", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::TextWrapped("Configure where and how captured frames are saved. Available filename tokens: %%metric%%, %%mass%%, %%spin%%, %%width%%, %%height%%, %%tick%%, plus any strftime token such as %%Y %%m %%d %%H %%M %%S.");
			ImGui::Separator();

			if (ImGui::InputText("Output Directory", screenshot_dir_buffer_, sizeof(screenshot_dir_buffer_))) {
				user_settings_.screenshot_output_directory = screenshot_dir_buffer_;
			}
			if (ImGui::InputText("Filename Pattern", screenshot_pattern_buffer_, sizeof(screenshot_pattern_buffer_))) {
				user_settings_.screenshot_filename_pattern = screenshot_pattern_buffer_;
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Smart Name")) {
				static constexpr const char* kSmartPattern = "%metric%_M%mass%_a%spin%_%width%x%height%_%Y%m%d_%H%M%S";
				std::strncpy(screenshot_pattern_buffer_, kSmartPattern, sizeof(screenshot_pattern_buffer_) - 1);
				screenshot_pattern_buffer_[sizeof(screenshot_pattern_buffer_) - 1] = '\0';
				user_settings_.screenshot_filename_pattern = screenshot_pattern_buffer_;
			}

			const char* format_names[] = {
				"PPM (Lossless, Fast)",
				"BMP (Lossless, Windows-Compatible)",
				"PNG (Lossless, Compressed, Metadata)",
				"TGA (Lossless, Uncompressed)",
				"HDR (Radiance, Linear Float)"
			};
			int format_idx = static_cast<int>(std::min<uint32_t>(user_settings_.screenshot_format, 4U));
			if (ImGui::Combo("File Format", &format_idx, format_names, IM_ARRAYSIZE(format_names))) {
				user_settings_.screenshot_format = static_cast<uint32_t>(format_idx);
			}
			render_setting_tooltip("PNG embeds an optional comment/watermark string directly in the file as metadata. HDR stores approximate linear radiance values and is intended for compositing rather than direct viewing.");

			const char* overwrite_names[] = {"Auto-Increment Filename", "Overwrite Existing File", "Skip If File Exists"};
			int overwrite_idx = static_cast<int>(std::min<uint32_t>(user_settings_.screenshot_overwrite_policy, 2U));
			if (ImGui::Combo("If File Exists", &overwrite_idx, overwrite_names, IM_ARRAYSIZE(overwrite_names))) {
				user_settings_.screenshot_overwrite_policy = static_cast<uint32_t>(overwrite_idx);
			}

			ImGui::Checkbox("Embed Watermark / Comment", &user_settings_.screenshot_watermark_enabled);
			if (user_settings_.screenshot_watermark_enabled) {
				if (ImGui::InputText("Watermark Text", screenshot_watermark_buffer_, sizeof(screenshot_watermark_buffer_))) {
					user_settings_.screenshot_watermark_text = screenshot_watermark_buffer_;
				}
				render_setting_tooltip("Written as a PNG tEXt chunk or an HDR comment line depending on the selected format. Formats without a metadata field ignore this setting.");
			}

			ImGui::SliderFloat("Capture Resolution Multiplier", &user_settings_.screenshot_resolution_scale, 1.0f, 4.0f, "%.2fx");
			ImGui::TextDisabled("Values above 1x render a dedicated higher-resolution frame for the capture only, independent of the live viewport resolution scale. Capturing now runs in the background and never freezes the interface.");

			IO::ScreenshotCaptureContext preview_ctx;
			preview_ctx.metric_name = orchestrator_.active_metric_name();
			preview_ctx.mass = orchestrator_.parameters().mass;
			preview_ctx.spin = orchestrator_.parameters().spin;
			preview_ctx.width = 1920;
			preview_ctx.height = 1080;
			preview_ctx.tick_index = orchestrator_.scheduler().snapshot().tick_index;
			const std::string preview_name = IO::ScreenshotFilenameBuilder::build(user_settings_.screenshot_filename_pattern, preview_ctx);
			ImGui::Text("Preview filename: %s", preview_name.c_str());

			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f), "Capture Mode");
			const char* capture_modes[] = {"Single Screenshot", "Image Sequence (For Video Encoding)"};
			ImGui::Combo("Mode", &screenshot_capture_mode_, capture_modes, IM_ARRAYSIZE(capture_modes));
			render_setting_tooltip("Single Screenshot captures one frame using the settings above. Image Sequence periodically writes numbered frames to disk, which can be assembled into a video afterward with the generated ffmpeg command.");

			if (screenshot_capture_mode_ == 0) {
				if (viewport_window_ && viewport_window_->is_high_res_capture_pending()) {
					ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "A high-resolution capture is currently rendering in the background...");
				}
				if (ImGui::Button("Capture Now", ImVec2(140.0f, 26.0f))) {
					trigger_screenshot_capture();
				}
			} else {
				ImGui::Separator();
				ImGui::TextColored(ImVec4(0.6f, 0.85f, 1.0f, 1.0f), "Sequence & Video Assembly");

				ImGui::SliderFloat("Sequence Frame Rate", &sequence_settings_.frames_per_second, 1.0f, 120.0f, "%.0f fps");
				ImGui::SliderFloat("Internal Resolution Scale", &sequence_settings_.resolution_scale, 0.1f, 2.0f, "%.2fx");
				render_setting_tooltip("Applied on top of the live viewport resolution scale for every captured sequence frame, independent of the single-screenshot multiplier above.");

				const char* trigger_names[] = {"Manual (Stop Button)", "Fixed Duration", "Fixed Frame Count", "Continuous Until Stopped"};
				int trigger_idx = static_cast<int>(sequence_settings_.trigger);
				if (ImGui::Combo("Stop Condition", &trigger_idx, trigger_names, IM_ARRAYSIZE(trigger_names))) {
					sequence_settings_.trigger = static_cast<IO::SequenceCaptureTrigger>(trigger_idx);
				}

				if (sequence_settings_.trigger == IO::SequenceCaptureTrigger::FixedDuration) {
					ImGui::SliderFloat("Sequence Duration", &sequence_settings_.duration_seconds, 0.5f, 600.0f, "%.1f s");
					const float estimated_frames = sequence_settings_.frames_per_second * sequence_settings_.duration_seconds;
					ImGui::TextDisabled("Approximately %.0f frames will be written.", static_cast<double>(estimated_frames));
				} else if (sequence_settings_.trigger == IO::SequenceCaptureTrigger::FixedFrameCount) {
					ImGui::SliderInt("Frame Count", &sequence_frame_count_, 1, 100000);
				}

				ImGui::Checkbox("Pause Simulation While Capturing", &sequence_settings_.pause_simulation_during_capture);
				render_setting_tooltip("Pauses the simulation clock for the duration of the sequence capture so every frame advances by exactly one render step, avoiding motion judder from real-time playback speed variance.");
				ImGui::Checkbox("Loop Output Video", &sequence_settings_.loop_output);

				ImGui::Separator();
				ImGui::TextColored(ImVec4(0.85f, 0.75f, 0.3f, 1.0f), "Video Encoding Preset (External ffmpeg)");
				const char* codec_names[] = {"H.264 (libx264)", "H.265 / HEVC (libx265)", "VP9 (WebM)", "ProRes (Editing)", "PNG Sequence Only (No Encode)"};
				int codec_idx = static_cast<int>(sequence_settings_.codec);
				if (ImGui::Combo("Video Codec", &codec_idx, codec_names, IM_ARRAYSIZE(codec_names))) {
					sequence_settings_.codec = static_cast<IO::VideoCodecPreset>(codec_idx);
				}
				const char* container_names[] = {"MP4", "MKV", "MOV", "WebM"};
				int container_idx = static_cast<int>(sequence_settings_.container);
				if (ImGui::Combo("Container", &container_idx, container_names, IM_ARRAYSIZE(container_names))) {
					sequence_settings_.container = static_cast<IO::VideoContainer>(container_idx);
				}
				if (sequence_settings_.codec != IO::VideoCodecPreset::PngSequence) {
					int crf_val = static_cast<int>(sequence_settings_.crf);
					if (ImGui::SliderInt("Quality (CRF, Lower = Better)", &crf_val, 0, 51)) {
						sequence_settings_.crf = static_cast<uint32_t>(crf_val);
					}
				}

				const std::string extension_for_ffmpeg = (user_settings_.screenshot_format == 2U) ? "png" : (user_settings_.screenshot_format == 3U) ? "tga" : (user_settings_.screenshot_format == 4U) ? "hdr" : (user_settings_.screenshot_format == 1U) ? "bmp" : "ppm";
				const std::string ffmpeg_cmd = sequence_settings_.build_ffmpeg_command(
					user_settings_.screenshot_output_directory,
					extension_for_ffmpeg,
					user_settings_.screenshot_output_directory + "/assembled_video"
				);
				ImGui::TextColored(ImVec4(0.6f, 0.85f, 1.0f, 1.0f), "Suggested Assembly Command (run externally with ffmpeg after capture):");
				char ffmpeg_cmd_buf[768]{};
				std::strncpy(ffmpeg_cmd_buf, ffmpeg_cmd.c_str(), sizeof(ffmpeg_cmd_buf) - 1);
				ImGui::InputText("##FfmpegCommand", ffmpeg_cmd_buf, sizeof(ffmpeg_cmd_buf), ImGuiInputTextFlags_ReadOnly);

				if (viewport_window_ && viewport_window_->is_sequence_capture_active()) {
					ImGui::ProgressBar(static_cast<float>(viewport_window_->sequence_capture_progress()), ImVec2(-1, 0));
					if (ImGui::Button("Stop Sequence Capture", ImVec2(180.0f, 26.0f))) {
						viewport_window_->stop_sequence_capture();
					}
				} else if (viewport_window_) {
					if (ImGui::Button("Start Sequence Capture", ImVec2(200.0f, 26.0f))) {
						viewport_window_->start_sequence_capture(
							user_settings_.screenshot_output_directory,
							user_settings_.screenshot_filename_pattern,
							static_cast<IO::ScreenshotFormat>(user_settings_.screenshot_format),
							static_cast<double>(sequence_settings_.frames_per_second),
							static_cast<double>(sequence_settings_.duration_seconds),
							sequence_settings_.trigger,
							static_cast<uint64_t>(sequence_frame_count_),
							sequence_settings_.pause_simulation_during_capture
						);
					}
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Close", ImVec2(100.0f, 26.0f))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	void render_main_menu_bar() noexcept {
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Screenshot Capture Settings...")) {
					ImGui::OpenPopup("Screenshot Capture Settings");
				}
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
				ImGui::MenuItem("Physical Constants Engine", key_hint(InputAction::ToggleConstantsWindow).c_str(), &constants_window_.open_state());
				ImGui::MenuItem("Performance Analysis & Profiling", key_hint(InputAction::TogglePerformanceAnalysisWindow).c_str(), &performance_analysis_window_.open_state());
				ImGui::MenuItem("Engine Log Console", nullptr, &log_console_window_.open_state());
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
				ImGui::Separator();
				ImGui::MenuItem("Secondary Viewports Manager", nullptr, &secondary_viewport_manager_panel_open_);
				if (ImGui::MenuItem("Add Secondary Viewport")) {
					if (secondary_viewport_manager_) secondary_viewport_manager_->add_view();
				}
				if (secondary_viewport_manager_ && !secondary_viewport_manager_->views().empty()) {
					ImGui::TextDisabled("Secondary Observer Viewports");
					for (auto& view : secondary_viewport_manager_->views()) {
						ImGui::MenuItem(view->name().c_str(), nullptr, &view->open_state());
					}
				}
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
	}
};

}
