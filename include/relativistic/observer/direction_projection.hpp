#pragma once

#include "relativistic/observer/camera_projections.hpp"
#include <cmath>
#include <numbers>
#include <optional>
#include <utility>
#include <algorithm>

namespace Relativistic::Observer {

[[nodiscard]] inline std::optional<std::pair<double, double>> direction_to_screen_uv(
	ProjectionMode mode,
	double fwd,
	double right,
	double up,
	double fov_rad
) noexcept {
	constexpr double eps = 1e-12;
	switch (mode) {
		case ProjectionMode::FisheyeEquidistant: {
			const double n_len = std::sqrt(fwd * fwd + right * right + up * up);
			if (n_len <= eps) return std::nullopt;
			const double n1 = std::clamp(fwd / n_len, -1.0, 1.0);
			const double theta = std::acos(n1);
			const double sin_t = std::sin(theta);
			if (sin_t < eps) return std::make_pair(0.0, 0.0);
			if (fov_rad < eps) return std::nullopt;
			const double r = (2.0 * theta) / fov_rad;
			return std::make_pair((right / n_len) * r / sin_t, -(up / n_len) * r / sin_t);
		}
		case ProjectionMode::FisheyeStereographic: {
			const double n_len = std::sqrt(fwd * fwd + right * right + up * up);
			if (n_len <= eps) return std::nullopt;
			const double n1 = std::clamp(fwd / n_len, -1.0, 1.0);
			const double theta = std::acos(n1);
			const double max_fov = std::clamp(fov_rad, 0.1, 260.0 * std::numbers::pi_v<double> / 180.0);
			const double tan_half_max = std::tan(max_fov * 0.25);
			if (tan_half_max < eps) return std::nullopt;
			const double sin_t = std::sin(theta);
			if (sin_t < eps) return std::make_pair(0.0, 0.0);
			const double r = std::tan(theta * 0.5) / tan_half_max;
			return std::make_pair((right / n_len) * r / sin_t, -(up / n_len) * r / sin_t);
		}
		case ProjectionMode::FisheyeOrthographic: {
			const double n_len = std::sqrt(fwd * fwd + right * right + up * up);
			if (n_len <= eps) return std::nullopt;
			const double n1 = std::clamp(fwd / n_len, -1.0, 1.0);
			if (n1 < 0.0) return std::nullopt;
			const double sin_t = std::sqrt(std::max(1.0 - n1 * n1, 0.0));
			const double sin_half_fov = std::sin(std::clamp(fov_rad * 0.5, 0.01, std::numbers::pi_v<double> * 0.499));
			if (sin_half_fov < eps) return std::nullopt;
			if (sin_t < eps) return std::make_pair(0.0, 0.0);
			const double r = sin_t / sin_half_fov;
			return std::make_pair((right / n_len) * r / sin_t, -(up / n_len) * r / sin_t);
		}
		case ProjectionMode::Equirectangular360: {
			const double n_len = std::sqrt(fwd * fwd + right * right + up * up);
			if (n_len <= eps) return std::nullopt;
			const double n1 = fwd / n_len;
			const double n2 = std::clamp(up / n_len, -1.0, 1.0);
			const double n3 = right / n_len;
			const double theta = std::acos(n2);
			const double phi = std::atan2(n3, n1);
			const double fov_scale = fov_rad / (60.0 * std::numbers::pi_v<double> / 180.0);
			if (fov_scale < eps) return std::nullopt;
			const double u = phi / (std::numbers::pi_v<double> * fov_scale);
			const double v = -(std::numbers::pi_v<double> * 0.5 - theta) / (std::numbers::pi_v<double> * 0.5 * fov_scale);
			return std::make_pair(u, v);
		}
		case ProjectionMode::PaniniCylindrical: {
			if (fwd <= eps) return std::nullopt;
			const double tan_half_fov = std::tan(fov_rad * 0.5);
			if (tan_half_fov < eps) return std::nullopt;
			const double tangent = right / fwd;
			return std::make_pair((2.0 * tangent / std::sqrt(1.0 + tangent * tangent)) / tan_half_fov, -(up / fwd) / tan_half_fov);
		}
		case ProjectionMode::HammerAitoff: {
			const double n_len = std::sqrt(fwd * fwd + right * right + up * up);
			if (n_len <= eps) return std::nullopt;
			const double latitude = std::asin(std::clamp(up / n_len, -1.0, 1.0));
			const double longitude = std::atan2(right, fwd);
			const double denom = std::sqrt(std::max(1.0 + std::cos(latitude) * std::cos(longitude), eps));
			const double zoom = (60.0 * std::numbers::pi_v<double> / 180.0) / std::max(fov_rad, 0.01);
			const double x = (2.0 * std::numbers::sqrt2 * std::cos(latitude) * std::sin(longitude * 0.5)) / denom;
			const double y = (std::numbers::sqrt2 * std::sin(latitude)) / denom;
			return std::make_pair(x / (std::numbers::sqrt2 * zoom), -y / (std::numbers::sqrt2 * 0.5 * zoom));
		}
		case ProjectionMode::Pinhole:
		case ProjectionMode::AutoZoomAberration:
		default: {
			if (fwd <= eps) return std::nullopt;
			const double tan_half_fov = std::tan(fov_rad * 0.5);
			if (tan_half_fov < eps) return std::nullopt;
			return std::make_pair((right / fwd) / tan_half_fov, -(up / fwd) / tan_half_fov);
		}
	}
}

}
