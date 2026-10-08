#pragma once

#include "relativistic/io/user_settings.hpp"
#include "relativistic/io/scenario/scenario_locator.hpp"
#include "relativistic/core/engine_log.hpp"
#include "relativistic/io/capture/screenshot_exporter.hpp"
#include "relativistic/io/capture/screenshot_capture_settings.hpp"
#include "relativistic/io/capture/video_capture_settings.hpp"
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
#include "relativistic/ui/hud/hud_manager_window.hpp"
#include "relativistic/ui/constants_window.hpp"
#include "relativistic/ui/input_actions.hpp"
#include "relativistic/ui/log_console_window.hpp"
#include "relativistic/ui/secondary_viewport_manager.hpp"
#include "relativistic/ui/capture_studio_window.hpp"
#include "relativistic/ui/window_chrome.hpp"
#include "relativistic/orchestrator/session_state.hpp"
#include "relativistic/core/system_console.hpp"
#include "relativistic/dynamics/bulk_body_actions.hpp"

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
#include <exception>
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
	WindowChromeController window_chrome_;
	InteractiveCameraController camera_controller_;
	KeybindSettingsWindow keybind_window_;
	HudManagerWindow hud_manager_window_;
	ActionEdgeTracker global_action_tracker_{};

	std::unique_ptr<ViewportPrimaryWindow> viewport_window_;
	std::unique_ptr<CaptureStudioWindow> capture_studio_window_;
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
	UiLayoutPreset current_layout_{UiLayoutPreset::MultiWindowDetached};

	std::chrono::steady_clock::time_point last_frame_time_;
	double autosave_elapsed_seconds_{0.0};
	static constexpr double kAutosaveIntervalSeconds = 20.0;

