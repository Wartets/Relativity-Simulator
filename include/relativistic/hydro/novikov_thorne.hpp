#pragma once

#include "relativistic/core/constants.hpp"
#include "relativistic/core/tensor.hpp"
#include "relativistic/metrics/kerr.hpp"
#include "relativistic/optics/spectrum.hpp"
#include <cmath>
#include <numbers>
#include <algorithm>
#include <array>
#include <concepts>
#include <limits>

namespace Relativistic::Hydro {

template <typename Scalar = double>
struct NovikovThorneConfig {
	Scalar mass{static_cast<Scalar>(10.0 * Core::PhysicalConstants<double>::SOLAR_MASS)};
	Scalar spin_parameter{static_cast<Scalar>(0.0)};
	Scalar accretion_rate{static_cast<Scalar>(1e15)};
	Scalar alpha_viscosity{static_cast<Scalar>(0.1)};
	Scalar speed_of_light{static_cast<Scalar>(Core::PhysicalConstants<double>::SPEED_OF_LIGHT)};
	Scalar gravitational_constant{static_cast<Scalar>(Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT)};
	Scalar stefan_boltzmann{static_cast<Scalar>(5.670374419e-8)};
};

template <typename Scalar = double>
class NovikovThorneDisk {
private:
	NovikovThorneConfig<Scalar> config_{};
	Metrics::KerrMetric<Scalar> metric_;

public:
	explicit constexpr NovikovThorneDisk(const NovikovThorneConfig<Scalar>& config = {}) noexcept
		: config_(config),
		  metric_(config.mass, config.spin_parameter, config.speed_of_light, config.gravitational_constant) {}

	[[nodiscard]] constexpr const NovikovThorneConfig<Scalar>& config() const noexcept {
		return config_;
	}

	[[nodiscard]] constexpr const Metrics::KerrMetric<Scalar>& metric() const noexcept {
		return metric_;
	}

	[[nodiscard]] constexpr Scalar mass() const noexcept {
		return config_.mass;
	}

	[[nodiscard]] constexpr Scalar spin() const noexcept {
		return config_.spin_parameter;
	}

	[[nodiscard]] constexpr Scalar dimensionless_spin() const noexcept {
		const Scalar m = config_.mass;
		const Scalar c = config_.speed_of_light;
		const Scalar g = config_.gravitational_constant;
		const Scalar r_g = g * m / (c * c);
		const Scalar a_star = config_.spin_parameter / r_g;
		return std::clamp(a_star, static_cast<Scalar>(-0.999999999), static_cast<Scalar>(0.999999999));
	}

	[[nodiscard]] Scalar isco_radius() const noexcept {
		const Scalar m = config_.mass;
		const Scalar c = config_.speed_of_light;
		const Scalar g = config_.gravitational_constant;
		const Scalar r_g = g * m / (c * c);
		const Scalar a_star = dimensionless_spin();

		const Scalar z1 = static_cast<Scalar>(1.0) + std::cbrt(static_cast<Scalar>(1.0) - a_star * a_star) * (std::cbrt(static_cast<Scalar>(1.0) + a_star) + std::cbrt(static_cast<Scalar>(1.0) - a_star));
		const Scalar z2 = std::sqrt(static_cast<Scalar>(3.0) * a_star * a_star + z1 * z1);

		Scalar term_sqrt = static_cast<Scalar>(0.0);
		if (a_star >= static_cast<Scalar>(0.0)) {
			term_sqrt = std::sqrt(std::max(static_cast<Scalar>(0.0), (static_cast<Scalar>(3.0) - z1) * (static_cast<Scalar>(3.0) + z1 + static_cast<Scalar>(2.0) * z2)));
			return r_g * (static_cast<Scalar>(3.0) + z2 - term_sqrt);
		} else {
			term_sqrt = std::sqrt(std::max(static_cast<Scalar>(0.0), (static_cast<Scalar>(3.0) - z1) * (static_cast<Scalar>(3.0) + z1 + static_cast<Scalar>(2.0) * z2)));
			return r_g * (static_cast<Scalar>(3.0) + z2 + term_sqrt);
		}
	}

