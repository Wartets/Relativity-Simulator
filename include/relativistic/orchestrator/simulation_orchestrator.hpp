#pragma once

#include "relativistic/core/spsc_queue.hpp"
#include "relativistic/orchestrator/command.hpp"
#include "relativistic/orchestrator/scheduler.hpp"
#include "relativistic/io/scenario_serializer.hpp"
#include <array>
#include <atomic>
#include <cstring>
#include <string>
#include <string_view>
#include <optional>
#include <cmath>
#include <numbers>
#include <fstream>
#include <sstream>

namespace Relativistic::Orchestrator {

struct PhysicalParameters {
	double mass{1.0};
	double spin{0.0};
	double charge{0.0};
	double cosmological_lambda{0.0};
	double wormhole_throat{1.0};
	double warp_velocity{1.0};
	uint32_t projection_mode{3};
	uint32_t time_flow_mode{0};
	double camera_speed{10.0};
	double camera_fov_deg{60.0};
	double camera_exposure{0.0};
	uint32_t tonemapping_mode{0};
	double integration_rtol{1e-10};
	double integration_atol{1e-14};
	double integration_min_step{1e-8};
	double integration_max_step{10.0};
	double resolution_scale{0.75};
	uint32_t max_ray_steps{1024};
	uint32_t performance_preset{1};
	uint32_t camera_mode{0};
	uint32_t visual_overlays_flags{0x0F};
	double sky_star_density{1.0};
	double sky_star_brightness{1.0};
	double sky_nebula_intensity{1.0};
	double sky_grid_opacity{1.0};
	double sky_rotation_deg{0.0};
	double sky_hue_shift_deg{0.0};
	double sky_saturation{1.0};
	double sky_background_r{0.0};
	double sky_background_g{0.0};
	double sky_background_b{0.0};
};

struct CustomParameterEntry {
	char name[64]{};
	double value{0.0};
	bool active{false};
};

struct CameraState {
	std::array<double, 3> position{0.0, 32.0, 0.0};
	std::array<double, 3> target{0.0, 0.0, 0.0};
	double pitch{0.0};
	double yaw{180.0};
	double roll{0.0};
	double fov_deg{60.0};
	double speed{15.0};
	double radius{32.0};
	double theta{1.5707963267948966};
	double phi{1.5707963267948966};
	double orbit_distance{32.0};
};

template <size_t QueueCapacity = 1024>
class SimulationOrchestrator {
private:
	Core::SpscQueue<Command, QueueCapacity> command_queue_;
	Core::SpscQueue<CommandResult, QueueCapacity> result_queue_;

	Scheduler<double> scheduler_;
	PhysicalParameters params_;
	CameraState camera_{};
	std::string active_metric_name_{"Schwarzschild"};
	std::string active_integrator_name_{"RK45"};
	std::string active_scenario_name_{"Custom Spacetime"};
	std::array<CustomParameterEntry, 32> custom_params_{};

	std::atomic<bool> is_running_{true};
	std::atomic<uint64_t> commands_processed_{0};

	void sync_camera_spherical_from_cartesian() noexcept {
		const double x = camera_.position[0];
		const double y = camera_.position[1];
		const double z = camera_.position[2];
		const double r = std::sqrt(x * x + y * y + z * z);
		camera_.radius = std::max(r, 1e-6);
		camera_.theta = (r > 0.0) ? std::acos(std::clamp(z / r, -1.0, 1.0)) : (std::numbers::pi_v<double> / 2.0);
		camera_.phi = std::atan2(y, x);
	}

public:
	constexpr SimulationOrchestrator() noexcept {
		sync_camera_spherical_from_cartesian();
	}

	[[nodiscard]] bool enqueue_command(const Command& cmd) noexcept {
		return command_queue_.try_push(cmd);
	}

	[[nodiscard]] bool enqueue_command(Command&& cmd) noexcept {
		return command_queue_.try_push(std::move(cmd));
	}

	[[nodiscard]] bool poll_result(CommandResult& res) noexcept {
		return result_queue_.try_pop(res);
	}

