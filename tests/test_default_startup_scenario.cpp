#include "relativistic/io/scenario_locator.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include <cmath>
#include <cstdio>
#include <string>

namespace {

int failure_count = 0;

void expect(bool condition, const std::string& description) {
	if (!condition) {
		std::fprintf(stderr, "[FAIL] %s\n", description.c_str());
		++failure_count;
	}
}

bool nearly_equal(double lhs, double rhs) {
	return std::abs(lhs - rhs) <= 1e-9;
}

void expect_startup_pose(const Relativistic::Orchestrator::CameraState& camera, const std::string& context) {
	expect(nearly_equal(camera.position[0], -5.0), context + ": position x");
	expect(nearly_equal(camera.position[1], 37.0), context + ": position y");
	expect(nearly_equal(camera.position[2], -7.0), context + ": position z");
	expect(nearly_equal(camera.pitch, 0.0), context + ": pitch");
	expect(nearly_equal(camera.yaw, 130.0), context + ": yaw");
	expect(nearly_equal(camera.roll, 0.0), context + ": roll");
}

}

int main() {
	namespace Orchestrator = Relativistic::Orchestrator;
	using Relativistic::IO::ScenarioLocator;

	const std::string expected_scenario_name = "Schwarzschild Black Hole & Accretion Disk";

	Orchestrator::SimulationOrchestrator<1024> orchestrator;
	expect_startup_pose(orchestrator.camera(), "initial camera");
	expect(nearly_equal(orchestrator.camera().radius, std::sqrt(1443.0)), "initial camera radius is synchronized");
	expect(orchestrator.active_scenario_path().empty(), "no scenario path before the startup load");

	const auto built_in = ScenarioLocator::resolve_file(ScenarioLocator::kBuiltInStartupScenario);
	expect(built_in.has_value(), "built-in startup scenario is reachable");
	if (!built_in.has_value()) {
		return 1;
	}
	expect(ScenarioLocator::portable_path(built_in->generic_string()) == ScenarioLocator::kBuiltInStartupScenario, "portable path of the built-in scenario");

	const auto empty_configuration = ScenarioLocator::resolve_startup_scenario("");
	expect(empty_configuration.has_value() && empty_configuration->used_fallback, "empty configuration falls back to the built-in scenario");

	const auto missing_configuration = ScenarioLocator::resolve_startup_scenario("scenarios/missing_scenario.yaml");
	expect(missing_configuration.has_value() && missing_configuration->used_fallback, "missing configuration falls back to the built-in scenario");

	const auto valid_configuration = ScenarioLocator::resolve_startup_scenario(ScenarioLocator::kBuiltInStartupScenario);
	expect(valid_configuration.has_value() && !valid_configuration->used_fallback, "valid configuration is kept as configured");
	if (!valid_configuration.has_value()) {
		return 1;
	}

	expect(orchestrator.enqueue_command(Orchestrator::Command::make_load_scenario(valid_configuration->path)), "load command is accepted");
	orchestrator.process_incoming_commands();
	Orchestrator::CommandResult load_result{};
	expect(orchestrator.poll_result(load_result) && load_result.success, "startup scenario loads successfully");
	expect(orchestrator.active_scenario_name() == expected_scenario_name, "active scenario name after the startup load");
	expect(ScenarioLocator::same_file(orchestrator.active_scenario_path(), ScenarioLocator::kBuiltInStartupScenario), "active scenario path after the startup load");
	expect_startup_pose(orchestrator.camera(), "camera after the startup load");

	orchestrator.camera().position = {12.0, -3.0, 4.0};
	orchestrator.camera().yaw = 15.0;
	orchestrator.camera().pitch = 20.0;
	expect(orchestrator.enqueue_command(Orchestrator::Command::make_camera_reset()), "camera reset command is accepted");
	orchestrator.process_incoming_commands();
	expect_startup_pose(orchestrator.camera(), "camera after the view reset");

	orchestrator.parameters().spin = 0.5;
	orchestrator.camera().yaw = 15.0;
	expect(orchestrator.enqueue_command(Orchestrator::Command::make_reset()), "reset command is accepted");
	orchestrator.process_incoming_commands();
	expect(nearly_equal(orchestrator.parameters().spin, 0.0), "reset restores the scenario parameters");
	expect(orchestrator.active_scenario_name() == expected_scenario_name, "reset keeps the active scenario");
	expect(orchestrator.scheduler().is_paused(), "reset pauses the scheduler");
	expect_startup_pose(orchestrator.camera(), "camera after the full reset");

	Orchestrator::CommandResult missing_result{};
	orchestrator.load_scenario_file("scenarios/missing_scenario.yaml", missing_result);
	expect(!missing_result.success, "loading a missing scenario fails");
	expect(orchestrator.active_scenario_name() == expected_scenario_name, "a failed load keeps the active scenario");

	return failure_count == 0 ? 0 : 1;
}