	[[nodiscard]] Scalar marginally_bound_radius() const noexcept {
		const Scalar m = config_.mass;
		const Scalar c = config_.speed_of_light;
		const Scalar g = config_.gravitational_constant;
		const Scalar r_g = g * m / (c * c);
		const Scalar a_star = dimensionless_spin();

		if (a_star >= static_cast<Scalar>(0.0)) {
			return r_g * (static_cast<Scalar>(2.0) - a_star + static_cast<Scalar>(2.0) * std::sqrt(static_cast<Scalar>(1.0) - a_star));
		} else {
			return r_g * (static_cast<Scalar>(2.0) - a_star + static_cast<Scalar>(2.0) * std::sqrt(static_cast<Scalar>(1.0) + a_star));
		}
	}

	[[nodiscard]] Scalar photon_orbit_radius() const noexcept {
		const Scalar m = config_.mass;
		const Scalar c = config_.speed_of_light;
		const Scalar g = config_.gravitational_constant;
		const Scalar r_g = g * m / (c * c);
		const Scalar a_star = dimensionless_spin();

		const Scalar psi = std::acos(std::clamp(-a_star, static_cast<Scalar>(-1.0), static_cast<Scalar>(1.0))) / static_cast<Scalar>(3.0);
		return r_g * static_cast<Scalar>(2.0) * (static_cast<Scalar>(1.0) + std::cos(static_cast<Scalar>(2.0) * psi));
	}

	[[nodiscard]] std::array<Scalar, 3> roots_x123() const noexcept {
		const Scalar a_star = dimensionless_spin();
		const Scalar psi = std::acos(std::clamp(a_star, static_cast<Scalar>(-1.0), static_cast<Scalar>(1.0))) / static_cast<Scalar>(3.0);
		const Scalar pi = std::numbers::pi_v<Scalar>;

		const Scalar x1 = static_cast<Scalar>(2.0) * std::cos(psi - pi / static_cast<Scalar>(3.0));
		const Scalar x2 = static_cast<Scalar>(2.0) * std::cos(psi + pi / static_cast<Scalar>(3.0));
		const Scalar x3 = -static_cast<Scalar>(2.0) * std::cos(psi);

		return {x1, x2, x3};
	}

	[[nodiscard]] Scalar page_thorne_f(Scalar x) const noexcept {
		const Scalar r_isco = isco_radius();
		const Scalar m = config_.mass;
		const Scalar c = config_.speed_of_light;
		const Scalar g = config_.gravitational_constant;
		const Scalar r_g = g * m / (c * c);
		const Scalar x0 = std::sqrt(r_isco / r_g);

		if (x < x0) {
			return static_cast<Scalar>(0.0);
		}

		const Scalar a_star = dimensionless_spin();
		const auto [x1, x2, x3] = roots_x123();

		const Scalar term_linear = x - x0;

		Scalar term_log0 = static_cast<Scalar>(0.0);
		if (std::abs(a_star) > static_cast<Scalar>(1e-15)) {
			term_log0 = static_cast<Scalar>(1.5) * a_star * std::log(x / x0);
		}

		const Scalar num1 = static_cast<Scalar>(3.0) * (x1 - a_star) * (x1 - a_star);
		const Scalar den1 = x1 * (x1 - x2) * (x1 - x3);
		const Scalar c1 = num1 / den1;
		const Scalar arg1 = std::max((x - x1) / (x0 - x1), static_cast<Scalar>(1e-30));
		const Scalar term_log1 = c1 * std::log(arg1);

		Scalar term_log2 = static_cast<Scalar>(0.0);
		if (std::abs(x2) > static_cast<Scalar>(1e-12)) {
			const Scalar num2 = static_cast<Scalar>(3.0) * (x2 - a_star) * (x2 - a_star);
			const Scalar den2 = x2 * (x2 - x1) * (x2 - x3);
			const Scalar c2 = num2 / den2;
			const Scalar arg2 = std::max((x - x2) / (x0 - x2), static_cast<Scalar>(1e-30));
			term_log2 = c2 * std::log(arg2);
		}

		const Scalar num3 = static_cast<Scalar>(3.0) * (x3 - a_star) * (x3 - a_star);
		const Scalar den3 = x3 * (x3 - x1) * (x3 - x2);
		const Scalar c3 = num3 / den3;
		const Scalar arg3 = std::max((x - x3) / (x0 - x3), static_cast<Scalar>(1e-30));
		const Scalar term_log3 = c3 * std::log(arg3);

		const Scalar f_val = term_linear - term_log0 - term_log1 - term_log2 - term_log3;
		return std::max(static_cast<Scalar>(0.0), f_val);
	}

