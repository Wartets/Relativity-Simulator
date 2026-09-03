#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <optional>
#include <algorithm>

namespace Relativistic::Orchestrator {

enum class CommandType : uint32_t {
	None = 0,
	Pause = 1,
	Resume = 2,
	Step = 3,
	Warp = 4,
	Reset = 5,
	SetParam = 6,
	SetTickRate = 7,
	Status = 8,
	Shutdown = 9,
	CameraMove = 10,
	CameraRotate = 11,
	CameraSetFov = 12,
	CameraSetSpeed = 13,
	CameraReset = 14,
	SetMetric = 15,
	LoadScenario = 16,
	SaveScenario = 17,
	SetIntegrator = 18,
	TriggerExport = 19,
	SetResolutionScale = 20,
	SetRenderSteps = 21,
	SetPerformancePreset = 22,
	SetCameraMode = 23,
	SetVisualOverlay = 24,
	CaptureScreenshot = 25
};

enum class ParameterType : uint32_t {
	Unknown = 0,
	Mass = 1,
	Spin = 2,
	Charge = 3,
	CosmologicalLambda = 4,
	WormholeThroat = 5,
	WarpVelocity = 6,
	TickRate = 7,
	ProjectionMode = 8,
	TimeFlowMode = 9,
	CameraSpeed = 10,
	CameraFov = 11,
	CameraExposure = 12,
	TonemappingMode = 13,
	IntegrationRtol = 14,
	IntegrationAtol = 15,
	IntegrationMinStep = 16,
	IntegrationMaxStep = 17,
	ResolutionScale = 18,
	MaxRaySteps = 19,
	PerformancePreset = 20,
	CameraMode = 21,
	VisualOverlays = 22,
	Custom = 23,
	SkyStarDensity = 24,
	SkyStarBrightness = 25,
	SkyNebulaIntensity = 26,
	SkyGridOpacity = 27,
	SkyRotation = 28,
	SkyHueShift = 29,
	SkySaturation = 30,
	SkyBackgroundR = 31,
	SkyBackgroundG = 32,
	SkyBackgroundB = 33,
	WorkDistributionMode = 34,
	ForceTextureReallocation = 35
};

struct Command {
	CommandType type{CommandType::None};
	ParameterType param_type{ParameterType::Unknown};
	uint64_t step_count{0};
	double numeric_value{0.0};
	double vec_values[4]{0.0, 0.0, 0.0, 0.0};
	char custom_param_name[64]{};
	char text_payload[256]{};
	uint64_t target_tick{0};

