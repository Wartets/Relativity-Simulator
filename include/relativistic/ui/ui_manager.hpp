#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/ui/telemetry_window.hpp"
#include "relativistic/ui/spectrograph_window.hpp"
#include "relativistic/ui/control_panel_window.hpp"
#include "relativistic/ui/secondary_view_window.hpp"
#include "relativistic/ui/viewport_primary_window.hpp"
#include "relativistic/ui/scenario_selector_window.hpp"
#include "relativistic/ui/performance_settings_window.hpp"
#include "relativistic/ui/visual_diagnostics_window.hpp"
#include "relativistic/ui/interactive_camera_controller.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <stdexcept>

namespace Relativistic::UI {

class UiManager {
private:
	GLFWwindow* main_window_{nullptr};
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	InteractiveCameraController camera_controller_;

	std::unique_ptr<ViewportPrimaryWindow> viewport_window_;
	std::unique_ptr<ScenarioSelectorWindow> scenario_window_;
	TelemetryWindow telemetry_window_;
	SpectrographWindow spectrograph_window_;
	ControlPanelWindow control_panel_window_;
	PerformanceSettingsWindow performance_window_;
	VisualDiagnosticsWindow diagnostics_window_;
	std::vector<SecondaryViewWindow> secondary_views_;

	bool show_viewport_{true};
	bool show_scenarios_{true};
	bool show_telemetry_{true};
	bool show_spectrograph_{true};
	bool show_controls_{true};
	bool show_performance_{true};
	bool show_diagnostics_{true};

	std::chrono::steady_clock::time_point last_frame_time_;

public:
	explicit UiManager(Orchestrator::SimulationOrchestrator<1024>& orchestrator)
		: orchestrator_(orchestrator),
		  camera_controller_(orchestrator),
		  control_panel_window_(orchestrator),
		  performance_window_(orchestrator),
		  diagnostics_window_(orchestrator) {}

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

		main_window_ = glfwCreateWindow(1920, 1080, "Relativistic Engine - Scientific Workspace", nullptr, nullptr);
		if (!main_window_) {
			glfwTerminate();
			throw std::runtime_error("Failed to create GLFW window");
		}

		glfwMakeContextCurrent(main_window_);
		glfwSwapInterval(1);

		glfwSetWindowUserPointer(main_window_, this);
		glfwSetScrollCallback(main_window_, [](GLFWwindow* win, double, double yoffset) {
			auto* self = static_cast<UiManager*>(glfwGetWindowUserPointer(win));
			if (self) {
				self->camera_controller_.handle_scroll(yoffset);
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

		ImGui_ImplGlfw_InitForOpenGL(main_window_, true);
		ImGui_ImplOpenGL3_Init("#version 330");

		viewport_window_ = std::make_unique<ViewportPrimaryWindow>(orchestrator_, camera_controller_);
		scenario_window_ = std::make_unique<ScenarioSelectorWindow>(orchestrator_);
		last_frame_time_ = std::chrono::steady_clock::now();
	}

	void add_secondary_view(const std::string& name) {
		secondary_views_.emplace_back(name);
	}

	void render_frame() {
		const auto now = std::chrono::steady_clock::now();
		const double dt = std::chrono::duration<double>(now - last_frame_time_).count();
		last_frame_time_ = now;

		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		render_main_menu_bar();

		ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(0U, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
		static_cast<void>(dockspace_id);

		if (show_viewport_ && viewport_window_) {
			viewport_window_->render(main_window_, dt);
		}

		if (show_scenarios_ && scenario_window_) {
			scenario_window_->render();
		}

		if (show_controls_) {
			control_panel_window_.render();
		}

		if (show_telemetry_) {
			telemetry_window_.render(orchestrator_);
		}

		if (show_spectrograph_) {
			spectrograph_window_.render();
		}

		if (show_performance_) {
			performance_window_.render();
		}

		if (show_diagnostics_) {
			diagnostics_window_.render();
		}

		for (auto& view : secondary_views_) {
			view.render();
		}

		ImGui::Render();

		int display_w = 0, display_h = 0;
		glfwGetFramebufferSize(main_window_, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
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
				if (ImGui::MenuItem("Pause / Resume", "Space / F5")) {
					if (orchestrator_.scheduler().is_paused()) {
						static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_resume()));
					} else {
						static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_pause()));
					}
				}
				if (ImGui::MenuItem("Single Step", "F6")) {
					static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_step(1)));
				}
				if (ImGui::MenuItem("Reset State", "F7")) {
					static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_reset()));
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Windows")) {
				ImGui::MenuItem("3D Primary Viewport", nullptr, &show_viewport_);
				ImGui::MenuItem("Scenario Manager", nullptr, &show_scenarios_);
				ImGui::MenuItem("Master Controls", nullptr, &show_controls_);
				ImGui::MenuItem("Performance & Optimization", nullptr, &show_performance_);
				ImGui::MenuItem("Curvature Diagnostics", nullptr, &show_diagnostics_);
				ImGui::MenuItem("Curvature Telemetry", nullptr, &show_telemetry_);
				ImGui::MenuItem("Spectrograph Monitor", nullptr, &show_spectrograph_);
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
	}
};

}