	[[nodiscard]] Scalar page_thorne_f_derivative(Scalar x) const noexcept {
		const Scalar a_star = dimensionless_spin();
		const auto [x1, x2, x3] = roots_x123();

		Scalar der = static_cast<Scalar>(1.0);

		if (std::abs(a_star) > static_cast<Scalar>(1e-15)) {
			der -= static_cast<Scalar>(1.5) * a_star / x;
		}

		const Scalar num1 = static_cast<Scalar>(3.0) * (x1 - a_star) * (x1 - a_star);
		const Scalar den1 = x1 * (x1 - x2) * (x1 - x3);
		der -= (num1 / den1) / (x - x1);

		if (std::abs(x2) > static_cast<Scalar>(1e-12)) {
			const Scalar num2 = static_cast<Scalar>(3.0) * (x2 - a_star) * (x2 - a_star);
			const Scalar den2 = x2 * (x2 - x1) * (x2 - x3);
			der -= (num2 / den2) / (x - x2);
		}

		const Scalar num3 = static_cast<Scalar>(3.0) * (x3 - a_star) * (x3 - a_star);
		const Scalar den3 = x3 * (x3 - x1) * (x3 - x2);
		der -= (num3 / den3) / (x - x3);

		return der;
	}

	[[nodiscard]] Scalar numerical_integral_f(Scalar x, size_t steps = 10000) const noexcept {
		const Scalar r_isco = isco_radius();
		const Scalar m = config_.mass;
		const Scalar c = config_.speed_of_light;
		const Scalar g = config_.gravitational_constant;
		const Scalar r_g = g * m / (c * c);
		const Scalar x0 = std::sqrt(r_isco / r_g);

		if (x <= x0) {
			return static_cast<Scalar>(0.0);
		}

		const size_t n_intervals = (steps % 2 == 0) ? steps : (steps + 1);
		const Scalar h = (x - x0) / static_cast<Scalar>(n_intervals);

		Scalar sum = page_thorne_f_derivative(x0) + page_thorne_f_derivative(x);
		for (size_t i = 1; i < n_intervals; ++i) {
			const Scalar xi = x0 + static_cast<Scalar>(i) * h;
			const Scalar w = (i % 2 == 0) ? static_cast<Scalar>(2.0) : static_cast<Scalar>(4.0);
			sum += w * page_thorne_f_derivative(xi);
		}

		return (h / static_cast<Scalar>(3.0)) * sum;
	}

	[[nodiscard]] Scalar keplerian_omega(Scalar r) const noexcept {
		const Scalar m = config_.mass;
		const Scalar c = config_.speed_of_light;
		const Scalar g = config_.gravitational_constant;
		const Scalar r_safe = std::max(r, isco_radius());
		const Scalar gm = g * m;
		const Scalar r_g = gm / (c * c);
		const Scalar a = config_.spin_parameter;

		const Scalar num = std::sqrt(gm);
		const Scalar den = r_safe * std::sqrt(r_safe) + (a / r_g) * std::sqrt(r_g * r_g * r_g);
		return num / den;
	}

