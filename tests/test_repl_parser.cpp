#include "support/test_harness.hpp"
#include "relativistic/orchestrator/command.hpp"
#include <array>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>

using namespace Relativistic::Orchestrator;
using Relativistic::Testing::TestSuite;

namespace {

struct Parsed {
	std::optional<Command> command{};
	CommandResult result{};
	[[nodiscard]] std::string_view message() const noexcept { return std::string_view(result.message); }
};

[[nodiscard]] Parsed parse(std::string_view line) {
	Parsed parsed;
	parsed.command = CommandParser::parse(line, &parsed.result);
	return parsed;
}

[[nodiscard]] bool is_type(const Parsed& parsed, CommandType type) noexcept {
	return parsed.command.has_value() && parsed.command->type == type;
}

void check_flow_commands(TestSuite& suite) {
	suite.section("Simulation flow commands");
	suite.check(is_type(parse("pause"), CommandType::Pause) && parse("pause").result.success, "pause is recognized");
	suite.check(is_type(parse("  PAUSE  "), CommandType::Pause), "commands are case-insensitive and trimmed");
	suite.check(is_type(parse("resume"), CommandType::Resume) && is_type(parse("RESUME"), CommandType::Resume), "resume is recognized in any case");
	suite.check(is_type(parse("reset"), CommandType::Reset), "reset is recognized");
	suite.check(is_type(parse("status"), CommandType::Status), "status is recognized");
	for (const char* word : {"shutdown", "quit", "exit"}) {
		suite.check(is_type(parse(word), CommandType::Shutdown), std::string(word) + " maps to the shutdown command");
	}

	const auto step_default = parse("step");
	suite.check(is_type(step_default, CommandType::Step) && step_default.command->step_count == 1, "step without argument advances one tick");
	const auto step_many = parse("step 100");
	suite.check(is_type(step_many, CommandType::Step) && step_many.command->step_count == 100, "step accepts an explicit tick count");
	suite.check(!parse("step -5").command.has_value() && !parse("step -5").result.success, "a negative step count is rejected");
	suite.check(!parse("step abc").command.has_value(), "a non-numeric step count is rejected");
	suite.check(!parse("step 0").command.has_value(), "a zero step count is rejected");

	const auto warp = parse("warp 2.5");
	suite.check(is_type(warp, CommandType::Warp), "warp is recognized");
	suite.check_near(warp.command ? warp.command->numeric_value : 0.0, 2.5, 1e-12, "warp stores its factor");
	suite.check(!parse("warp -1.0").command.has_value() && !parse("warp 0").command.has_value(), "non-positive warp factors are rejected");
	suite.check(!parse("warp").command.has_value() && parse("warp").message() == "Missing warp factor argument", "warp without argument reports the missing argument");

	const auto tick = parse("tickrate 120");
	suite.check(is_type(tick, CommandType::SetTickRate), "tickrate is recognized");
	suite.check_near(tick.command ? tick.command->numeric_value : 0.0, 120.0, 1e-12, "tickrate stores its frequency");
	suite.check(!parse("tickrate 5").command.has_value() && !parse("tickrate 1500").command.has_value(), "tick rates outside 10 to 1000 Hz are rejected");
	suite.check(!parse("tickrate").command.has_value(), "tickrate without argument is rejected");
}

void check_set_commands(TestSuite& suite) {
	suite.section("Parameter commands");
	struct Entry {
		const char* key;
		ParameterType type;
	};
	constexpr std::array<Entry, 33> entries{{
		{"mass", ParameterType::Mass}, {"spin", ParameterType::Spin}, {"charge", ParameterType::Charge},
		{"lambda", ParameterType::CosmologicalLambda}, {"throat", ParameterType::WormholeThroat},
		{"warp_velocity", ParameterType::WarpVelocity}, {"warp_vel", ParameterType::WarpVelocity},
		{"projection", ParameterType::ProjectionMode}, {"proj", ParameterType::ProjectionMode},
		{"timeflow", ParameterType::TimeFlowMode}, {"speed", ParameterType::CameraSpeed},
		{"fov", ParameterType::CameraFov}, {"exposure", ParameterType::CameraExposure},
		{"tonemapper", ParameterType::TonemappingMode}, {"rtol", ParameterType::IntegrationRtol},
		{"atol", ParameterType::IntegrationAtol}, {"min_step", ParameterType::IntegrationMinStep},
		{"max_step", ParameterType::IntegrationMaxStep}, {"step_factor", ParameterType::IntegrationStepFactor},
		{"scale", ParameterType::ResolutionScale}, {"ray_steps", ParameterType::MaxRaySteps},
		{"perf", ParameterType::PerformancePreset}, {"camera_mode", ParameterType::CameraMode},
		{"sky_star_density", ParameterType::SkyStarDensity}, {"sky_nebula", ParameterType::SkyNebulaIntensity},
		{"tiling", ParameterType::WorkDistributionMode}, {"space_skip", ParameterType::SpaceSkippingEnabled},
		{"pole_precision", ParameterType::PoleGuardPrecisionScale}, {"light_mode", ParameterType::LightSourceMode},
		{"collision", ParameterType::CameraCollisionEnabled}, {"warp_radius", ParameterType::WarpBubbleRadius},
		{"teves_a0", ParameterType::TevesCriticalAcceleration}, {"tickrate", ParameterType::TickRate}
	}};
	size_t mismatches = 0;
	for (const auto& entry : entries) {
		const auto parsed = parse(std::string("set ") + entry.key + " 60");
		if (!is_type(parsed, CommandType::SetParam) || parsed.command->param_type != entry.type) {
			++mismatches;
		}
	}
	suite.check(mismatches == 0, "every documented parameter key resolves to its parameter type", "mismatches=" + std::to_string(mismatches) + " keys=" + std::to_string(entries.size()));

	const auto mass = parse("set mass 1.989e30");
	suite.check_relative(mass.command ? mass.command->numeric_value : 0.0, 1.989e30, 1e-12, "scientific notation values are parsed");
	const auto charge = parse("set charge -0.5");
	suite.check_near(charge.command ? charge.command->numeric_value : 0.0, -0.5, 1e-12, "negative values are parsed");
	suite.check(is_type(parse("SET MASS 2"), CommandType::SetParam), "the set command and its keys are case-insensitive");

	const auto custom = parse("set my_parameter 42.125");
	suite.check(is_type(custom, CommandType::SetParam) && custom.command->param_type == ParameterType::Custom, "an unknown key becomes a custom parameter");
	suite.check(custom.command && std::strcmp(custom.command->custom_param_name, "my_parameter") == 0, "the custom parameter keeps its name");
	suite.check_near(custom.command ? custom.command->numeric_value : 0.0, 42.125, 1e-12, "the custom parameter keeps its value");
	const auto long_name = parse("set " + std::string(80, 'a') + " 1");
	suite.check(long_name.command && std::strlen(long_name.command->custom_param_name) == 63, "custom parameter names are truncated to 63 characters");

	suite.check(!parse("set").command.has_value() && parse("set").message() == "Missing parameter name in set command", "set without arguments reports the missing name");
	suite.check(!parse("set mass").command.has_value() && parse("set mass").message() == "Missing value in set command", "set without value reports the missing value");
	suite.check(!parse("set mass not_a_number").command.has_value(), "a non-numeric value is rejected");
}

void check_camera_commands(TestSuite& suite) {
	suite.section("Camera commands");
	suite.check(is_type(parse("camera reset"), CommandType::CameraReset), "camera reset is recognized");
	const auto fov = parse("camera fov 65");
	suite.check(is_type(fov, CommandType::CameraSetFov) && fov.command->numeric_value == 65.0, "camera fov stores the angle");
	suite.check(!parse("camera fov 4").command.has_value() && !parse("camera fov 176").command.has_value(), "fields of view outside 5 to 175 degrees are rejected");
	suite.check(is_type(parse("camera speed 25"), CommandType::CameraSetSpeed) && !parse("camera speed 0").command.has_value(), "camera speed requires a positive value");
	const auto move = parse("camera move 1 -2 3.5");
	suite.check(is_type(move, CommandType::CameraMove) && move.command->vec_values[0] == 1.0 && move.command->vec_values[1] == -2.0 && move.command->vec_values[2] == 3.5, "camera move stores the three components");
	suite.check(!parse("camera move 1 2").command.has_value(), "camera move requires three components");
	const auto rotate = parse("camera rotate -15 45");
	suite.check(is_type(rotate, CommandType::CameraRotate) && rotate.command->vec_values[2] == 0.0, "camera rotate defaults the roll to zero");
	const auto rotate_roll = parse("camera rotate 1 2 3");
	suite.check(is_type(rotate_roll, CommandType::CameraRotate) && rotate_roll.command->vec_values[2] == 3.0, "camera rotate accepts an optional roll");
	suite.check(!parse("camera jump").command.has_value() && parse("camera jump").message() == "Unrecognized command", "an unknown camera sub-command is rejected");
}

void check_text_commands(TestSuite& suite) {
	suite.section("Text commands and error handling");
	const auto metric = parse("metric Kerr");
	suite.check(is_type(metric, CommandType::SetMetric) && std::string_view(metric.command->text_payload) == "Kerr", "metric stores the metric name");
	suite.check(!parse("metric").command.has_value(), "metric without name is rejected");
	const auto integrator = parse("integrator Vernier9");
	suite.check(is_type(integrator, CommandType::SetIntegrator) && std::string_view(integrator.command->text_payload) == "Vernier9", "integrator stores the scheme name");
	suite.check(!parse("integrator").command.has_value(), "integrator without name is rejected");
	const auto load = parse("load scenarios/my scenario.yaml");
	suite.check(is_type(load, CommandType::LoadScenario) && std::string_view(load.command->text_payload) == "scenarios/my scenario.yaml", "load keeps the complete path, spaces included");
	suite.check(is_type(parse("save scenarios/out.yaml"), CommandType::SaveScenario) && !parse("save").command.has_value(), "save requires a path");
	const auto export_default = parse("export");
	suite.check(is_type(export_default, CommandType::TriggerExport) && std::string_view(export_default.command->text_payload) == "all", "export defaults to every format");
	suite.check(std::string_view(parse("export fits").command->text_payload) == "fits", "export stores the requested format");
	suite.check(!parse("   ").command.has_value() && parse("   ").message() == "Empty command line", "a blank line is reported as empty");
	suite.check(!parse("frobnicate").command.has_value() && parse("frobnicate").message() == "Unrecognized command", "an unknown command is reported");
	suite.check(!parse("frobnicate").result.success, "a rejected command reports failure");
}

}

int main() {
	TestSuite suite(
		"repl_parser",
		"Verifies the terminal command parser: flow commands, argument validation, every documented parameter key, custom parameters, camera commands, text commands, case-insensitivity and error messages."
	);
	check_flow_commands(suite);
	check_set_commands(suite);
	check_camera_commands(suite);
	check_text_commands(suite);
	return suite.finish();
}
