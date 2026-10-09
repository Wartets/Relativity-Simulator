#pragma once

#include "relativistic/core/math/tensor.hpp"
#include "relativistic/metrics/kerr.hpp"
#include "relativistic/metrics/kerr_newman.hpp"
#include "relativistic/metrics/reissner_nordstrom.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/metrics/schwarzschild_de_sitter.hpp"
#include "relativistic/observer/observer_tetrad.hpp"
#include "relativistic/optics/disk_thermal_profile.hpp"
#include "relativistic/render/accretion_disk/accretion_disk_model.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <utility>
#include <vector>

namespace Relativistic::Optics {

enum class RayTermination : uint32_t {
	Unresolved = 0,
	HorizonAbsorbed = 1,
	SkyEscape = 2,
	StepBudgetExhausted = 3,
	UnsupportedMetric = 4,
	InvalidState = 5
};

[[nodiscard]] inline const char* ray_termination_name(RayTermination termination) noexcept {
	switch (termination) {
		case RayTermination::HorizonAbsorbed: return "Event Horizon";
		case RayTermination::SkyEscape: return "Celestial Sky";
		case RayTermination::StepBudgetExhausted: return "Step Budget Exhausted";
		case RayTermination::UnsupportedMetric: return "Unsupported Metric";
		case RayTermination::InvalidState: return "Invalid Integration State";
		case RayTermination::Unresolved:
		default: return "Unresolved";
	}
}

struct RayProbeQuery {
	uint32_t metric_type{1};
	double mass{1.0};
	double spin{0.0};
	double charge{0.0};
	double cosmological_lambda{0.0};
	double observer_radius{50.0};
	double observer_theta{1.5707963267948966};
	double observer_phi{0.0};
	std::array<double, 3> direction{1.0, 0.0, 0.0};
	double escape_radius{100.0};
	uint32_t max_steps{2048};
	double step_size_factor{0.45};
	double min_step_size{0.0008};
	double max_step_size{38.0};
	double far_field_step_scale{1.0};
	double pole_guard_precision_scale{1.0};
	bool disk_enabled{true};
	double disk_inner_radius_scale{1.0};
	double disk_outer_radius_mass_units{24.0};
	double disk_peak_temperature_k{12546.0};
	double disk_floor_temperature_k{1200.0};
	double disk_temperature_exponent{0.75};
	double disk_zero_torque_strength{1.0};
	double disk_temperature_normalization{1.0};
	uint32_t pixel_x{0};
	uint32_t pixel_y{0};

	[[nodiscard]] bool operator==(const RayProbeQuery&) const noexcept = default;
};

struct RayProbeResult {
	bool valid{false};
	RayTermination termination{RayTermination::Unresolved};
	bool disk_hit{false};
	uint32_t disk_crossings{0};
	uint32_t iterations{0};
	uint32_t pixel_x{0};
	uint32_t pixel_y{0};
	double spectral_shift_g{1.0};
	double static_redshift_factor{1.0};
	double energy{0.0};
	double angular_momentum_z{0.0};
	double impact_parameter{0.0};
	double total_impact_parameter{0.0};
	double carter_constant{0.0};
	double carter_eta{0.0};
	double emission_r{0.0};
	double emission_theta{0.0};
	double emission_phi{0.0};
	double terminal_r{0.0};
	double terminal_theta{0.0};
	double terminal_phi{0.0};
	double minimum_radius{0.0};
	double disk_radius{0.0};
	double disk_azimuth{0.0};
	double disk_temperature_k{0.0};
	double disk_observed_temperature_k{0.0};
	double disk_emitter_shift{1.0};
	double deflection_angle{0.0};
	uint32_t image_order{0};
	double horizon_radius{0.0};
	double disk_inner_radius{0.0};
	double disk_outer_radius{0.0};
	double affine_length{0.0};
	std::vector<double> path_x{};
	std::vector<double> path_y{};
	std::vector<double> path_z{};
	std::vector<double> path_r{};
};

class GeodesicRayProbe {
private:
	struct SphericalBasis {
		std::array<double, 3> radial;
		std::array<double, 3> polar;
		std::array<double, 3> azimuthal;
	};

