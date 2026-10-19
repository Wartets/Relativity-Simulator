#include "support/test_harness.hpp"
#include "relativistic/orchestrator/command.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/repl.hpp"
#include "relativistic/ui/interactive_camera_controller.hpp"
#include "relativistic/ui/scenario_selector_window.hpp"
#include "relativistic/ui/ui_manager.hpp"
#include <array>
#include <cmath>
#include <memory>

using namespace Relativistic;
using Testing::TestSuite;

namespace {

using Host = Orchestrator::SimulationOrchestrator<1024>;
using Repl = Orchestrator::MasterTerminalRepl<1024>;

[[nodiscard]] bool run_command(Host& host, Repl& repl, const char* line) {
	Orchestrator::CommandResult result{};
	const bool accepted = repl.execute_line(line, &result);
	host.process_incoming_commands();
	return accepted;
}

}

int main() {
	TestSuite suite(
		"ui_multi_window",
		"Verifies the terminal command path and the interactive camera controller that the multi-window interface relies on: parameter, camera, metric, integrator, render and preset commands applied through the REPL, controller speed and sensitivity accessors, scroll zoom, navigation mode switching and the snap helpers."
	);

	auto host = std::make_unique<Host>();
	Repl repl(*host);

	suite.section("Parameter commands");
	suite.check(run_command(*host, repl, "set mass 5.0"), "the mass command is accepted");
	suite.check_near(host->parameters().mass, 5.0, 1e-12, "the mass is applied");
	suite.check(run_command(*host, repl, "set spin 0.94"), "the spin command is accepted");
	suite.check_near(host->parameters().spin, 0.94, 1e-12, "the spin is applied");

	suite.section("Camera commands");
	suite.check(run_command(*host, repl, "camera fov 45.0"), "the field of view command is accepted");
	suite.check_near(host->camera().fov_deg, 45.0, 1e-12, "the field of view is applied");
	suite.check(run_command(*host, repl, "camera speed 25.0"), "the speed command is accepted");
	suite.check_near(host->camera().speed, 25.0, 1e-12, "the speed is applied");
	const auto origin = host->camera().position;
	suite.check(run_command(*host, repl, "camera move 10.0 -5.0 2.0"), "the move command is accepted");
	suite.check_near(host->camera().position[0], origin[0] + 10.0, 1e-12, "the x translation is applied relative to the previous position");
	suite.check_near(host->camera().position[1], origin[1] - 5.0, 1e-12, "the y translation is applied relative to the previous position");
	suite.check_near(host->camera().position[2], origin[2] + 2.0, 1e-12, "the z translation is applied relative to the previous position");

	suite.section("Metric, integrator and render commands");
	suite.check(run_command(*host, repl, "metric Kerr") && host->active_metric_name() == "Kerr", "the metric name is stored");
	suite.check(run_command(*host, repl, "integrator Vernier9") && host->active_integrator_name() == "Vernier9", "the integrator name is stored");
	suite.check(run_command(*host, repl, "set render_scale 1.5"), "the render scale command is accepted");
	suite.check_near(host->parameters().resolution_scale, 1.5, 1e-12, "the render scale is applied");
	suite.check(run_command(*host, repl, "set ray_steps 4096") && host->parameters().max_ray_steps == 4096, "the ray step budget is applied");
	suite.check(host->parameters().performance_preset == Host::CUSTOM_PERFORMANCE_PRESET, "editing a render parameter selects the custom preset");
	suite.check(run_command(*host, repl, "set performance 1"), "the preset command is accepted");
	suite.check(host->parameters().performance_preset == 1, "the preset index is applied");
	suite.check_near(host->parameters().resolution_scale, 0.5, 1e-12, "the Performance preset applies its documented resolution scale of 0.5");
	suite.check(run_command(*host, repl, "set camera_mode 1") && host->parameters().camera_mode == 1, "the camera mode is applied");

	suite.section("Interactive camera controller");
	UI::InteractiveCameraController controller(*host);
	controller.set_move_speed(15.0);
	suite.check_near(controller.move_speed(), 15.0, 1e-12, "the move speed accessor returns the stored value");
	controller.set_mouse_sensitivity(0.25);
	suite.check_near(controller.mouse_sensitivity(), 0.25, 1e-12, "the mouse sensitivity accessor returns the stored value");
	const double fov_before = host->camera().fov_deg;
	controller.handle_scroll(-1.0);
	host->process_incoming_commands();
	suite.check(std::abs(host->camera().fov_deg - fov_before) > 1e-9, "a scroll step changes the field of view", "before=" + Testing::format_number(fov_before, 6) + " after=" + Testing::format_number(host->camera().fov_deg, 6));
	controller.set_navigation_mode(UI::CameraNavigationMode::OrbitCenter);
	suite.check(controller.navigation_mode() == UI::CameraNavigationMode::OrbitCenter, "the orbit navigation mode is selected");
	controller.set_navigation_mode(UI::CameraNavigationMode::SphericalBoyerLindquist);
	suite.check(controller.navigation_mode() == UI::CameraNavigationMode::SphericalBoyerLindquist, "the spherical navigation mode is selected");

	suite.section("Snap helpers");
	controller.snap_to_equatorial_front(60.0);
	host->process_incoming_commands();
	host->camera().synchronize_spherical();
	suite.check_near(host->camera().radius, 60.0, 1e-6, "the equatorial snap places the camera at the requested radius");
	suite.check_near(std::cos(host->camera().theta), 0.0, 1e-6, "the equatorial snap places the camera in the equatorial plane");
	controller.snap_to_north_pole(40.0);
	host->process_incoming_commands();
	host->camera().synchronize_spherical();
	suite.check(std::cos(host->camera().theta) > 0.999, "the polar snap places the camera on the positive polar axis");
	suite.check_near(host->camera().radius, 40.0, 1e-2, "the polar snap keeps the requested distance");
	controller.snap_to_isco();
	host->process_incoming_commands();
	host->camera().synchronize_spherical();
	suite.check(std::isfinite(host->camera().radius) && host->camera().radius > 0.0, "the ISCO snap produces a finite positive radius");
	controller.snap_to_photon_sphere();
	host->process_incoming_commands();
	host->camera().synchronize_spherical();
	suite.check(std::isfinite(host->camera().radius) && host->camera().radius > 0.0, "the photon sphere snap produces a finite positive radius");
	return suite.finish();
}
