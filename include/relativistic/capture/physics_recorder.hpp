#pragma once

#include "relativistic/capture/camera_path.hpp"
#include "relativistic/core/engine_log.hpp"
#include "relativistic/io/capture/recording_settings.hpp"
#include "relativistic/io/telemetry_table.hpp"
#include "relativistic/metrics/kerr.hpp"
#include "relativistic/metrics/kerr_invariants.hpp"
#include "relativistic/optics/disk_thermal_profile.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <numbers>
#include <string>
#include <vector>

namespace Relativistic::Capture {

struct RecordSample {
	uint64_t frame_index{0};
	double session_time{0.0};
	double script_time{0.0};
	double script_progress{0.0};
	size_t segment_index{0};
	double segment_progress{0.0};
	double simulation_rate{1.0};
	CameraPose pose{};
};

struct RecordedEvent {
	uint64_t frame{0};
	double time{0.0};
	size_t index{0};
	std::string action{};
	std::string label{};
	double value{0.0};
};

class PhysicsRecorder {
private:
	using OrchestratorType = Orchestrator::SimulationOrchestrator<1024>;

	struct UnitScales {
		double length{1.0};
		double time{1.0};
		double mass{1.0};
		double energy{1.0};
		double angular_momentum{1.0};
		double frequency{1.0};
		double body_velocity{1.0};
	};

	struct Derived {
		std::array<double, 3> velocity{0.0, 0.0, 0.0};
		std::array<double, 3> acceleration{0.0, 0.0, 0.0};
		double speed{0.0};
		double acceleration_magnitude{0.0};
		double angular_rate{0.0};
		double path_length{0.0};
		double radius{0.0};
		double theta{0.0};
		double phi{0.0};
		double lapse{1.0};
		double time_dilation{1.0};
		double zamo_omega{0.0};
		double proper_time{0.0};
		double grav_redshift{1.0};
		double escape_fraction{0.0};
		double disk_doppler{1.0};
		double on_disk{0.0};
		double horizon{0.0};
		double distance_to_horizon{0.0};
		double photon_sphere{0.0};
		double isco{0.0};
		double mass{0.0};
		double spin{0.0};
		double charge{0.0};
		double lambda{0.0};
		double logical_time{0.0};
		double tick_index{0.0};
		double warp{1.0};
		double body_count{0.0};
		double body_kinetic_energy{0.0};
		double body_angular_momentum{0.0};
		double body_nearest_distance{std::numeric_limits<double>::quiet_NaN()};
		double body_nearest_id{-1.0};
		double body_mass{0.0};
	};

	OrchestratorType* orchestrator_{nullptr};
	IO::RecordingSettings settings_{};
	std::filesystem::path directory_{};
	std::string script_name_{};
	double frames_per_second_{30.0};
	bool active_{false};
	IO::TelemetryTable table_{};
	std::vector<IO::RecordChannel> channels_{};
	std::vector<uint32_t> body_ids_{};
	std::vector<RecordedEvent> events_{};
	std::vector<double> row_{};
	std::vector<double> body_values_{};
	UnitScales scales_{};
	bool has_previous_{false};
	CameraPose previous_pose_{};
	double previous_time_{0.0};
	double previous_logical_time_{0.0};
	std::array<double, 3> previous_velocity_{0.0, 0.0, 0.0};
	double path_length_{0.0};
	double proper_time_{0.0};
	size_t last_row_count_{0};

	[[nodiscard]] static double norm(const std::array<double, 3>& v) noexcept {
		return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	}

	[[nodiscard]] double scale_of(IO::RecordUnitKind kind) const noexcept {
		switch (kind) {
			case IO::RecordUnitKind::Length: return scales_.length;
			case IO::RecordUnitKind::Mass: return scales_.mass;
			case IO::RecordUnitKind::Energy: return scales_.energy;
			case IO::RecordUnitKind::AngularMomentum: return scales_.angular_momentum;
			case IO::RecordUnitKind::Frequency: return scales_.frequency;
			case IO::RecordUnitKind::SimulationTime: return scales_.time;
			case IO::RecordUnitKind::None:
			default: return 1.0;
		}
	}