	[[nodiscard]] Scalar specific_energy(Scalar r) const noexcept {
		const Scalar m = config_.mass;
		const Scalar c = config_.speed_of_light;
		const Scalar g = config_.gravitational_constant;
		const Scalar r_g = g * m / (c * c);
		const Scalar x = std::sqrt(r / r_g);
		const Scalar a_star = dimensionless_spin();

		const Scalar x2 = x * x;
		const Scalar x3 = x2 * x;
		const Scalar denom_term = x3 - static_cast<Scalar>(3.0) * x + static_cast<Scalar>(2.0) * a_star;
		if (denom_term <= static_cast<Scalar>(1e-24)) {
			return static_cast<Scalar>(1.0);
		}

		const Scalar num = x3 - static_cast<Scalar>(2.0) * x + a_star;
		const Scalar den = std::sqrt(x3 * denom_term);
		return num / den;
	}

	[[nodiscard]] Scalar specific_angular_momentum(Scalar r) const noexcept {
		const Scalar m = config_.mass;
		const Scalar c = config_.speed_of_light;
		const Scalar g = config_.gravitational_constant;
		const Scalar r_g = g * m / (c * c);
		const Scalar x = std::sqrt(r / r_g);
		const Scalar a_star = dimensionless_spin();

		const Scalar x2 = x * x;
		const Scalar x3 = x2 * x;
		const Scalar x4 = x3 * x;
		const Scalar denom_term = x3 - static_cast<Scalar>(3.0) * x + static_cast<Scalar>(2.0) * a_star;
		if (denom_term <= static_cast<Scalar>(1e-24)) {
			return static_cast<Scalar>(0.0);
		}

		const Scalar num = std::sqrt(g * m) * (x4 - static_cast<Scalar>(2.0) * a_star * x + a_star * a_star);
		const Scalar den = std::sqrt(x3 * denom_term);
		return num / den;
	}

	[[nodiscard]] Core::FourVector<Scalar> four_velocity(Scalar r) const noexcept {
		const Scalar m = config_.mass;
		const Scalar c = config_.speed_of_light;
		const Scalar g = config_.gravitational_constant;
		const Scalar r_g = g * m / (c * c);
		const Scalar x = std::sqrt(r / r_g);
		const Scalar a_star = dimensionless_spin();

		const Scalar x3 = x * x * x;
		const Scalar denom_term = x3 - static_cast<Scalar>(3.0) * x + static_cast<Scalar>(2.0) * a_star;
		if (denom_term <= static_cast<Scalar>(1e-24)) {
			return Core::FourVector<Scalar>(static_cast<Scalar>(1.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0));
		}

		const Scalar u_t = (x3 + a_star) / std::sqrt(x3 * denom_term);
		const Scalar omega = keplerian_omega(r);
		const Scalar u_phi = omega * u_t;

		return Core::FourVector<Scalar>(u_t, static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), u_phi);
	}

	[[nodiscard]] Scalar radiative_flux(Scalar r) const noexcept {
		const Scalar r_isco = isco_radius();
		if (r < r_isco) {
			return static_cast<Scalar>(0.0);
		}

		const Scalar m = config_.mass;
		const Scalar c = config_.speed_of_light;
		const Scalar g = config_.gravitational_constant;
		const Scalar r_g = g * m / (c * c);
		const Scalar x = std::sqrt(r / r_g);
		const Scalar a_star = dimensionless_spin();

		const Scalar f_val = page_thorne_f(x);
		const Scalar x3 = x * x * x;
		const Scalar denom_factor = x * (x3 + a_star);

		if (denom_factor <= static_cast<Scalar>(1e-24)) {
			return static_cast<Scalar>(0.0);
		}

		const Scalar r3 = r * r * r;
		const Scalar m_dot = config_.accretion_rate;
		const Scalar pi = std::numbers::pi_v<Scalar>;

		const Scalar factor = (static_cast<Scalar>(3.0) * g * m * m_dot) / (static_cast<Scalar>(8.0) * pi * r3);
		const Scalar flux_val = factor * (f_val / denom_factor);

		return std::max(static_cast<Scalar>(0.0), flux_val);
	}

