#pragma once

#include "relativistic/io/capture/capture_settings_io.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace Relativistic::IO {

enum class RecordingFormat : uint32_t {
	CSV = 0,
	TSV = 1,
	Json = 2,
	JsonLines = 3,
	BinaryColumns = 4,
	Container = 5,
	VtkPolyline = 6
};

inline constexpr uint32_t kRecordingFormatCount = 7;

[[nodiscard]] constexpr const char* recording_format_extension(RecordingFormat format) noexcept {
	switch (format) {
		case RecordingFormat::TSV: return "tsv";
		case RecordingFormat::Json: return "json";
		case RecordingFormat::JsonLines: return "jsonl";
		case RecordingFormat::BinaryColumns: return "bin";
		case RecordingFormat::Container: return "rcap";
		case RecordingFormat::VtkPolyline: return "vtp";
		case RecordingFormat::CSV:
		default: return "csv";
	}
}

enum class RecordUnitKind : uint32_t {
	None = 0,
	Length = 1,
	Mass = 2,
	Energy = 3,
	AngularMomentum = 4,
	Frequency = 5,
	SimulationTime = 6
};

enum class RecordChannel : uint32_t {
	FrameIndex = 0,
	SessionTime,
	ScriptTime,
	ScriptProgress,
	SegmentIndex,
	SegmentProgress,
	LogicalTime,
	TickIndex,
	SimulationRate,
	TimeWarp,
	PositionX,
	PositionY,
	PositionZ,
	SphericalRadius,
	SphericalTheta,
	SphericalPhi,
	Pitch,
	Yaw,
	Roll,
	FieldOfView,
	Exposure,
	VelocityX,
	VelocityY,
	VelocityZ,
	Speed,
	AccelerationX,
	AccelerationY,
	AccelerationZ,
	AccelerationMagnitude,
	AngularRate,
	PathLength,
	DistanceToCenter,
	DistanceToHorizon,
	StaticLapse,
	TimeDilation,
	ZamoAngularVelocity,
	StaticProperTime,
	GravitationalRedshift,
	EscapeVelocityFraction,
	DiskDopplerFactor,
	OnDiskBand,
	HorizonRadius,
	PhotonSphereRadius,
	IscoRadius,
	Mass,
	Spin,
	Charge,
	CosmologicalLambda,
	BodyCount,
	BodyTotalKineticEnergy,
	BodyTotalAngularMomentum,
	BodyNearestDistance,
	BodyNearestId,
	BodyTotalMass
};

inline constexpr size_t kRecordChannelCount = 54;
inline constexpr uint64_t kAllRecordChannelsMask = (1ULL << kRecordChannelCount) - 1ULL;

[[nodiscard]] constexpr uint64_t record_channel_bit(RecordChannel channel) noexcept {
	return 1ULL << static_cast<uint32_t>(channel);
}

[[nodiscard]] constexpr uint64_t record_channel_range(RecordChannel first, RecordChannel last) noexcept {
	return (record_channel_bit(last) << 1U) - record_channel_bit(first);
}

struct RecordChannelInfo {
	RecordChannel channel;
	const char* name;
	const char* unit;
	const char* si_unit;
	const char* group;
	RecordUnitKind kind;
};

