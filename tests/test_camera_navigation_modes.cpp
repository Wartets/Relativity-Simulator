#include "support/test_harness.hpp"
#include "relativistic/orchestrator/command.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/ui/interactive_camera_controller.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>

using namespace Relativistic;
using Orchestrator::Command;
using Orchestrator::CommandResult;
using Testing::TestSuite;

namespace {

using Host = Orchestrator::SimulationOrchestrator<1024>;

constexpr std::array<const char*, 6> kModeNames{"Free Fly 6-DOF", "Orbit Center", "Spherical", "Rocket 6-DOF Thrust", "Surface Walk", "Planet Orbit"};

void select_mode(Host& host, uint32_t mode) {
	static_cast<void>(host.enqueue_command(Command::make_set_camera_mode(mode)));
	host.process_incoming_commands();
}

[[nodiscard]] double distance_between(const std::array<double, 3>& a, const std::array<double, 3>& b) noexcept {
	return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

[[nodiscard]] double dot(const std::array<double, 3>& a, const std::array<double, 3>& b) noexcept {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

[[nodiscard]] std::array<double, 3> cross(const std::array<double, 3>& a, const std::array<double, 3>& b) noexcept {
	return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

void check_mode_catalogue(TestSuite& suite) {
	suite.section("Navigation mode catalogue");
	suite.check(Observer::kCameraNavigationModeCount == kModeNames.size(), "the engine exposes exactly six navigation modes");
	suite.check(Observer::kPlanetOrbitNavigationMode == 5U, "Planet Orbit is navigation mode 5");
	auto host = std::make_unique<Host>();
	for (uint32_t mode = 0; mode < kModeNames.size(); ++mode) {
		select_mode(*host, mode);
		suite.check(host->parameters().camera_mode == mode, std::string("mode ") + std::to_string(mode) + " (" + kModeNames[mode] + ") can be selected through the command queue");
	}
	std::array<bool, 6> visited{};
	uint32_t mode = 0;
	for (uint32_t step = 0; step < Observer::kCameraNavigationModeCount; ++step) {
		visited[mode] = true;
		mode = (mode + 1) % Observer::kCameraNavigationModeCount;
	}
	suite.check(std::all_of(visited.begin(), visited.end(), [](bool value) noexcept { return value; }) && mode == 0U, "cycling with the interface formula visits every mode and wraps to the first");
}

void check_translation_and_rotation(TestSuite& suite) {
	suite.section("Free-fly translation and rotation commands");
	auto host = std::make_unique<Host>();
	CommandResult result{};
	const auto origin = host->camera().position;

	host->apply_command(Command::make_camera_move(1.0, 2.0, 3.0), result);
	const auto& camera = host->camera();
	suite.check(result.success, "finite translation is accepted");
	suite.check_near(camera.position[0], origin[0] + 1.0, 1e-12, "translation updates x");
	suite.check_near(camera.position[1], origin[1] + 2.0, 1e-12, "translation updates y");
	suite.check_near(camera.position[2], origin[2] + 3.0, 1e-12, "translation updates z");
	const double radius = std::sqrt(dot(camera.position, camera.position));
	suite.check_near(camera.radius, radius, 1e-9, "spherical radius is synchronised after translation");
	suite.check_near(camera.theta, std::acos(camera.position[2] / radius), 1e-9, "spherical polar angle is synchronised after translation");
	suite.check_near(camera.phi, std::atan2(camera.position[1], camera.position[0]), 1e-9, "spherical azimuth is synchronised after translation");

	const auto before_invalid = camera.position;
	host->apply_command(Command::make_camera_move(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0), result);
	suite.check(!result.success && distance_between(camera.position, before_invalid) == 0.0, "non-finite translation is rejected and leaves the camera untouched");

	host->apply_command(Command::make_camera_move(1.0e15, -1.0e15, 0.0), result);
	suite.check_near(camera.position[0], 1.0e12, 1e-3, "positive translation is clamped to the position limit");
	suite.check_near(camera.position[1], -1.0e12, 1e-3, "negative translation is clamped to the position limit");

	host->apply_command(Command::make_camera_rotate(500.0, 400.0, 270.0), result);
	suite.check_near(camera.pitch, 89.0, 1e-12, "pitch is clamped to 89 degrees");
	suite.check(std::abs(camera.yaw) <= 180.0 + 1e-9 && std::abs(camera.roll) <= 180.0 + 1e-9, "yaw and roll are wrapped to a half turn");
	host->apply_command(Command::make_camera_rotate(-500.0, 0.0, 0.0), result);
	suite.check_near(camera.pitch, -89.0, 1e-12, "negative pitch is clamped to -89 degrees");
	const double pitch_before = camera.pitch;
	host->apply_command(Command::make_camera_rotate(std::numeric_limits<double>::infinity(), 0.0, 0.0), result);
	suite.check(!result.success && camera.pitch == pitch_before, "non-finite rotation is rejected");

	host->apply_command(Command::make_camera_set_fov(500.0), result);
	suite.check_near(camera.fov_deg, 175.0, 1e-12, "field of view is clamped to 175 degrees");
	host->apply_command(Command::make_camera_set_fov(-3.0), result);
	suite.check_near(camera.fov_deg, 5.0, 1e-12, "field of view is clamped to 5 degrees");
	host->apply_command(Command::make_camera_set_speed(-1.0), result);
	suite.check_near(camera.speed, 0.001, 1e-12, "navigation speed keeps a positive minimum");
}

void check_orientation_basis(TestSuite& suite) {
	suite.section("Orientation basis");
	constexpr std::array<std::array<double, 3>, 6> samples{{{0.0, 0.0, 0.0}, {30.0, 45.0, 0.0}, {-60.0, 200.0, 15.0}, {89.0, -120.0, 90.0}, {10.0, 10.0, -45.0}, {-89.0, 0.0, 180.0}}};
	bool orthonormal = true;
	double handedness_reference = 0.0;
	bool consistent_handedness = true;
	for (const auto& angles : samples) {
		Orchestrator::CameraState state;
		state.pitch = angles[0];
		state.yaw = angles[1];
		state.roll = angles[2];
		const auto basis = state.orientation_basis();
		const bool unit = std::abs(dot(basis.forward, basis.forward) - 1.0) < 1e-12 && std::abs(dot(basis.right, basis.right) - 1.0) < 1e-12 && std::abs(dot(basis.up, basis.up) - 1.0) < 1e-12;
		const bool perpendicular = std::abs(dot(basis.forward, basis.right)) < 1e-12 && std::abs(dot(basis.forward, basis.up)) < 1e-12 && std::abs(dot(basis.right, basis.up)) < 1e-12;
		orthonormal = orthonormal && unit && perpendicular;
		const double handedness = dot(cross(basis.forward, basis.right), basis.up);
		if (handedness_reference == 0.0) {
			handedness_reference = handedness;
		}
		consistent_handedness = consistent_handedness && std::abs(std::abs(handedness) - 1.0) < 1e-12 && handedness * handedness_reference > 0.0;
	}
	suite.check(orthonormal, "forward, right and up form an orthonormal frame for every sampled attitude");
	suite.check(consistent_handedness, "the frame keeps a constant handedness for every sampled attitude");
}

void check_rocket_and_inertial_modes(TestSuite& suite) {
	suite.section("Rocket mode integration and inertial modes");
	{
		auto host = std::make_unique<Host>();
		host->camera().velocity = {0.5, 0.0, 0.0};
		const auto origin = host->camera().position;
		select_mode(*host, 3U);
		host->scheduler().resume();
		for (int step = 0; step < 10; ++step) {
			host->advance_simulation(0.1);
		}
		suite.check_near(host->camera().position[0] - origin[0], 0.5, 1e-9, "rocket mode integrates velocity over simulated time");
		suite.check_near(host->camera().position[1], origin[1], 1e-12, "rocket mode leaves the transverse position untouched");
		suite.check_near(host->camera().radius, std::sqrt(dot(host->camera().position, host->camera().position)), 1e-9, "rocket mode keeps spherical coordinates synchronised");
	}
	{
		auto host = std::make_unique<Host>();
		host->camera().velocity = {0.5, 0.0, 0.0};
		const auto origin = host->camera().position;
		select_mode(*host, 3U);
		host->scheduler().pause();
		host->advance_simulation(1.0);
		suite.check(distance_between(host->camera().position, origin) == 0.0, "rocket mode does not move while the simulation clock is paused");
	}
	{
		auto host = std::make_unique<Host>();
		host->camera().velocity = {2.0e13, 0.0, 0.0};
		select_mode(*host, 3U);
		host->scheduler().resume();
		host->advance_simulation(1.0);
		suite.check(std::abs(host->camera().position[0]) <= 1.0e12 + 1e-3, "rocket mode clamps runaway positions to the position limit");
	}
	{
		auto host = std::make_unique<Host>();
		host->camera().velocity = {std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0};
		select_mode(*host, 3U);
		host->scheduler().resume();
		host->advance_simulation(1.0);
		suite.check(host->camera().velocity[0] == 0.0 && host->camera().velocity[1] == 0.0 && host->camera().velocity[2] == 0.0, "rocket mode zeroes a non-finite velocity");
	}
	for (const uint32_t mode : {0U, 1U, 2U, 4U, 5U}) {
		auto host = std::make_unique<Host>();
		host->camera().velocity = {0.5, 0.5, 0.5};
		const auto origin = host->camera().position;
		select_mode(*host, mode);
		host->scheduler().resume();
		host->advance_simulation(1.0);
		suite.check(distance_between(host->camera().position, origin) == 0.0, std::string(kModeNames[mode]) + ": residual velocity does not drift the camera in non-thrust modes");
	}
}

void check_mode_switch_preserves_pose(TestSuite& suite) {
	suite.section("Pose preservation across mode switches");
	auto host = std::make_unique<Host>();
	CommandResult result{};
	host->apply_command(Command::make_camera_move(4.0, -3.0, 2.0), result);
	const auto pose = host->camera().position;
	for (uint32_t mode = 0; mode < kModeNames.size(); ++mode) {
		select_mode(*host, mode);
		suite.check(distance_between(host->camera().position, pose) == 0.0, std::string("switching to ") + kModeNames[mode] + " does not teleport the camera");
	}
	host->apply_command(Command::make_camera_reset(), result);
	const Orchestrator::CameraState defaults;
	suite.check(distance_between(host->camera().position, defaults.position) < 1e-12, "camera reset restores the home position");
	suite.check(host->camera().velocity[0] == 0.0 && host->camera().velocity[1] == 0.0 && host->camera().velocity[2] == 0.0, "camera reset clears the velocity");
}

void check_controller_helpers(TestSuite& suite) {
	suite.section("Interactive controller helpers");
	auto host = std::make_unique<Host>();
	UI::InteractiveCameraController controller(*host);

	controller.snap_to_equatorial_front(50.0);
	host->process_incoming_commands();
	host->camera().synchronize_spherical();
	suite.check_near(host->camera().radius, 50.0, 1e-6, "equatorial snap places the camera at the requested radius");
	suite.check_near(std::cos(host->camera().theta), 0.0, 1e-6, "equatorial snap places the camera in the equatorial plane");

	controller.snap_to_north_pole(50.0);
	host->process_incoming_commands();
	host->camera().synchronize_spherical();
	suite.check(std::cos(host->camera().theta) > 0.999, "polar snap places the camera on the positive polar axis");
	suite.check_near(host->camera().radius, 50.0, 1e-2, "polar snap keeps the requested distance");

	controller.snap_to_isco();
	host->process_incoming_commands();
	host->camera().synchronize_spherical();
	suite.check(std::isfinite(host->camera().radius) && host->camera().radius > 0.0, "ISCO snap produces a finite positive radius");

	host->camera().position = {30.0, 0.0, 10.0};
	host->camera().synchronize_spherical();
	const std::array<double, 3> target{-5.0, 12.0, 3.0};
	controller.look_at_target(target);
	host->process_incoming_commands();
	std::array<double, 3> toward{target[0] - host->camera().position[0], target[1] - host->camera().position[1], target[2] - host->camera().position[2]};
	const double length = std::sqrt(dot(toward, toward));
	toward = {toward[0] / length, toward[1] / length, toward[2] / length};
	suite.check(dot(host->camera().orientation_basis().forward, toward) > 1.0 - 1e-6, "look-at aligns the forward axis with the target");

	auto walk_host = std::make_unique<Host>();
	const uint32_t body_id = walk_host->nbody_system().add_body(Dynamics::PostNewtonianBody(7U, 1.0e-3, 2.0, {40.0, 0.0, 0.0}, {0.0, 0.0, 0.0}));
	UI::InteractiveCameraController walker(*walk_host);
	walker.enter_surface_walk(body_id);
	walk_host->process_incoming_commands();
	suite.check(walk_host->parameters().camera_mode == 4U, "entering surface walk switches to the surface walk mode");
	const std::array<double, 3> body_center{40.0, 0.0, 0.0};
	suite.check(distance_between(walk_host->camera().position, body_center) >= 2.0 * (1.0 - 1e-6), "the surface walker is never placed inside the body");
	walker.leave_surface_walk();
	walk_host->process_incoming_commands();
	suite.check(walk_host->parameters().camera_mode == 0U, "leaving surface walk returns to free fly");
}

}

int main() {
	TestSuite suite(
		"camera_navigation_modes",
		"Verifies every camera navigation mode of the engine: mode catalogue and command-queue selection, free-fly translation and rotation limits, orientation basis, rocket-mode integration under pause, clamp and non-finite conditions, pose preservation across mode switches, and the interactive controller snap, look-at and surface-walk helpers."
	);
	check_mode_catalogue(suite);
	check_translation_and_rotation(suite);
	check_orientation_basis(suite);
	check_rocket_and_inertial_modes(suite);
	check_mode_switch_preserves_pose(suite);
	check_controller_helpers(suite);
	return suite.finish();
}
