#pragma once

#include "relativistic/hydro/eos.hpp"
#include "relativistic/core/constants.hpp"
#include <vector>
#include <span>
#include <array>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <optional>
#include <concepts>
#include <cstdint>

namespace Relativistic::Hydro {

template <typename Scalar = double>
struct TOVConfig {
	Scalar gravitational_constant{static_cast<Scalar>(Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT)};
	Scalar speed_of_light{static_cast<Scalar>(Core::PhysicalConstants<double>::SPEED_OF_LIGHT)};
	Scalar initial_step_meters{static_cast<Scalar>(1.0)};
	Scalar min_step_meters{static_cast<Scalar>(1e-4)};
	Scalar max_step_meters{static_cast<Scalar>(20000.0)};
	Scalar max_radius_meters{static_cast<Scalar>(1.0e8)};
	Scalar rtol{static_cast<Scalar>(1e-10)};
	Scalar atol{static_cast<Scalar>(1e-14)};
	Scalar pressure_floor_fraction{static_cast<Scalar>(1e-14)};
};

template <typename Scalar = double>
struct alignas(64) TOVStarProfile {
	Scalar central_density{static_cast<Scalar>(0.0)};
	Scalar central_pressure{static_cast<Scalar>(0.0)};
	Scalar gravitational_mass{static_cast<Scalar>(0.0)};
	Scalar baryonic_mass{static_cast<Scalar>(0.0)};
	Scalar surface_radius{static_cast<Scalar>(0.0)};
	Scalar surface_redshift{static_cast<Scalar>(0.0)};
	Scalar compactness{static_cast<Scalar>(0.0)};
	Scalar central_redshift{static_cast<Scalar>(0.0)};
	Scalar gravitational_mass_solar{static_cast<Scalar>(0.0)};
	Scalar baryonic_mass_solar{static_cast<Scalar>(0.0)};
	Scalar surface_radius_km{static_cast<Scalar>(0.0)};

	std::vector<Scalar> r{};
	std::vector<Scalar> mass_enclosed{};
	std::vector<Scalar> baryonic_mass_enclosed{};
	std::vector<Scalar> pressure{};
	std::vector<Scalar> density{};
	std::vector<Scalar> energy_density{};
	std::vector<Scalar> metric_phi{};
	std::vector<Scalar> sound_speed_squared{};
	bool converged{false};
};

template <typename Scalar = double>
struct alignas(32) MassRadiusPoint {
	Scalar central_density{static_cast<Scalar>(0.0)};
	Scalar central_pressure{static_cast<Scalar>(0.0)};
	Scalar gravitational_mass{static_cast<Scalar>(0.0)};
	Scalar gravitational_mass_solar{static_cast<Scalar>(0.0)};
	Scalar baryonic_mass_solar{static_cast<Scalar>(0.0)};
	Scalar surface_radius_km{static_cast<Scalar>(0.0)};
	Scalar compactness{static_cast<Scalar>(0.0)};
	Scalar surface_redshift{static_cast<Scalar>(0.0)};
	bool is_dynamically_stable{true};
};

template <typename Scalar = double>
struct MassRadiusCurve {
	std::vector<MassRadiusPoint<Scalar>> points{};
	Scalar maximum_mass_solar{static_cast<Scalar>(0.0)};
	Scalar radius_at_maximum_mass_km{static_cast<Scalar>(0.0)};
	Scalar central_density_at_max_mass{static_cast<Scalar>(0.0)};
	Scalar radius_at_1_4_solar_km{static_cast<Scalar>(0.0)};
	std::string eos_name{"Unknown"};
};

template <typename EOS, typename Scalar = double>
class TOVSolver {
private:
	const EOS& eos_;
	TOVConfig<Scalar> config_{};

public:
	explicit constexpr TOVSolver(const EOS& eos, const TOVConfig<Scalar>& config = {}) noexcept
		: eos_(eos), config_(config) {}

	[[nodiscard]] constexpr const TOVConfig<Scalar>& config() const noexcept {
		return config_;
	}

	[[nodiscard]] constexpr const EOS& eos() const noexcept {
		return eos_;
	}