	[[nodiscard]] Derived derive(const RecordSample& sample) {
		Derived d;
		const CameraPose& pose = sample.pose;
		const auto& params = orchestrator_->parameters();
		const auto snapshot = orchestrator_->scheduler().snapshot();

		const double dt = has_previous_ ? (sample.session_time - previous_time_) : 0.0;
		if (has_previous_) {
			const std::array<double, 3> delta{
				pose.position[0] - previous_pose_.position[0],
				pose.position[1] - previous_pose_.position[1],
				pose.position[2] - previous_pose_.position[2]
			};
			path_length_ += norm(delta);
			if (dt > 1.0e-12) {
				for (size_t i = 0; i < 3; ++i) {
					d.velocity[i] = delta[i] / dt;
					d.acceleration[i] = (d.velocity[i] - previous_velocity_[i]) / dt;
				}
				const double dp = pose.pitch_deg - previous_pose_.pitch_deg;
				const double dy = std::remainder(pose.yaw_deg - previous_pose_.yaw_deg, 360.0);
				const double dr = std::remainder(pose.roll_deg - previous_pose_.roll_deg, 360.0);
				d.angular_rate = std::sqrt(dp * dp + dy * dy + dr * dr) / dt;
			}
		}
		d.speed = norm(d.velocity);
		d.acceleration_magnitude = norm(d.acceleration);
		d.path_length = path_length_;

		d.radius = norm(pose.position);
		d.theta = (d.radius > 1.0e-12) ? std::acos(std::clamp(pose.position[2] / d.radius, -1.0, 1.0)) : (std::numbers::pi_v<double> * 0.5);
		d.phi = std::atan2(pose.position[1], pose.position[0]);

		d.mass = params.mass;
		d.spin = params.spin;
		d.charge = params.charge;
		d.lambda = params.cosmological_lambda;
		d.logical_time = snapshot.logical_time;
		d.tick_index = static_cast<double>(snapshot.tick_index);
		d.warp = snapshot.warp_factor;

		if (params.mass > 1.0e-9) {
			const double mass = params.mass;
			const double spin = std::clamp(params.spin, -0.999 * mass, 0.999 * mass);
			d.horizon = mass + std::sqrt(std::max(mass * mass - spin * spin, 0.0));
			d.distance_to_horizon = d.radius - d.horizon;
			const double safe_theta = std::clamp(d.theta, 0.001, std::numbers::pi_v<double> - 0.001);
			Metrics::KerrMetric<double> metric(mass, spin, 1.0, 1.0);
			const Core::FourVector<double> metric_position(0.0, std::max(d.radius, 2.05 * mass), safe_theta, d.phi);
			const auto g = metric.metric_tensor(metric_position);
			d.lapse = std::sqrt(std::max(-g(0, 0), 1.0e-30));
			d.time_dilation = 1.0 / std::max(d.lapse, 1.0e-12);
			d.zamo_omega = (std::abs(spin) > 1.0e-9) ? Metrics::compute_zamo_angular_velocity(metric, metric_position) : 0.0;
			d.grav_redshift = std::sqrt(std::max(1.0 - 2.0 * mass / std::max(d.radius, 1.0e-12), 0.0));
			d.escape_fraction = std::sqrt(std::min(2.0 * mass / std::max(d.radius, 1.0e-12), 1.0));
			const bool on_disk = Optics::DiskThermalProfile::radius_within_disk(mass, spin, d.radius);
			d.on_disk = on_disk ? 1.0 : 0.0;
			d.disk_doppler = on_disk ? Optics::DiskThermalProfile::circular_orbit_redshift_factor(mass, d.radius) : 1.0;
			d.photon_sphere = (std::abs(spin) > 1.0e-6)
				? (2.0 * mass * (1.0 + std::cos(2.0 / 3.0 * std::acos(-std::clamp(spin / mass, -1.0, 1.0)))))
				: (3.0 * mass);
			d.isco = Optics::DiskThermalProfile::kerr_isco_radius(mass, spin);
		}

		const double logical_dt = has_previous_ ? (snapshot.logical_time - previous_logical_time_) : 0.0;
		proper_time_ += d.lapse * std::max(logical_dt, 0.0);
		d.proper_time = proper_time_;

		body_values_.clear();
		{
			std::lock_guard<std::recursive_mutex> lock(orchestrator_->nbody_system().bodies_mutex());
			const auto bodies = orchestrator_->nbody_system().bodies();
			std::array<double, 3> angular{0.0, 0.0, 0.0};
			for (const auto& body : bodies) {
				if (!body.enabled) continue;
				d.body_count += 1.0;
				d.body_kinetic_energy += body.kinetic_energy();
				d.body_mass += body.mass;
				angular[0] += body.mass * (body.position[1] * body.velocity[2] - body.position[2] * body.velocity[1]);
				angular[1] += body.mass * (body.position[2] * body.velocity[0] - body.position[0] * body.velocity[2]);
				angular[2] += body.mass * (body.position[0] * body.velocity[1] - body.position[1] * body.velocity[0]);
				const std::array<double, 3> offset{body.position[0] - pose.position[0], body.position[1] - pose.position[1], body.position[2] - pose.position[2]};
				const double distance = norm(offset);
				if (!(distance >= d.body_nearest_distance)) {
					d.body_nearest_distance = distance;
					d.body_nearest_id = static_cast<double>(body.id);
				}
			}
			d.body_angular_momentum = norm(angular);

			for (const uint32_t id : body_ids_) {
				const Dynamics::PostNewtonianBody* found = nullptr;
				for (const auto& body : bodies) {
					if (body.id == id && body.enabled) {
						found = &body;
						break;
					}
				}
				const double nan = std::numeric_limits<double>::quiet_NaN();
				if (settings_.body_positions) {
					for (size_t axis = 0; axis < 3; ++axis) body_values_.push_back(found ? found->position[axis] * scales_.length : nan);
				}
				if (settings_.body_velocities) {
					for (size_t axis = 0; axis < 3; ++axis) body_values_.push_back(found ? found->velocity[axis] * scales_.body_velocity : nan);
				}
				if (settings_.body_camera_distance) {
					if (found) {
						const std::array<double, 3> offset{found->position[0] - pose.position[0], found->position[1] - pose.position[1], found->position[2] - pose.position[2]};
						body_values_.push_back(norm(offset) * scales_.length);
					} else {
						body_values_.push_back(nan);
					}
				}
				if (settings_.body_speeds) {
					body_values_.push_back(found ? found->speed() * scales_.body_velocity : nan);
				}
			}
		}

		previous_pose_ = pose;
		previous_time_ = sample.session_time;
		previous_logical_time_ = snapshot.logical_time;
		previous_velocity_ = d.velocity;
		has_previous_ = true;
		return d;
	}