	void process_incoming_commands() noexcept {
		Command cmd;
		while (command_queue_.try_pop(cmd)) {
			CommandResult result{.success = true, .message = {}};
			apply_command(cmd, result);
			commands_processed_.fetch_add(1, std::memory_order_relaxed);
			static_cast<void>(result_queue_.try_push(result));
		}
	}

	void apply_command(const Command& cmd, CommandResult& res) noexcept {
		res.success = true;
		switch (cmd.type) {
			case CommandType::Pause:
				scheduler_.pause();
				std::strncpy(res.message, "Simulation paused", sizeof(res.message) - 1);
				break;
			case CommandType::Resume:
				scheduler_.resume();
				std::strncpy(res.message, "Simulation resumed", sizeof(res.message) - 1);
				break;
			case CommandType::Step:
				scheduler_.request_steps(cmd.step_count > 0 ? cmd.step_count : 1);
				std::strncpy(res.message, "Stepping simulation", sizeof(res.message) - 1);
				break;
			case CommandType::Warp:
				scheduler_.set_warp_factor(cmd.numeric_value);
				std::strncpy(res.message, "Warp factor updated", sizeof(res.message) - 1);
				break;
			case CommandType::Reset:
				scheduler_.reset();
				params_ = PhysicalParameters{};
				camera_ = CameraState{};
				sync_camera_spherical_from_cartesian();
				for (auto& entry : custom_params_) {
					entry.active = false;
				}
				std::strncpy(res.message, "Simulation reset to initial state", sizeof(res.message) - 1);
				break;
			case CommandType::SetTickRate:
				scheduler_.set_tick_rate(cmd.numeric_value);
				std::strncpy(res.message, "Tick rate updated", sizeof(res.message) - 1);
				break;
			case CommandType::SetParam:
				set_physical_param(cmd.param_type, cmd.numeric_value, cmd.custom_param_name);
				std::strncpy(res.message, "Parameter updated", sizeof(res.message) - 1);
				break;
			case CommandType::CameraMove:
				camera_.position[0] += cmd.vec_values[0];
				camera_.position[1] += cmd.vec_values[1];
				camera_.position[2] += cmd.vec_values[2];
				sync_camera_spherical_from_cartesian();
				std::strncpy(res.message, "Camera translated", sizeof(res.message) - 1);
				break;
			case CommandType::CameraRotate:
				camera_.pitch = std::clamp(camera_.pitch + cmd.vec_values[0], -89.0, 89.0);
				camera_.yaw += cmd.vec_values[1];
				camera_.roll += cmd.vec_values[2];
				std::strncpy(res.message, "Camera rotated", sizeof(res.message) - 1);
				break;
			case CommandType::CameraSetFov:
				camera_.fov_deg = std::clamp(cmd.numeric_value, 5.0, 175.0);
				params_.camera_fov_deg = camera_.fov_deg;
				std::strncpy(res.message, "Camera FOV set", sizeof(res.message) - 1);
				break;
			case CommandType::CameraSetSpeed:
				camera_.speed = std::max(cmd.numeric_value, 0.001);
				params_.camera_speed = camera_.speed;
				std::strncpy(res.message, "Camera speed set", sizeof(res.message) - 1);
				break;
			case CommandType::CameraReset:
				camera_ = CameraState{};
				sync_camera_spherical_from_cartesian();
				std::strncpy(res.message, "Camera reset to default", sizeof(res.message) - 1);
				break;
			case CommandType::SetMetric:
				if (cmd.text_payload[0] != '\0') {
					active_metric_name_ = cmd.text_payload;
				}
				std::strncpy(res.message, "Metric set", sizeof(res.message) - 1);
				break;
			case CommandType::SetIntegrator:
				if (cmd.text_payload[0] != '\0') {
					active_integrator_name_ = cmd.text_payload;
				}
				std::strncpy(res.message, "Integrator set", sizeof(res.message) - 1);
				break;
			case CommandType::LoadScenario:
				if (cmd.text_payload[0] != '\0') {
					load_scenario_file(cmd.text_payload, res);
				}
				break;
			case CommandType::SaveScenario:
				if (cmd.text_payload[0] != '\0') {
					save_scenario_file(cmd.text_payload, res);
				}
				break;
			case CommandType::Status:
				std::strncpy(res.message, "Status reported", sizeof(res.message) - 1);
				break;
			case CommandType::SetResolutionScale:
				params_.resolution_scale = std::clamp(cmd.numeric_value, 0.1, 2.0);
				std::strncpy(res.message, "Resolution scale updated", sizeof(res.message) - 1);
				break;
			case CommandType::SetRenderSteps:
				params_.max_ray_steps = static_cast<uint32_t>(std::clamp(cmd.step_count, uint64_t{64}, uint64_t{16384}));
				std::strncpy(res.message, "Max ray steps limit updated", sizeof(res.message) - 1);
				break;
			case CommandType::SetPerformancePreset:
				apply_performance_preset(static_cast<uint32_t>(cmd.step_count));
				std::strncpy(res.message, "Performance preset applied", sizeof(res.message) - 1);
				break;
			case CommandType::SetCameraMode:
				params_.camera_mode = static_cast<uint32_t>(cmd.step_count);
				std::strncpy(res.message, "Camera mode updated", sizeof(res.message) - 1);
				break;
			case CommandType::SetVisualOverlay:
				if (cmd.numeric_value > 0.5) {
					params_.visual_overlays_flags |= static_cast<uint32_t>(cmd.step_count);
				} else {
					params_.visual_overlays_flags &= ~static_cast<uint32_t>(cmd.step_count);
				}
				std::strncpy(res.message, "Visual overlay state toggled", sizeof(res.message) - 1);
				break;
			case CommandType::CaptureScreenshot:
				std::strncpy(res.message, "Screenshot requested", sizeof(res.message) - 1);
				break;
			case CommandType::Shutdown:
				is_running_.store(false, std::memory_order_release);
				std::strncpy(res.message, "Shutdown initiated", sizeof(res.message) - 1);
				break;
			case CommandType::TriggerExport:
			case CommandType::None:
			default:
				res.success = false;
				std::strncpy(res.message, "Unknown or unsupported command", sizeof(res.message) - 1);
				break;
		}
	}

