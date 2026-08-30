#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/core/four_vector_bundle.hpp"
#include "relativistic/optics/spectral_shift.hpp"
#include <array>
#include <cmath>
#include <algorithm>
#include <optional>
#include <concepts>

namespace Relativistic::Optics {

template <typename Scalar = double>
struct RayHit3D {
	Scalar distance{static_cast<Scalar>(0)};
	Scalar emission_time{static_cast<Scalar>(0)};
	std::array<Scalar, 3> hit_point{};
	std::array<Scalar, 3> normal_emit_frame{};
	Scalar doppler_shift{static_cast<Scalar>(1)};
};

template <typename Scalar = double>
class TerrellPenroseOptics {
public:
	[[nodiscard]] static constexpr std::array<Scalar, 3> aberrate_forward(
		const std::array<Scalar, 3>& n_emit,
		const std::array<Scalar, 3>& beta
	) noexcept {
		const Scalar beta_sq = beta[0] * beta[0] + beta[1] * beta[1] + beta[2] * beta[2];
		if (beta_sq <= static_cast<Scalar>(0)) {
			return n_emit;
		}

		const Scalar gamma = static_cast<Scalar>(1) / std::sqrt(std::max(static_cast<Scalar>(1) - beta_sq, static_cast<Scalar>(1e-30)));
		const Scalar beta_dot_n = beta[0] * n_emit[0] + beta[1] * n_emit[1] + beta[2] * n_emit[2];
		const Scalar denom = static_cast<Scalar>(1) + beta_dot_n;
		const Scalar factor_beta = static_cast<Scalar>(1) + ((gamma - static_cast<Scalar>(1)) / (gamma * beta_sq)) * beta_dot_n;

		std::array<Scalar, 3> n_obs;
		n_obs[0] = (n_emit[0] / gamma + factor_beta * beta[0]) / denom;
		n_obs[1] = (n_emit[1] / gamma + factor_beta * beta[1]) / denom;
		n_obs[2] = (n_emit[2] / gamma + factor_beta * beta[2]) / denom;

		const Scalar len = std::sqrt(n_obs[0] * n_obs[0] + n_obs[1] * n_obs[1] + n_obs[2] * n_obs[2]);
		if (len > static_cast<Scalar>(0)) {
			n_obs[0] /= len;
			n_obs[1] /= len;
			n_obs[2] /= len;
		}
		return n_obs;
	}

	[[nodiscard]] static constexpr std::array<Scalar, 3> aberrate_backward(
		const std::array<Scalar, 3>& n_obs,
		const std::array<Scalar, 3>& beta
	) noexcept {
		const std::array<Scalar, 3> neg_beta = {-beta[0], -beta[1], -beta[2]};
		return aberrate_forward(n_obs, neg_beta);
	}

	[[nodiscard]] static constexpr Scalar rotation_angle(Scalar beta_magnitude) noexcept {
		const Scalar b = std::clamp(beta_magnitude, static_cast<Scalar>(0), static_cast<Scalar>(0.99999999));
		return std::asin(b);
	}

	[[nodiscard]] static std::optional<RayHit3D<Scalar>> intersect_moving_sphere(
		const std::array<Scalar, 3>& obs_pos,
		const std::array<Scalar, 3>& ray_dir,
		Scalar t_obs,
		const std::array<Scalar, 3>& sphere_center_0,
		const std::array<Scalar, 3>& sphere_velocity,
		Scalar sphere_radius,
		Scalar speed_of_light = static_cast<Scalar>(1)
	) noexcept {
		const Scalar c = speed_of_light;
		const std::array<Scalar, 3> w = {
			c * ray_dir[0] + sphere_velocity[0],
			c * ray_dir[1] + sphere_velocity[1],
			c * ray_dir[2] + sphere_velocity[2]
		};

		const std::array<Scalar, 3> r0 = {
			obs_pos[0] + c * t_obs * ray_dir[0] - sphere_center_0[0],
			obs_pos[1] + c * t_obs * ray_dir[1] - sphere_center_0[1],
			obs_pos[2] + c * t_obs * ray_dir[2] - sphere_center_0[2]
		};

		const Scalar a_coeff = w[0] * w[0] + w[1] * w[1] + w[2] * w[2];
		const Scalar b_coeff = -static_cast<Scalar>(2) * (r0[0] * w[0] + r0[1] * w[1] + r0[2] * w[2]);
		const Scalar c_coeff = r0[0] * r0[0] + r0[1] * r0[1] + r0[2] * r0[2] - sphere_radius * sphere_radius;

		if (a_coeff <= static_cast<Scalar>(0)) {
			return std::nullopt;
		}

		const Scalar discr = b_coeff * b_coeff - static_cast<Scalar>(4) * a_coeff * c_coeff;
		if (discr < static_cast<Scalar>(0)) {
			return std::nullopt;
		}

		const Scalar sqrt_discr = std::sqrt(discr);
		const Scalar t1 = (-b_coeff - sqrt_discr) / (static_cast<Scalar>(2) * a_coeff);
		const Scalar t2 = (-b_coeff + sqrt_discr) / (static_cast<Scalar>(2) * a_coeff);

		Scalar t_emit = static_cast<Scalar>(0);
		if (t1 <= t_obs && t2 <= t_obs) {
			t_emit = std::max(t1, t2);
		} else if (t1 <= t_obs) {
			t_emit = t1;
		} else if (t2 <= t_obs) {
			t_emit = t2;
		} else {
			return std::nullopt;
		}

		const std::array<Scalar, 3> hit_pos = {
			obs_pos[0] + c * (t_obs - t_emit) * ray_dir[0],
			obs_pos[1] + c * (t_obs - t_emit) * ray_dir[1],
			obs_pos[2] + c * (t_obs - t_emit) * ray_dir[2]
		};

		const std::array<Scalar, 3> sphere_center_emit = {
			sphere_center_0[0] + sphere_velocity[0] * t_emit,
			sphere_center_0[1] + sphere_velocity[1] * t_emit,
			sphere_center_0[2] + sphere_velocity[2] * t_emit
		};

		std::array<Scalar, 3> normal = {
			(hit_pos[0] - sphere_center_emit[0]) / sphere_radius,
			(hit_pos[1] - sphere_center_emit[1]) / sphere_radius,
			(hit_pos[2] - sphere_center_emit[2]) / sphere_radius
		};

		const std::array<Scalar, 3> beta = {
			sphere_velocity[0] / c,
			sphere_velocity[1] / c,
			sphere_velocity[2] / c
		};

		const Scalar g = compute_kinematic_doppler(beta[0], beta[1], beta[2], ray_dir[0], ray_dir[1], ray_dir[2]);
		const Scalar distance = c * (t_obs - t_emit);

		RayHit3D<Scalar> hit;
		hit.distance = distance;
		hit.emission_time = t_emit;
		hit.hit_point = hit_pos;
		hit.normal_emit_frame = normal;
		hit.doppler_shift = g;
		return hit;
	}
};

}