	[[nodiscard]] static double channel_value(IO::RecordChannel channel, const RecordSample& s, const Derived& d) noexcept {
		using C = IO::RecordChannel;
		const CameraPose& p = s.pose;
		switch (channel) {
			case C::FrameIndex: return static_cast<double>(s.frame_index);
			case C::SessionTime: return s.session_time;
			case C::ScriptTime: return s.script_time;
			case C::ScriptProgress: return s.script_progress;
			case C::SegmentIndex: return static_cast<double>(s.segment_index);
			case C::SegmentProgress: return s.segment_progress;
			case C::LogicalTime: return d.logical_time;
			case C::TickIndex: return d.tick_index;
			case C::SimulationRate: return s.simulation_rate;
			case C::TimeWarp: return d.warp;
			case C::PositionX: return p.position[0];
			case C::PositionY: return p.position[1];
			case C::PositionZ: return p.position[2];
			case C::SphericalRadius: return d.radius;
			case C::SphericalTheta: return d.theta;
			case C::SphericalPhi: return d.phi;
			case C::Pitch: return p.pitch_deg;
			case C::Yaw: return p.yaw_deg;
			case C::Roll: return p.roll_deg;
			case C::FieldOfView: return p.fov_deg;
			case C::Exposure: return p.exposure_ev;
			case C::VelocityX: return d.velocity[0];
			case C::VelocityY: return d.velocity[1];
			case C::VelocityZ: return d.velocity[2];
			case C::Speed: return d.speed;
			case C::AccelerationX: return d.acceleration[0];
			case C::AccelerationY: return d.acceleration[1];
			case C::AccelerationZ: return d.acceleration[2];
			case C::AccelerationMagnitude: return d.acceleration_magnitude;
			case C::AngularRate: return d.angular_rate;
			case C::PathLength: return d.path_length;
			case C::DistanceToCenter: return d.radius;
			case C::DistanceToHorizon: return d.distance_to_horizon;
			case C::StaticLapse: return d.lapse;
			case C::TimeDilation: return d.time_dilation;
			case C::ZamoAngularVelocity: return d.zamo_omega;
			case C::StaticProperTime: return d.proper_time;
			case C::GravitationalRedshift: return d.grav_redshift;
			case C::EscapeVelocityFraction: return d.escape_fraction;
			case C::DiskDopplerFactor: return d.disk_doppler;
			case C::OnDiskBand: return d.on_disk;
			case C::HorizonRadius: return d.horizon;
			case C::PhotonSphereRadius: return d.photon_sphere;
			case C::IscoRadius: return d.isco;
			case C::Mass: return d.mass;
			case C::Spin: return d.spin;
			case C::Charge: return d.charge;
			case C::CosmologicalLambda: return d.lambda;
			case C::BodyCount: return d.body_count;
			case C::BodyTotalKineticEnergy: return d.body_kinetic_energy;
			case C::BodyTotalAngularMomentum: return d.body_angular_momentum;
			case C::BodyNearestDistance: return d.body_nearest_distance;
			case C::BodyNearestId: return d.body_nearest_id;
			case C::BodyTotalMass: return d.body_mass;
			default: return 0.0;
		}
	}

