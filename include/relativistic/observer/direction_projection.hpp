#pragma once

#include "relativistic/observer/camera_projections.hpp"
#include <cmath>
#include <numbers>
#include <optional>
#include <utility>
#include <algorithm>

namespace Relativistic::Observer {

namespace {

[[nodiscard]] inline std::optional<std::pair<double, double>> invert_non_linear_projection(
	ProjectionMode mode,
	double fwd,
	double right,
	double up,
	double fov_rad
) noexcept {
	constexpr double eps = 1e-12;
	constexpr double step = 1e-6;

	const double n_len = std::sqrt(fwd * fwd + right * right + up * up);
	if (!std::isfinite(n_len) || n_len <= eps) {
		return std::nullopt;
	}

	double u = 0.0;
	double v = 0.0;

	for (int iter = 0; iter < 24; ++iter) {
		auto sample = CameraProjector<double>::compute_ray_direction(mode, u, v, fov_rad);
		const std::array<double, 3> residual{fwd - sample[0], up - sample[1], right - sample[2]};
		const double max_residual = std::max({std::abs(residual[0]), std::abs(residual[1]), std::abs(residual[2])});
		if (max_residual <= 1e-12) {
			return std::make_pair(u, v);
		}

		auto sample_plus_u = CameraProjector<double>::compute_ray_direction(mode, u + step, v, fov_rad);
		auto sample_minus_u = CameraProjector<double>::compute_ray_direction(mode, u - step, v, fov_rad);
		auto sample_plus_v = CameraProjector<double>::compute_ray_direction(mode, u, v + step, fov_rad);
		auto sample_minus_v = CameraProjector<double>::compute_ray_direction(mode, u, v - step, fov_rad);

		const std::array<double, 3> du{(sample_plus_u[0] - sample_minus_u[0]) / (2.0 * step),
				(sample_plus_u[1] - sample_minus_u[1]) / (2.0 * step),
				(sample_plus_u[2] - sample_minus_u[2]) / (2.0 * step)};
		const std::array<double, 3> dv{(sample_plus_v[0] - sample_minus_v[0]) / (2.0 * step),
				(sample_plus_v[1] - sample_minus_v[1]) / (2.0 * step),
				(sample_plus_v[2] - sample_minus_v[2]) / (2.0 * step)};

		const double ata00 = du[0] * du[0] + du[1] * du[1] + du[2] * du[2];
		const double ata01 = du[0] * dv[0] + du[1] * dv[1] + du[2] * dv[2];
		const double ata11 = dv[0] * dv[0] + dv[1] * dv[1] + dv[2] * dv[2];
		const double bt0 = du[0] * residual[0] + du[1] * residual[1] + du[2] * residual[2];
		const double bt1 = dv[0] * residual[0] + dv[1] * residual[1] + dv[2] * residual[2];
		const double det = ata00 * ata11 - ata01 * ata01;
		if (!std::isfinite(det) || std::abs(det) <= 1e-18) {
			break;
		}

		const double delta_u = (bt0 * ata11 - bt1 * ata01) / det;
		const double delta_v = (ata00 * bt1 - ata01 * bt0) / det;
		u += delta_u;
		v += delta_v;
		if (!std::isfinite(u) || !std::isfinite(v)) {
			return std::nullopt;
		}
	}

	auto sample = CameraProjector<double>::compute_ray_direction(mode, u, v, fov_rad);
	const double final_error = std::max({std::abs(sample[0] - fwd), std::abs(sample[1] - up), std::abs(sample[2] - right)});
	if (final_error <= 1e-9) {
		return std::make_pair(u, v);
	}
	return std::nullopt;
}

} // namespace

[[nodiscard]] inline std::optional<std::pair<double, double>> direction_to_screen_uv(
	ProjectionMode mode,
	double fwd,
	double right,
	double up,
	double fov_rad
) noexcept {
	constexpr double eps = 1e-12;
	switch (mode) {
		case ProjectionMode::PaniniCylindrical: {
			if (const auto inverse = invert_non_linear_projection(mode, fwd, right, up, fov_rad)) {
				return inverse;
			}
			if (fwd <= eps) return std::nullopt;
			const double tan_half_fov = std::tan(fov_rad * 0.5);
			if (tan_half_fov < eps) return std::nullopt;
			const double tangent = right / fwd;
			return std::make_pair((2.0 * tangent / std::sqrt(1.0 + tangent * tangent)) / tan_half_fov, (up / fwd) / tan_half_fov);
		}
		case ProjectionMode::HammerAitoff: {
			if (const auto inverse = invert_non_linear_projection(mode, fwd, right, up, fov_rad)) {
				return inverse;
			}
			const double n_len = std::sqrt(fwd * fwd + right * right + up * up);
			if (n_len <= eps) return std::nullopt;
			const double latitude = std::asin(std::clamp(up / n_len, -1.0, 1.0));
			const double longitude = std::atan2(right, fwd);
			const double denom = std::sqrt(std::max(1.0 + std::cos(latitude) * std::cos(longitude), eps));
			const double zoom = (60.0 * std::numbers::pi_v<double> / 180.0) / std::max(fov_rad, 0.01);
			const double x = (2.0 * std::numbers::sqrt2 * std::cos(latitude) * std::sin(longitude * 0.5)) / denom;
			const double y = (std::numbers::sqrt2 * std::sin(latitude)) / denom;
			return std::make_pair(x / (std::numbers::sqrt2 * zoom), y / (std::numbers::sqrt2 * 0.5 * zoom));
		}
		case ProjectionMode::FisheyeEquidistant: {
			const double n_len = std::sqrt(fwd * fwd + right * right + up * up);
			if (n_len <= eps) return std::nullopt;
			const double n1 = std::clamp(fwd / n_len, -1.0, 1.0);
			const double theta = std::acos(n1);
			const double sin_t = std::sin(theta);
			if (sin_t < eps) return std::make_pair(0.0, 0.0);
			if (fov_rad < eps) return std::nullopt;
			const double r = (2.0 * theta) / fov_rad;
			return std::make_pair((right / n_len) * r / sin_t, (up / n_len) * r / sin_t);
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
			return std::make_pair((right / n_len) * r / sin_t, (up / n_len) * r / sin_t);
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
			return std::make_pair((right / n_len) * r / sin_t, (up / n_len) * r / sin_t);
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
			const double v = (std::numbers::pi_v<double> * 0.5 - theta) / (std::numbers::pi_v<double> * 0.5 * fov_scale);
			return std::make_pair(u, v);
		}
		case ProjectionMode::Pinhole:
		case ProjectionMode::AutoZoomAberration:
		default: {
			if (fwd <= eps) return std::nullopt;
			const double tan_half_fov = std::tan(fov_rad * 0.5);
			if (tan_half_fov < eps) return std::nullopt;
			return std::make_pair((right / fwd) / tan_half_fov, (up / fwd) / tan_half_fov);
		}
		}
	}

}