inline constexpr std::array<RecordChannelInfo, kRecordChannelCount> kRecordChannelInfos{{
	{RecordChannel::FrameIndex, "frame_index", "", "", "Timing", RecordUnitKind::None},
	{RecordChannel::SessionTime, "session_time", "s", "s", "Timing", RecordUnitKind::None},
	{RecordChannel::ScriptTime, "script_time", "s", "s", "Timing", RecordUnitKind::None},
	{RecordChannel::ScriptProgress, "script_progress", "", "", "Timing", RecordUnitKind::None},
	{RecordChannel::SegmentIndex, "segment_index", "", "", "Timing", RecordUnitKind::None},
	{RecordChannel::SegmentProgress, "segment_progress", "", "", "Timing", RecordUnitKind::None},
	{RecordChannel::LogicalTime, "logical_time", "T", "s", "Timing", RecordUnitKind::SimulationTime},
	{RecordChannel::TickIndex, "tick_index", "", "", "Timing", RecordUnitKind::None},
	{RecordChannel::SimulationRate, "simulation_rate", "", "", "Timing", RecordUnitKind::None},
	{RecordChannel::TimeWarp, "time_warp", "", "", "Timing", RecordUnitKind::None},
	{RecordChannel::PositionX, "camera_x", "L", "m", "Camera Kinematics", RecordUnitKind::Length},
	{RecordChannel::PositionY, "camera_y", "L", "m", "Camera Kinematics", RecordUnitKind::Length},
	{RecordChannel::PositionZ, "camera_z", "L", "m", "Camera Kinematics", RecordUnitKind::Length},
	{RecordChannel::SphericalRadius, "camera_r", "L", "m", "Camera Kinematics", RecordUnitKind::Length},
	{RecordChannel::SphericalTheta, "camera_theta", "rad", "rad", "Camera Kinematics", RecordUnitKind::None},
	{RecordChannel::SphericalPhi, "camera_phi", "rad", "rad", "Camera Kinematics", RecordUnitKind::None},
	{RecordChannel::Pitch, "camera_pitch", "deg", "deg", "Camera Kinematics", RecordUnitKind::None},
	{RecordChannel::Yaw, "camera_yaw", "deg", "deg", "Camera Kinematics", RecordUnitKind::None},
	{RecordChannel::Roll, "camera_roll", "deg", "deg", "Camera Kinematics", RecordUnitKind::None},
	{RecordChannel::FieldOfView, "camera_fov", "deg", "deg", "Camera Kinematics", RecordUnitKind::None},
	{RecordChannel::Exposure, "camera_exposure", "EV", "EV", "Camera Kinematics", RecordUnitKind::None},
	{RecordChannel::VelocityX, "camera_vx", "L/s", "m/s", "Camera Kinematics", RecordUnitKind::Length},
	{RecordChannel::VelocityY, "camera_vy", "L/s", "m/s", "Camera Kinematics", RecordUnitKind::Length},
	{RecordChannel::VelocityZ, "camera_vz", "L/s", "m/s", "Camera Kinematics", RecordUnitKind::Length},
	{RecordChannel::Speed, "camera_speed", "L/s", "m/s", "Camera Kinematics", RecordUnitKind::Length},
	{RecordChannel::AccelerationX, "camera_ax", "L/s^2", "m/s^2", "Camera Kinematics", RecordUnitKind::Length},
	{RecordChannel::AccelerationY, "camera_ay", "L/s^2", "m/s^2", "Camera Kinematics", RecordUnitKind::Length},
	{RecordChannel::AccelerationZ, "camera_az", "L/s^2", "m/s^2", "Camera Kinematics", RecordUnitKind::Length},
	{RecordChannel::AccelerationMagnitude, "camera_acceleration", "L/s^2", "m/s^2", "Camera Kinematics", RecordUnitKind::Length},
	{RecordChannel::AngularRate, "camera_angular_rate", "deg/s", "deg/s", "Camera Kinematics", RecordUnitKind::None},
	{RecordChannel::PathLength, "camera_path_length", "L", "m", "Camera Kinematics", RecordUnitKind::Length},
	{RecordChannel::DistanceToCenter, "distance_to_center", "L", "m", "Relativity At Camera", RecordUnitKind::Length},
	{RecordChannel::DistanceToHorizon, "distance_to_horizon", "L", "m", "Relativity At Camera", RecordUnitKind::Length},
	{RecordChannel::StaticLapse, "static_lapse", "", "", "Relativity At Camera", RecordUnitKind::None},
	{RecordChannel::TimeDilation, "time_dilation", "", "", "Relativity At Camera", RecordUnitKind::None},
	{RecordChannel::ZamoAngularVelocity, "zamo_omega", "1/T", "rad/s", "Relativity At Camera", RecordUnitKind::Frequency},
	{RecordChannel::StaticProperTime, "static_proper_time", "T", "s", "Relativity At Camera", RecordUnitKind::SimulationTime},
	{RecordChannel::GravitationalRedshift, "grav_redshift", "", "", "Relativity At Camera", RecordUnitKind::None},
	{RecordChannel::EscapeVelocityFraction, "escape_velocity_fraction", "c", "c", "Relativity At Camera", RecordUnitKind::None},
	{RecordChannel::DiskDopplerFactor, "disk_doppler_factor", "", "", "Relativity At Camera", RecordUnitKind::None},
	{RecordChannel::OnDiskBand, "on_disk_band", "", "", "Relativity At Camera", RecordUnitKind::None},
	{RecordChannel::HorizonRadius, "horizon_radius", "L", "m", "Relativity At Camera", RecordUnitKind::Length},
	{RecordChannel::PhotonSphereRadius, "photon_sphere_radius", "L", "m", "Relativity At Camera", RecordUnitKind::Length},
	{RecordChannel::IscoRadius, "isco_radius", "L", "m", "Relativity At Camera", RecordUnitKind::Length},
	{RecordChannel::Mass, "central_mass", "M", "kg", "Spacetime Parameters", RecordUnitKind::Mass},
	{RecordChannel::Spin, "central_spin", "L", "m", "Spacetime Parameters", RecordUnitKind::Length},
	{RecordChannel::Charge, "central_charge", "Q", "Q", "Spacetime Parameters", RecordUnitKind::None},
	{RecordChannel::CosmologicalLambda, "cosmological_lambda", "1/L^2", "1/L^2", "Spacetime Parameters", RecordUnitKind::None},
	{RecordChannel::BodyCount, "body_count", "", "", "Bodies Aggregate", RecordUnitKind::None},
	{RecordChannel::BodyTotalKineticEnergy, "body_total_kinetic_energy", "E", "J", "Bodies Aggregate", RecordUnitKind::Energy},
	{RecordChannel::BodyTotalAngularMomentum, "body_total_angular_momentum", "J", "kg m^2/s", "Bodies Aggregate", RecordUnitKind::AngularMomentum},
	{RecordChannel::BodyNearestDistance, "body_nearest_distance", "L", "m", "Bodies Aggregate", RecordUnitKind::Length},
	{RecordChannel::BodyNearestId, "body_nearest_id", "", "", "Bodies Aggregate", RecordUnitKind::None},
	{RecordChannel::BodyTotalMass, "body_total_mass", "M", "kg", "Bodies Aggregate", RecordUnitKind::Mass}
}};

