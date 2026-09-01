#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/ui/telemetry_window.hpp"
#include "relativistic/ui/spectrograph_window.hpp"
#include "relativistic/ui/control_panel_window.hpp"
#include "relativistic/ui/secondary_view_window.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <memory>
#include <string>
#include <stdexcept>

namespace Relativistic::UI {

class UiManager {
private:
	GLFWwindow* main_window_{nullptr};
	Orchestrator::SimulationOrchestrator<16384>& orchestrator_;
	
	TelemetryWindow telemetry_window_;
	SpectrographWindow spectrograph_window_;
	ControlPanelWindow control_panel_window_;
	std::vector<SecondaryViewWindow> secondary_views_;

public:
	explicit UiManager(Orchestrator::SimulationOrchestrator<16384>& orchestrator)
		: orchestrator_(orchestrator), control_panel_window_(orchestrator) {}

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

		main_window_ = glfwCreateWindow(1920, 1080, "Relativistic Engine - Primary View", nullptr, nullptr);
		if (!main_window_) {
			glfwTerminate();
			throw std::runtime_error("Failed to create GLFW window");
		}

		glfwMakeContextCurrent(main_window_);
		glfwSwapInterval(0);

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
	}

	void add_secondary_view(const std::string& name) {
		secondary_views_.emplace_back(name);
	}

	void render_frame() {
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(0U, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
		static_cast<void>(dockspace_id);

		telemetry_window_.render(orchestrator_);
		spectrograph_window_.render();
		control_panel_window_.render();

		for (auto& view : secondary_views_) {
			view.render();
		}

		ImGui::Render();
		
		int display_w = 0, display_h = 0;
		glfwGetFramebufferSize(main_window_, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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

	bool should_close() const {
		return glfwWindowShouldClose(main_window_);
	}

	void shutdown() {
		if (main_window_) {
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
};

}
