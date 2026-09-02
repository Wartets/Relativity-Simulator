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
	Equirectangular360 = 3,
	FisheyeEquidistant = 4,
	FisheyeOrthographic = 5,
	PaniniCylindrical = 6,
	HammerAitoff = 7
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
		Scalar fov_rad = static_cast<Scalar>(120.0 * std::numbers::pi / 180.0)
	) noexcept {
		const Scalar max_fov_rad = std::clamp(fov_rad * static_cast<Scalar>(1.6), static_cast<Scalar>(0.1), static_cast<Scalar>(260.0 * std::numbers::pi / 180.0));
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

	[[nodiscard]] static std::array<Scalar, 3> compute_fisheye_equidistant_ray(
		Scalar u_screen,
		Scalar v_screen,
		Scalar fov_rad
	) noexcept {
		const Scalar r = std::sqrt(u_screen * u_screen + v_screen * v_screen);
		if (r < static_cast<Scalar>(1e-15)) {
			return {static_cast<Scalar>(1.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0)};
		}
		const Scalar theta = std::min(r * fov_rad * static_cast<Scalar>(0.5), static_cast<Scalar>(std::numbers::pi));
		const Scalar sin_t = std::sin(theta);
		const Scalar cos_t = std::cos(theta);
		const Scalar n1 = cos_t;
		const Scalar n2 = -v_screen * (sin_t / r);
		const Scalar n3 = u_screen * (sin_t / r);
		const Scalar len = std::sqrt(n1 * n1 + n2 * n2 + n3 * n3);
		const Scalar inv_len = (len > static_cast<Scalar>(0.0)) ? (static_cast<Scalar>(1.0) / len) : static_cast<Scalar>(1.0);
		return {n1 * inv_len, n2 * inv_len, n3 * inv_len};
	}

	[[nodiscard]] static std::array<Scalar, 3> compute_fisheye_orthographic_ray(
		Scalar u_screen,
		Scalar v_screen,
		Scalar fov_rad
	) noexcept {
		const Scalar r = std::sqrt(u_screen * u_screen + v_screen * v_screen);
		const Scalar sin_half_fov = std::sin(std::clamp(fov_rad * static_cast<Scalar>(0.5), static_cast<Scalar>(0.01), static_cast<Scalar>(std::numbers::pi * 0.499)));
		const Scalar sin_t = std::clamp(r * sin_half_fov, static_cast<Scalar>(0.0), static_cast<Scalar>(1.0));
		const Scalar cos_t = std::sqrt(std::max(static_cast<Scalar>(1.0) - sin_t * sin_t, static_cast<Scalar>(0.0)));
		if (r < static_cast<Scalar>(1e-15)) {
			return {static_cast<Scalar>(1.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0)};
		}
		const Scalar n1 = cos_t;
		const Scalar n2 = -v_screen * (sin_t / r);
		const Scalar n3 = u_screen * (sin_t / r);
		const Scalar len = std::sqrt(n1 * n1 + n2 * n2 + n3 * n3);
		const Scalar inv_len = (len > static_cast<Scalar>(0.0)) ? (static_cast<Scalar>(1.0) / len) : static_cast<Scalar>(1.0);
		return {n1 * inv_len, n2 * inv_len, n3 * inv_len};
	}

	[[nodiscard]] static std::array<Scalar, 3> compute_panini_cylindrical_ray(
		Scalar u_screen,
		Scalar v_screen,
		Scalar fov_rad
	) noexcept {
		const Scalar d = static_cast<Scalar>(1.0);
		const Scalar tan_h = std::tan(fov_rad * static_cast<Scalar>(0.5));
		const Scalar px = u_screen * tan_h;
		const Scalar py = -v_screen * tan_h;
		const Scalar s = (d + static_cast<Scalar>(1.0)) / (d + std::sqrt(std::max(static_cast<Scalar>(1.0) + (static_cast<Scalar>(1.0) - d * d) * px * px / ((d + static_cast<Scalar>(1.0)) * (d + static_cast<Scalar>(1.0))), static_cast<Scalar>(1e-15))));
		const Scalar sin_phi = (px * s) / (d + static_cast<Scalar>(1.0));
		const Scalar cos_phi = std::sqrt(std::max(static_cast<Scalar>(1.0) - sin_phi * sin_phi, static_cast<Scalar>(0.0)));
		const Scalar n1 = cos_phi;
		const Scalar n2 = py;
		const Scalar n3 = sin_phi;
		const Scalar len = std::sqrt(n1 * n1 + n2 * n2 + n3 * n3);
		const Scalar inv_len = (len > static_cast<Scalar>(0.0)) ? (static_cast<Scalar>(1.0) / len) : static_cast<Scalar>(1.0);
		return {n1 * inv_len, n2 * inv_len, n3 * inv_len};
	}

	[[nodiscard]] static std::array<Scalar, 3> compute_hammer_aitoff_ray(
		Scalar u_screen,
		Scalar v_screen,
		Scalar fov_rad
	) noexcept {
		const Scalar zoom = static_cast<Scalar>(60.0 * std::numbers::pi / 180.0) / std::max(fov_rad, static_cast<Scalar>(0.01));
		const Scalar x = std::clamp(u_screen * zoom * static_cast<Scalar>(std::numbers::sqrt2), static_cast<Scalar>(-std::numbers::sqrt2), static_cast<Scalar>(std::numbers::sqrt2));
		const Scalar y = std::clamp(-v_screen * zoom * static_cast<Scalar>(std::numbers::sqrt2 * 0.5), static_cast<Scalar>(-std::numbers::sqrt2 * 0.5), static_cast<Scalar>(std::numbers::sqrt2 * 0.5));
		const Scalar z = std::sqrt(std::max(static_cast<Scalar>(1.0) - (x * x * static_cast<Scalar>(0.25)) - (y * y), static_cast<Scalar>(1e-15)));
		const Scalar phi = static_cast<Scalar>(2.0) * std::atan2(z * x, static_cast<Scalar>(2.0) * (static_cast<Scalar>(2.0) * z * z - static_cast<Scalar>(1.0)));
		const Scalar theta = static_cast<Scalar>(std::numbers::pi * 0.5) - std::asin(std::clamp(z * y * static_cast<Scalar>(std::numbers::sqrt2), static_cast<Scalar>(-1.0), static_cast<Scalar>(1.0)));
		const Scalar sin_t = std::sin(theta);
		const Scalar cos_t = std::cos(theta);
		const Scalar sin_p = std::sin(phi);
		const Scalar cos_p = std::cos(phi);
		return {sin_t * cos_p, cos_t, sin_t * sin_p};
	}

	[[nodiscard]] static std::array<Scalar, 3> compute_equirectangular_360_ray(
		Scalar u_screen,
		Scalar v_screen,
		Scalar fov_rad = static_cast<Scalar>(60.0 * std::numbers::pi / 180.0)
	) noexcept {
		const Scalar fov_scale = fov_rad / static_cast<Scalar>(60.0 * std::numbers::pi / 180.0);
		const Scalar phi = u_screen * std::numbers::pi_v<Scalar> * fov_scale;
		const Scalar theta = std::clamp(static_cast<Scalar>(std::numbers::pi * 0.5) - v_screen * static_cast<Scalar>(std::numbers::pi * 0.5) * fov_scale, static_cast<Scalar>(0.0001), static_cast<Scalar>(std::numbers::pi - 0.0001));

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
				return compute_equirectangular_360_ray(u_screen, v_screen, fov_rad);
			case ProjectionMode::FisheyeEquidistant:
				return compute_fisheye_equidistant_ray(u_screen, v_screen, fov_rad);
			case ProjectionMode::FisheyeOrthographic:
				return compute_fisheye_orthographic_ray(u_screen, v_screen, fov_rad);
			case ProjectionMode::PaniniCylindrical:
				return compute_panini_cylindrical_ray(u_screen, v_screen, fov_rad);
			case ProjectionMode::HammerAitoff:
				return compute_hammer_aitoff_ray(u_screen, v_screen, fov_rad);
			case ProjectionMode::Pinhole:
			default:
				return compute_pinhole_ray(u_screen, v_screen, fov_rad);
		}
	}
};

}