[[nodiscard]] constexpr const RecordChannelInfo& record_channel_info(RecordChannel channel) noexcept {
	return kRecordChannelInfos[std::min(static_cast<size_t>(channel), kRecordChannelCount - 1)];
}

inline constexpr uint64_t kDefaultRecordChannelMask =
	record_channel_bit(RecordChannel::FrameIndex)
	| record_channel_bit(RecordChannel::SessionTime)
	| record_channel_bit(RecordChannel::LogicalTime)
	| record_channel_range(RecordChannel::PositionX, RecordChannel::PositionZ)
	| record_channel_bit(RecordChannel::Speed)
	| record_channel_bit(RecordChannel::DistanceToCenter)
	| record_channel_bit(RecordChannel::StaticLapse);

inline constexpr uint64_t kVtkRequiredRecordChannels =
	record_channel_bit(RecordChannel::SessionTime)
	| record_channel_range(RecordChannel::PositionX, RecordChannel::PositionZ)
	| record_channel_bit(RecordChannel::GravitationalRedshift);

struct RecordingSettings {
	bool enabled{false};
	RecordingFormat format{RecordingFormat::CSV};
	uint64_t channel_mask{kDefaultRecordChannelMask};
	bool body_positions{false};
	bool body_velocities{false};
	bool body_camera_distance{false};
	bool body_speeds{false};
	uint32_t max_bodies{16};
	uint32_t decimation{1};
	uint32_t precision{9};
	bool si_units{false};
	bool write_events{true};
	std::string file_stem{"telemetry"};

	[[nodiscard]] bool channel_enabled(RecordChannel channel) const noexcept {
		return (channel_mask & record_channel_bit(channel)) != 0ULL;
	}

	void set_channel(RecordChannel channel, bool enabled_state) noexcept {
		if (enabled_state) {
			channel_mask |= record_channel_bit(channel);
		} else {
			channel_mask &= ~record_channel_bit(channel);
		}
	}

	[[nodiscard]] uint64_t effective_mask() const noexcept {
		const uint64_t required = (format == RecordingFormat::VtkPolyline) ? kVtkRequiredRecordChannels : 0ULL;
		return (channel_mask | required) & kAllRecordChannelsMask;
	}

