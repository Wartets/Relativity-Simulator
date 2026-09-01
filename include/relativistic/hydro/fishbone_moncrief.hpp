#pragma once

#include "relativistic/core/constants.hpp"
#include "relativistic/core/tensor.hpp"
#include "relativistic/metrics/kerr.hpp"
#include "relativistic/hydro/hydro_types.hpp"
#include <cmath>
#include <numbers>
#include <algorithm>
#include <array>
#include <concepts>
#include <limits>

namespace Relativistic::Hydro {

template <typename Scalar = double>
struct FishboneMoncriefConfig {
	Scalar mass{static_cast<Scalar>(10.0 * Core::PhysicalConstants<double>::SOLAR_MASS)};
	Scalar spin_parameter{static_cast<Scalar>(0.0)};
	Scalar r_in{static_cast<Scalar>(6.0)};
	Scalar r_center{static_cast<Scalar>(12.0)};
	Scalar gamma{static_cast<Scalar>(4.0 / 3.0)};
	Scalar max_density{static_cast<Scalar>(1e3)};
	Scalar speed_of_light{static_cast<Scalar>(Core::PhysicalConstants<double>::SPEED_OF_LIGHT)};
	Scalar gravitational_constant{static_cast<Scalar>(Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT)};
};

template <typename Scalar = double>
class FishboneMoncriefTorus {
private:
	FishboneMoncriefConfig<Scalar> config_{};
	Metrics::KerrMetric<Scalar> metric_;
	Scalar specific_l_{static_cast<Scalar>(0.0)};
	Scalar w_in_{static_cast<Scalar>(0.0)};
	Scalar w_center_{static_cast<Scalar>(0.0)};
	Scalar polytropic_k_{static_cast<Scalar>(1.0)};
	Scalar h_max_{static_cast<Scalar>(1.0)};

	void initialize_torus_equilibrium() noexcept {
		const Scalar r_g = (config_.gravitational_constant * config_.mass) / (config_.speed_of_light * config_.speed_of_light);
		const Scalar r_c = (config_.r_center > static_cast<Scalar>(0.0)) ? config_.r_center : (static_cast<Scalar>(12.0) * r_g);
		const Scalar r_in = (config_.r_in > static_cast<Scalar>(0.0)) ? config_.r_in : (static_cast<Scalar>(6.0) * r_g);

		specific_l_ = keplerian_specific_angular_momentum(r_c);

		w_in_ = potential_w(r_in, std::numbers::pi_v<Scalar> * static_cast<Scalar>(0.5));
		w_center_ = potential_w(r_c, std::numbers::pi_v<Scalar> * static_cast<Scalar>(0.5));

		h_max_ = std::exp(std::max(static_cast<Scalar>(0.0), w_in_ - w_center_));
		const Scalar gamma = config_.gamma;

		if (h_max_ > static_cast<Scalar>(1.0) && config_.max_density > static_cast<Scalar>(0.0)) {
			const Scalar num = (gamma - static_cast<Scalar>(1.0)) * (h_max_ - static_cast<Scalar>(1.0));
			const Scalar den = gamma * std::pow(config_.max_density, gamma - static_cast<Scalar>(1.0));
			polytropic_k_ = num / den;
		} else {
			polytropic_k_ = static_cast<Scalar>(1.0);
		}
	}

public:
	explicit FishboneMoncriefTorus(const FishboneMoncriefConfig<Scalar>& config = {}) noexcept
		: config_(config),
		  metric_(config.mass, config.spin_parameter, config.speed_of_light, config.gravitational_constant) {
		initialize_torus_equilibrium();
	}

	[[nodiscard]] constexpr const FishboneMoncriefConfig<Scalar>& config() const noexcept {
		return config_;
	}

	[[nodiscard]] constexpr const Metrics::KerrMetric<Scalar>& metric() const noexcept {
		return metric_;
	}

	[[nodiscard]] constexpr Scalar specific_angular_momentum() const noexcept {
		return specific_l_;
	}

	[[nodiscard]] constexpr Scalar w_in() const noexcept {
		return w_in_;
	}

	[[nodiscard]] constexpr Scalar w_center() const noexcept {
		return w_center_;
	}

	[[nodiscard]] constexpr Scalar max_enthalpy() const noexcept {
		return h_max_;
	}