	[[nodiscard]] static constexpr Command make_pause() noexcept {
		Command cmd{};
		cmd.type = CommandType::Pause;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_resume() noexcept {
		Command cmd{};
		cmd.type = CommandType::Resume;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_step(uint64_t n = 1) noexcept {
		Command cmd{};
		cmd.type = CommandType::Step;
		cmd.step_count = n;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_warp(double factor) noexcept {
		Command cmd{};
		cmd.type = CommandType::Warp;
		cmd.numeric_value = factor;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_reset() noexcept {
		Command cmd{};
		cmd.type = CommandType::Reset;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_set_param(ParameterType param, double val) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetParam;
		cmd.param_type = param;
		cmd.numeric_value = val;
		return cmd;
	}

	[[nodiscard]] static Command make_set_custom_param(std::string_view name, double val) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetParam;
		cmd.param_type = ParameterType::Custom;
		cmd.numeric_value = val;
		const size_t len = std::min(name.size(), sizeof(cmd.custom_param_name) - 1);
		std::memcpy(cmd.custom_param_name, name.data(), len);
		cmd.custom_param_name[len] = '\0';
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_set_tickrate(double rate_hz) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetTickRate;
		cmd.numeric_value = rate_hz;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_status() noexcept {
		Command cmd{};
		cmd.type = CommandType::Status;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_shutdown() noexcept {
		Command cmd{};
		cmd.type = CommandType::Shutdown;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_camera_move(double dx, double dy, double dz) noexcept {
		Command cmd{};
		cmd.type = CommandType::CameraMove;
		cmd.vec_values[0] = dx;
		cmd.vec_values[1] = dy;
		cmd.vec_values[2] = dz;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_camera_rotate(double pitch, double yaw, double roll = 0.0) noexcept {
		Command cmd{};
		cmd.type = CommandType::CameraRotate;
		cmd.vec_values[0] = pitch;
		cmd.vec_values[1] = yaw;
		cmd.vec_values[2] = roll;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_camera_set_fov(double fov_deg) noexcept {
		Command cmd{};
		cmd.type = CommandType::CameraSetFov;
		cmd.numeric_value = fov_deg;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_camera_set_speed(double speed) noexcept {
		Command cmd{};
		cmd.type = CommandType::CameraSetSpeed;
		cmd.numeric_value = speed;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_camera_reset() noexcept {
		Command cmd{};
		cmd.type = CommandType::CameraReset;
		return cmd;
	}

	[[nodiscard]] static Command make_set_metric(std::string_view metric_name) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetMetric;
		const size_t len = std::min(metric_name.size(), sizeof(cmd.text_payload) - 1);
		std::memcpy(cmd.text_payload, metric_name.data(), len);
		cmd.text_payload[len] = '\0';
		return cmd;
	}

	[[nodiscard]] static Command make_load_scenario(std::string_view scenario_path) noexcept {
		Command cmd{};
		cmd.type = CommandType::LoadScenario;
		const size_t len = std::min(scenario_path.size(), sizeof(cmd.text_payload) - 1);
		std::memcpy(cmd.text_payload, scenario_path.data(), len);
		cmd.text_payload[len] = '\0';
		return cmd;
	}

	[[nodiscard]] static Command make_save_scenario(std::string_view scenario_path) noexcept {
		Command cmd{};
		cmd.type = CommandType::SaveScenario;
		const size_t len = std::min(scenario_path.size(), sizeof(cmd.text_payload) - 1);
		std::memcpy(cmd.text_payload, scenario_path.data(), len);
		cmd.text_payload[len] = '\0';
		return cmd;
	}

	[[nodiscard]] static Command make_set_integrator(std::string_view scheme_name) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetIntegrator;
		const size_t len = std::min(scheme_name.size(), sizeof(cmd.text_payload) - 1);
		std::memcpy(cmd.text_payload, scheme_name.data(), len);
		cmd.text_payload[len] = '\0';
		return cmd;
	}

	[[nodiscard]] static Command make_trigger_export(std::string_view format_name) noexcept {
		Command cmd{};
		cmd.type = CommandType::TriggerExport;
		const size_t len = std::min(format_name.size(), sizeof(cmd.text_payload) - 1);
		std::memcpy(cmd.text_payload, format_name.data(), len);
		cmd.text_payload[len] = '\0';
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_set_resolution_scale(double scale) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetResolutionScale;
		cmd.numeric_value = scale;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_set_render_steps(uint64_t steps) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetRenderSteps;
		cmd.step_count = steps;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_set_performance_preset(uint32_t preset) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetPerformancePreset;
		cmd.step_count = preset;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_set_camera_mode(uint32_t mode) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetCameraMode;
		cmd.step_count = mode;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_set_visual_overlay(uint32_t overlay_flag, bool enable) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetVisualOverlay;
		cmd.step_count = overlay_flag;
		cmd.numeric_value = enable ? 1.0 : 0.0;
		return cmd;
	}

	[[nodiscard]] static Command make_capture_screenshot(std::string_view filename = {}) noexcept {
		Command cmd{};
		cmd.type = CommandType::CaptureScreenshot;
		if (!filename.empty()) {
			const size_t len = std::min(filename.size(), sizeof(cmd.text_payload) - 1);
			std::memcpy(cmd.text_payload, filename.data(), len);
			cmd.text_payload[len] = '\0';
		}
		return cmd;
	}
};

struct CommandResult {
	bool success{false};
	char message[128]{};
};

class CommandParser {
public:
	[[nodiscard]] static std::optional<Command> parse(std::string_view line, CommandResult* result_out = nullptr) noexcept {
		auto trim_sv = [](std::string_view s) noexcept -> std::string_view {
			while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) {
				s.remove_prefix(1);
			}
			while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) {
				s.remove_suffix(1);
			}
			return s;
		};

		auto iequals_sv = [](std::string_view a, std::string_view b) noexcept -> bool {
			if (a.size() != b.size()) return false;
			for (size_t i = 0; i < a.size(); ++i) {
				const char ca = (a[i] >= 'A' && a[i] <= 'Z') ? static_cast<char>(a[i] + ('a' - 'A')) : a[i];
				const char cb = (b[i] >= 'A' && b[i] <= 'Z') ? static_cast<char>(b[i] + ('a' - 'A')) : b[i];
				if (ca != cb) return false;
			}
			return true;
		};

		auto parse_u64 = [](std::string_view s, uint64_t& out) noexcept -> bool {
			if (s.empty()) return false;
			uint64_t v = 0;
			for (char c : s) {
				if (c < '0' || c > '9') return false;
				v = v * 10 + static_cast<uint64_t>(c - '0');
			}
			out = v;
			return true;
		};

		auto parse_dbl = [](std::string_view s, double& out) noexcept -> bool {
			if (s.empty()) return false;
			char buf[64];
			if (s.size() >= sizeof(buf)) return false;
			std::memcpy(buf, s.data(), s.size());
			buf[s.size()] = '\0';
			char* end_ptr = nullptr;
			const double v = std::strtod(buf, &end_ptr);
			if (end_ptr != buf + s.size()) return false;
			out = v;
			return true;
		};

		auto set_msg = [](CommandResult* r, bool s, std::string_view m) noexcept {
			if (!r) return;
			r->success = s;
			const size_t len = std::min(m.size(), sizeof(r->message) - 1);
			std::memcpy(r->message, m.data(), len);
			r->message[len] = '\0';
		};

		std::string_view remaining = trim_sv(line);
		if (remaining.empty()) {
			set_msg(result_out, false, "Empty command line");
			return std::nullopt;
		}

		auto extract_token = [&](std::string_view& rem) noexcept -> std::string_view {
			rem = trim_sv(rem);
			if (rem.empty()) return {};
			size_t end = 0;
			while (end < rem.size() && rem[end] != ' ' && rem[end] != '\t' && rem[end] != '\r' && rem[end] != '\n') {
				++end;
			}
			const std::string_view token = rem.substr(0, end);
			rem = trim_sv(rem.substr(end));
			return token;
		};

		const std::string_view token1 = extract_token(remaining);

		if (iequals_sv(token1, "pause")) {
			set_msg(result_out, true, "Simulation paused");
			return Command::make_pause();
		}

		if (iequals_sv(token1, "resume")) {
			set_msg(result_out, true, "Simulation resumed");
			return Command::make_resume();
		}

		if (iequals_sv(token1, "step")) {
			const std::string_view token2 = extract_token(remaining);
			uint64_t n = 1;
			if (!token2.empty()) {
				if (!parse_u64(token2, n) || n == 0) {
					set_msg(result_out, false, "Invalid step count argument");
					return std::nullopt;
				}
			}
			set_msg(result_out, true, "Step command queued");
			return Command::make_step(n);
		}

		if (iequals_sv(token1, "warp")) {
			const std::string_view token2 = extract_token(remaining);
			if (token2.empty()) {
				set_msg(result_out, false, "Missing warp factor argument");
				return std::nullopt;
			}
			double factor = 0.0;
			if (!parse_dbl(token2, factor) || factor <= 0.0) {
				set_msg(result_out, false, "Invalid warp factor");
				return std::nullopt;
			}
			set_msg(result_out, true, "Warp factor updated");
			return Command::make_warp(factor);
		}

		if (iequals_sv(token1, "reset")) {
			set_msg(result_out, true, "Simulation reset");
			return Command::make_reset();
		}

		if (iequals_sv(token1, "tickrate")) {
			const std::string_view token2 = extract_token(remaining);
			if (token2.empty()) {
				set_msg(result_out, false, "Missing tickrate argument");
				return std::nullopt;
			}
			double rate = 0.0;
			if (!parse_dbl(token2, rate) || rate < 10.0 || rate > 1000.0) {
				set_msg(result_out, false, "Invalid tickrate");
				return std::nullopt;
			}
			set_msg(result_out, true, "Tick rate updated");
			return Command::make_set_tickrate(rate);
		}

		if (iequals_sv(token1, "status")) {
			set_msg(result_out, true, "Status query accepted");
			return Command::make_status();
		}

		if (iequals_sv(token1, "shutdown") || iequals_sv(token1, "quit") || iequals_sv(token1, "exit")) {
			set_msg(result_out, true, "Shutdown command registered");
			return Command::make_shutdown();
		}

		if (iequals_sv(token1, "camera")) {
			const std::string_view sub = extract_token(remaining);
			if (iequals_sv(sub, "reset")) {
				set_msg(result_out, true, "Camera reset");
				return Command::make_camera_reset();
			}
			if (iequals_sv(sub, "fov")) {
				const std::string_view val_tok = extract_token(remaining);
				double fov_val = 0.0;
				if (!parse_dbl(val_tok, fov_val) || fov_val < 5.0 || fov_val > 175.0) {
					set_msg(result_out, false, "Invalid FOV (range: 5 to 175 degrees)");
					return std::nullopt;
				}
				set_msg(result_out, true, "Camera FOV set");
				return Command::make_camera_set_fov(fov_val);
			}
			if (iequals_sv(sub, "speed")) {
				const std::string_view val_tok = extract_token(remaining);
				double spd_val = 0.0;
				if (!parse_dbl(val_tok, spd_val) || spd_val <= 0.0) {
					set_msg(result_out, false, "Invalid camera speed");
					return std::nullopt;
				}
				set_msg(result_out, true, "Camera speed set");
				return Command::make_camera_set_speed(spd_val);
			}
			if (iequals_sv(sub, "move")) {
				double dx = 0.0, dy = 0.0, dz = 0.0;
				const auto tx = extract_token(remaining);
				const auto ty = extract_token(remaining);
				const auto tz = extract_token(remaining);
				if (!parse_dbl(tx, dx) || !parse_dbl(ty, dy) || !parse_dbl(tz, dz)) {
					set_msg(result_out, false, "Invalid move delta (expected dx dy dz)");
					return std::nullopt;
				}
				set_msg(result_out, true, "Camera translation applied");
				return Command::make_camera_move(dx, dy, dz);
			}
			if (iequals_sv(sub, "rotate")) {
				double dp = 0.0, dy = 0.0, dr = 0.0;
				const auto tp = extract_token(remaining);
				const auto ty = extract_token(remaining);
				const auto tr = extract_token(remaining);
				if (!parse_dbl(tp, dp) || !parse_dbl(ty, dy)) {
					set_msg(result_out, false, "Invalid rotation delta (expected pitch yaw [roll])");
					return std::nullopt;
				}
				if (!tr.empty()) static_cast<void>(parse_dbl(tr, dr));
				set_msg(result_out, true, "Camera rotation applied");
				return Command::make_camera_rotate(dp, dy, dr);
			}
		}

		if (iequals_sv(token1, "metric")) {
			const std::string_view name = extract_token(remaining);
			if (name.empty()) {
				set_msg(result_out, false, "Missing metric name");
				return std::nullopt;
			}
			set_msg(result_out, true, "Metric changed");
			return Command::make_set_metric(name);
		}

		if (iequals_sv(token1, "load")) {
			const std::string_view path = remaining;
			if (path.empty()) {
				set_msg(result_out, false, "Missing scenario path");
				return std::nullopt;
			}
			set_msg(result_out, true, "Loading scenario");
			return Command::make_load_scenario(path);
		}

		if (iequals_sv(token1, "save")) {
			const std::string_view path = remaining;
			if (path.empty()) {
				set_msg(result_out, false, "Missing scenario save path");
				return std::nullopt;
			}
			set_msg(result_out, true, "Saving scenario");
			return Command::make_save_scenario(path);
		}

		if (iequals_sv(token1, "integrator")) {
			const std::string_view name = extract_token(remaining);
			if (name.empty()) {
				set_msg(result_out, false, "Missing integrator name");
				return std::nullopt;
			}
			set_msg(result_out, true, "Integrator updated");
			return Command::make_set_integrator(name);
		}

		if (iequals_sv(token1, "export")) {
			const std::string_view fmt = extract_token(remaining);
			set_msg(result_out, true, "Export triggered");
			return Command::make_trigger_export(fmt.empty() ? "all" : fmt);
		}

		if (iequals_sv(token1, "set")) {
			const std::string_view token2 = extract_token(remaining);
			if (token2.empty()) {
				set_msg(result_out, false, "Missing parameter name in set command");
				return std::nullopt;
			}
			const std::string_view token3 = extract_token(remaining);
			if (token3.empty()) {
				set_msg(result_out, false, "Missing value in set command");
				return std::nullopt;
			}
			double val = 0.0;
			if (!parse_dbl(token3, val)) {
				set_msg(result_out, false, "Invalid numeric value in set command");
				return std::nullopt;
			}

			ParameterType ptype = ParameterType::Custom;
			if (iequals_sv(token2, "mass")) ptype = ParameterType::Mass;
			else if (iequals_sv(token2, "spin")) ptype = ParameterType::Spin;
			else if (iequals_sv(token2, "charge")) ptype = ParameterType::Charge;
			else if (iequals_sv(token2, "lambda")) ptype = ParameterType::CosmologicalLambda;
			else if (iequals_sv(token2, "throat")) ptype = ParameterType::WormholeThroat;
			else if (iequals_sv(token2, "warp_velocity") || iequals_sv(token2, "warp_vel")) ptype = ParameterType::WarpVelocity;
			else if (iequals_sv(token2, "projection") || iequals_sv(token2, "proj")) ptype = ParameterType::ProjectionMode;
			else if (iequals_sv(token2, "timeflow") || iequals_sv(token2, "time_flow")) ptype = ParameterType::TimeFlowMode;
			else if (iequals_sv(token2, "cameran_speed") || iequals_sv(token2, "speed")) ptype = ParameterType::CameraSpeed;
			else if (iequals_sv(token2, "fov")) ptype = ParameterType::CameraFov;
			else if (iequals_sv(token2, "exposure")) ptype = ParameterType::CameraExposure;
			else if (iequals_sv(token2, "tonemapper") || iequals_sv(token2, "tonemap")) ptype = ParameterType::TonemappingMode;
			else if (iequals_sv(token2, "rtol")) ptype = ParameterType::IntegrationRtol;
			else if (iequals_sv(token2, "atol")) ptype = ParameterType::IntegrationAtol;
			else if (iequals_sv(token2, "min_step")) ptype = ParameterType::IntegrationMinStep;
			else if (iequals_sv(token2, "max_step")) ptype = ParameterType::IntegrationMaxStep;
			else if (iequals_sv(token2, "render_scale") || iequals_sv(token2, "scale")) ptype = ParameterType::ResolutionScale;
			else if (iequals_sv(token2, "ray_steps") || iequals_sv(token2, "steps_limit")) ptype = ParameterType::MaxRaySteps;
			else if (iequals_sv(token2, "performance") || iequals_sv(token2, "perf")) ptype = ParameterType::PerformancePreset;
			else if (iequals_sv(token2, "camera_mode") || iequals_sv(token2, "cam_mode")) ptype = ParameterType::CameraMode;
			else if (iequals_sv(token2, "sky_star_density")) ptype = ParameterType::SkyStarDensity;
			else if (iequals_sv(token2, "sky_star_brightness")) ptype = ParameterType::SkyStarBrightness;
			else if (iequals_sv(token2, "sky_nebula")) ptype = ParameterType::SkyNebulaIntensity;
			else if (iequals_sv(token2, "sky_grid_opacity")) ptype = ParameterType::SkyGridOpacity;
			else if (iequals_sv(token2, "sky_rotation")) ptype = ParameterType::SkyRotation;
			else if (iequals_sv(token2, "sky_hue")) ptype = ParameterType::SkyHueShift;
			else if (iequals_sv(token2, "sky_saturation")) ptype = ParameterType::SkySaturation;
			else if (iequals_sv(token2, "sky_bg_r")) ptype = ParameterType::SkyBackgroundR;
			else if (iequals_sv(token2, "sky_bg_g")) ptype = ParameterType::SkyBackgroundG;
			else if (iequals_sv(token2, "sky_bg_b")) ptype = ParameterType::SkyBackgroundB;
			else if (iequals_sv(token2, "work_distribution") || iequals_sv(token2, "tiling")) ptype = ParameterType::WorkDistributionMode;
			else if (iequals_sv(token2, "force_realloc") || iequals_sv(token2, "realloc_texture")) ptype = ParameterType::ForceTextureReallocation;
			else if (iequals_sv(token2, "tickrate")) {
				if (val < 10.0 || val > 1000.0) {
					set_msg(result_out, false, "Invalid tickrate (must be between 10.0 and 1000.0 Hz)");
					return std::nullopt;
				}
				ptype = ParameterType::TickRate;
			}

			Command cmd = Command::make_set_param(ptype, val);
			if (ptype == ParameterType::Custom) {
				const size_t len = std::min(token2.size(), sizeof(cmd.custom_param_name) - 1);
				std::memcpy(cmd.custom_param_name, token2.data(), len);
				cmd.custom_param_name[len] = '\0';
			}
			set_msg(result_out, true, "Parameter updated");
			return cmd;
		}

		set_msg(result_out, false, "Unrecognized command");
		return std::nullopt;
	}
};

}