	[[nodiscard]] static SphericalBasis make_basis(double theta, double phi) noexcept {
		const double sin_t = std::sin(theta);
		const double cos_t = std::cos(theta);
		const double sin_p = std::sin(phi);
		const double cos_p = std::cos(phi);
		return SphericalBasis{
			{sin_t * cos_p, sin_t * sin_p, cos_t},
			{cos_t * cos_p, cos_t * sin_p, -sin_t},
			{-sin_p, cos_p, 0.0}
		};
	}

	[[nodiscard]] static double dot3(const std::array<double, 3>& a, const std::array<double, 3>& b) noexcept {
		return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
	}

	[[nodiscard]] static std::array<double, 3> normalized_direction(const std::array<double, 3>& direction) noexcept {
		const double length = std::sqrt(dot3(direction, direction));
		if (length <= 1e-14) {
			return {1.0, 0.0, 0.0};
		}
		return {direction[0] / length, direction[1] / length, direction[2] / length};
	}

	static void append_path_point(RayProbeResult& result, double r, double theta, double phi) {
		const double sin_t = std::sin(theta);
		result.path_x.push_back(r * sin_t * std::cos(phi));
		result.path_y.push_back(r * sin_t * std::sin(phi));
		result.path_z.push_back(r * std::cos(theta));
		result.path_r.push_back(r);
	}

	[[nodiscard]] static double path_deflection(const std::array<double, 3>& initial, const RayProbeResult& result) noexcept {
		const size_t count = result.path_x.size();
		if (count < 2) {
			return 0.0;
		}
		const std::array<double, 3> last{
			result.path_x[count - 1] - result.path_x[count - 2],
			result.path_y[count - 1] - result.path_y[count - 2],
			result.path_z[count - 1] - result.path_z[count - 2]
		};
		const double length = std::sqrt(dot3(last, last));
		if (length <= 1e-14) {
			return 0.0;
		}
		return std::acos(std::clamp(dot3(initial, last) / length, -1.0, 1.0));
	}

	[[nodiscard]] static RayProbeResult trace_flat(const RayProbeQuery& query, RayProbeResult result) {
		constexpr double pi = std::numbers::pi_v<double>;
		const double theta_obs = std::clamp(query.observer_theta, 0.001, pi - 0.001);
		const double r_obs = std::max(query.observer_radius, 1e-6);
		const auto basis = make_basis(theta_obs, query.observer_phi);
		const auto direction = normalized_direction(query.direction);
		const std::array<double, 3> origin{r_obs * basis.radial[0], r_obs * basis.radial[1], r_obs * basis.radial[2]};

		const double b = dot3(origin, direction);
		const double c = dot3(origin, origin) - query.escape_radius * query.escape_radius;
		const double discriminant = std::max(b * b - c, 0.0);
		const double travel = std::max(-b + std::sqrt(discriminant), 0.0);
		const std::array<double, 3> end{origin[0] + travel * direction[0], origin[1] + travel * direction[1], origin[2] + travel * direction[2]};
		const double end_r = std::sqrt(dot3(end, end));

		const double n_theta = dot3(direction, basis.polar);
		const double n_phi = dot3(direction, basis.azimuthal);
		const double sin_to = std::sin(theta_obs);
		const double cos_to = std::cos(theta_obs);
		const double lz = -n_phi * r_obs * sin_to;
		const double p_theta = -n_theta * r_obs;
		const double carter = p_theta * p_theta + cos_to * cos_to * lz * lz / std::max(sin_to * sin_to, 1e-12);

		result.valid = true;
		result.termination = RayTermination::SkyEscape;
		result.iterations = 1;
		result.energy = 1.0;
		result.angular_momentum_z = lz;
		result.impact_parameter = lz;
		result.carter_constant = carter;
		result.carter_eta = carter;
		result.total_impact_parameter = std::sqrt(std::max(carter + lz * lz, 0.0));
		result.spectral_shift_g = 1.0;
		result.static_redshift_factor = 1.0;
		result.terminal_r = end_r;
		result.terminal_theta = (end_r > 1e-12) ? std::acos(std::clamp(end[2] / end_r, -1.0, 1.0)) : 0.0;
		result.terminal_phi = std::atan2(end[1], end[0]);
		result.emission_r = result.terminal_r;
		result.emission_theta = result.terminal_theta;
		result.emission_phi = result.terminal_phi;
		result.minimum_radius = std::min(r_obs, end_r);
		result.affine_length = travel;
		append_path_point(result, r_obs, theta_obs, query.observer_phi);
		append_path_point(result, result.terminal_r, result.terminal_theta, result.terminal_phi);
		return result;
	}