	void apply_performance_preset(uint32_t preset_index) noexcept {
		params_.performance_preset = preset_index;
		switch (preset_index) {
			case 0:
				params_.resolution_scale = 0.50;
				params_.max_ray_steps = 256;
				params_.integration_rtol = 1e-6;
				params_.integration_atol = 1e-9;
				break;
			case 1:
				params_.resolution_scale = 0.75;
				params_.max_ray_steps = 512;
				params_.integration_rtol = 1e-8;
				params_.integration_atol = 1e-12;
				break;
			case 2:
				params_.resolution_scale = 1.00;
				params_.max_ray_steps = 1024;
				params_.integration_rtol = 1e-10;
				params_.integration_atol = 1e-14;
				break;
			case 3:
				params_.resolution_scale = 1.25;
				params_.max_ray_steps = 2048;
				params_.integration_rtol = 1e-12;
				params_.integration_atol = 1e-15;
				break;
			case 4:
			default:
				params_.resolution_scale = 1.50;
				params_.max_ray_steps = 4096;
				params_.integration_rtol = 1e-14;
				params_.integration_atol = 1e-17;
				break;
		}
	}

	void load_scenario_file(const char* filepath, CommandResult& res) noexcept {
		std::ifstream file(filepath);
		if (!file.is_open()) {
			res.success = false;
			std::strncpy(res.message, "Failed to open scenario file", sizeof(res.message) - 1);
			return;
		}
		std::stringstream buffer;
		buffer << file.rdbuf();
		const std::string content = buffer.str();
		const auto scenario_opt = IO::ScenarioSerializer::from_yaml(content);
		if (!scenario_opt.has_value()) {
			res.success = false;
			std::strncpy(res.message, "Failed to parse scenario YAML", sizeof(res.message) - 1);
			return;
		}

		const auto& s = *scenario_opt;
		active_scenario_name_ = s.scenario_name;
		active_metric_name_ = s.metric_type;
		params_.mass = s.central_mass;
		params_.spin = s.central_spin;
		params_.charge = s.central_charge;
		params_.cosmological_lambda = s.cosmological_lambda;
		params_.wormhole_throat = s.wormhole_throat;
		params_.warp_velocity = s.warp_velocity;
		active_integrator_name_ = s.integrator.scheme;
		params_.integration_rtol = s.integrator.relative_tolerance;
		params_.integration_atol = s.integrator.absolute_tolerance;

		if (!s.observers.empty()) {
			camera_.position[0] = s.observers[0].position[1];
			camera_.position[1] = s.observers[0].position[2];
			camera_.position[2] = s.observers[0].position[3];
			camera_.fov_deg = s.observers[0].field_of_view_deg;
			params_.camera_fov_deg = camera_.fov_deg;
			sync_camera_spherical_from_cartesian();
		}

		res.success = true;
		std::strncpy(res.message, "Scenario loaded successfully", sizeof(res.message) - 1);
	}