	[[nodiscard]] bool write_events_file() const {
		std::string content = "frame,time,event_index,action,label,value\n";
		for (const RecordedEvent& event : events_) {
			std::string label;
			for (const char c : event.label) {
				if (c == '"') label += "\"\"";
				else if (c != '\n' && c != '\r') label.push_back(c);
			}
			content += std::to_string(event.frame) + ",";
			IO::TelemetryDetail::append_number(content, event.time, static_cast<int>(settings_.precision), "nan");
			content += "," + std::to_string(event.index) + "," + event.action + ",\"" + label + "\",";
			IO::TelemetryDetail::append_number(content, event.value, static_cast<int>(settings_.precision), "nan");
			content += "\n";
		}
		return IO::TelemetryDetail::write_bytes(directory_ / (settings_.file_stem + "_events.csv"), content.data(), content.size());
	}

public:
	void begin(
		const IO::RecordingSettings& settings,
		OrchestratorType& orchestrator,
		const std::filesystem::path& directory,
		std::string script_name,
		double frames_per_second
	) {
		orchestrator_ = &orchestrator;
		settings_ = settings;
		settings_.sanitize();
		directory_ = directory;
		script_name_ = std::move(script_name);
		frames_per_second_ = frames_per_second;
		table_.clear();
		channels_.clear();
		body_ids_.clear();
		events_.clear();
		row_.clear();
		body_values_.clear();
		has_previous_ = false;
		path_length_ = 0.0;
		proper_time_ = 0.0;
		last_row_count_ = 0;
		scales_ = UnitScales{};

		if (settings_.si_units) {
			const auto& engine = orchestrator.constants_engine();
			scales_.length = engine.length_scale();
			scales_.time = engine.time_scale();
			scales_.mass = engine.mass_scale();
			scales_.energy = scales_.mass * scales_.length * scales_.length / (scales_.time * scales_.time);
			scales_.angular_momentum = scales_.mass * scales_.length * scales_.length / scales_.time;
			scales_.frequency = 1.0 / scales_.time;
			scales_.body_velocity = scales_.length / scales_.time;
		}

		const uint64_t mask = settings_.effective_mask();
		for (const IO::RecordChannelInfo& info : IO::kRecordChannelInfos) {
			if ((mask & IO::record_channel_bit(info.channel)) == 0ULL) continue;
			channels_.push_back(info.channel);
			table_.add_column(info.name, settings_.si_units ? info.si_unit : info.unit);
		}

		if (settings_.any_body_channel()) {
			std::lock_guard<std::recursive_mutex> lock(orchestrator.nbody_system().bodies_mutex());
			for (const auto& body : orchestrator.nbody_system().bodies()) {
				if (!body.enabled || body_ids_.size() >= settings_.max_bodies) continue;
				body_ids_.push_back(body.id);
			}
		}
		const std::string length_unit = settings_.si_units ? "m" : "L";
		const std::string velocity_unit = settings_.si_units ? "m/s" : "L/T";
		for (const uint32_t id : body_ids_) {
			const std::string prefix = "body" + std::to_string(id) + "_";
			if (settings_.body_positions) {
				table_.add_column(prefix + "x", length_unit);
				table_.add_column(prefix + "y", length_unit);
				table_.add_column(prefix + "z", length_unit);
			}
			if (settings_.body_velocities) {
				table_.add_column(prefix + "vx", velocity_unit);
				table_.add_column(prefix + "vy", velocity_unit);
				table_.add_column(prefix + "vz", velocity_unit);
			}
			if (settings_.body_camera_distance) {
				table_.add_column(prefix + "camera_distance", length_unit);
			}
			if (settings_.body_speeds) {
				table_.add_column(prefix + "speed", velocity_unit);
			}
		}
		active_ = true;
	}

