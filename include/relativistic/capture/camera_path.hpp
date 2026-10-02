#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <numbers>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace Relativistic::Capture {

struct CameraPose {
	std::array<double, 3> position{0.0, 0.0, 0.0};
	double pitch_deg{0.0};
	double yaw_deg{0.0};
	double roll_deg{0.0};
	double fov_deg{60.0};
	double exposure_ev{0.0};
};

enum class PathKind : uint32_t {
	Keyframes = 0,
	Orbit = 1,
	LinearFlyBy = 2,
	TargetTrackingOrbit = 3,
	DollyZoom = 4,
	LogarithmicSpiral = 5,
	Helical = 6
};

enum class PathInterpolation : uint32_t {
	Step = 0,
	Linear = 1,
	Smoothstep = 2,
	CatmullRom = 3
};

enum class PathEasing : uint32_t {
	Linear = 0,
	EaseIn = 1,
	EaseOut = 2,
	EaseInOut = 3,
	Smootherstep = 4
};

enum class PathOrientation : uint32_t {
	Keyframed = 0,
	LookAtTarget = 1,
	AlongTravel = 2
};

enum class PathEnd : uint32_t {
	Clamp = 0,
	Loop = 1,
	PingPong = 2
};

struct CameraKeyframe {
	double time_seconds{0.0};
	CameraPose pose{};
	PathInterpolation interpolation{PathInterpolation::CatmullRom};
};

struct OrbitParameters {
	std::array<double, 3> center{0.0, 0.0, 0.0};
	double radius_start{40.0};
	double radius_end{40.0};
	double elevation_start_deg{10.0};
	double elevation_end_deg{10.0};
	double azimuth_start_deg{0.0};
	double revolutions{1.0};
};

struct FlyByParameters {
	std::array<double, 3> start{-60.0, 40.0, 0.0};
	std::array<double, 3> end{60.0, 40.0, 0.0};
};

struct SpiralParameters {
	std::array<double, 3> center{0.0, 0.0, 0.0};
	double radius_start{80.0};
	double radius_end{15.0};
	double height_start{20.0};
	double height_end{2.0};
	double revolutions{3.0};
	double expansion_rate{1.0};
};

struct HelicalParameters {
	std::array<double, 3> start{0.0, 0.0, -40.0};
	std::array<double, 3> end{0.0, 0.0, 40.0};
	double radius{25.0};
	double revolutions{4.0};
	double phase_deg{0.0};
};

struct DollyZoomParameters {
	std::array<double, 3> start_position{0.0, 80.0, 0.0};
	std::array<double, 3> end_position{0.0, 18.0, 0.0};
	std::array<double, 3> target{0.0, 0.0, 0.0};
	double fov_start_deg{90.0};
	double fov_end_deg{25.0};
};