	void save_scenario_file(const char* filepath, CommandResult& res) noexcept {
		IO::ScenarioDefinition s;
		s.scenario_name = active_scenario_name_;
		s.metric_type = active_metric_name_;
		s.central_mass = params_.mass;
		s.central_spin = params_.spin;
		s.central_charge = params_.charge;
		s.cosmological_lambda = params_.cosmological_lambda;
		s.wormhole_throat = params_.wormhole_throat;
		s.warp_velocity = params_.warp_velocity;
		s.integrator.scheme = active_integrator_name_;
		s.integrator.relative_tolerance = params_.integration_rtol;
		s.integrator.absolute_tolerance = params_.integration_atol;

		IO::ScenarioObserverConfig obs;
		obs.position = {0.0, camera_.position[0], camera_.position[1], camera_.position[2]};
		obs.field_of_view_deg = camera_.fov_deg;
		s.observers.push_back(obs);

		const std::string yaml_str = IO::ScenarioSerializer::to_yaml(s);
		std::ofstream out(filepath);
		if (!out.is_open()) {
			res.success = false;
			std::strncpy(res.message, "Failed to write scenario file", sizeof(res.message) - 1);
			return;
		}
		out << yaml_str;
		res.success = true;
		std::strncpy(res.message, "Scenario saved successfully", sizeof(res.message) - 1);
	}

