#include "support/test_harness.hpp"
#include "relativistic/io/scenario/scenario_locator.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include <cmath>
#include <string>

using Relativistic::Testing::TestSuite;
namespace Orchestrator = Relativistic::Orchestrator;
using Relativistic::IO::ScenarioLocator;

namespace {

[[nodiscard]] bool nearly_equal(double lhs, double rhs) noexcept {
	return std::abs(lhs - rhs) <= 1e-9;
}

void check_startup_pose(TestSuite& suite, const Orchestrator::CameraState& camera, const std::string& context) {
	suite.check(nearly_equal(camera.position[0], -5.0) && nearly_equal(camera.position[1], 37.0) && nearly_equal(camera.position[2], -7.0), context + ": the camera is at (-5, 37, -7)");
	suite.check(nearly_equal(camera.pitch, 0.0) && nearly_equal(camera.yaw, 130.0) && nearly_equal(camera.roll, 0.0), context + ": the camera attitude is pitch 0, yaw 130, roll 0");
}

}

int main() {
	TestSuite suite(
		"default_startup_scenario",
		"Verifies the startup path: the initial camera pose, the resolution of the built-in startup scenario and the fallback rules for empty, missing and valid configurations, the load through the command queue, and the preservation of the scenario and of its camera pose across a view reset, a full reset and a failed load."
	);
	const std::string expected_name = "Schwarzschild Black Hole & Accretion Disk";

	suite.section("Initial state");
	Orchestrator::SimulationOrchestrator<1024> orchestrator;
	check_startup_pose(suite, orchestrator.camera(), "initial camera");
	suite.check_near(orchestrator.camera().radius, std::sqrt(1443.0), 1e-9, "the initial spherical radius is synchronized with the position");
	suite.check(orchestrator.active_scenario_path().empty(), "no scenario path exists before the startup load");

	suite.section("Scenario resolution");
	const auto built_in = ScenarioLocator::resolve_file(ScenarioLocator::kBuiltInStartupScenario);
	suite.check(built_in.has_value(), "the built-in startup scenario is reachable");
	if (!built_in.has_value()) {
		return suite.finish();
	}
	suite.check(ScenarioLocator::portable_path(built_in->generic_string()) == ScenarioLocator::kBuiltInStartupScenario, "the portable path of the built-in scenario is the canonical one");
	const auto empty = ScenarioLocator::resolve_startup_scenario("");
	suite.check(empty.has_value() && empty->used_fallback, "an empty configuration falls back to the built-in scenario");
	const auto missing = ScenarioLocator::resolve_startup_scenario("scenarios/missing_scenario.yaml");
	suite.check(missing.has_value() && missing->used_fallback, "a missing configuration falls back to the built-in scenario");
	const auto valid = ScenarioLocator::resolve_startup_scenario(ScenarioLocator::kBuiltInStartupScenario);
	suite.check(valid.has_value() && !valid->used_fallback, "a valid configuration is kept as configured");
	if (!valid.has_value()) {
		return suite.finish();
	}

	suite.section("Startup load");
	suite.check(orchestrator.enqueue_command(Orchestrator::Command::make_load_scenario(valid->path)), "the load command is accepted by the queue");
	orchestrator.process_incoming_commands();
	Orchestrator::CommandResult load_result{};
	suite.check(orchestrator.poll_result(load_result) && load_result.success, "the startup scenario loads successfully");
	suite.check(orchestrator.active_scenario_name() == expected_name, "the active scenario name matches the startup scenario");
	suite.check(ScenarioLocator::same_file(orchestrator.active_scenario_path(), ScenarioLocator::kBuiltInStartupScenario), "the active scenario path matches the startup scenario");
	check_startup_pose(suite, orchestrator.camera(), "camera after the startup load");

	suite.section("Resets");
	orchestrator.camera().position = {12.0, -3.0, 4.0};
	orchestrator.camera().yaw = 15.0;
	orchestrator.camera().pitch = 20.0;
	suite.check(orchestrator.enqueue_command(Orchestrator::Command::make_camera_reset()), "the camera reset command is accepted");
	orchestrator.process_incoming_commands();
	check_startup_pose(suite, orchestrator.camera(), "camera after the view reset");

	orchestrator.parameters().spin = 0.5;
	orchestrator.camera().yaw = 15.0;
	suite.check(orchestrator.enqueue_command(Orchestrator::Command::make_reset()), "the reset command is accepted");
	orchestrator.process_incoming_commands();
	suite.check(nearly_equal(orchestrator.parameters().spin, 0.0), "the reset restores the scenario parameters");
	suite.check(orchestrator.active_scenario_name() == expected_name, "the reset keeps the active scenario");
	suite.check(orchestrator.scheduler().is_paused(), "the reset pauses the scheduler");
	check_startup_pose(suite, orchestrator.camera(), "camera after the full reset");

	suite.section("Failed load");
	Orchestrator::CommandResult missing_result{};
	orchestrator.load_scenario_file("scenarios/missing_scenario.yaml", missing_result);
	suite.check(!missing_result.success, "loading a missing scenario fails");
	suite.check(orchestrator.active_scenario_name() == expected_name, "a failed load keeps the active scenario");
	return suite.finish();
}
