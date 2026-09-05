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
#include "relativistic/ui/visual_diagnostics_window.hpp"
#include "relativistic/ui/body_manager_window.hpp"
#include "relativistic/ui/interactive_camera_controller.hpp"

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

	std::unique_ptr<ViewportPrimaryWindow> viewport_window_;
	std::unique_ptr<ScenarioSelectorWindow> scenario_window_;
	TelemetryWindow telemetry_window_;
	SpectrographWindow spectrograph_window_;
	ControlPanelWindow control_panel_window_;
	PerformanceSettingsWindow performance_window_;
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
		  control_panel_window_(orchestrator, camera_controller_, user_settings_.hud_preferences),
		  performance_window_(orchestrator),
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
				if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) {
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

		viewport_window_ = std::make_unique<ViewportPrimaryWindow>(orchestrator_, camera_controller_, user_settings_.hud_preferences);
		scenario_window_ = std::make_unique<ScenarioSelectorWindow>(orchestrator_, &camera_controller_);
		performance_window_.attach_render_pipeline(viewport_window_->pipeline_ref());
		last_frame_time_ = std::chrono::steady_clock::now();

		telemetry_window_.open_state() = false;
		spectrograph_window_.open_state() = false;
		diagnostics_window_.open_state() = false;
		control_panel_window_.open_state() = true;
		performance_window_.open_state() = true;
		scenario_window_->open_state() = true;

		camera_controller_.config() = user_settings_.camera_controls;
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

		if (show_viewport_ && viewport_window_) {
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
			spectrograph_window_.render();
		}

		if (performance_window_.open_state()) {
			performance_window_.render();
		}

		if (diagnostics_window_.open_state()) {
			diagnostics_window_.render();
		}

		if (body_manager_window_.open_state()) {
			body_manager_window_.render();
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
	void process_global_hotkeys() noexcept {
		ImGuiIO& io = ImGui::GetIO();
		if (io.WantCaptureKeyboard) return;

		if (ImGui::IsKeyPressed(ImGuiKey_F1, false)) {
			control_panel_window_.open_state() = !control_panel_window_.open_state();
		}
		if (ImGui::IsKeyPressed(ImGuiKey_F2, false)) {
			apply_multi_window_layout_preset(UiLayoutPreset::MultiWindowDetached);
		}
		if (ImGui::IsKeyPressed(ImGuiKey_F3, false)) {
			apply_multi_window_layout_preset(UiLayoutPreset::DockedWorkspace);
		}
		if (ImGui::IsKeyPressed(ImGuiKey_F4, false)) {
			apply_multi_window_layout_preset(UiLayoutPreset::ViewportFocused);
		}
		if (ImGui::IsKeyPressed(ImGuiKey_F5, false) || ImGui::IsKeyPressed(ImGuiKey_P, false)) {
			if (orchestrator_.scheduler().is_paused()) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_resume()));
			} else {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_pause()));
			}
		}
		if (ImGui::IsKeyPressed(ImGuiKey_F6, false)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_step(1)));
		}
		if (ImGui::IsKeyPressed(ImGuiKey_F7, false)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_reset()));
		}
		if (ImGui::IsKeyPressed(ImGuiKey_F8, false)) {
			body_manager_window_.open_state() = !body_manager_window_.open_state();
		}
		if (ImGui::IsKeyPressed(ImGuiKey_F9, false)) {
			const uint32_t next_mode = (orchestrator_.parameters().camera_mode + 1) % 4;
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_camera_mode(next_mode)));
		}
		if (ImGui::IsKeyPressed(ImGuiKey_F10, false)) {
			telemetry_window_.open_state() = !telemetry_window_.open_state();
		}
		if (ImGui::IsKeyPressed(ImGuiKey_F11, false)) {
			performance_window_.open_state() = !performance_window_.open_state();
			diagnostics_window_.open_state() = !diagnostics_window_.open_state();
		}
		if (ImGui::IsKeyPressed(ImGuiKey_F12, false)) {
			trigger_screenshot_capture();
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
				if (ImGui::MenuItem("Snap Camera to Equatorial (r=50)", "Alt+1")) {
					camera_controller_.snap_to_equatorial_front(50.0);
				}
				if (ImGui::MenuItem("Snap Camera to ISCO Orbit", "Alt+5")) {
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
				ImGui::MenuItem("3D Primary Viewport", nullptr, &show_viewport_);
				if (scenario_window_) {
					ImGui::MenuItem("Scenario Catalog", nullptr, &scenario_window_->open_state());
				}
				ImGui::MenuItem("Master Controls", nullptr, &control_panel_window_.open_state());
				ImGui::MenuItem("Performance Profiles", nullptr, &performance_window_.open_state());
				ImGui::MenuItem("Celestial Body & N-Body Manager", "F8", &body_manager_window_.open_state());
				ImGui::MenuItem("Curvature Diagnostics", nullptr, &diagnostics_window_.open_state());
				ImGui::MenuItem("Curvature Telemetry", nullptr, &telemetry_window_.open_state());
				ImGui::MenuItem("Spectrograph Monitor", nullptr, &spectrograph_window_.open_state());
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
	}
};

}