	[[nodiscard]] TOVStarProfile<Scalar> solve_star_from_central_density(
		Scalar central_rho,
		size_t reserve_steps = 10000
	) const {
		TOVStarProfile<Scalar> profile{};
		profile.central_density = central_rho;
		profile.central_pressure = eos_.pressure(central_rho);

		const Scalar c = config_.speed_of_light;
		const Scalar c2 = c * c;
		const Scalar g = config_.gravitational_constant;
		const Scalar solar_mass = static_cast<Scalar>(Core::PhysicalConstants<double>::SOLAR_MASS);

		const Scalar p_c = profile.central_pressure;
		const Scalar eps_c = eos_.energy_density(central_rho);
		const Scalar rho_c = central_rho;

		const Scalar p_surface_cutoff = p_c * config_.pressure_floor_fraction;

		profile.r.reserve(reserve_steps);
		profile.mass_enclosed.reserve(reserve_steps);
		profile.baryonic_mass_enclosed.reserve(reserve_steps);
		profile.pressure.reserve(reserve_steps);
		profile.density.reserve(reserve_steps);
		profile.energy_density.reserve(reserve_steps);
		profile.metric_phi.reserve(reserve_steps);
		profile.sound_speed_squared.reserve(reserve_steps);

		Scalar r = config_.initial_step_meters;
		const Scalar r2 = r * r;
		const Scalar r3 = r2 * r;
		const Scalar pi = std::numbers::pi_v<Scalar>;

		Scalar m = (static_cast<Scalar>(4.0 / 3.0) * pi * r3 * eps_c) / c2;
		Scalar m_b = static_cast<Scalar>(4.0 / 3.0) * pi * r3 * rho_c;
		Scalar p = p_c - (static_cast<Scalar>(2.0 / 3.0) * pi * g * (eps_c / c2 + p_c / c2) * (eps_c / c2 + static_cast<Scalar>(3.0) * p_c / c2)) * r2;
		Scalar phi = (static_cast<Scalar>(2.0 / 3.0) * pi * (g / c2) * (eps_c / c2 + static_cast<Scalar>(3.0) * p_c / c2)) * r2;

		profile.r.push_back(static_cast<Scalar>(0.0));
		profile.mass_enclosed.push_back(static_cast<Scalar>(0.0));
		profile.baryonic_mass_enclosed.push_back(static_cast<Scalar>(0.0));
		profile.pressure.push_back(p_c);
		profile.density.push_back(rho_c);
		profile.energy_density.push_back(eps_c);
		profile.metric_phi.push_back(static_cast<Scalar>(0.0));
		profile.sound_speed_squared.push_back(eos_.sound_speed_squared(rho_c, p_c));

		auto tov_derivatives = [&](Scalar cur_r, Scalar cur_m, Scalar cur_mb, Scalar cur_p) noexcept -> std::array<Scalar, 4> {
			static_cast<void>(cur_mb);
			if (cur_p <= static_cast<Scalar>(0.0)) {
				return {static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0)};
			}

			const Scalar cur_rho = eos_.density_from_pressure(cur_p);
			const Scalar cur_eps = eos_.energy_density(cur_rho, cur_p);

			const Scalar dm_dr = static_cast<Scalar>(4.0) * pi * cur_r * cur_r * cur_eps / c2;
			const Scalar metric_factor = static_cast<Scalar>(1.0) - (static_cast<Scalar>(2.0) * g * cur_m) / (c2 * cur_r);
			const Scalar safe_metric = std::max(metric_factor, static_cast<Scalar>(1e-12));

			const Scalar num_dp = g * (cur_eps / c2 + cur_p / c2) * (cur_m + static_cast<Scalar>(4.0) * pi * cur_r * cur_r * cur_r * cur_p / c2);
			const Scalar den_dp = cur_r * cur_r * safe_metric;
			const Scalar dp_dr = -num_dp / den_dp;

			const Scalar dphi_dr = -dp_dr / ((cur_eps + cur_p > static_cast<Scalar>(0.0)) ? (cur_eps + cur_p) : static_cast<Scalar>(1.0));
			const Scalar dmb_dr = (static_cast<Scalar>(4.0) * pi * cur_r * cur_r * cur_rho) / std::sqrt(safe_metric);

			return {dm_dr, dmb_dr, dp_dr, dphi_dr};
		};

		Scalar dr = config_.initial_step_meters;