namespace PathDetail {

inline constexpr double kDegToRad = std::numbers::pi_v<double> / 180.0;
inline constexpr double kRadToDeg = 180.0 / std::numbers::pi_v<double>;

[[nodiscard]] inline double lerp(double a, double b, double t) noexcept {
	return a + (b - a) * t;
}

[[nodiscard]] inline double smoothstep(double t) noexcept {
	return t * t * (3.0 - 2.0 * t);
}

[[nodiscard]] inline double wrap_degrees(double angle) noexcept {
	double wrapped = std::fmod(angle + 180.0, 360.0);
	if (wrapped < 0.0) {
		wrapped += 360.0;
	}
	return wrapped - 180.0;
}

[[nodiscard]] inline double angle_near(double angle, double reference) noexcept {
	return reference + wrap_degrees(angle - reference);
}

[[nodiscard]] inline double catmull_rom(double p0, double p1, double p2, double p3, double s) noexcept {
	const double s2 = s * s;
	const double s3 = s2 * s;
	return 0.5 * ((2.0 * p1) + (-p0 + p2) * s + (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * s2 + (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * s3);
}

[[nodiscard]] inline std::vector<double> parse_numbers(std::string_view text) {
	std::vector<double> values;
	size_t begin = 0;
	while (begin <= text.size()) {
		size_t end = text.find(',', begin);
		if (end == std::string_view::npos) {
			end = text.size();
		}
		const std::string token(text.substr(begin, end - begin));
		if (!token.empty()) {
			values.push_back(std::strtod(token.c_str(), nullptr));
		}
		begin = end + 1;
	}
	return values;
}

}

struct CameraPath {
	PathKind kind{PathKind::Keyframes};
	int32_t tracked_body_id{-1};
	std::vector<CameraKeyframe> keyframes{};
	OrbitParameters orbit{};
	FlyByParameters fly_by{};
	SpiralParameters spiral{};
	HelicalParameters helical{};
	DollyZoomParameters dolly_zoom{};
	double duration_seconds{10.0};
	PathInterpolation interpolation{PathInterpolation::CatmullRom};
	PathEasing easing{PathEasing::Linear};
	PathOrientation orientation{PathOrientation::Keyframed};
	PathEnd end_behavior{PathEnd::Clamp};
	std::array<double, 3> look_target{0.0, 0.0, 0.0};
	std::array<double, 2> fixed_pitch_yaw{0.0, 0.0};
	double roll_deg{0.0};
	double fov_start_deg{60.0};
	double fov_end_deg{60.0};
	double exposure_start_ev{0.0};
	double exposure_end_ev{0.0};

	[[nodiscard]] bool is_usable() const noexcept {
		return kind != PathKind::Keyframes || !keyframes.empty();
	}

	[[nodiscard]] double effective_duration() const noexcept {
		if (kind == PathKind::Keyframes && !keyframes.empty()) {
			return std::max(keyframes.back().time_seconds, 1.0e-6);
		}
		return std::max(duration_seconds, 1.0e-6);
	}

	void sort_keyframes() {
		std::stable_sort(keyframes.begin(), keyframes.end(), [](const CameraKeyframe& a, const CameraKeyframe& b) noexcept {
			return a.time_seconds < b.time_seconds;
		});
	}

	void add_keyframe(const CameraPose& pose, double time_seconds) {
		keyframes.push_back(CameraKeyframe{time_seconds, pose, interpolation});
		sort_keyframes();
	}

	[[nodiscard]] CameraPose evaluate(double time_seconds, std::array<double, 3> dynamic_target = {0.0, 0.0, 0.0}) const noexcept {
		const double total = effective_duration();
		const double progress = eased_progress(time_seconds, total);
		CameraPose pose{};
		if (kind == PathKind::Keyframes) {
			pose = keyframe_pose_at(progress * total);
		} else if (kind == PathKind::DollyZoom) {
			pose.position = {
				PathDetail::lerp(dolly_zoom.start_position[0], dolly_zoom.end_position[0], progress),
				PathDetail::lerp(dolly_zoom.start_position[1], dolly_zoom.end_position[1], progress),
				PathDetail::lerp(dolly_zoom.start_position[2], dolly_zoom.end_position[2], progress)
			};
			pose.roll_deg = roll_deg;
			pose.fov_deg = PathDetail::lerp(dolly_zoom.fov_start_deg, dolly_zoom.fov_end_deg, progress);
			pose.exposure_ev = PathDetail::lerp(exposure_start_ev, exposure_end_ev, progress);
			pose.pitch_deg = fixed_pitch_yaw[0];
			pose.yaw_deg = fixed_pitch_yaw[1];
		} else {
			pose.position = position_at(progress, dynamic_target);
			pose.roll_deg = roll_deg;
			pose.fov_deg = PathDetail::lerp(fov_start_deg, fov_end_deg, progress);
			pose.exposure_ev = PathDetail::lerp(exposure_start_ev, exposure_end_ev, progress);
			pose.pitch_deg = fixed_pitch_yaw[0];
			pose.yaw_deg = fixed_pitch_yaw[1];
		}

		const std::array<double, 3> effective_target = (kind == PathKind::TargetTrackingOrbit && tracked_body_id >= 0)
			? dynamic_target
			: ((kind == PathKind::DollyZoom) ? dolly_zoom.target : look_target);

		if (orientation == PathOrientation::LookAtTarget || kind == PathKind::TargetTrackingOrbit || kind == PathKind::DollyZoom) {
			orient_along(pose, {effective_target[0] - pose.position[0], effective_target[1] - pose.position[1], effective_target[2] - pose.position[2]});
		} else if (orientation == PathOrientation::AlongTravel) {
			constexpr double epsilon = 1.0e-4;
			const double before = std::clamp(progress - epsilon, 0.0, 1.0);
			const double after = std::clamp(progress + epsilon, 0.0, 1.0);
			const auto p0 = position_at(before, dynamic_target);
			const auto p1 = position_at(after, dynamic_target);
			orient_along(pose, {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]});
		}
		return pose;
	}

	[[nodiscard]] std::string to_text() const {
		std::ostringstream out;
		out << std::setprecision(17);
		out << "kind=" << static_cast<uint32_t>(kind) << '\n';
		out << "tracked_body=" << tracked_body_id << '\n';
		out << "duration=" << duration_seconds << '\n';
		out << "interpolation=" << static_cast<uint32_t>(interpolation) << '\n';
		out << "easing=" << static_cast<uint32_t>(easing) << '\n';
		out << "orientation=" << static_cast<uint32_t>(orientation) << '\n';
		out << "end=" << static_cast<uint32_t>(end_behavior) << '\n';
		out << "roll=" << roll_deg << '\n';
		out << "fov=" << fov_start_deg << ',' << fov_end_deg << '\n';
		out << "exposure=" << exposure_start_ev << ',' << exposure_end_ev << '\n';
		out << "target=" << look_target[0] << ',' << look_target[1] << ',' << look_target[2] << '\n';
		out << "fixed=" << fixed_pitch_yaw[0] << ',' << fixed_pitch_yaw[1] << '\n';
		out << "orbit=" << orbit.center[0] << ',' << orbit.center[1] << ',' << orbit.center[2] << ','
			<< orbit.radius_start << ',' << orbit.radius_end << ',' << orbit.elevation_start_deg << ','
			<< orbit.elevation_end_deg << ',' << orbit.azimuth_start_deg << ',' << orbit.revolutions << '\n';
		out << "flyby=" << fly_by.start[0] << ',' << fly_by.start[1] << ',' << fly_by.start[2] << ','
			<< fly_by.end[0] << ',' << fly_by.end[1] << ',' << fly_by.end[2] << '\n';
		out << "spiral=" << spiral.center[0] << ',' << spiral.center[1] << ',' << spiral.center[2] << ','
			<< spiral.radius_start << ',' << spiral.radius_end << ',' << spiral.height_start << ','
			<< spiral.height_end << ',' << spiral.revolutions << ',' << spiral.expansion_rate << '\n';
		out << "helical=" << helical.start[0] << ',' << helical.start[1] << ',' << helical.start[2] << ','
			<< helical.end[0] << ',' << helical.end[1] << ',' << helical.end[2] << ','
			<< helical.radius << ',' << helical.revolutions << ',' << helical.phase_deg << '\n';
		out << "dolly=" << dolly_zoom.start_position[0] << ',' << dolly_zoom.start_position[1] << ',' << dolly_zoom.start_position[2] << ','
			<< dolly_zoom.end_position[0] << ',' << dolly_zoom.end_position[1] << ',' << dolly_zoom.end_position[2] << ','
			<< dolly_zoom.target[0] << ',' << dolly_zoom.target[1] << ',' << dolly_zoom.target[2] << ','
			<< dolly_zoom.fov_start_deg << ',' << dolly_zoom.fov_end_deg << '\n';
		for (const auto& key : keyframes) {
			out << "key=" << key.time_seconds << ',' << key.pose.position[0] << ',' << key.pose.position[1] << ',' << key.pose.position[2] << ','
				<< key.pose.pitch_deg << ',' << key.pose.yaw_deg << ',' << key.pose.roll_deg << ',' << key.pose.fov_deg << ','
				<< key.pose.exposure_ev << ',' << static_cast<uint32_t>(key.interpolation) << '\n';
		}
		return out.str();
	}

	[[nodiscard]] static std::optional<CameraPath> from_text(std::string_view text) {
		CameraPath path;
		bool recognized = false;
		size_t position = 0;
		while (position < text.size()) {
			size_t line_end = text.find('\n', position);
			if (line_end == std::string_view::npos) {
				line_end = text.size();
			}
			std::string_view line = text.substr(position, line_end - position);
			position = line_end + 1;
			while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
				line.remove_suffix(1);
			}
			const size_t equals = line.find('=');
			if (equals == std::string_view::npos) {
				continue;
			}
			const std::string_view key = line.substr(0, equals);
			const std::vector<double> v = PathDetail::parse_numbers(line.substr(equals + 1));
			if (v.empty()) {
				continue;
			}
			if (key == "kind") {
				path.kind = static_cast<PathKind>(std::min<uint32_t>(static_cast<uint32_t>(std::max(v[0], 0.0)), 6U));
			} else if (key == "tracked_body") {
				path.tracked_body_id = static_cast<int32_t>(v[0]);
			} else if (key == "duration") {
				path.duration_seconds = std::max(v[0], 1.0e-3);
			} else if (key == "interpolation") {
				path.interpolation = static_cast<PathInterpolation>(std::min<uint32_t>(static_cast<uint32_t>(std::max(v[0], 0.0)), 3U));
			} else if (key == "easing") {
				path.easing = static_cast<PathEasing>(std::min<uint32_t>(static_cast<uint32_t>(std::max(v[0], 0.0)), 4U));
			} else if (key == "orientation") {
				path.orientation = static_cast<PathOrientation>(std::min<uint32_t>(static_cast<uint32_t>(std::max(v[0], 0.0)), 2U));
			} else if (key == "end") {
				path.end_behavior = static_cast<PathEnd>(std::min<uint32_t>(static_cast<uint32_t>(std::max(v[0], 0.0)), 2U));
			} else if (key == "roll") {
				path.roll_deg = v[0];
			} else if (key == "fov" && v.size() >= 2) {
				path.fov_start_deg = v[0];
				path.fov_end_deg = v[1];
			} else if (key == "exposure" && v.size() >= 2) {
				path.exposure_start_ev = v[0];
				path.exposure_end_ev = v[1];
			} else if (key == "target" && v.size() >= 3) {
				path.look_target = {v[0], v[1], v[2]};
			} else if (key == "fixed" && v.size() >= 2) {
				path.fixed_pitch_yaw = {v[0], v[1]};
			} else if (key == "orbit" && v.size() >= 9) {
				path.orbit = OrbitParameters{{v[0], v[1], v[2]}, v[3], v[4], v[5], v[6], v[7], v[8]};
			} else if (key == "flyby" && v.size() >= 6) {
				path.fly_by = FlyByParameters{{v[0], v[1], v[2]}, {v[3], v[4], v[5]}};
			} else if (key == "spiral" && v.size() >= 9) {
				path.spiral = SpiralParameters{{v[0], v[1], v[2]}, v[3], v[4], v[5], v[6], v[7], v[8]};
			} else if (key == "helical" && v.size() >= 9) {
				path.helical = HelicalParameters{{v[0], v[1], v[2]}, {v[3], v[4], v[5]}, v[6], v[7], v[8]};
			} else if (key == "dolly" && v.size() >= 11) {
				path.dolly_zoom = DollyZoomParameters{{v[0], v[1], v[2]}, {v[3], v[4], v[5]}, {v[6], v[7], v[8]}, v[9], v[10]};
			} else if (key == "key" && v.size() >= 10) {
				CameraKeyframe keyframe;
				keyframe.time_seconds = v[0];
				keyframe.pose.position = {v[1], v[2], v[3]};
				keyframe.pose.pitch_deg = v[4];
				keyframe.pose.yaw_deg = v[5];
				keyframe.pose.roll_deg = v[6];
				keyframe.pose.fov_deg = v[7];
				keyframe.pose.exposure_ev = v[8];
				keyframe.interpolation = static_cast<PathInterpolation>(std::min<uint32_t>(static_cast<uint32_t>(std::max(v[9], 0.0)), 3U));
				path.keyframes.push_back(keyframe);
			} else {
				continue;
			}
			recognized = true;
		}
		if (!recognized) {
			return std::nullopt;
		}
		path.sort_keyframes();
		return path;
	}

private:
	[[nodiscard]] double eased_progress(double time_seconds, double total) const noexcept {
		double x = time_seconds / total;
		switch (end_behavior) {
			case PathEnd::Loop:
				x = x - std::floor(x);
				break;
			case PathEnd::PingPong:
				x = std::fmod(x, 2.0);
				if (x < 0.0) {
					x += 2.0;
				}
				if (x > 1.0) {
					x = 2.0 - x;
				}
				break;
			case PathEnd::Clamp:
			default:
				x = std::clamp(x, 0.0, 1.0);
				break;
		}
		switch (easing) {
			case PathEasing::EaseIn: return x * x;
			case PathEasing::EaseOut: return 1.0 - (1.0 - x) * (1.0 - x);
			case PathEasing::EaseInOut: return PathDetail::smoothstep(x);
			case PathEasing::Smootherstep: return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
			case PathEasing::Linear:
			default: return x;
		}
	}