	[[nodiscard]] constexpr Scalar polytropic_constant() const noexcept {
		return polytropic_k_;
	}

	[[nodiscard]] Scalar keplerian_specific_angular_momentum(Scalar r) const noexcept {
		const Scalar m = config_.mass;
		const Scalar c = config_.speed_of_light;
		const Scalar g = config_.gravitational_constant;
		const Scalar r_g = g * m / (c * c);
		const Scalar a = config_.spin_parameter;

		const Scalar r_safe = std::max(r, static_cast<Scalar>(1.001) * metric_.outer_horizon_radius());
		const Scalar sqrt_m = std::sqrt(r_g);
		const Scalar sqrt_r = std::sqrt(r_safe);

		const Scalar num = sqrt_m * (r_safe * r_safe - static_cast<Scalar>(2.0) * a * sqrt_m * sqrt_r + a * a);
		const Scalar den = r_safe * sqrt_r - static_cast<Scalar>(2.0) * r_g * sqrt_r + a * sqrt_m;

		if (std::abs(den) < static_cast<Scalar>(1e-30)) {
			return static_cast<Scalar>(0.0);
		}
		return num / den;
	}

	[[nodiscard]] Scalar potential_w(Scalar r, Scalar theta) const noexcept {
		const Scalar r_h = metric_.outer_horizon_radius();
		if (r <= r_h * static_cast<Scalar>(1.0001)) {
			return static_cast<Scalar>(1e30);
		}

		const Scalar m = config_.mass;
		const Scalar c = config_.speed_of_light;
		const Scalar g = config_.gravitational_constant;
		const Scalar r_g = g * m / (c * c);
		const Scalar a = config_.spin_parameter;

		const Scalar sin_t = std::sin(theta);
		const Scalar cos_t = std::cos(theta);
		const Scalar sin2_t = std::max(sin_t * sin_t, static_cast<Scalar>(1e-30));
		const Scalar cos2_t = cos_t * cos_t;

		const Scalar r2 = r * r;
		const Scalar a2 = a * a;
		const Scalar rho2 = r2 + a2 * cos2_t;
		const Scalar delta = r2 - static_cast<Scalar>(2.0) * r_g * r + a2;
		const Scalar sigma = (r2 + a2) * (r2 + a2) - a2 * delta * sin2_t;

		const Scalar l = specific_l_;
		const Scalar l2 = l * l;

		const Scalar denom = sigma * sin2_t - static_cast<Scalar>(4.0) * r_g * r * a * l * sin2_t - l2 * (delta - a2 * sin2_t);

		if (denom <= static_cast<Scalar>(0.0) || delta <= static_cast<Scalar>(0.0)) {
			return static_cast<Scalar>(1e30);
		}

		const Scalar u_t_sq = (rho2 * delta * sin2_t) / denom;
		return static_cast<Scalar>(0.5) * std::log(std::max(u_t_sq, static_cast<Scalar>(1e-30)));
	}

	[[nodiscard]] Scalar specific_enthalpy(Scalar r, Scalar theta) const noexcept {
		const Scalar w = potential_w(r, theta);
		if (w >= w_in_) {
			return static_cast<Scalar>(1.0);
		}
		const Scalar h = std::exp(w_in_ - w);
		return std::max(static_cast<Scalar>(1.0), h);
	}

	[[nodiscard]] bool is_inside_torus(Scalar r, Scalar theta) const noexcept {
		const Scalar w = potential_w(r, theta);
		return (w < w_in_);
	}

	[[nodiscard]] Scalar density(Scalar r, Scalar theta) const noexcept {
		if (!is_inside_torus(r, theta)) {
			return static_cast<Scalar>(0.0);
		}

		const Scalar h = specific_enthalpy(r, theta);
		if (h <= static_cast<Scalar>(1.0)) {
			return static_cast<Scalar>(0.0);
		}

		const Scalar gamma = config_.gamma;
		const Scalar k = polytropic_k_;
		const Scalar factor = (gamma - static_cast<Scalar>(1.0)) / (gamma * k);
		const Scalar rho = std::pow(factor * (h - static_cast<Scalar>(1.0)), static_cast<Scalar>(1.0) / (gamma - static_cast<Scalar>(1.0)));

		return std::max(static_cast<Scalar>(0.0), rho);
	}