		while (r < config_.max_radius_meters && p > p_surface_cutoff) {
			const Scalar cur_rho = eos_.density_from_pressure(p);
			const Scalar cur_eps = eos_.energy_density(cur_rho, p);

			profile.r.push_back(r);
			profile.mass_enclosed.push_back(m);
			profile.baryonic_mass_enclosed.push_back(m_b);
			profile.pressure.push_back(p);
			profile.density.push_back(cur_rho);
			profile.energy_density.push_back(cur_eps);
			profile.metric_phi.push_back(phi);
			profile.sound_speed_squared.push_back(eos_.sound_speed_squared(cur_rho, p));

			const auto k1 = tov_derivatives(r, m, m_b, p);

			const Scalar r_mid1 = r + static_cast<Scalar>(0.5) * dr;
			const Scalar m_mid1 = m + static_cast<Scalar>(0.5) * dr * k1[0];
			const Scalar mb_mid1 = m_b + static_cast<Scalar>(0.5) * dr * k1[1];
			const Scalar p_mid1 = p + static_cast<Scalar>(0.5) * dr * k1[2];
			const auto k2 = tov_derivatives(r_mid1, m_mid1, mb_mid1, p_mid1);

			const Scalar r_mid2 = r + static_cast<Scalar>(0.5) * dr;
			const Scalar m_mid2 = m + static_cast<Scalar>(0.5) * dr * k2[0];
			const Scalar mb_mid2 = m_b + static_cast<Scalar>(0.5) * dr * k2[1];
			const Scalar p_mid2 = p + static_cast<Scalar>(0.5) * dr * k2[2];
			const auto k3 = tov_derivatives(r_mid2, m_mid2, mb_mid2, p_mid2);

			const Scalar r_end = r + dr;
			const Scalar m_end = m + dr * k3[0];
			const Scalar mb_end = m_b + dr * k3[1];
			const Scalar p_end = p + dr * k3[2];
			const auto k4 = tov_derivatives(r_end, m_end, mb_end, p_end);

			const Scalar sixth_dr = dr / static_cast<Scalar>(6.0);
			const Scalar next_m = m + sixth_dr * (k1[0] + static_cast<Scalar>(2.0) * k2[0] + static_cast<Scalar>(2.0) * k3[0] + k4[0]);
			const Scalar next_mb = m_b + sixth_dr * (k1[1] + static_cast<Scalar>(2.0) * k2[1] + static_cast<Scalar>(2.0) * k3[1] + k4[1]);
			const Scalar next_p = p + sixth_dr * (k1[2] + static_cast<Scalar>(2.0) * k2[2] + static_cast<Scalar>(2.0) * k3[2] + k4[2]);
			const Scalar next_phi = phi + sixth_dr * (k1[3] + static_cast<Scalar>(2.0) * k2[3] + static_cast<Scalar>(2.0) * k3[3] + k4[3]);

			if (next_p <= static_cast<Scalar>(0.0)) {
				const Scalar frac = (static_cast<Scalar>(0.0) - p) / (next_p - p);
				r += frac * dr;
				m += frac * (next_m - m);
				m_b += frac * (next_mb - m_b);
				phi += frac * (next_phi - phi);
				p = static_cast<Scalar>(0.0);
				break;
			}

			r += dr;
			m = next_m;
			m_b = next_mb;
			p = next_p;
			phi = next_phi;

			if (p > static_cast<Scalar>(0.0)) {
				const Scalar abs_dp_dr = std::abs(k1[2]);
				const Scalar h_p = (abs_dp_dr > static_cast<Scalar>(1e-30)) ? (p / abs_dp_dr) : config_.max_step_meters;
				const Scalar dr_optimal = std::clamp(static_cast<Scalar>(0.05) * h_p, config_.min_step_meters, config_.max_step_meters);
				dr = std::clamp(dr_optimal, static_cast<Scalar>(0.5) * dr, static_cast<Scalar>(1.5) * dr);
			}
		}

		profile.surface_radius = r;
		profile.gravitational_mass = m;
		profile.baryonic_mass = m_b;
		profile.gravitational_mass_solar = m / solar_mass;
		profile.baryonic_mass_solar = m_b / solar_mass;
		profile.surface_radius_km = r * static_cast<Scalar>(0.001);