public:
	explicit UiManager(Orchestrator::SimulationOrchestrator<1024>& orchestrator, IO::UserSettings& user_settings)
		: orchestrator_(orchestrator),
		  user_settings_(user_settings),
		  window_chrome_(user_settings.window_chrome),
		  camera_controller_(orchestrator),
		  keybind_window_(camera_controller_.config()),
		  hud_manager_window_(user_settings_.hud_layout),
		  control_panel_window_(orchestrator, camera_controller_, user_settings_.hud_layout, user_settings_.schematic_view, user_settings_.spatial_reference, hud_manager_window_.open_state(), keybind_window_.open_state()),
		  performance_window_(orchestrator),
		  performance_analysis_window_(orchestrator),
		  diagnostics_window_(orchestrator),
		  body_manager_window_(orchestrator),
		  constants_window_(orchestrator),
		  secondary_viewport_manager_(std::make_unique<SecondaryViewportManager>(orchestrator)) {
		body_manager_window_.attach_persisted_settings(user_settings_);
	}

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
		glfwSetScrollCallback(main_window_, [](GLFWwindow* win, double, double yoffset) noexcept {
			try {
				auto* self = static_cast<UiManager*>(glfwGetWindowUserPointer(win));
				if (self && self->viewport_window_ && self->viewport_window_->is_hovered()) {
					if (self->camera_controller_.config().keybinds.is_active(InputAction::ZoomModifier, win)) {
						self->viewport_window_->handle_zoom_scroll(yoffset);
					} else {
						self->camera_controller_.handle_scroll(yoffset);
					}
				}
			} catch (const std::exception& ex) {
				Core::log_error(std::string("Scroll callback failed and was ignored: ") + ex.what());
			} catch (...) {
				Core::log_error("Scroll callback failed with an unknown error and was ignored.");
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

		viewport_window_ = std::make_unique<ViewportPrimaryWindow>(orchestrator_, camera_controller_, user_settings_.hud_layout, user_settings_.schematic_view, user_settings_.spatial_reference);
		capture_studio_window_ = std::make_unique<CaptureStudioWindow>(orchestrator_, *viewport_window_, user_settings_);
		viewport_window_->set_fullscreen_toggle_callback([this]() { multi_window_mode_ = !multi_window_mode_; });
		viewport_window_->set_open_screenshot_settings_callback([this]() { open_capture_studio(); });
		scenario_window_ = std::make_unique<ScenarioSelectorWindow>(orchestrator_, user_settings_, &camera_controller_);
		scenario_window_->set_settings_persist_callback([this]() {
			export_runtime_settings();
			user_settings_.save();
		});
		performance_window_.attach_render_pipeline(viewport_window_->pipeline_ref());
		performance_window_.attach_performance_analysis_window(performance_analysis_window_.open_state());
		control_panel_window_.attach_render_pipeline(viewport_window_->pipeline_ref());
		performance_analysis_window_.attach_render_pipeline(viewport_window_->pipeline_ref());
		spectrograph_window_.attach_ray_probe(&viewport_window_->ray_probe_result());
		spectrograph_window_.set_intensity_provider([this](Interferometry::IntensityImage& image, uint32_t size) {
			return viewport_window_ != nullptr && viewport_window_->capture_intensity_image(image, size);
		});
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
		capture_studio_window_->open_state() = user_settings_.window_capture_studio_open;
		log_console_window_.open_state() = user_settings_.window_log_console_open;
		log_console_window_.attach_system_console_flag(user_settings_.show_system_console);

		orchestrator_.unit_preferences() = user_settings_.unit_preferences;

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

		{
			auto& ic = orchestrator_.interaction_config();
			ic.electromagnetic.electricity_enabled = user_settings_.interaction_electricity_enabled;
			ic.electromagnetic.magnetism_enabled = user_settings_.interaction_magnetism_enabled;
			if (user_settings_.constants_preset == static_cast<uint32_t>(Core::ConstantsPreset::Custom)) {
				ic.electromagnetic.vacuum_permittivity = user_settings_.interaction_vacuum_permittivity;
				ic.electromagnetic.vacuum_permeability = user_settings_.interaction_vacuum_permeability;
			} else {
				ic.electromagnetic.vacuum_permittivity = orchestrator_.constants_engine().sim_vacuum_permittivity();
				ic.electromagnetic.vacuum_permeability = orchestrator_.constants_engine().sim_vacuum_permeability();
			}
			ic.collisions.enabled = user_settings_.interaction_collisions_enabled;
			ic.collisions.response_model = static_cast<Dynamics::CollisionResponseModel>(user_settings_.interaction_collision_response_model);
			ic.collisions.consider_rotation = user_settings_.interaction_collision_consider_rotation;
			ic.collisions.consider_friction = user_settings_.interaction_collision_consider_friction;
			ic.collisions.restitution_multiplier = user_settings_.interaction_collision_restitution_multiplier;
			ic.thermodynamics.enabled = user_settings_.interaction_thermodynamics_enabled;
			ic.thermodynamics.ambient_temperature_kelvin = user_settings_.interaction_ambient_temperature;
			ic.thermodynamics.radiative_coupling_scale = user_settings_.interaction_radiative_coupling_scale;
			ic.fragmentation.enabled = user_settings_.interaction_fragmentation_enabled;
			ic.fragmentation.minimum_fragment_mass = user_settings_.interaction_minimum_fragment_mass;
			ic.fragmentation.max_fragments_per_event = user_settings_.interaction_fragmentation_max_fragments;
			ic.fragmentation.collision_energy_to_integrity_loss = user_settings_.interaction_collision_energy_to_integrity_loss;
			ic.annihilation.enabled = user_settings_.interaction_annihilation_enabled;
			ic.annihilation.contact_distance_scale = user_settings_.interaction_annihilation_contact_scale;
			ic.annihilation.require_opposite_charge = user_settings_.interaction_annihilation_require_opposite_charge;
			ic.collisions.contact_stiffness_scale = user_settings_.interaction_collision_stiffness_scale;
			ic.collisions.position_correction_factor = user_settings_.interaction_collision_position_correction_factor;
			ic.fragmentation.enable_tidal_stress = user_settings_.interaction_fragmentation_tidal_stress_enabled;
		}
		multi_window_mode_ = user_settings_.multi_window_mode;
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_camera_mode(user_settings_.default_camera_mode)));

		queue_startup_scenario();
		queue_session_restore();

		if (secondary_viewport_manager_) {
			for (const auto& saved : user_settings_.secondary_views) {
				if (!saved.active) continue;
				auto& view = secondary_viewport_manager_->add_view_with_state(saved.name, saved.open);
				view.apply_saved_state(saved.radius, saved.theta, saved.phi, saved.fov_deg, saved.exposure, saved.tonemapping_mode, saved.projection_mode, saved.max_steps, saved.resolution_scale, saved.follow_primary, saved.follow_offset_theta, saved.follow_offset_phi);
			}
		}

		current_layout_ = static_cast<UiLayoutPreset>(std::min(user_settings_.last_window_layout, static_cast<uint32_t>(UiLayoutPreset::DeepAnalysis)));
		if (user_settings_.window_chrome.geometries.empty()) {
			pending_layout_reset_ = true;
		}
	}

	void add_secondary_view(const std::string& name) {
		if (secondary_viewport_manager_) {
			secondary_viewport_manager_->add_view(name);
		}
	}

	void trigger_screenshot_capture() noexcept {
		if (!capture_studio_window_) {
			return;
		}
		try {
			capture_studio_window_->capture_screenshot_now();
		} catch (const std::exception& ex) {
			Core::log_error(std::string("Quick screenshot failed: ") + ex.what());
		} catch (...) {
			Core::log_error("Quick screenshot failed with an unknown error.");
		}
	}

	void toggle_capture_studio() noexcept {
		if (capture_studio_window_) {
			capture_studio_window_->open_state() = !capture_studio_window_->open_state();
		}
	}

	void trigger_sequence_start() noexcept {
		if (!capture_studio_window_) {
			return;
		}
		try {
			capture_studio_window_->start_sequence();
		} catch (const std::exception& ex) {
			Core::log_error(std::string("Sequence start failed: ") + ex.what());
		} catch (...) {
			Core::log_error("Sequence start failed with an unknown error.");
		}
	}

	void open_capture_studio() noexcept {
		if (capture_studio_window_) {
			capture_studio_window_->open_state() = true;
		}
	}

	void export_runtime_settings() noexcept {
		if (capture_studio_window_) {
			capture_studio_window_->save_settings();
		}
		user_settings_.camera_controls = camera_controller_.config();
		user_settings_.unit_preferences = orchestrator_.unit_preferences();
		user_settings_.multi_window_mode = multi_window_mode_;
		user_settings_.last_window_layout = static_cast<uint32_t>(current_layout_);
		user_settings_.default_camera_mode = orchestrator_.parameters().camera_mode;
		window_chrome_.capture_geometries();
		Orchestrator::SessionStateStore::capture(orchestrator_, user_settings_.session_values);
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
		user_settings_.window_capture_studio_open = capture_studio_window_ ? capture_studio_window_->open_state() : user_settings_.window_capture_studio_open;
		user_settings_.window_log_console_open = log_console_window_.open_state();
		user_settings_.constants_preset = static_cast<uint32_t>(orchestrator_.constants_engine().active_preset());
		user_settings_.constants_c = orchestrator_.constants_engine().sim_speed_of_light();
		user_settings_.constants_g = orchestrator_.constants_engine().sim_gravitational_constant();
		user_settings_.constants_h = orchestrator_.constants_engine().sim_planck_constant();
		user_settings_.constants_kb = orchestrator_.constants_engine().sim_boltzmann_constant();
		user_settings_.constants_na = orchestrator_.constants_engine().sim_avogadro_constant();
		user_settings_.constants_ke = orchestrator_.constants_engine().sim_coulomb_constant();
		user_settings_.constants_kcd = orchestrator_.constants_engine().sim_luminous_efficacy();

		{
			const auto& ic = orchestrator_.interaction_config();
			user_settings_.interaction_electricity_enabled = ic.electromagnetic.electricity_enabled;
			user_settings_.interaction_magnetism_enabled = ic.electromagnetic.magnetism_enabled;
			user_settings_.interaction_vacuum_permittivity = ic.electromagnetic.vacuum_permittivity;
			user_settings_.interaction_vacuum_permeability = ic.electromagnetic.vacuum_permeability;
			user_settings_.interaction_collisions_enabled = ic.collisions.enabled;
			user_settings_.interaction_collision_response_model = static_cast<uint32_t>(ic.collisions.response_model);
			user_settings_.interaction_collision_consider_rotation = ic.collisions.consider_rotation;
			user_settings_.interaction_collision_consider_friction = ic.collisions.consider_friction;
			user_settings_.interaction_collision_restitution_multiplier = ic.collisions.restitution_multiplier;
			user_settings_.interaction_thermodynamics_enabled = ic.thermodynamics.enabled;
			user_settings_.interaction_ambient_temperature = ic.thermodynamics.ambient_temperature_kelvin;
			user_settings_.interaction_radiative_coupling_scale = ic.thermodynamics.radiative_coupling_scale;
			user_settings_.interaction_fragmentation_enabled = ic.fragmentation.enabled;
			user_settings_.interaction_minimum_fragment_mass = ic.fragmentation.minimum_fragment_mass;
			user_settings_.interaction_fragmentation_max_fragments = ic.fragmentation.max_fragments_per_event;
			user_settings_.interaction_collision_energy_to_integrity_loss = ic.fragmentation.collision_energy_to_integrity_loss;
			user_settings_.interaction_annihilation_enabled = ic.annihilation.enabled;
			user_settings_.interaction_annihilation_contact_scale = ic.annihilation.contact_distance_scale;
			user_settings_.interaction_annihilation_require_opposite_charge = ic.annihilation.require_opposite_charge;
			user_settings_.interaction_collision_stiffness_scale = ic.collisions.contact_stiffness_scale;
			user_settings_.interaction_collision_position_correction_factor = ic.collisions.position_correction_factor;
			user_settings_.interaction_fragmentation_tidal_stress_enabled = ic.fragmentation.enable_tidal_stress;
		}

		for (auto& slot : user_settings_.secondary_views) {
			slot = IO::SecondaryViewPersistedState{};
		}
		if (secondary_viewport_manager_) {
			size_t slot_index = 0;
			for (auto& view_ptr : secondary_viewport_manager_->views()) {
				if (slot_index >= user_settings_.secondary_views.size()) break;
				auto& slot = user_settings_.secondary_views[slot_index++];
				slot.active = true;
				slot.open = view_ptr->open_state();
				slot.name = view_ptr->name();
				slot.radius = view_ptr->radius();
				slot.theta = view_ptr->theta();
				slot.phi = view_ptr->phi();
				slot.fov_deg = view_ptr->fov_deg();
				slot.exposure = view_ptr->exposure();
				slot.tonemapping_mode = view_ptr->tonemapping_mode();
				slot.projection_mode = view_ptr->projection_mode();
				slot.max_steps = view_ptr->max_steps();
				slot.resolution_scale = view_ptr->resolution_scale();
				slot.follow_primary = view_ptr->follow_primary_camera();
				slot.follow_offset_theta = view_ptr->follow_offset_theta();
				slot.follow_offset_phi = view_ptr->follow_offset_phi();
			}
		}
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

		autosave_elapsed_seconds_ += dt;
		if (autosave_elapsed_seconds_ >= kAutosaveIntervalSeconds) {
			autosave_elapsed_seconds_ = 0.0;
			export_runtime_settings();
			user_settings_.save();
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		window_chrome_.begin_frame();

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
			try {
				synchronize_analysis_links();
				auto theme = window_chrome_.scope(WindowThemeId::Viewport);
				viewport_window_->render(main_window_, dt, multi_window_mode_);
			} catch (const std::exception& ex) {
				Core::log_error(std::string("Viewport render frame failed and was skipped: ") + ex.what());
			} catch (...) {
				Core::log_error("Viewport render frame failed with an unknown error and was skipped.");
			}
		}

		if (viewport_window_) {
			viewport_window_->capture_coordinator().update(dt);
		}

		if (scenario_window_ && scenario_window_->open_state()) {
			auto theme = window_chrome_.scope(WindowThemeId::Scenarios);
			scenario_window_->render();
		}

		if (control_panel_window_.open_state()) {
			auto theme = window_chrome_.scope(WindowThemeId::Controls);
			control_panel_window_.render();
		}

		if (telemetry_window_.open_state()) {
			auto theme = window_chrome_.scope(WindowThemeId::Telemetry);
			telemetry_window_.render(orchestrator_);
		}

		if (spectrograph_window_.open_state()) {
			auto theme = window_chrome_.scope(WindowThemeId::Spectrograph);
			spectrograph_window_.render(orchestrator_);
		}

		if (performance_window_.open_state()) {
			auto theme = window_chrome_.scope(WindowThemeId::Performance);
			performance_window_.render();
		}

		if (performance_analysis_window_.open_state()) {
			auto theme = window_chrome_.scope(WindowThemeId::Analysis);
			performance_analysis_window_.render();
		}

		if (diagnostics_window_.open_state()) {
			auto theme = window_chrome_.scope(WindowThemeId::Diagnostics);
			diagnostics_window_.render();
		}

		if (body_manager_window_.open_state()) {
			auto theme = window_chrome_.scope(WindowThemeId::Bodies);
			body_manager_window_.render();
		}

		if (keybind_window_.open_state()) {
			auto theme = window_chrome_.scope(WindowThemeId::Keybinds);
			keybind_window_.render(main_window_);
		}

		if (constants_window_.open_state()) {
			auto theme = window_chrome_.scope(WindowThemeId::Constants);
			constants_window_.render();
		}

		if (log_console_window_.open_state()) {
			auto theme = window_chrome_.scope(WindowThemeId::LogConsole);
			log_console_window_.render();
		}

		if (capture_studio_window_) {
			auto theme = window_chrome_.scope(WindowThemeId::CaptureStudio);
			capture_studio_window_->render();
		}

		if (secondary_viewport_manager_) {
			auto theme = window_chrome_.scope(WindowThemeId::SecondaryViews);
			secondary_viewport_manager_->render_all();
			secondary_viewport_manager_->render_management_panel(secondary_viewport_manager_panel_open_);
		}

		window_chrome_.process_context_requests();
		window_chrome_.render_popup();

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
			capture_studio_window_.reset();
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
	void synchronize_analysis_links() noexcept {
		if (!viewport_window_) {
			return;
		}
		const auto& hud = user_settings_.hud_layout;
		const bool hud_needs_probe = hud.master_enabled
			&& (hud.element(HudElementId::RayProbeReadout).enabled || hud.element(HudElementId::RayProbeEmissionReadout).enabled);
		viewport_window_->configure_ray_probe(
			spectrograph_window_.open_state() || hud_needs_probe,
			spectrograph_window_.ray_probe_source(),
			spectrograph_window_.ray_probe_frozen()
		);
		viewport_window_->set_linked_readouts(spectrograph_window_.linked_readouts());
	}

	void queue_session_restore() {
		if (user_settings_.session_values.empty()) {
			return;
		}
		orchestrator_.queue_post_command_action([values = user_settings_.session_values](Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
			Orchestrator::SessionStateStore::apply(orchestrator, values);
		});
	}

	void queue_startup_scenario() {
		if (!user_settings_.load_scenario_on_startup) {
			return;
		}

		const auto startup = IO::ScenarioLocator::resolve_startup_scenario(user_settings_.default_scenario_path);
		if (!startup.has_value()) {
			Core::log_error("No valid startup scenario could be located; the simulation starts empty.");
			return;
		}

		if (startup->used_fallback) {
			const std::string configured = user_settings_.default_scenario_path.empty() ? std::string("(none)") : user_settings_.default_scenario_path;
			Core::log_warning("Startup scenario '" + configured + "' is unavailable or invalid; falling back to '" + startup->path + "'.");
			user_settings_.default_scenario_path = std::string(IO::ScenarioLocator::kBuiltInStartupScenario);
		}

		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_load_scenario(startup->path)));
	}

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
			const uint32_t next_mode = (orchestrator_.parameters().camera_mode + 1) % Observer::kCameraNavigationModeCount;
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
			open_capture_studio();
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
		if (global_action_tracker_.just_pressed(keybinds, InputAction::ToggleCaptureStudio, main_window_)) {
			toggle_capture_studio();
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
		if (global_action_tracker_.just_pressed(keybinds, InputAction::ToggleRayProbeFreeze, main_window_)) {
			spectrograph_window_.toggle_ray_probe_freeze();
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
		if (global_action_tracker_.just_pressed(keybinds, InputAction::BulkInvertAllVelocities, main_window_)) {
			Dynamics::BulkBodyActions::invert_all_velocities(orchestrator_.nbody_system());
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::BulkScatterBodyPositions, main_window_)) {
			Dynamics::BulkBodyActions::scatter_positions(orchestrator_.nbody_system());
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::BulkSnapBodiesToGrid, main_window_)) {
			Dynamics::BulkBodyActions::snap_to_grid(orchestrator_.nbody_system(), 1.0);
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::BulkCullBodiesOutsideView, main_window_)) {
			const auto& p = orchestrator_.parameters();
			const double limit = (p.render_distance_scale > 0.0) ? (p.render_distance_scale * std::max(p.mass, 1e-6)) : 1.0e7;
			Dynamics::BulkBodyActions::cull_outside_radius(orchestrator_.nbody_system(), limit);
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::BulkEqualizeBodyMasses, main_window_)) {
			Dynamics::BulkBodyActions::equalize_masses(orchestrator_.nbody_system());
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::BulkAverageBodyMasses, main_window_)) {
			Dynamics::BulkBodyActions::average_masses(orchestrator_.nbody_system());
		}
		if (global_action_tracker_.just_pressed(keybinds, InputAction::BulkZeroAllSpins, main_window_)) {
			Dynamics::BulkBodyActions::zero_all_spins(orchestrator_.nbody_system());
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

			auto set_window_rect = [this](const char* name, ImVec2 pos, ImVec2 size) {
				window_chrome_.place(name, WindowGeometry{pos.x, pos.y, size.x, size.y});
			};

			set_window_rect("Scenario Manager & Presets", ImVec2(offset_x + 15.0f, offset_y + 30.0f), ImVec2(left_col_w, top_h * 0.50f));
			set_window_rect("Celestial Body & N-Body Manager", ImVec2(offset_x + 15.0f, offset_y + 30.0f + top_h * 0.50f + 8.0f), ImVec2(left_col_w, top_h * 0.50f - 16.0f));
			set_window_rect("Master Simulation Controls", ImVec2(offset_x + screen_w - right_col_w - 15.0f, offset_y + 30.0f), ImVec2(right_col_w, top_h));
			set_window_rect("Telemetry & Invariants", ImVec2(offset_x + 15.0f, offset_y + top_h + 24.0f), ImVec2(left_col_w, bottom_h + 10.0f));
			set_window_rect("Radiative Transfer & Spectrograph Monitor", ImVec2(offset_x + left_col_w + 25.0f, offset_y + top_h + 24.0f), ImVec2(center_w * 0.5f - 8.0f, bottom_h + 10.0f));
			set_window_rect("Performance & Engine Optimization", ImVec2(offset_x + left_col_w + 25.0f + center_w * 0.5f, offset_y + top_h + 24.0f), ImVec2(center_w * 0.5f - 8.0f, bottom_h + 10.0f));
			set_window_rect("Curvature Diagnostics & Tensor Inspector", ImVec2(offset_x + screen_w - right_col_w - 15.0f, offset_y + top_h + 24.0f), ImVec2(right_col_w, bottom_h + 10.0f));
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
				if (ImGui::MenuItem("Capture Studio...", key_hint(InputAction::CaptureScreenshot).c_str())) {
					open_capture_studio();
				}
				if (ImGui::MenuItem("Quick Screenshot (Current Studio Settings)")) {
					trigger_screenshot_capture();
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
					const uint32_t next_mode = (orchestrator_.parameters().camera_mode + 1) % Observer::kCameraNavigationModeCount;
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

			if (ImGui::BeginMenu("Capture")) {
				if (capture_studio_window_ && viewport_window_) {
					auto& coordinator = viewport_window_->capture_coordinator();
					const bool busy = coordinator.is_busy();
					const bool sequence_active = coordinator.is_sequence_active();
					ImGui::MenuItem("Capture Studio", key_hint(InputAction::ToggleCaptureStudio).c_str(), &capture_studio_window_->open_state());
					ImGui::Separator();
					if (ImGui::MenuItem("Screenshot Settings...")) {
						capture_studio_window_->open_tab(CaptureStudioWindow::StudioTab::Screenshot);
					}
					if (ImGui::MenuItem("Sequence Settings...")) {
						capture_studio_window_->open_tab(CaptureStudioWindow::StudioTab::Sequence);
					}
					if (ImGui::MenuItem("Motion Script Editor...")) {
						capture_studio_window_->open_tab(CaptureStudioWindow::StudioTab::Script);
					}
					if (ImGui::MenuItem("Data Recording...")) {
						capture_studio_window_->open_tab(CaptureStudioWindow::StudioTab::Recording);
					}
					if (ImGui::MenuItem("Video Encoding...")) {
						capture_studio_window_->open_tab(CaptureStudioWindow::StudioTab::Encoding);
					}
					ImGui::Separator();
					ImGui::BeginDisabled(busy);
					if (ImGui::MenuItem("Quick Screenshot", key_hint(InputAction::CaptureScreenshot).c_str())) {
						trigger_screenshot_capture();
					}
					if (ImGui::MenuItem("Start Sequence")) {
						trigger_sequence_start();
					}
					ImGui::EndDisabled();
					ImGui::BeginDisabled(!sequence_active);
					if (ImGui::MenuItem("Finish Sequence")) {
						coordinator.stop_sequence();
					}
					ImGui::EndDisabled();
					ImGui::BeginDisabled(!busy);
					if (ImGui::MenuItem("Cancel Capture")) {
						coordinator.cancel_all();
					}
					ImGui::EndDisabled();
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
				if (capture_studio_window_) {
					ImGui::MenuItem("Capture Studio", key_hint(InputAction::ToggleCaptureStudio).c_str(), &capture_studio_window_->open_state());
				}
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