	[[nodiscard]] Scalar effective_temperature(Scalar r) const noexcept {
		const Scalar f = radiative_flux(r);
		if (f <= static_cast<Scalar>(0.0)) {
			return static_cast<Scalar>(0.0);
		}
		const Scalar sigma = config_.stefan_boltzmann;
		return std::pow(f / sigma, static_cast<Scalar>(0.25));
	}

	[[nodiscard]] Scalar disk_scale_height(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, isco_radius());
		const Scalar t_eff = effective_temperature(r_safe);
		const Scalar omega = keplerian_omega(r_safe);
		const Scalar k_b = static_cast<Scalar>(Core::PhysicalConstants<double>::BOLTZMANN_CONSTANT);
		const Scalar m_p = static_cast<Scalar>(Core::PhysicalConstants<double>::PROTON_MASS);

		const Scalar c_s = std::sqrt(k_b * t_eff / (static_cast<Scalar>(0.5) * m_p));
		const Scalar h = (omega > static_cast<Scalar>(1e-30)) ? (c_s / omega) : static_cast<Scalar>(0.0);
		return std::max(static_cast<Scalar>(1e-3), h);
	}

	[[nodiscard]] Scalar surface_density(Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, isco_radius());
		const Scalar m_dot = config_.accretion_rate;
		const Scalar alpha = config_.alpha_viscosity;
		const Scalar c_s = std::sqrt(static_cast<Scalar>(Core::PhysicalConstants<double>::BOLTZMANN_CONSTANT) * effective_temperature(r_safe) / (static_cast<Scalar>(0.5 * Core::PhysicalConstants<double>::PROTON_MASS)));
		const Scalar h = disk_scale_height(r_safe);
		const Scalar nu = alpha * c_s * h;
		const Scalar pi = std::numbers::pi_v<Scalar>;
		if (nu <= static_cast<Scalar>(1e-30)) return static_cast<Scalar>(0.0);
		return m_dot / (static_cast<Scalar>(3.0) * pi * nu);
	}

	[[nodiscard]] Scalar radiative_efficiency() const noexcept {
		const Scalar r_isco = isco_radius();
		const Scalar e_isco = specific_energy(r_isco);
		return static_cast<Scalar>(1.0) - e_isco;
	}

	[[nodiscard]] Optics::ContinuousSpectrum<Scalar> sample_spectrum(Scalar r) const noexcept {
		const Scalar t_eff = effective_temperature(r);
		if (t_eff <= static_cast<Scalar>(0.0)) {
			return Optics::ContinuousSpectrum<Scalar>(static_cast<Scalar>(0.0));
		}
		return Optics::ContinuousSpectrum<Scalar>::make_blackbody(t_eff);
	}

	[[nodiscard]] Scalar doppler_factor(
		Scalar r,
		const Core::FourVector<Scalar>& p_photon,
		const Core::FourVector<Scalar>& u_obs,
		const Core::MetricTensor<Scalar>& g_obs
	) const noexcept {
		const auto u_emit = four_velocity(r);
		const auto g_emit = metric_.metric_tensor(Core::FourVector<Scalar>(static_cast<Scalar>(0.0), r, std::numbers::pi_v<Scalar> * static_cast<Scalar>(0.5), static_cast<Scalar>(0.0)));

		Scalar num = static_cast<Scalar>(0.0);
		Scalar den = static_cast<Scalar>(0.0);

		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				num += g_obs(mu, nu) * p_photon(mu) * u_obs(nu);
				den += g_emit(mu, nu) * p_photon(mu) * u_emit(nu);
			}
		}

		if (std::abs(den) < static_cast<Scalar>(1e-30)) {
			return static_cast<Scalar>(1.0);
		}
		return num / den;
	}
};

}