	[[nodiscard]] std::array<double, 3> position_at(double progress, std::array<double, 3> dynamic_target = {0.0, 0.0, 0.0}) const noexcept {
		switch (kind) {
			case PathKind::Orbit: {
				const double radius = PathDetail::lerp(orbit.radius_start, orbit.radius_end, progress);
				const double elevation = PathDetail::lerp(orbit.elevation_start_deg, orbit.elevation_end_deg, progress) * PathDetail::kDegToRad;
				const double azimuth = (orbit.azimuth_start_deg + 360.0 * orbit.revolutions * progress) * PathDetail::kDegToRad;
				return {
					orbit.center[0] + radius * std::cos(elevation) * std::cos(azimuth),
					orbit.center[1] + radius * std::cos(elevation) * std::sin(azimuth),
					orbit.center[2] + radius * std::sin(elevation)
				};
			}
			case PathKind::TargetTrackingOrbit: {
				const std::array<double, 3> center = (tracked_body_id >= 0) ? dynamic_target : orbit.center;
				const double radius = PathDetail::lerp(orbit.radius_start, orbit.radius_end, progress);
				const double elevation = PathDetail::lerp(orbit.elevation_start_deg, orbit.elevation_end_deg, progress) * PathDetail::kDegToRad;
				const double azimuth = (orbit.azimuth_start_deg + 360.0 * orbit.revolutions * progress) * PathDetail::kDegToRad;
				return {
					center[0] + radius * std::cos(elevation) * std::cos(azimuth),
					center[1] + radius * std::cos(elevation) * std::sin(azimuth),
					center[2] + radius * std::sin(elevation)
				};
			}
			case PathKind::LogarithmicSpiral: {
				const double log_ratio = std::log(std::max(spiral.radius_end / std::max(spiral.radius_start, 1.0e-6), 1.0e-6));
				const double radius = spiral.radius_start * std::exp(log_ratio * std::pow(progress, std::max(spiral.expansion_rate, 0.01)));
				const double height = PathDetail::lerp(spiral.height_start, spiral.height_end, progress);
				const double angle = 2.0 * std::numbers::pi_v<double> * spiral.revolutions * progress;
				return {
					spiral.center[0] + radius * std::cos(angle),
					spiral.center[1] + radius * std::sin(angle),
					spiral.center[2] + height
				};
			}
			case PathKind::Helical: {
				const double cx = PathDetail::lerp(helical.start[0], helical.end[0], progress);
				const double cy = PathDetail::lerp(helical.start[1], helical.end[1], progress);
				const double cz = PathDetail::lerp(helical.start[2], helical.end[2], progress);
				const double angle = (helical.phase_deg * PathDetail::kDegToRad) + (2.0 * std::numbers::pi_v<double> * helical.revolutions * progress);
				return {
					cx + helical.radius * std::cos(angle),
					cy + helical.radius * std::sin(angle),
					cz
				};
			}
			case PathKind::DollyZoom:
				return {
					PathDetail::lerp(dolly_zoom.start_position[0], dolly_zoom.end_position[0], progress),
					PathDetail::lerp(dolly_zoom.start_position[1], dolly_zoom.end_position[1], progress),
					PathDetail::lerp(dolly_zoom.start_position[2], dolly_zoom.end_position[2], progress)
				};
			case PathKind::LinearFlyBy:
				return {
					PathDetail::lerp(fly_by.start[0], fly_by.end[0], progress),
					PathDetail::lerp(fly_by.start[1], fly_by.end[1], progress),
					PathDetail::lerp(fly_by.start[2], fly_by.end[2], progress)
				};
			case PathKind::Keyframes:
			default:
				return keyframe_pose_at(progress * effective_duration()).position;
		}
	}