	[[nodiscard]] bool any_body_channel() const noexcept {
		return body_positions || body_velocities || body_camera_distance || body_speeds;
	}

	void apply_preset(uint32_t preset) noexcept {
		using C = RecordChannel;
		body_positions = false;
		body_velocities = false;
		body_camera_distance = false;
		body_speeds = false;
		const uint64_t timing = record_channel_range(C::FrameIndex, C::TimeWarp);
		switch (preset) {
			case 0:
				channel_mask = 0ULL;
				break;
			case 1:
				channel_mask = record_channel_bit(C::SessionTime) | record_channel_range(C::PositionX, C::PositionZ) | record_channel_bit(C::Speed);
				break;
			case 2:
				channel_mask = record_channel_bit(C::FrameIndex) | record_channel_bit(C::SessionTime) | record_channel_bit(C::LogicalTime)
					| record_channel_range(C::PositionX, C::Exposure) | record_channel_bit(C::Speed)
					| record_channel_bit(C::DistanceToCenter) | record_channel_bit(C::StaticLapse) | record_channel_bit(C::TimeDilation);
				break;
			case 3:
				channel_mask = timing | record_channel_range(C::PositionX, C::PathLength);
				break;
			case 4:
				channel_mask = timing | record_channel_range(C::PositionX, C::PositionZ) | record_channel_range(C::DistanceToCenter, C::IscoRadius)
					| record_channel_range(C::Mass, C::CosmologicalLambda);
				break;
			case 5:
			default:
				channel_mask = kAllRecordChannelsMask;
				body_positions = true;
				body_velocities = true;
				body_camera_distance = true;
				body_speeds = true;
				break;
		}
	}

	void sanitize() {
		decimation = std::clamp<uint32_t>(decimation, 1U, 100000U);
		precision = std::clamp<uint32_t>(precision, 3U, 17U);
		max_bodies = std::min<uint32_t>(max_bodies, 256U);
		channel_mask &= kAllRecordChannelsMask;
		for (char& c : file_stem) {
			const bool allowed = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '-';
			if (!allowed) c = '_';
		}
		if (file_stem.empty()) file_stem = "telemetry";
	}

	void write_settings(SettingsWriter& writer, const std::string& prefix) const {
		writer.flag(prefix + "enabled", enabled);
		writer.enumeration(prefix + "format", format);
		writer.unsigned_value(prefix + "channel_mask", channel_mask);
		writer.flag(prefix + "body_positions", body_positions);
		writer.flag(prefix + "body_velocities", body_velocities);
		writer.flag(prefix + "body_camera_distance", body_camera_distance);
		writer.flag(prefix + "body_speeds", body_speeds);
		writer.unsigned_value(prefix + "max_bodies", max_bodies);
		writer.unsigned_value(prefix + "decimation", decimation);
		writer.unsigned_value(prefix + "precision", precision);
		writer.flag(prefix + "si_units", si_units);
		writer.flag(prefix + "write_events", write_events);
		writer.text(prefix + "file_stem", file_stem);
	}

	void read_settings(const SettingsReader& reader, const std::string& prefix) {
		enabled = reader.flag(prefix + "enabled", enabled);
		format = reader.enumeration(prefix + "format", format, RecordingFormat::VtkPolyline);
		channel_mask = reader.wide_value(prefix + "channel_mask", channel_mask);
		body_positions = reader.flag(prefix + "body_positions", body_positions);
		body_velocities = reader.flag(prefix + "body_velocities", body_velocities);
		body_camera_distance = reader.flag(prefix + "body_camera_distance", body_camera_distance);
		body_speeds = reader.flag(prefix + "body_speeds", body_speeds);
		max_bodies = reader.unsigned_value(prefix + "max_bodies", max_bodies);
		decimation = reader.unsigned_value(prefix + "decimation", decimation);
		precision = reader.unsigned_value(prefix + "precision", precision);
		si_units = reader.flag(prefix + "si_units", si_units);
		write_events = reader.flag(prefix + "write_events", write_events);
		file_stem = reader.text(prefix + "file_stem", file_stem);
		sanitize();
	}
};

}