	void set_physical_param(ParameterType param, double val, const char* custom_name = nullptr) noexcept {
		switch (param) {
			case ParameterType::Mass:
				params_.mass = val;
				break;
			case ParameterType::Spin:
				params_.spin = val;
				break;
			case ParameterType::Charge:
				params_.charge = val;
				break;
			case ParameterType::CosmologicalLambda:
				params_.cosmological_lambda = val;
				break;
			case ParameterType::WormholeThroat:
				params_.wormhole_throat = val;
				break;
			case ParameterType::WarpVelocity:
				params_.warp_velocity = val;
				break;
			case ParameterType::ProjectionMode:
				params_.projection_mode = static_cast<uint32_t>(val);
				break;
			case ParameterType::TimeFlowMode:
				params_.time_flow_mode = static_cast<uint32_t>(val);
				break;
			case ParameterType::CameraSpeed:
				camera_.speed = std::max(val, 0.001);
				params_.camera_speed = camera_.speed;
				break;
			case ParameterType::CameraFov:
				camera_.fov_deg = std::clamp(val, 5.0, 175.0);
				params_.camera_fov_deg = camera_.fov_deg;
				break;
			case ParameterType::CameraExposure:
				params_.camera_exposure = val;
				break;
			case ParameterType::TonemappingMode:
				params_.tonemapping_mode = static_cast<uint32_t>(val);
				break;
			case ParameterType::IntegrationRtol:
				params_.integration_rtol = val;
				break;
			case ParameterType::IntegrationAtol:
				params_.integration_atol = val;
				break;
			case ParameterType::IntegrationMinStep:
				params_.integration_min_step = val;
				break;
			case ParameterType::IntegrationMaxStep:
				params_.integration_max_step = val;
				break;
			case ParameterType::ResolutionScale:
				params_.resolution_scale = std::clamp(val, 0.1, 2.0);
				break;
			case ParameterType::MaxRaySteps:
				params_.max_ray_steps = static_cast<uint32_t>(std::clamp(val, 64.0, 16384.0));
				break;
			case ParameterType::PerformancePreset:
				apply_performance_preset(static_cast<uint32_t>(val));
				break;
			case ParameterType::CameraMode:
				params_.camera_mode = static_cast<uint32_t>(val);
				break;
			case ParameterType::VisualOverlays:
				params_.visual_overlays_flags = static_cast<uint32_t>(val);
				break;
			case ParameterType::SkyStarDensity:
				params_.sky_star_density = std::max(val, 0.0);
				break;
			case ParameterType::SkyStarBrightness:
				params_.sky_star_brightness = std::max(val, 0.0);
				break;
			case ParameterType::SkyNebulaIntensity:
				params_.sky_nebula_intensity = std::max(val, 0.0);
				break;
			case ParameterType::SkyGridOpacity:
				params_.sky_grid_opacity = std::max(val, 0.0);
				break;
			case ParameterType::SkyRotation:
				params_.sky_rotation_deg = val;
				break;
			case ParameterType::SkyHueShift:
				params_.sky_hue_shift_deg = val;
				break;
			case ParameterType::SkySaturation:
				params_.sky_saturation = std::max(val, 0.0);
				break;
			case ParameterType::SkyBackgroundR:
				params_.sky_background_r = std::clamp(val, 0.0, 1.0);
				break;
			case ParameterType::SkyBackgroundG:
				params_.sky_background_g = std::clamp(val, 0.0, 1.0);
				break;
			case ParameterType::SkyBackgroundB:
				params_.sky_background_b = std::clamp(val, 0.0, 1.0);
				break;
			case ParameterType::TickRate:
				scheduler_.set_tick_rate(val);
				break;
			case ParameterType::Custom:
				if (custom_name != nullptr && custom_name[0] != '\0') {
					for (auto& entry : custom_params_) {
						if (entry.active && std::strncmp(entry.name, custom_name, sizeof(entry.name)) == 0) {
							entry.value = val;
							return;
						}
					}
					for (auto& entry : custom_params_) {
						if (!entry.active) {
							entry.active = true;
							std::strncpy(entry.name, custom_name, sizeof(entry.name) - 1);
							entry.value = val;
							return;
						}
					}
				}
				break;
			default:
				break;
		}
	}

	[[nodiscard]] double get_custom_param(std::string_view name, double fallback = 0.0) const noexcept {
		for (const auto& entry : custom_params_) {
			if (entry.active && name == entry.name) {
				return entry.value;
			}
		}
		return fallback;
	}

	[[nodiscard]] constexpr Scheduler<double>& scheduler() noexcept {
		return scheduler_;
	}

	[[nodiscard]] constexpr const Scheduler<double>& scheduler() const noexcept {
		return scheduler_;
	}

	[[nodiscard]] constexpr const PhysicalParameters& parameters() const noexcept {
		return params_;
	}

	[[nodiscard]] constexpr PhysicalParameters& parameters() noexcept {
		return params_;
	}

	[[nodiscard]] constexpr const CameraState& camera() const noexcept {
		return camera_;
	}

	[[nodiscard]] constexpr CameraState& camera() noexcept {
		return camera_;
	}

	[[nodiscard]] const std::string& active_metric_name() const noexcept {
		return active_metric_name_;
	}

	void set_active_metric_name(std::string_view name) {
		active_metric_name_ = name;
	}

	[[nodiscard]] const std::string& active_integrator_name() const noexcept {
		return active_integrator_name_;
	}

	void set_active_integrator_name(std::string_view name) {
		active_integrator_name_ = name;
	}

	[[nodiscard]] const std::string& active_scenario_name() const noexcept {
		return active_scenario_name_;
	}

	void set_active_scenario_name(std::string_view name) {
		active_scenario_name_ = name;
	}

	[[nodiscard]] bool is_running() const noexcept {
		return is_running_.load(std::memory_order_acquire);
	}

	void stop() noexcept {
		is_running_.store(false, std::memory_order_release);
	}

	[[nodiscard]] uint64_t total_commands_processed() const noexcept {
		return commands_processed_.load(std::memory_order_relaxed);
	}
};

}