	[[nodiscard]] Scalar pressure(Scalar r, Scalar theta) const noexcept {
		const Scalar rho_val = density(r, theta);
		if (rho_val <= static_cast<Scalar>(0.0)) {
			return static_cast<Scalar>(0.0);
		}
		return polytropic_k_ * std::pow(rho_val, config_.gamma);
	}

	[[nodiscard]] Scalar specific_internal_energy(Scalar r, Scalar theta) const noexcept {
		const Scalar rho_val = density(r, theta);
		if (rho_val <= static_cast<Scalar>(0.0)) {
			return static_cast<Scalar>(0.0);
		}
		const Scalar p_val = pressure(r, theta);
		return p_val / ((config_.gamma - static_cast<Scalar>(1.0)) * rho_val);
	}

	[[nodiscard]] Scalar energy_density(Scalar r, Scalar theta) const noexcept {
		const Scalar rho_val = density(r, theta);
		if (rho_val <= static_cast<Scalar>(0.0)) {
			return static_cast<Scalar>(0.0);
		}
		const Scalar eps = specific_internal_energy(r, theta);
		return rho_val * (static_cast<Scalar>(1.0) + eps);
	}

	[[nodiscard]] Core::FourVector<Scalar> four_velocity(Scalar r, Scalar theta) const noexcept {
		const Scalar w = potential_w(r, theta);
		if (w >= static_cast<Scalar>(1e20)) {
			return Core::FourVector<Scalar>(static_cast<Scalar>(1.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0));
		}

		const Scalar u_t_lower = -std::exp(w);
		const Scalar u_phi_lower = -specific_l_ * u_t_lower;

		const auto inv_g = metric_.inverse_metric(Core::FourVector<Scalar>(static_cast<Scalar>(0.0), r, theta, static_cast<Scalar>(0.0)));

		const Scalar u_t = inv_g(0, 0) * u_t_lower + inv_g(0, 3) * u_phi_lower;
		const Scalar u_phi = inv_g(3, 0) * u_t_lower + inv_g(3, 3) * u_phi_lower;

		return Core::FourVector<Scalar>(u_t, static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), u_phi);
	}

	[[nodiscard]] PrimitiveVariables<Scalar> evaluate_primitive(Scalar r, Scalar theta, Scalar phi = static_cast<Scalar>(0.0)) const noexcept {
		static_cast<void>(phi);
		if (!is_inside_torus(r, theta)) {
			return PrimitiveVariables<Scalar>(static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0));
		}

		const Scalar rho_val = density(r, theta);
		const Scalar p_val = pressure(r, theta);
		const auto u = four_velocity(r, theta);

		const Scalar v_phi = (u(0) > static_cast<Scalar>(1e-15)) ? (u(3) / u(0)) : static_cast<Scalar>(0.0);

		PrimitiveVariables<Scalar> prim;
		prim.rho = rho_val;
		prim.p = p_val;
		prim.vx = static_cast<Scalar>(0.0);
		prim.vy = static_cast<Scalar>(0.0);
		prim.vz = v_phi;
		prim.recompute_derived(config_.gamma);

		return prim;
	}

	[[nodiscard]] PrimitiveVariables<Scalar> evaluate_cartesian(Scalar x, Scalar y, Scalar z) const noexcept {
		const Scalar a = config_.spin_parameter;
		const Scalar a2 = a * a;
		const Scalar r_cyl2 = x * x + y * y;
		const Scalar r2_approx = r_cyl2 + z * z;
		const Scalar diff = r2_approx - a2;
		const Scalar discr = diff * diff + static_cast<Scalar>(4.0) * a2 * z * z;
		const Scalar sqrt_discr = std::sqrt(std::max(discr, static_cast<Scalar>(0.0)));
		const Scalar r = std::sqrt(std::max(static_cast<Scalar>(0.5) * (diff + sqrt_discr), static_cast<Scalar>(1e-24)));

		const Scalar cos_t = std::clamp(z / r, static_cast<Scalar>(-1.0), static_cast<Scalar>(1.0));
		const Scalar theta = std::acos(cos_t);
		const Scalar phi = std::atan2(y, x);

		return evaluate_primitive(r, theta, phi);
	}
};

}