	template <typename MetricType>
	[[nodiscard]] static RayProbeResult integrate(
		const MetricType& metric,
		const RayProbeQuery& query,
		double horizon_radius,
		bool has_disk,
		bool lapse_guard,
		double spin,
		RayProbeResult result
	) {
		constexpr double pi = std::numbers::pi_v<double>;
		constexpr double half_pi = pi * 0.5;
		const double m = std::max(query.mass, 1e-4);
		const double rs = 2.0 * m;
		const double r_obs = std::max(query.observer_radius, horizon_radius * 1.02);
		const double theta_obs = std::clamp(query.observer_theta, 0.001, pi - 0.001);
		const double phi_obs = query.observer_phi;
		const auto basis = make_basis(theta_obs, phi_obs);
		const auto direction = normalized_direction(query.direction);
		const double n_r = dot3(direction, basis.radial);
		const double n_theta = dot3(direction, basis.polar);
		const double n_phi = dot3(direction, basis.azimuthal);

		Core::FourVector<double> x(0.0, r_obs, theta_obs, phi_obs);
		const auto tetrad = Observer::ObserverTetrad<double>::make_zamo(metric, x);
		Core::FourVector<double> u = tetrad.construct_light_ray(-n_r, -n_theta, -n_phi);

		const auto g_obs = metric.metric_tensor(x);
		const double p_t = g_obs(0, 0) * u(0) + g_obs(0, 3) * u(3);
		const double p_phi = g_obs(3, 0) * u(0) + g_obs(3, 3) * u(3);
		const double p_theta = g_obs(2, 2) * u(2);
		const double energy = -p_t;
		if (!std::isfinite(energy) || !std::isfinite(p_phi) || !std::isfinite(p_theta) || energy <= 1e-12) {
			result.termination = RayTermination::InvalidState;
			return result;
		}

		const double sin_to = std::sin(theta_obs);
		const double cos_to = std::cos(theta_obs);
		const double carter = p_theta * p_theta + cos_to * cos_to * (-spin * spin * energy * energy + p_phi * p_phi / std::max(sin_to * sin_to, 1e-12));
		const double xi = p_phi / energy;
		const double eta = carter / (energy * energy);

		result.energy = energy;
		result.angular_momentum_z = p_phi;
		result.impact_parameter = xi;
		result.carter_constant = carter;
		result.carter_eta = eta;
		result.total_impact_parameter = std::sqrt(std::max(eta + xi * xi, 0.0));
		result.static_redshift_factor = 1.0 / energy;
		result.horizon_radius = horizon_radius;

		const double isco = DiskThermalProfile::kerr_isco_radius(m, spin);
		const double disk_inner = isco * std::max(query.disk_inner_radius_scale, 1.0);
		const double disk_outer = std::max(query.disk_outer_radius_mass_units * m, disk_inner * 1.05);
		const bool disk_active = has_disk && query.disk_enabled;
		if (disk_active) {
			result.disk_inner_radius = disk_inner;
			result.disk_outer_radius = disk_outer;
		}

		const auto compute_acceleration = [&](const Core::FourVector<double>& xs, const Core::FourVector<double>& us) noexcept -> Core::FourVector<double> {
			const auto gamma = metric.christoffel_symbols(xs);
			Core::FourVector<double> acceleration;
			acceleration.zero();
			for (size_t mu = 0; mu < 4; ++mu) {
				double sum = 0.0;
				for (size_t alpha = 0; alpha < 4; ++alpha) {
					const double u_a = us(alpha);
					if (u_a == 0.0) continue;
					sum -= gamma(mu, alpha, alpha) * u_a * u_a;
					for (size_t beta = alpha + 1; beta < 4; ++beta) {
						const double u_b = us(beta);
						if (u_b != 0.0) {
							sum -= 2.0 * gamma(mu, alpha, beta) * u_a * u_b;
						}
					}
				}
				acceleration(mu) = sum;
			}
			return acceleration;
		};

		const uint32_t max_steps = std::max(query.max_steps, 1U);
		const uint32_t stride = std::max(1U, max_steps / 384U);
		result.path_x.reserve(400);
		result.path_y.reserve(400);
		result.path_z.reserve(400);
		result.path_r.reserve(400);
		append_path_point(result, x(1), x(2), x(3));

		RayTermination fate = RayTermination::StepBudgetExhausted;
		double minimum_radius = x(1);
		double affine = 0.0;
		uint32_t iterations = 0;
		double disk_g = 1.0;

		for (uint32_t step = 0; step < max_steps; ++step) {
			iterations = step + 1;

			if (x(1) <= horizon_radius * 1.0001) {
				fate = RayTermination::HorizonAbsorbed;
				break;
			}
			if (x(1) >= query.escape_radius) {
				fate = RayTermination::SkyEscape;
				break;
			}
			if (lapse_guard) {
				const auto g_local = metric.metric_tensor(x);
				if (-g_local(0, 0) <= 1e-9) {
					fate = (x(1) < r_obs) ? RayTermination::HorizonAbsorbed : RayTermination::SkyEscape;
					break;
				}
			}

			const double current_r = x(1);
			const double r_scale = std::max(current_r - horizon_radius, 0.02 * m);
			const double pole_guard = std::clamp(std::abs(std::sin(x(2))) * 12.0 * query.pole_guard_precision_scale, 0.02, 1.0);
			const double far_field = 1.0 + (query.far_field_step_scale - 1.0) * std::clamp((current_r - 20.0 * horizon_radius) / (80.0 * horizon_radius), 0.0, 1.0);
			const double dt = -std::clamp(query.step_size_factor * std::sqrt(current_r * r_scale) * far_field, query.min_step_size, query.max_step_size * query.far_field_step_scale) * pole_guard;

			const double prev_r = x(1);
			const double prev_theta = x(2);
			const double prev_phi = x(3);

			const auto k1_x = u;
			const auto k1_u = compute_acceleration(x, u);

			Core::FourVector<double> x2 = x;
			Core::FourVector<double> u2 = u;
			for (size_t i = 0; i < 4; ++i) {
				x2(i) += 0.5 * dt * k1_x(i);
				u2(i) += 0.5 * dt * k1_u(i);
			}
			const auto k2_x = u2;
			const auto k2_u = compute_acceleration(x2, u2);

			Core::FourVector<double> x3 = x;
			Core::FourVector<double> u3 = u;
			for (size_t i = 0; i < 4; ++i) {
				x3(i) += 0.5 * dt * k2_x(i);
				u3(i) += 0.5 * dt * k2_u(i);
			}
			const auto k3_x = u3;
			const auto k3_u = compute_acceleration(x3, u3);

			Core::FourVector<double> x4 = x;
			Core::FourVector<double> u4 = u;
			for (size_t i = 0; i < 4; ++i) {
				x4(i) += dt * k3_x(i);
				u4(i) += dt * k3_u(i);
			}
			const auto k4_x = u4;
			const auto k4_u = compute_acceleration(x4, u4);

			const double sixth = dt / 6.0;
			for (size_t i = 0; i < 4; ++i) {
				x(i) += sixth * (k1_x(i) + 2.0 * k2_x(i) + 2.0 * k3_x(i) + k4_x(i));
				u(i) += sixth * (k1_u(i) + 2.0 * k2_u(i) + 2.0 * k3_u(i) + k4_u(i));
			}
			affine += std::abs(dt);

			if (!std::isfinite(x(1)) || !std::isfinite(x(2)) || !std::isfinite(x(3)) || !std::isfinite(u(1)) || !std::isfinite(u(2)) || !std::isfinite(u(3))) {
				fate = RayTermination::InvalidState;
				break;
			}

			if (x(2) < 0.0) {
				x(2) = -x(2);
				x(3) += pi;
				u(2) = -u(2);
			} else if (x(2) > pi) {
				x(2) = 2.0 * pi - x(2);
				x(3) += pi;
				u(2) = -u(2);
			}

			minimum_radius = std::min(minimum_radius, x(1));

			if (disk_active && (prev_theta - half_pi) * (x(2) - half_pi) <= 0.0) {
				const double span = std::abs(x(2) - prev_theta);
				const double s_cross = (span > 1e-12) ? std::clamp(std::abs(prev_theta - half_pi) / span, 0.0, 1.0) : 0.5;
				const double r_cross = prev_r + s_cross * (x(1) - prev_r);
				const double phi_cross = prev_phi + s_cross * (x(3) - prev_phi);
				if (r_cross >= disk_inner && r_cross <= disk_outer && r_cross > horizon_radius * 1.02) {
					++result.disk_crossings;
					if (!result.disk_hit) {
						result.disk_hit = true;
						result.disk_radius = r_cross;
						result.disk_azimuth = phi_cross;
						result.disk_temperature_k = DiskThermalProfile::profile_temperature_kelvin(disk_inner, r_cross, query.disk_peak_temperature_k, query.disk_floor_temperature_k, query.disk_temperature_exponent, query.disk_zero_torque_strength, query.disk_temperature_normalization);
						disk_g = Render::AccretionDiskModel::redshift_at(m, spin, r_cross, xi);
					}
				}
			}

			if ((step % stride) == 0U) {
				append_path_point(result, x(1), x(2), x(3));
			}
		}

		if (fate == RayTermination::StepBudgetExhausted && x(1) < 3.0 * rs) {
			fate = RayTermination::HorizonAbsorbed;
		}
		append_path_point(result, x(1), x(2), x(3));

		result.deflection_angle = path_deflection(direction, result);
		result.image_order = result.disk_crossings;
		result.valid = (fate != RayTermination::InvalidState);
		result.termination = fate;
		result.iterations = iterations;
		result.minimum_radius = minimum_radius;
		result.affine_length = affine;
		result.terminal_r = x(1);
		result.terminal_theta = x(2);
		result.terminal_phi = x(3);
		if (result.disk_hit) {
			result.emission_r = result.disk_radius;
			result.emission_theta = half_pi;
			result.emission_phi = result.disk_azimuth;
			result.disk_emitter_shift = disk_g;
			result.spectral_shift_g = disk_g * result.static_redshift_factor;
			result.disk_observed_temperature_k = result.disk_temperature_k * result.spectral_shift_g;
		} else {
			result.emission_r = result.terminal_r;
			result.emission_theta = result.terminal_theta;
			result.emission_phi = result.terminal_phi;
			result.spectral_shift_g = (fate == RayTermination::HorizonAbsorbed) ? 0.0 : result.static_redshift_factor;
		}
		return result;
	}

public:
	[[nodiscard]] static RayProbeResult trace(const RayProbeQuery& query) {
		RayProbeResult result;
		result.pixel_x = query.pixel_x;
		result.pixel_y = query.pixel_y;
		const double m = std::max(query.mass, 1e-4);

		switch (query.metric_type) {
			case 0U:
				return trace_flat(query, std::move(result));
			case 1U: {
				const Metrics::SchwarzschildMetric<double> metric(m);
				return integrate(metric, query, 2.0 * m, true, false, 0.0, std::move(result));
			}
			case 2U:
			case 3U: {
				const double a = std::clamp(query.spin, -0.999 * m, 0.999 * m);
				const Metrics::KerrMetric<double> metric(m, a);
				return integrate(metric, query, metric.outer_horizon_radius(), true, false, a, std::move(result));
			}
			case 4U: {
				const Metrics::ReissnerNordstromMetric<double> metric(m, query.charge, 1.0, 1.0, 1.0);
				return integrate(metric, query, metric.outer_horizon_radius(), true, false, 0.0, std::move(result));
			}
			case 5U: {
				const double a = std::clamp(query.spin, -0.999 * m, 0.999 * m);
				const Metrics::KerrNewmanMetric<double> metric(m, a, query.charge, 1.0, 1.0, 1.0);
				return integrate(metric, query, metric.outer_horizon_radius(), true, false, a, std::move(result));
			}
			case 6U: {
				const Metrics::SchwarzschildDeSitterMetric<double> metric(m, query.cosmological_lambda, 1.0, 1.0);
				const double rs = 2.0 * m;
				double horizon = rs;
				for (int iteration = 0; iteration < 48; ++iteration) {
					const double lapse = metric.lapse_function(horizon);
					const double slope = rs / (horizon * horizon) - 2.0 * query.cosmological_lambda * horizon / 3.0;
					if (std::abs(slope) < 1e-30) break;
					horizon -= lapse / slope;
				}
				if (!std::isfinite(horizon) || horizon <= 0.0) {
					horizon = rs;
				}
				return integrate(metric, query, horizon, false, true, 0.0, std::move(result));
			}
			default:
				result.termination = RayTermination::UnsupportedMetric;
				return result;
		}
	}
};

}