		const Scalar metric_surf = static_cast<Scalar>(1.0) - (static_cast<Scalar>(2.0) * g * m) / (c2 * r);
		const Scalar safe_metric_surf = std::max(metric_surf, static_cast<Scalar>(1e-12));
		profile.compactness = (g * m) / (c2 * r);
		profile.surface_redshift = static_cast<Scalar>(1.0) / std::sqrt(safe_metric_surf) - static_cast<Scalar>(1.0);

		const Scalar phi_surf_analytical = static_cast<Scalar>(0.5) * std::log(safe_metric_surf);
		const Scalar phi_offset = phi_surf_analytical - phi;

		for (auto& val : profile.metric_phi) {
			val += phi_offset;
		}

		profile.central_redshift = std::exp(-profile.metric_phi[0]) - static_cast<Scalar>(1.0);
		profile.converged = (r < config_.max_radius_meters);

		return profile;
	}

	[[nodiscard]] MassRadiusCurve<Scalar> compute_mass_radius_curve(
		Scalar min_central_rho,
		Scalar max_central_rho,
		size_t num_samples = 120
	) const {
		MassRadiusCurve<Scalar> curve{};
		curve.points.reserve(num_samples);

		const Scalar log_min = std::log10(min_central_rho);
		const Scalar log_max = std::log10(max_central_rho);
		const Scalar step = (log_max - log_min) / static_cast<Scalar>(num_samples - 1);

		Scalar max_mass = static_cast<Scalar>(0.0);
		Scalar r_at_max = static_cast<Scalar>(0.0);
		Scalar rho_at_max = static_cast<Scalar>(0.0);

		for (size_t i = 0; i < num_samples; ++i) {
			const Scalar log_rho = log_min + static_cast<Scalar>(i) * step;
			const Scalar central_rho = std::pow(static_cast<Scalar>(10.0), log_rho);

			const auto prof = solve_star_from_central_density(central_rho);

			if (prof.converged && prof.gravitational_mass_solar > static_cast<Scalar>(0.01)) {
				MassRadiusPoint<Scalar> pt;
				pt.central_density = central_rho;
				pt.central_pressure = prof.central_pressure;
				pt.gravitational_mass = prof.gravitational_mass;
				pt.gravitational_mass_solar = prof.gravitational_mass_solar;
				pt.baryonic_mass_solar = prof.baryonic_mass_solar;
				pt.surface_radius_km = prof.surface_radius_km;
				pt.compactness = prof.compactness;
				pt.surface_redshift = prof.surface_redshift;
				pt.is_dynamically_stable = true;

				if (prof.gravitational_mass_solar > max_mass) {
					max_mass = prof.gravitational_mass_solar;
					r_at_max = prof.surface_radius_km;
					rho_at_max = central_rho;
				}

				curve.points.push_back(pt);
			}
		}

		bool passed_peak = false;
		for (size_t i = 1; i < curve.points.size(); ++i) {
			if (curve.points[i].gravitational_mass_solar < curve.points[i - 1].gravitational_mass_solar && curve.points[i - 1].gravitational_mass_solar >= static_cast<Scalar>(0.98) * max_mass) {
				passed_peak = true;
			}
			if (passed_peak) {
				curve.points[i].is_dynamically_stable = false;
			}
		}

		curve.maximum_mass_solar = max_mass;
		curve.radius_at_maximum_mass_km = r_at_max;
		curve.central_density_at_max_mass = rho_at_max;

		for (size_t i = 1; i < curve.points.size(); ++i) {
			const auto& p0 = curve.points[i - 1];
			const auto& p1 = curve.points[i];
			if (p0.is_dynamically_stable && (p0.gravitational_mass_solar <= static_cast<Scalar>(1.4)) && (p1.gravitational_mass_solar >= static_cast<Scalar>(1.4))) {
				const Scalar frac = (static_cast<Scalar>(1.4) - p0.gravitational_mass_solar) / (p1.gravitational_mass_solar - p0.gravitational_mass_solar);
				curve.radius_at_1_4_solar_km = p0.surface_radius_km + frac * (p1.surface_radius_km - p0.surface_radius_km);
				break;
			}
		}

		return curve;
	}
};

}
