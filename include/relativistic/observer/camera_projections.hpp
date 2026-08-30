#pragma once

#include "relativistic/core/tensor.hpp"
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <numbers>
#include <array>
#include <algorithm>

namespace Relativistic::Observer {

enum class ProjectionMode : uint32_t {
	Pinhole = 0,
	AutoZoomAberration = 1,
	FisheyeStereographic = 2,
	Equirectangular360 = 3
};

template <typename Scalar = double>
class CameraProjector {
public:
	[[nodiscard]] static constexpr std::array<Scalar, 3> compute_pinhole_ray(
		Scalar u_screen,
		Scalar v_screen,
		Scalar fov_rad
	) noexcept {
		const Scalar tan_half_fov = std::tan(fov_rad * static_cast<Scalar>(0.5));
		const Scalar n1 = static_cast<Scalar>(1.0);
		const Scalar n2 = -v_screen * tan_half_fov;
		const Scalar n3 = u_screen * tan_half_fov;

		const Scalar len = std::sqrt(n1 * n1 + n2 * n2 + n3 * n3);
		const Scalar inv_len = (len > static_cast<Scalar>(0.0)) ? (static_cast<Scalar>(1.0) / len) : static_cast<Scalar>(1.0);

		return {n1 * inv_len, n2 * inv_len, n3 * inv_len};
	}

	[[nodiscard]] static constexpr std::array<Scalar, 3> compute_auto_zoom_ray(
		Scalar u_screen,
		Scalar v_screen,
		Scalar fov_rad,
		Scalar lorentz_gamma = static_cast<Scalar>(1.0),
		Scalar beta_forward = static_cast<Scalar>(0.0)
	) noexcept {
		Scalar comp_factor = static_cast<Scalar>(1.0);
		if (lorentz_gamma > static_cast<Scalar>(1.0) || std::abs(beta_forward) > static_cast<Scalar>(0.0)) {
			const Scalar beta = (std::abs(beta_forward) > static_cast<Scalar>(0.0))
				? std::clamp(beta_forward, static_cast<Scalar>(-0.99999999), static_cast<Scalar>(0.99999999))
				: std::sqrt(std::max(static_cast<Scalar>(1.0) - static_cast<Scalar>(1.0) / (lorentz_gamma * lorentz_gamma), static_cast<Scalar>(0.0)));
			comp_factor = std::sqrt((static_cast<Scalar>(1.0) + beta) / (static_cast<Scalar>(1.0) - beta));
		}

		const Scalar tan_half_fov = std::tan(fov_rad * static_cast<Scalar>(0.5)) / comp_factor;
		const Scalar n1 = static_cast<Scalar>(1.0);
		const Scalar n2 = -v_screen * tan_half_fov;
		const Scalar n3 = u_screen * tan_half_fov;

		const Scalar len = std::sqrt(n1 * n1 + n2 * n2 + n3 * n3);
		const Scalar inv_len = (len > static_cast<Scalar>(0.0)) ? (static_cast<Scalar>(1.0) / len) : static_cast<Scalar>(1.0);

		return {n1 * inv_len, n2 * inv_len, n3 * inv_len};
	}

	[[nodiscard]] static std::array<Scalar, 3> compute_fisheye_stereographic_ray(
		Scalar u_screen,
		Scalar v_screen,
		Scalar max_fov_rad = static_cast<Scalar>(220.0 * std::numbers::pi / 180.0)
	) noexcept {
		const Scalar r = std::sqrt(u_screen * u_screen + v_screen * v_screen);
		if (r < static_cast<Scalar>(1e-15)) {
			return {static_cast<Scalar>(1.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0)};
		}

		const Scalar half_max_fov = max_fov_rad * static_cast<Scalar>(0.5);
		const Scalar theta = static_cast<Scalar>(2.0) * std::atan(r * std::tan(half_max_fov * static_cast<Scalar>(0.5)));

		const Scalar sin_t = std::sin(theta);
		const Scalar cos_t = std::cos(theta);

		const Scalar n1 = cos_t;
		const Scalar n2 = -v_screen * (sin_t / r);
		const Scalar n3 = u_screen * (sin_t / r);

		const Scalar len = std::sqrt(n1 * n1 + n2 * n2 + n3 * n3);
		const Scalar inv_len = (len > static_cast<Scalar>(0.0)) ? (static_cast<Scalar>(1.0) / len) : static_cast<Scalar>(1.0);

		return {n1 * inv_len, n2 * inv_len, n3 * inv_len};
	}

	[[nodiscard]] static std::array<Scalar, 3> compute_equirectangular_360_ray(
		Scalar u_screen,
		Scalar v_screen
	) noexcept {
		const Scalar phi = u_screen * std::numbers::pi_v<Scalar>;
		const Scalar theta = (static_cast<Scalar>(0.5) - static_cast<Scalar>(0.5) * v_screen) * std::numbers::pi_v<Scalar>;

		const Scalar sin_t = std::sin(theta);
		const Scalar cos_t = std::cos(theta);
		const Scalar sin_p = std::sin(phi);
		const Scalar cos_p = std::cos(phi);

		const Scalar n1 = sin_t * cos_p;
		const Scalar n2 = cos_t;
		const Scalar n3 = sin_t * sin_p;

		const Scalar len = std::sqrt(n1 * n1 + n2 * n2 + n3 * n3);
		const Scalar inv_len = (len > static_cast<Scalar>(0.0)) ? (static_cast<Scalar>(1.0) / len) : static_cast<Scalar>(1.0);

		return {n1 * inv_len, n2 * inv_len, n3 * inv_len};
	}

	[[nodiscard]] static std::array<Scalar, 3> compute_ray_direction(
		ProjectionMode mode,
		Scalar u_screen,
		Scalar v_screen,
		Scalar fov_rad,
		Scalar lorentz_gamma = static_cast<Scalar>(1.0),
		Scalar beta_forward = static_cast<Scalar>(0.0)
	) noexcept {
		switch (mode) {
			case ProjectionMode::AutoZoomAberration:
				return compute_auto_zoom_ray(u_screen, v_screen, fov_rad, lorentz_gamma, beta_forward);
			case ProjectionMode::FisheyeStereographic:
				return compute_fisheye_stereographic_ray(u_screen, v_screen, fov_rad);
			case ProjectionMode::Equirectangular360:
				return compute_equirectangular_360_ray(u_screen, v_screen);
			case ProjectionMode::Pinhole:
			default:
				return compute_pinhole_ray(u_screen, v_screen, fov_rad);
		}
	}
};

}