	static void orient_along(CameraPose& pose, const std::array<double, 3>& direction) noexcept {
		const double length = std::sqrt(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
		if (length < 1.0e-12) {
			return;
		}
		pose.yaw_deg = std::atan2(direction[1], direction[0]) * PathDetail::kRadToDeg;
		pose.pitch_deg = std::asin(std::clamp(direction[2] / length, -1.0, 1.0)) * PathDetail::kRadToDeg;
	}

	template <typename Getter>
	[[nodiscard]] double blend_component(size_t index, double s, PathInterpolation mode, Getter&& get, bool angular) const noexcept {
		const size_t last = keyframes.size() - 1;
		const size_t previous = (index == 0) ? 0 : index - 1;
		const size_t following = std::min(index + 2, last);
		const double p1 = get(keyframes[index].pose);
		double p2 = get(keyframes[index + 1].pose);
		double p0 = get(keyframes[previous].pose);
		double p3 = get(keyframes[following].pose);
		if (angular) {
			p2 = PathDetail::angle_near(p2, p1);
			p0 = PathDetail::angle_near(p0, p1);
			p3 = PathDetail::angle_near(p3, p2);
		}
		switch (mode) {
			case PathInterpolation::Step: return p1;
			case PathInterpolation::Linear: return PathDetail::lerp(p1, p2, s);
			case PathInterpolation::Smoothstep: return PathDetail::lerp(p1, p2, PathDetail::smoothstep(s));
			case PathInterpolation::CatmullRom:
			default: return PathDetail::catmull_rom(p0, p1, p2, p3, s);
		}
	}

	[[nodiscard]] CameraPose keyframe_pose_at(double tau) const noexcept {
		if (keyframes.empty()) {
			return CameraPose{};
		}
		if (keyframes.size() == 1 || tau <= keyframes.front().time_seconds) {
			return keyframes.front().pose;
		}
		if (tau >= keyframes.back().time_seconds) {
			return keyframes.back().pose;
		}
		size_t index = 0;
		while (index + 2 < keyframes.size() && keyframes[index + 1].time_seconds <= tau) {
			++index;
		}
		const double t0 = keyframes[index].time_seconds;
		const double t1 = keyframes[index + 1].time_seconds;
		const double s = (t1 - t0 > 1.0e-12) ? std::clamp((tau - t0) / (t1 - t0), 0.0, 1.0) : 0.0;
		const PathInterpolation mode = keyframes[index].interpolation;

		CameraPose pose{};
		pose.position[0] = blend_component(index, s, mode, [](const CameraPose& p) noexcept { return p.position[0]; }, false);
		pose.position[1] = blend_component(index, s, mode, [](const CameraPose& p) noexcept { return p.position[1]; }, false);
		pose.position[2] = blend_component(index, s, mode, [](const CameraPose& p) noexcept { return p.position[2]; }, false);
		pose.pitch_deg = blend_component(index, s, mode, [](const CameraPose& p) noexcept { return p.pitch_deg; }, false);
		pose.yaw_deg = blend_component(index, s, mode, [](const CameraPose& p) noexcept { return p.yaw_deg; }, true);
		pose.roll_deg = blend_component(index, s, mode, [](const CameraPose& p) noexcept { return p.roll_deg; }, true);
		pose.fov_deg = blend_component(index, s, mode, [](const CameraPose& p) noexcept { return p.fov_deg; }, false);
		pose.exposure_ev = blend_component(index, s, mode, [](const CameraPose& p) noexcept { return p.exposure_ev; }, false);
		return pose;
	}
};

}
