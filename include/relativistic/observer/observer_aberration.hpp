#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace Relativistic::Observer {

[[nodiscard]] inline bool has_observer_boost(const std::array<double, 4>& boost) noexcept {
	return boost[0] * boost[0] + boost[1] * boost[1] + boost[2] * boost[2] > 1.0e-18;
}

template <typename Direction>
[[nodiscard]] inline std::array<double, 3> apply_observer_boost(const Direction& direction, const std::array<double, 4>& boost) noexcept {
	const std::array<double, 3> seen{
		static_cast<double>(direction[0]),
		static_cast<double>(direction[1]),
		static_cast<double>(direction[2])
	};
	const double speed_squared = boost[0] * boost[0] + boost[1] * boost[1] + boost[2] * boost[2];
	if (speed_squared <= 1.0e-18) {
		return seen;
	}
	const double raw_speed = std::sqrt(speed_squared);
	const double speed = std::min(raw_speed, 0.999999999);
	const double inverse_speed = 1.0 / raw_speed;
	const std::array<double, 3> axis{boost[0] * inverse_speed, boost[1] * inverse_speed, boost[2] * inverse_speed};
	const double gamma = 1.0 / std::sqrt(1.0 - speed * speed);
	const double parallel = seen[0] * axis[0] + seen[1] * axis[1] + seen[2] * axis[2];
	const double denominator = std::max(1.0 - speed * parallel, 1.0e-12);
	const double static_parallel = (parallel - speed) / denominator;
	const double perpendicular_scale = 1.0 / (gamma * denominator);
	return {
		static_parallel * axis[0] + (seen[0] - parallel * axis[0]) * perpendicular_scale,
		static_parallel * axis[1] + (seen[1] - parallel * axis[1]) * perpendicular_scale,
		static_parallel * axis[2] + (seen[2] - parallel * axis[2]) * perpendicular_scale
	};
}

}