	[[nodiscard]] bool active() const noexcept { return active_; }

	[[nodiscard]] size_t row_count() const noexcept { return active_ ? table_.row_count() : last_row_count_; }

	void note_event(RecordedEvent event) {
		if (active_) events_.push_back(std::move(event));
	}

	void record(const RecordSample& sample) {
		if (!active_ || orchestrator_ == nullptr) return;
		const Derived derived = derive(sample);
		if ((sample.frame_index % settings_.decimation) != 0ULL) return;
		row_.clear();
		for (const IO::RecordChannel channel : channels_) {
			row_.push_back(channel_value(channel, sample, derived) * scale_of(IO::record_channel_info(channel).kind));
		}
		row_.insert(row_.end(), body_values_.begin(), body_values_.end());
		table_.append_row(row_);
	}

	[[nodiscard]] std::string finish() {
		if (!active_) return {};
		active_ = false;
		last_row_count_ = table_.row_count();

		IO::TelemetryMetadata meta;
		meta.session = directory_.filename().string();
		meta.script = script_name_;
		meta.unit_system = settings_.si_units ? "si" : "geometric";
		meta.frames_per_second = frames_per_second_;
		meta.decimation = settings_.decimation;

		std::string error;
		const auto path = IO::write_telemetry_table(table_, settings_, directory_, meta, error);
		if (settings_.write_events && !events_.empty() && !write_events_file()) {
			Core::log_warning("The recorded event log could not be written to " + directory_.string());
		}
		table_.clear();
		events_.clear();
		if (!path.has_value()) {
			Core::log_error("Physical data recording failed: " + error);
			return {};
		}
		return path->string();
	}
};

}
