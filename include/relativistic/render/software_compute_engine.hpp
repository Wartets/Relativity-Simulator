#pragma once

#include "relativistic/render/gpu_types.hpp"
#include "relativistic/render/double_single.hpp"
#include "relativistic/observer/observer_tetrad.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/metrics/kerr.hpp"
#include "relativistic/metrics/kerr_schild.hpp"
#include "relativistic/metrics/reissner_nordstrom.hpp"
#include "relativistic/metrics/kerr_newman.hpp"
#include "relativistic/core/christoffel.hpp"
#include "relativistic/core/thread_pool.hpp"
#include "relativistic/core/geodesic_bundle.hpp"
#include <vector>
#include <span>
#include <thread>
#include <atomic>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <array>

namespace Relativistic::Render {

class SoftwareComputeEngine {
private:
	[[nodiscard]] static constexpr uint32_t hash_u32(uint32_t x) noexcept {
		x ^= x >> 16;
		x *= 0x7feb352dU;
		x ^= x >> 15;
		x *= 0x846ca68bU;
		x ^= x >> 16;
		return x;
	}

	[[nodiscard]] static float hash_to_float(uint32_t x) noexcept {
		return static_cast<float>(hash_u32(x) & 0x00FFFFFFU) * (1.0f / 16777216.0f);
	}

public:
	[[nodiscard]] static bool requires_exact_metric_path(const GpuCameraPushConstants& params) noexcept {
		const double mass_scale = std::max(params.metric_mass, 1e-4);
		const bool is_spin_family = (params.metric_type == 2U || params.metric_type == 3U || params.metric_type == 5U);
		const bool has_significant_spin = is_spin_family && std::abs(params.metric_spin) > 1e-9 * mass_scale;
		const bool is_charge_family = (params.metric_type == 4U || params.metric_type == 5U);
		const bool has_significant_charge = is_charge_family && std::abs(params.metric_charge) > 1e-9 * mass_scale;
		return has_significant_spin || has_significant_charge;
	}

private:

	[[nodiscard]] static double kerr_isco_radius(double m, double a_spin) noexcept {
		const double a_star = std::clamp(a_spin / std::max(m, 1e-12), -0.9999999, 0.9999999);
		const double abs_a = std::abs(a_star);
		const double orbit_sign = (a_star >= 0.0) ? -1.0 : 1.0;
		const double cbrt_term = std::cbrt(std::max(1.0 - abs_a * abs_a, 0.0));
		const double z1 = 1.0 + cbrt_term * (std::cbrt(1.0 + abs_a) + std::cbrt(1.0 - abs_a));
		const double z2 = std::sqrt(3.0 * abs_a * abs_a + z1 * z1);
		const double r_isco_over_m = 3.0 + z2 + orbit_sign * std::sqrt(std::max((3.0 - z1) * (3.0 + z1 + 2.0 * z2), 0.0));
		return r_isco_over_m * m;
	}

	template <typename Scalar>
	[[nodiscard]] static bool attempt_analytic_space_skip(
		Scalar& ray_r, Scalar& ray_theta, Scalar& ray_phi,
		Scalar& ray_pr, Scalar& ray_ptheta, Scalar& ray_pphi,
		Scalar space_skip_radius, Scalar escape_radius
	) noexcept {
		if (ray_r <= space_skip_radius) {
			return false;
		}

		const Scalar sin_t = std::sin(ray_theta);
		const Scalar cos_t = std::cos(ray_theta);
		const Scalar sin_p = std::sin(ray_phi);
		const Scalar cos_p = std::cos(ray_phi);

		const Scalar px = ray_r * sin_t * cos_p;
		const Scalar py = ray_r * sin_t * sin_p;
		const Scalar pz = ray_r * cos_t;

		const Scalar dx = ray_pr * sin_t * cos_p + ray_r * ray_ptheta * cos_t * cos_p - ray_r * sin_t * ray_pphi * sin_p;
		const Scalar dy = ray_pr * sin_t * sin_p + ray_r * ray_ptheta * cos_t * sin_p + ray_r * sin_t * ray_pphi * cos_p;
		const Scalar dz = ray_pr * cos_t - ray_r * ray_ptheta * sin_t;

		const Scalar dir_norm_sq = dx * dx + dy * dy + dz * dz;
		if (dir_norm_sq <= static_cast<Scalar>(1e-30)) {
			return false;
		}

		const Scalar p_dot_d = px * dx + py * dy + pz * dz;
		const Scalar p_dot_p = px * px + py * py + pz * pz;

		auto sphere_hit = [&](Scalar radius, Scalar& t_out) noexcept -> bool {
			const Scalar c_coeff = p_dot_p - radius * radius;
			const Scalar b_coeff = static_cast<Scalar>(2) * p_dot_d;
			const Scalar discr = b_coeff * b_coeff - static_cast<Scalar>(4) * dir_norm_sq * c_coeff;
			if (discr < static_cast<Scalar>(0)) {
				return false;
			}
			const Scalar sqrt_discr = std::sqrt(discr);
			const Scalar t1 = (-b_coeff - sqrt_discr) / (static_cast<Scalar>(2) * dir_norm_sq);
			const Scalar t2 = (-b_coeff + sqrt_discr) / (static_cast<Scalar>(2) * dir_norm_sq);
			Scalar best = static_cast<Scalar>(-1);
			if (t1 > static_cast<Scalar>(1e-9)) best = t1;
			if (t2 > static_cast<Scalar>(1e-9) && (best < static_cast<Scalar>(0) || t2 < best)) best = t2;
			if (best <= static_cast<Scalar>(0)) return false;
			t_out = best;
			return true;
		};

		Scalar t_skip = static_cast<Scalar>(0);
		Scalar t_escape = static_cast<Scalar>(0);
		const bool hits_skip_boundary = sphere_hit(space_skip_radius, t_skip);
		const bool hits_escape = sphere_hit(escape_radius, t_escape);

		Scalar t_leap;
		if (hits_skip_boundary && hits_escape) {
			t_leap = std::min(t_skip, t_escape);
		} else if (hits_skip_boundary) {
			t_leap = t_skip;
		} else if (hits_escape) {
			t_leap = t_escape;
		} else {
			return false;
		}

		if (t_leap <= static_cast<Scalar>(1e-6)) {
			return false;
		}

		const Scalar new_px = px + t_leap * dx;
		const Scalar new_py = py + t_leap * dy;
		const Scalar new_pz = pz + t_leap * dz;

		const Scalar new_r = std::sqrt(new_px * new_px + new_py * new_py + new_pz * new_pz);
		if (new_r <= static_cast<Scalar>(1e-9)) {
			return false;
		}

		const Scalar new_theta = std::acos(std::clamp(new_pz / new_r, static_cast<Scalar>(-1), static_cast<Scalar>(1)));
		const Scalar new_phi = std::atan2(new_py, new_px);
		const Scalar new_sin_t = std::sin(new_theta);
		const Scalar safe_sin_t = (std::abs(new_sin_t) > static_cast<Scalar>(1e-9)) ? new_sin_t : ((new_sin_t >= static_cast<Scalar>(0)) ? static_cast<Scalar>(1e-9) : static_cast<Scalar>(-1e-9));

		const Scalar new_pr = (new_px * dx + new_py * dy + new_pz * dz) / new_r;
		const Scalar new_ptheta = (new_pr * (new_pz / new_r) - dz) / (new_r * safe_sin_t);
		const Scalar r2_sin2 = new_r * new_r * new_sin_t * new_sin_t;
		const Scalar new_pphi = (std::abs(r2_sin2) > static_cast<Scalar>(1e-12)) ? ((new_px * dy - new_py * dx) / r2_sin2) : static_cast<Scalar>(0);

		ray_r = new_r;
		ray_theta = new_theta;
		ray_phi = new_phi;
		ray_pr = new_pr;
		ray_ptheta = new_ptheta;
		ray_pphi = new_pphi;
		return true;
	}

	template <typename MetricType>
		requires Metrics::SpacetimeMetric<MetricType, double>
	[[nodiscard]] static GpuPixelOutput trace_exact_photon(
		const MetricType& metric,
		double n1, double n2, double n3,
		double r_obs, double theta_obs, double phi_obs,
		double m, double rs, double rh, double isco,
		double escape_radius, uint32_t max_steps,
		bool has_accretion_disk,
		double turbulence_aa_factor,
		const GpuCameraPushConstants& params
	) noexcept {
		const auto tetrad = Observer::ObserverTetrad<double>::make_zamo(metric, Core::FourVector<double>(0.0, r_obs, theta_obs, phi_obs));

		Core::FourVector<double> x(0.0, r_obs, theta_obs, phi_obs);
		Core::FourVector<double> u = tetrad.construct_light_ray(n1, n2, n3);

		const double disk_outer = 24.0 * m;

		auto compute_acc = [&](const Core::FourVector<double>& xs, const Core::FourVector<double>& us) noexcept -> Core::FourVector<double> {
			const auto gamma = Core::compute_christoffel<Core::DerivativeOrder::EighthOrder, MetricType, double>(metric, xs);
			Core::FourVector<double> acc;
			acc.zero();
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
				acc(mu) = sum;
			}
			return acc;
		};

		double accum_r = 0.0, accum_g = 0.0, accum_b = 0.0;
		double throughput = 1.0;
		double redshift_rec = 1.0;
		uint32_t status = 0;
		uint32_t iters = 0;

		for (uint32_t step = 0; step < max_steps && throughput > 0.01; ++step) {
			iters = step + 1;

			if (x(1) <= rh) {
				status = PixelFlags::HORIZON_ABSORBED;
				throughput = 0.0;
				break;
			}
			if (x(1) >= escape_radius) {
				status = PixelFlags::CELESTIAL_HIT;
				break;
			}

			if ((params.render_flags & RenderFlags::SPACE_SKIP_ENABLED) != 0U) {
				const double effective_skip_r = has_accretion_disk
					? std::max(params.space_skip_radius_scale * m, disk_outer * 1.05)
					: std::max(params.space_skip_radius_scale * m, rh * 2.0);
				if (x(1) > effective_skip_r && attempt_analytic_space_skip(x(1), x(2), x(3), u(1), u(2), u(3), effective_skip_r, escape_radius)) {
					continue;
				}
			}

			const double cur_r = x(1);
			const double r_scale = std::max(cur_r - rh, 0.02 * m);
			const double pole_guard = std::clamp(std::abs(std::sin(x(2))) * 12.0, 0.15, 1.0);
			const double dt = -std::clamp(0.05 * std::sqrt(cur_r * r_scale), 0.004, 3.5) * pole_guard;

			const double prev_r = x(1);
			const double prev_theta = x(2);
			const double prev_phi = x(3);

			const auto k1_x = u;
			const auto k1_u = compute_acc(x, u);

			Core::FourVector<double> x2 = x, u2 = u;
			for (size_t i = 0; i < 4; ++i) {
				x2(i) += 0.5 * dt * k1_x(i);
				u2(i) += 0.5 * dt * k1_u(i);
			}
			const auto k2_x = u2;
			const auto k2_u = compute_acc(x2, u2);

			Core::FourVector<double> x3 = x, u3 = u;
			for (size_t i = 0; i < 4; ++i) {
				x3(i) += 0.5 * dt * k2_x(i);
				u3(i) += 0.5 * dt * k2_u(i);
			}
			const auto k3_x = u3;
			const auto k3_u = compute_acc(x3, u3);

			Core::FourVector<double> x4 = x, u4 = u;
			for (size_t i = 0; i < 4; ++i) {
				x4(i) += dt * k3_x(i);
				u4(i) += dt * k3_u(i);
			}
			const auto k4_x = u4;
			const auto k4_u = compute_acc(x4, u4);

			const double sixth_dt = dt / 6.0;
			for (size_t i = 0; i < 4; ++i) {
				x(i) += sixth_dt * (k1_x(i) + 2.0 * k2_x(i) + 2.0 * k3_x(i) + k4_x(i));
				u(i) += sixth_dt * (k1_u(i) + 2.0 * k2_u(i) + 2.0 * k3_u(i) + k4_u(i));
			}

			if (x(2) < 0.0) {
				x(2) = -x(2);
				x(3) += std::numbers::pi_v<double>;
				u(2) = -u(2);
			} else if (x(2) > std::numbers::pi_v<double>) {
				x(2) = 2.0 * std::numbers::pi_v<double> - x(2);
				x(3) += std::numbers::pi_v<double>;
				u(2) = -u(2);
			}

			const double mid_plane = std::numbers::pi_v<double> * 0.5;
			if (has_accretion_disk && (prev_theta - mid_plane) * (x(2) - mid_plane) <= 0.0) {
				const double d_th_span = std::abs(x(2) - prev_theta);
				const double s_cross = (d_th_span > 1e-12) ? std::clamp(std::abs(prev_theta - mid_plane) / d_th_span, 0.0, 1.0) : 0.5;
				const double r_cross = prev_r + s_cross * (x(1) - prev_r);
				const double phi_cross = prev_phi + s_cross * (x(3) - prev_phi);

				if (r_cross >= isco && r_cross <= disk_outer && r_cross > rh * 1.02) {
					status |= PixelFlags::ACCRETION_DISK_HIT;

					const double v_orb = std::sqrt(m / r_cross);
					const double omega_orb = v_orb / r_cross;
					const double gamma_orb = 1.0 / std::sqrt(std::max(1.0 - 3.0 * m / r_cross, 1e-4));

					const double sin2_x = std::max(std::sin(x(2)) * std::sin(x(2)), 1e-8);
					const double lz_val = u(3) * (r_cross * r_cross * sin2_x);
					const double e_val = 1.0;
					const double l_over_e = lz_val / std::max(std::abs(e_val), 1e-12);
					const double denom_g = gamma_orb * (1.0 - omega_orb * l_over_e);
					const double g_doppler = (std::abs(denom_g) > 1e-12) ? (std::sqrt(std::max(1.0 - rs / r_cross, 1e-4)) / denom_g) : 1.0;
					redshift_rec = g_doppler;

					const double t_norm = std::pow(isco / r_cross, 0.75) * std::pow(std::max(1.0 - std::sqrt(isco / r_cross), 0.0), 0.25);
					const double t_eff_k = 18000.0 * t_norm + 1200.0;
					const double t_obs = t_eff_k * g_doppler;

					const double g4 = g_doppler * g_doppler * g_doppler * g_doppler;
					const double radial_envelope = std::clamp((disk_outer - r_cross) / (1.5 * m), 0.0, 1.0) * std::clamp((r_cross - isco) / (0.8 * m), 0.0, 1.0);
					const double turbulence = 1.0 - (0.12 * turbulence_aa_factor) + (0.12 * turbulence_aa_factor) * std::sin(8.0 * phi_cross - 4.0 * std::log(r_cross / isco));
					const double flux_intensity = std::max(g4 * t_norm * radial_envelope * turbulence, 0.0) * 1.5;

					const auto disk_rgb = temperature_to_linear_rgb(t_obs, flux_intensity);
					const double alpha_opacity = std::clamp(radial_envelope * 0.95, 0.0, 0.98);

					accum_r += throughput * static_cast<double>(disk_rgb[0]);
					accum_g += throughput * static_cast<double>(disk_rgb[1]);
					accum_b += throughput * static_cast<double>(disk_rgb[2]);
					throughput *= (1.0 - alpha_opacity);
				}
			}
		}

		if (status & PixelFlags::CELESTIAL_HIT || throughput > 0.01) {
			const double sin_t = std::sin(x(2));
			const double cos_t = std::cos(x(2));
			const double sin_p = std::sin(x(3));
			const double cos_p = std::cos(x(3));

			const double px = u(1) * sin_t * cos_p + x(1) * u(2) * cos_t * cos_p - x(1) * sin_t * u(3) * sin_p;
			const double py = u(1) * sin_t * sin_p + x(1) * u(2) * cos_t * sin_p + x(1) * sin_t * u(3) * cos_p;
			const double pz = u(1) * cos_t - x(1) * u(2) * sin_t;

			const double p_len = std::sqrt(px * px + py * py + pz * pz);
			const double inv_plen = (p_len > 1e-12) ? (1.0 / p_len) : 1.0;

			const auto sky_rgb = compute_sky_radiance(px * inv_plen, py * inv_plen, pz * inv_plen, params);
			accum_r += throughput * static_cast<double>(sky_rgb[0]);
			accum_g += throughput * static_cast<double>(sky_rgb[1]);
			accum_b += throughput * static_cast<double>(sky_rgb[2]);
		}

		const auto mapped_srgb = apply_tonemapping({accum_r, accum_g, accum_b}, params.tonemapping_mode, params.camera_exposure);

		return GpuPixelOutput{
			.r = mapped_srgb[0],
			.g = mapped_srgb[1],
			.b = mapped_srgb[2],
			.a = 1.0f,
			.redshift = static_cast<float>(redshift_rec),
			.affine_parameter = static_cast<float>(iters * 0.05),
			.status_flags = status,
			.iterations_used = iters
		};
	}

	[[nodiscard]] static GpuPixelOutput trace_exact_photon_dispatch(
		uint32_t metric_type,
		double m, double a_spin, double charge,
		double n1, double n2, double n3,
		double r_obs, double theta_obs, double phi_obs,
		double rs, double escape_radius, uint32_t max_steps,
		bool has_accretion_disk,
		double turbulence_aa_factor,
		const GpuCameraPushConstants& params
	) noexcept {
		if (metric_type == 4U) {
			const Metrics::ReissnerNordstromMetric<double> metric(m, charge, 1.0, 1.0, 1.0);
			const double rh = metric.outer_horizon_radius();
			const double isco = kerr_isco_radius(m, 0.0);
			return trace_exact_photon(metric, n1, n2, n3, r_obs, theta_obs, phi_obs, m, rs, rh, isco, escape_radius, max_steps, has_accretion_disk, turbulence_aa_factor, params);
		}
		if (metric_type == 5U) {
			const Metrics::KerrNewmanMetric<double> metric(m, a_spin, charge, 1.0, 1.0, 1.0);
			const double rh = metric.outer_horizon_radius();
			const double isco = kerr_isco_radius(m, a_spin);
			return trace_exact_photon(metric, n1, n2, n3, r_obs, theta_obs, phi_obs, m, rs, rh, isco, escape_radius, max_steps, has_accretion_disk, turbulence_aa_factor, params);
		}
		const Metrics::KerrMetric<double> metric(m, a_spin, 1.0, 1.0);
		const double rh = metric.outer_horizon_radius();
		const double isco = kerr_isco_radius(m, a_spin);
		return trace_exact_photon(metric, n1, n2, n3, r_obs, theta_obs, phi_obs, m, rs, rh, isco, escape_radius, max_steps, has_accretion_disk, turbulence_aa_factor, params);
	}

	[[nodiscard]] static std::array<double, 3> rotate_direction_around_z(double x, double y, double z, double angle_rad) noexcept {
		if (angle_rad == 0.0) {
			return {x, y, z};
		}
		const double c = std::cos(angle_rad);
		const double s = std::sin(angle_rad);
		return {x * c - y * s, x * s + y * c, z};
	}

	[[nodiscard]] static std::array<float, 3> apply_hue_saturation(const std::array<float, 3>& rgb, double hue_shift_rad, double saturation) noexcept {
		if (hue_shift_rad == 0.0 && saturation == 1.0) {
			return rgb;
		}
		const float r = rgb[0], g = rgb[1], b = rgb[2];
		const float max_c = std::max({r, g, b});
		const float min_c = std::min({r, g, b});
		const float delta = max_c - min_c;
		float h = 0.0f;
		if (delta > 1e-8f) {
			if (max_c == r) {
				h = std::fmod((g - b) / delta, 6.0f);
			} else if (max_c == g) {
				h = (b - r) / delta + 2.0f;
			} else {
				h = (r - g) / delta + 4.0f;
			}
			h *= 60.0f;
			if (h < 0.0f) h += 360.0f;
		}
		const float s_val = (max_c > 1e-8f) ? (delta / max_c) : 0.0f;
		const float v_val = max_c;

		h += static_cast<float>(hue_shift_rad * (180.0 / std::numbers::pi_v<double>));
		h = std::fmod(h, 360.0f);
		if (h < 0.0f) h += 360.0f;
		const float s_new = std::clamp(s_val * static_cast<float>(saturation), 0.0f, 1.0f);

		const float c_val = v_val * s_new;
		const float x_val = c_val * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
		const float m_val = v_val - c_val;

		float r2 = 0.0f, g2 = 0.0f, b2 = 0.0f;
		if (h < 60.0f) { r2 = c_val; g2 = x_val; b2 = 0.0f; }
		else if (h < 120.0f) { r2 = x_val; g2 = c_val; b2 = 0.0f; }
		else if (h < 180.0f) { r2 = 0.0f; g2 = c_val; b2 = x_val; }
		else if (h < 240.0f) { r2 = 0.0f; g2 = x_val; b2 = c_val; }
		else if (h < 300.0f) { r2 = x_val; g2 = 0.0f; b2 = c_val; }
		else { r2 = c_val; g2 = 0.0f; b2 = x_val; }

		return {r2 + m_val, g2 + m_val, b2 + m_val};
	}

	[[nodiscard]] static std::array<float, 3> sample_celestial_starfield(
		double dir_x, double dir_y, double dir_z,
		double star_density = 1.0, double star_brightness = 1.0, double nebula_intensity = 1.0
	) noexcept {
		const double theta = std::acos(std::clamp(dir_z, -1.0, 1.0));
		const double phi = std::atan2(dir_y, dir_x);

		const double u = (phi + std::numbers::pi_v<double>) * (1.0 / (2.0 * std::numbers::pi_v<double>));
		const double v = theta * (1.0 / std::numbers::pi_v<double>);

		const double gal_lat = 0.5 - v;
		const double gal_band = std::exp(-(gal_lat * gal_lat) / 0.015);
		const double gal_core = std::exp(-(gal_lat * gal_lat) / 0.008) * std::exp(-std::pow(u - 0.5, 2.0) / 0.05);

		const double nebula_scale = std::max(nebula_intensity, 0.0);
		float r_bg = static_cast<float>((0.015 * gal_band + 0.12 * gal_core) * nebula_scale);
		float g_bg = static_cast<float>((0.020 * gal_band + 0.09 * gal_core) * nebula_scale);
		float b_bg = static_cast<float>((0.045 * gal_band + 0.06 * gal_core) * nebula_scale);

		const double u_grid = u * 720.0;
		const double v_grid = v * 360.0;
		const int iu = static_cast<int>(std::floor(u_grid));
		const int iv = static_cast<int>(std::floor(v_grid));

		const uint32_t cell_seed = static_cast<uint32_t>(iu * 73856093 ^ iv * 19349663);
		const float star_prob = hash_to_float(cell_seed);
		const float star_threshold = static_cast<float>(std::clamp(1.0 - 0.12 * std::max(star_density, 0.0), 0.0, 0.999));

		if (star_prob > star_threshold) {
			const float star_offset_u = hash_to_float(cell_seed + 101U);
			const float star_offset_v = hash_to_float(cell_seed + 202U);
			const double du = (u_grid - static_cast<double>(iu)) - static_cast<double>(star_offset_u);
			const double dv = (v_grid - static_cast<double>(iv)) - static_cast<double>(star_offset_v);
			const double dist_sq = du * du + dv * dv;

			if (dist_sq < 0.35) {
				const float star_brightness_base = std::pow(hash_to_float(cell_seed + 303U), 6.0f) * 4.5f;
				const float star_falloff = static_cast<float>(std::exp(-dist_sq * 10.0));
				const float color_temp = hash_to_float(cell_seed + 404U);

				float sr = 1.0f, sg = 1.0f, sb = 1.0f;
				if (color_temp < 0.3f) {
					sr = 0.8f; sg = 0.85f; sb = 1.3f;
				} else if (color_temp > 0.7f) {
					sr = 1.3f; sg = 0.85f; sb = 0.6f;
				}

				const float lum = star_brightness_base * star_falloff * static_cast<float>(std::max(star_brightness, 0.0));
				r_bg += sr * lum;
				g_bg += sg * lum;
				b_bg += sb * lum;
			}
		}

		return {r_bg, g_bg, b_bg};
	}

	[[nodiscard]] static std::array<float, 3> sample_celestial_grid_sphere(double dir_x, double dir_y, double dir_z, double grid_opacity = 1.0) noexcept {
		const double theta = std::acos(std::clamp(dir_z, -1.0, 1.0));
		const double phi = std::atan2(dir_y, dir_x);
		const double lat_deg = 90.0 - theta * (180.0 / std::numbers::pi_v<double>);
		const double lon_deg = phi * (180.0 / std::numbers::pi_v<double>);
		const double lat_line = std::abs(std::remainder(lat_deg, 15.0));
		const double lon_line = std::abs(std::remainder(lon_deg, 15.0));
		constexpr double line_width = 0.35;
		const bool on_major_lat = std::abs(std::remainder(lat_deg, 90.0)) < line_width;
		const bool on_major_lon = std::abs(std::remainder(lon_deg, 90.0)) < line_width;
		const bool on_minor_line = (lat_line < line_width) || (lon_line < line_width);
		const float opacity = static_cast<float>(std::max(grid_opacity, 0.0));
		if (on_major_lat || on_major_lon) {
			return {0.14f * opacity, 0.20f * opacity, 0.28f * opacity};
		}
		if (on_minor_line) {
			return {0.05f * opacity, 0.07f * opacity, 0.10f * opacity};
		}
		return {0.008f * opacity, 0.009f * opacity, 0.012f * opacity};
	}

	[[nodiscard]] static std::array<float, 3> compute_sky_radiance(double dir_x, double dir_y, double dir_z, const GpuCameraPushConstants& params) noexcept {
		const auto rotated = rotate_direction_around_z(dir_x, dir_y, dir_z, params.sky_rotation_rad);
		std::array<float, 3> sky_rgb{0.0f, 0.0f, 0.0f};
		const uint32_t sky_mode = params.render_flags & RenderFlags::SKYBOX_MODE_MASK;

		if (sky_mode == RenderFlags::SKYBOX_GRID || (params.render_flags & RenderFlags::USE_GRID_SKYBOX)) {
			sky_rgb = sample_celestial_grid_sphere(rotated[0], rotated[1], rotated[2], params.sky_grid_opacity);
		} else if (sky_mode == RenderFlags::SKYBOX_COMPOSITE) {
			const auto stars = sample_celestial_starfield(rotated[0], rotated[1], rotated[2], params.sky_star_density, params.sky_star_brightness, params.sky_nebula_intensity);
			const auto grid = sample_celestial_grid_sphere(rotated[0], rotated[1], rotated[2], params.sky_grid_opacity);
			sky_rgb = {stars[0] + grid[0] * 0.7f, stars[1] + grid[1] * 0.7f, stars[2] + grid[2] * 0.7f};
		} else if (sky_mode == RenderFlags::SKYBOX_STARS) {
			sky_rgb = sample_celestial_starfield(rotated[0], rotated[1], rotated[2], params.sky_star_density, params.sky_star_brightness, params.sky_nebula_intensity);
		} else if (sky_mode == RenderFlags::SKYBOX_STARS_NO_NEBULA) {
			sky_rgb = sample_celestial_starfield(rotated[0], rotated[1], rotated[2], params.sky_star_density, params.sky_star_brightness, 0.0);
		} else if (sky_mode == RenderFlags::SKYBOX_GRID_STARS) {
			const auto stars = sample_celestial_starfield(rotated[0], rotated[1], rotated[2], params.sky_star_density, params.sky_star_brightness, 0.0);
			const auto grid = sample_celestial_grid_sphere(rotated[0], rotated[1], rotated[2], params.sky_grid_opacity);
			sky_rgb = {stars[0] + grid[0], stars[1] + grid[1], stars[2] + grid[2]};
		}

		sky_rgb = apply_hue_saturation(sky_rgb, params.sky_hue_shift_rad, params.sky_saturation);
		sky_rgb[0] = std::max(0.0f, sky_rgb[0] + static_cast<float>(params.sky_background_r));
		sky_rgb[1] = std::max(0.0f, sky_rgb[1] + static_cast<float>(params.sky_background_g));
		sky_rgb[2] = std::max(0.0f, sky_rgb[2] + static_cast<float>(params.sky_background_b));
		return sky_rgb;
	}

	[[nodiscard]] static std::array<float, 3> temperature_to_linear_rgb(double t_kelvin, double intensity) noexcept {
		const double t = std::clamp(t_kelvin, 800.0, 60000.0);
		constexpr double h = 6.62607015e-34;
		constexpr double c = 299792458.0;
		constexpr double kb = 1.380649e-23;

		auto planck = [&](double lambda_m) noexcept -> double {
			const double exponent = (h * c) / (lambda_m * kb * t);
			if (exponent > 80.0) return 0.0;
			const double denom = std::expm1(exponent);
			if (denom <= 0.0) return 0.0;
			return (2.0 * h * c * c) / (std::pow(lambda_m, 5.0) * denom);
		};

		const double i_red = planck(680e-9);
		const double i_green = planck(540e-9);
		const double i_blue = planck(440e-9);

		constexpr double ref_t = 6500.0;
		auto planck_ref = [&](double lambda_m) noexcept -> double {
			const double exponent = (h * c) / (lambda_m * kb * ref_t);
			return (2.0 * h * c * c) / (std::pow(lambda_m, 5.0) * std::expm1(exponent));
		};

		const double norm_r = planck_ref(680e-9);
		const double norm_g = planck_ref(540e-9);
		const double norm_b = planck_ref(440e-9);

		double r = i_red / (norm_r > 0.0 ? norm_r : 1.0);
		double g = i_green / (norm_g > 0.0 ? norm_g : 1.0);
		double b = i_blue / (norm_b > 0.0 ? norm_b : 1.0);

		const double max_c = std::max({r, g, b, 1e-12});
		r /= max_c;
		g /= max_c;
		b /= max_c;

		const float scale = static_cast<float>(std::max(0.0, intensity));
		return {static_cast<float>(r) * scale, static_cast<float>(g) * scale, static_cast<float>(b) * scale};
	}

	[[nodiscard]] static std::array<float, 3> apply_tonemapping(
		const std::array<double, 3>& linear_color,
		uint32_t tonemap_mode,
		double exposure_ev
	) noexcept {
		const double exp_scale = std::exp2(exposure_ev);
		double r = std::max(0.0, linear_color[0] * exp_scale);
		double g = std::max(0.0, linear_color[1] * exp_scale);
		double b = std::max(0.0, linear_color[2] * exp_scale);

		auto aces_curve = [](double x) noexcept -> double {
			constexpr double aces_a = 2.51;
			constexpr double aces_b = 0.03;
			constexpr double aces_c = 2.43;
			constexpr double aces_d = 0.59;
			constexpr double aces_e = 0.14;
			return std::clamp((x * (aces_a * x + aces_b)) / (x * (aces_c * x + aces_d) + aces_e), 0.0, 1.0);
		};

		switch (tonemap_mode) {
			case 1:
				r = aces_curve(r);
				g = aces_curve(g);
				b = aces_curve(b);
				break;
			case 2: {
				const double lum = 0.2126 * r + 0.7152 * g + 0.0722 * b;
				if (lum > 0.0) {
					const double mapped_lum = std::log10(1.0 + lum * 100.0) / std::log10(1.0 + 10000.0);
					const double scale = std::clamp(mapped_lum / lum, 0.0, 1.0);
					r = std::clamp(r * scale, 0.0, 1.0);
					g = std::clamp(g * scale, 0.0, 1.0);
					b = std::clamp(b * scale, 0.0, 1.0);
				}
				break;
			}
			case 3:
				r = (r * (1.0 + r / 25.0)) / (1.0 + r);
				g = (g * (1.0 + g / 25.0)) / (1.0 + g);
				b = (b * (1.0 + b / 25.0)) / (1.0 + b);
				break;
			case 0:
			default:
				r = std::clamp(r, 0.0, 1.0);
				g = std::clamp(g, 0.0, 1.0);
				b = std::clamp(b, 0.0, 1.0);
				break;
		}

		auto to_srgb = [](double linear) noexcept -> float {
			if (linear <= 0.0031308) {
				return static_cast<float>(12.92 * linear);
			}
			return static_cast<float>(1.055 * std::pow(linear, 1.0 / 2.4) - 0.055);
		};

		return {to_srgb(r), to_srgb(g), to_srgb(b)};
	}

public:
	static void dispatch_fp64_scalar(
		const GpuCameraPushConstants& params,
		std::span<GpuPixelOutput> output_framebuffer,
		Core::ThreadPool* pool = nullptr,
		const std::atomic<bool>* cancel_flag = nullptr
	) noexcept {
		const size_t width = params.screen_width;
		const size_t height = params.screen_height;
		const size_t total_pixels = width * height;

		if (output_framebuffer.size() < total_pixels || width == 0 || height == 0) {
			return;
		}

		const unsigned int num_threads = std::max(1u, std::thread::hardware_concurrency());
		std::vector<std::jthread> workers;
		workers.reserve(num_threads);

		const uint32_t metric_id = params.metric_type;
		const bool is_wormhole = (metric_id == 8);
		const bool is_warp = (metric_id == 9);
		const bool is_flat = (metric_id == 0);
		const bool has_accretion_disk = (metric_id == 1 || metric_id == 2 || metric_id == 3 || metric_id == 4 || metric_id == 5);
		const bool has_event_horizon = (!is_wormhole && !is_warp && !is_flat);

		const double m = std::max(params.metric_mass, 1e-4);
		const double a_spin = std::clamp(params.metric_spin, -0.999 * m, 0.999 * m);
		const double rs = 2.0 * m;
		const double rh = (std::abs(a_spin) > 1e-12) ? (m + std::sqrt(std::max(m * m - a_spin * a_spin, 0.0))) : rs;
		const double aspect = static_cast<double>(width) / static_cast<double>(height);

		const double isco = (std::abs(a_spin) > 1e-12) ? std::max(rh * 1.05, 6.0 * m - 4.0 * a_spin) : (6.0 * m);
		const double disk_outer = 24.0 * m;

		const double fwd_x = params.tetrad_e1[1], fwd_y = params.tetrad_e1[2], fwd_z = params.tetrad_e1[3];
		const double rgt_x = params.tetrad_e2[1], rgt_y = params.tetrad_e2[2], rgt_z = params.tetrad_e2[3];
		const double up_x  = params.tetrad_e3[1], up_y  = params.tetrad_e3[2], up_z  = params.tetrad_e3[3];

		const bool space_skip_enabled = (params.render_flags & RenderFlags::SPACE_SKIP_ENABLED) != 0U;
		const double effective_space_skip_radius = has_accretion_disk
			? std::max(params.space_skip_radius_scale * m, disk_outer * 1.05)
			: std::max(params.space_skip_radius_scale * m, rh * 2.0);

		const bool use_exact_metric_path = requires_exact_metric_path(params);
		const bool lod_active = (params.render_flags & RenderFlags::USE_LOD_SYSTEM) != 0U && params.lod_distance_threshold > 0.0;
		const double r_obs_frame = std::max(params.observer_position[1], rh * 1.02);
		const uint32_t effective_max_steps = (lod_active && r_obs_frame > params.lod_distance_threshold)
			? std::min(params.max_integration_steps, params.lod_reduced_steps)
			: params.max_integration_steps;

		const double angular_pixel_size = params.field_of_view_rad / std::max(static_cast<double>(width), 1.0);
		const double bh_angular_diameter = (2.0 * rh) / r_obs_frame;
		const double turbulence_aa_factor = std::clamp(bh_angular_diameter / std::max(angular_pixel_size * 24.0, 1e-9), 0.0, 1.0);

		auto render_rect = [&](size_t x_start, size_t x_end, size_t y_start, size_t y_end) noexcept {
			for (size_t y = y_start; y < y_end; ++y) {
				if (cancel_flag && cancel_flag->load(std::memory_order_relaxed)) return;
				const double v_norm = 1.0 - (static_cast<double>(y) + 0.5) / static_cast<double>(height) * 2.0;
				for (size_t x = x_start; x < x_end; ++x) {
					const size_t pixel_idx = y * width + x;
					if (pixel_idx >= output_framebuffer.size()) continue;
					const double u_norm = ((static_cast<double>(x) + 0.5) / static_cast<double>(width) * 2.0 - 1.0) * aspect;

					const bool is_allsky = (params.projection_mode == 3 || params.projection_mode == 7);
					const double u_coord = is_allsky ? (((static_cast<double>(x) + 0.5) / static_cast<double>(width)) * 2.0 - 1.0) : u_norm;
					const auto n_local = Observer::CameraProjector<double>::compute_ray_direction(
						static_cast<Observer::ProjectionMode>(params.projection_mode),
						u_coord, v_norm, params.field_of_view_rad
					);

					const double ray_dir_x = n_local[0] * fwd_x + n_local[2] * rgt_x + n_local[1] * up_x;
					const double ray_dir_y = n_local[0] * fwd_y + n_local[2] * rgt_y + n_local[1] * up_y;
					const double ray_dir_z = n_local[0] * fwd_z + n_local[2] * rgt_z + n_local[1] * up_z;

					const double r_obs = std::max(params.observer_position[1], rh * 1.02);
					const double theta_obs = std::clamp(params.observer_position[2], 0.001, std::numbers::pi_v<double> - 0.001);
					const double phi_obs = params.observer_position[3];

					const double sin_to = std::sin(theta_obs);
					const double cos_to = std::cos(theta_obs);
					const double sin_po = std::sin(phi_obs);
					const double cos_po = std::cos(phi_obs);

					const double er_x = sin_to * cos_po, er_y = sin_to * sin_po, er_z = cos_to;
					const double eth_x = cos_to * cos_po, eth_y = cos_to * sin_po, eth_z = -sin_to;
					const double eph_x = -sin_po, eph_y = cos_po, eph_z = 0.0;

					const double n_r = ray_dir_x * er_x + ray_dir_y * er_y + ray_dir_z * er_z;
					const double n_th = ray_dir_x * eth_x + ray_dir_y * eth_y + ray_dir_z * eth_z;
					const double n_ph = ray_dir_x * eph_x + ray_dir_y * eph_y + ray_dir_z * eph_z;

					if (use_exact_metric_path) {
						output_framebuffer[pixel_idx] = trace_exact_photon_dispatch(
							params.metric_type, m, a_spin, params.metric_charge,
							n_r, n_th, n_ph,
							r_obs, theta_obs, phi_obs,
							rs, params.escape_radius, effective_max_steps,
							has_accretion_disk, turbulence_aa_factor, params
						);
						continue;
					}

					const double factor_obs = std::max(1.0 - rs / r_obs, 1e-6);
					const double sqrt_factor_obs = std::sqrt(factor_obs);

					const double p_r_init = n_r / sqrt_factor_obs;
					const double p_theta_init = n_th / r_obs;
					const double p_phi_init = n_ph / (r_obs * sin_to);

					double ray_r = r_obs;
					double ray_theta = theta_obs;
					double ray_phi = phi_obs;

					double ray_pr = p_r_init;
					double ray_ptheta = p_theta_init;
					double ray_pphi = p_phi_init;

					const double E_cons = 1.0;
					const double Lz_cons = p_phi_init * (r_obs * r_obs * sin_to * sin_to);

					double accumulated_r = 0.0;
					double accumulated_g = 0.0;
					double accumulated_b = 0.0;
					double throughput = 1.0;
					double redshift_rec = 1.0;

					uint32_t status = 0;
					uint32_t iters = 0;

					for (uint32_t step = 0; step < effective_max_steps && throughput > 0.01; ++step) {
						iters = step + 1;

						const double delta_kerr = ray_r * ray_r - 2.0 * m * ray_r + a_spin * a_spin;
						if (has_event_horizon && (ray_r <= rh * 1.0001 || delta_kerr <= 0.0)) {
							status = PixelFlags::HORIZON_ABSORBED;
							throughput = 0.0;
							break;
						}

						if (ray_r >= params.escape_radius) {
							status = PixelFlags::CELESTIAL_HIT;
							break;
						}

						if (space_skip_enabled && ray_r > effective_space_skip_radius &&
							attempt_analytic_space_skip(ray_r, ray_theta, ray_phi, ray_pr, ray_ptheta, ray_pphi, effective_space_skip_radius, params.escape_radius)) {
							continue;
						}

						const double r_scale = std::max(ray_r - rh, 0.02 * m);
						const double smooth_dt = 0.05 * std::sqrt(ray_r * r_scale);
						const double pole_guard = std::clamp(std::abs(std::sin(ray_theta)) * 12.0, 0.15, 1.0);
						const double dt = -std::clamp(smooth_dt, 0.004, 3.5) * pole_guard;

						const double prev_r = ray_r;
						const double prev_theta = ray_theta;
						const double prev_phi = ray_phi;

						auto eval_acc = [&](double r_eval, double th_eval, double phi_eval,
											double pr_eval, double pth_eval, double pphi_eval,
											double& d_r, double& d_th, double& d_phi,
											double& d_pr, double& d_pth, double& d_pphi) noexcept {
							static_cast<void>(phi_eval);
							const double r = std::max(r_eval, rh * 1.0001);
							const double th = std::clamp(th_eval, 1e-5, std::numbers::pi_v<double> - 1e-5);
							const double sin_t = std::sin(th);
							const double cos_t = std::cos(th);
							const double sin2_t = std::max(sin_t * sin_t, 1e-10);
							const double r2 = r * r;
							const double f = 1.0 - rs / r;
							const double safe_f = std::max(f, 1e-6);

							d_r = pr_eval;
							d_th = pth_eval;
							d_phi = pphi_eval;

							const double p_t = E_cons / safe_f;
							const double g001 = rs / (2.0 * r2 * safe_f);
							const double g100 = (rs * safe_f) / (2.0 * r2);
							const double g111 = -g001;
							const double g122 = -r * safe_f;
							const double g133 = -r * safe_f * sin2_t;
							const double g212 = 1.0 / r;
							const double g233 = -sin_t * cos_t;
							const double g313 = 1.0 / r;
							const double safe_sin_t = (std::abs(sin_t) > 1e-7) ? sin_t : ((sin_t >= 0.0) ? 1e-7 : -1e-7);
							const double g323 = cos_t / safe_sin_t;

							d_pr = -(g100 * p_t * p_t + g111 * pr_eval * pr_eval + g122 * pth_eval * pth_eval + g133 * pphi_eval * pphi_eval);
							d_pth = -(2.0 * g212 * pr_eval * pth_eval + g233 * pphi_eval * pphi_eval);
							d_pphi = -2.0 * (g313 * pr_eval * pphi_eval + g323 * pth_eval * pphi_eval);
						};

						double k1_r = 0.0, k1_th = 0.0, k1_phi = 0.0, k1_pr = 0.0, k1_pth = 0.0, k1_pphi = 0.0;
						eval_acc(ray_r, ray_theta, ray_phi, ray_pr, ray_ptheta, ray_pphi, k1_r, k1_th, k1_phi, k1_pr, k1_pth, k1_pphi);

						const double half_dt = 0.5 * dt;
						double k2_r = 0.0, k2_th = 0.0, k2_phi = 0.0, k2_pr = 0.0, k2_pth = 0.0, k2_pphi = 0.0;
						eval_acc(ray_r + half_dt * k1_r, ray_theta + half_dt * k1_th, ray_phi + half_dt * k1_phi,
								 ray_pr + half_dt * k1_pr, ray_ptheta + half_dt * k1_pth, ray_pphi + half_dt * k1_pphi,
								 k2_r, k2_th, k2_phi, k2_pr, k2_pth, k2_pphi);

						double k3_r = 0.0, k3_th = 0.0, k3_phi = 0.0, k3_pr = 0.0, k3_pth = 0.0, k3_pphi = 0.0;
						eval_acc(ray_r + half_dt * k2_r, ray_theta + half_dt * k2_th, ray_phi + half_dt * k2_phi,
								 ray_pr + half_dt * k2_pr, ray_ptheta + half_dt * k2_pth, ray_pphi + half_dt * k2_pphi,
								 k3_r, k3_th, k3_phi, k3_pr, k3_pth, k3_pphi);

						double k4_r = 0.0, k4_th = 0.0, k4_phi = 0.0, k4_pr = 0.0, k4_pth = 0.0, k4_pphi = 0.0;
						eval_acc(ray_r + dt * k3_r, ray_theta + dt * k3_th, ray_phi + dt * k3_phi,
								 ray_pr + dt * k3_pr, ray_ptheta + dt * k3_pth, ray_pphi + dt * k3_pphi,
								 k4_r, k4_th, k4_phi, k4_pr, k4_pth, k4_pphi);

						const double sixth_dt = dt * (1.0 / 6.0);
						ray_r += sixth_dt * (k1_r + 2.0 * k2_r + 2.0 * k3_r + k4_r);
						ray_theta += sixth_dt * (k1_th + 2.0 * k2_th + 2.0 * k3_th + k4_th);
						ray_phi += sixth_dt * (k1_phi + 2.0 * k2_phi + 2.0 * k3_phi + k4_phi);
						ray_pr += sixth_dt * (k1_pr + 2.0 * k2_pr + 2.0 * k3_pr + k4_pr);
						ray_ptheta += sixth_dt * (k1_pth + 2.0 * k2_pth + 2.0 * k3_pth + k4_pth);
						ray_pphi += sixth_dt * (k1_pphi + 2.0 * k2_pphi + 2.0 * k3_pphi + k4_pphi);

						if (ray_theta < 0.0) {
							ray_theta = -ray_theta;
							ray_phi += std::numbers::pi_v<double>;
							ray_ptheta = -ray_ptheta;
						} else if (ray_theta > std::numbers::pi_v<double>) {
							ray_theta = 2.0 * std::numbers::pi_v<double> - ray_theta;
							ray_phi += std::numbers::pi_v<double>;
							ray_ptheta = -ray_ptheta;
						}

						const double mid_plane = std::numbers::pi_v<double> * 0.5;
						if (has_accretion_disk && (prev_theta - mid_plane) * (ray_theta - mid_plane) <= 0.0) {
							const double d_th_span = std::abs(ray_theta - prev_theta);
							const double s_cross = (d_th_span > 1e-12) ? std::clamp(std::abs(prev_theta - mid_plane) / d_th_span, 0.0, 1.0) : 0.5;
							const double r_cross = prev_r + s_cross * (ray_r - prev_r);
							const double phi_cross = prev_phi + s_cross * (ray_phi - prev_phi);

							if (r_cross >= isco && r_cross <= disk_outer) {
								status |= PixelFlags::ACCRETION_DISK_HIT;

								const double v_orb = std::sqrt(m / r_cross);
								const double omega_orb = v_orb / r_cross;
								const double gamma_orb = 1.0 / std::sqrt(std::max(1.0 - 3.0 * m / r_cross, 1e-4));

								const double l_over_e = Lz_cons / std::max(std::abs(E_cons), 1e-12);
								const double denom_g = gamma_orb * (1.0 - omega_orb * l_over_e);
								const double g_doppler = (std::abs(denom_g) > 1e-12) ? (std::sqrt(std::max(1.0 - rs / r_cross, 1e-4)) / denom_g) : 1.0;
								redshift_rec = g_doppler;

								const double t_norm = std::pow(isco / r_cross, 0.75) * std::pow(std::max(1.0 - std::sqrt(isco / r_cross), 0.0), 0.25);
								const double t_eff_k = 18000.0 * t_norm + 1200.0;
								const double t_obs = t_eff_k * g_doppler;

								const double g4 = g_doppler * g_doppler * g_doppler * g_doppler;
								const double radial_envelope = std::clamp((disk_outer - r_cross) / (1.5 * m), 0.0, 1.0) * std::clamp((r_cross - isco) / (0.8 * m), 0.0, 1.0);
								const double turbulence = 1.0 - (0.12 * turbulence_aa_factor) + (0.12 * turbulence_aa_factor) * std::sin(8.0 * phi_cross - 4.0 * std::log(r_cross / isco));
								const double flux_intensity = std::max(g4 * t_norm * radial_envelope * turbulence, 0.0) * 1.5;

								const auto disk_rgb = temperature_to_linear_rgb(t_obs, flux_intensity);
								const double alpha_opacity = std::clamp(radial_envelope * 0.95, 0.0, 0.98);

								accumulated_r += throughput * static_cast<double>(disk_rgb[0]);
								accumulated_g += throughput * static_cast<double>(disk_rgb[1]);
								accumulated_b += throughput * static_cast<double>(disk_rgb[2]);
								throughput *= (1.0 - alpha_opacity);
							}
						}
					}

					if (status & PixelFlags::CELESTIAL_HIT || throughput > 0.01) {
						const double sin_t = std::sin(ray_theta);
						const double cos_t = std::cos(ray_theta);
						const double sin_p = std::sin(ray_phi);
						const double cos_p = std::cos(ray_phi);

						const double px = ray_pr * sin_t * cos_p + ray_r * ray_ptheta * cos_t * cos_p - ray_r * sin_t * ray_pphi * sin_p;
						const double py = ray_pr * sin_t * sin_p + ray_r * ray_ptheta * cos_t * sin_p + ray_r * sin_t * ray_pphi * cos_p;
						const double pz = ray_pr * cos_t - ray_r * ray_ptheta * sin_t;

						const double p_len = std::sqrt(px * px + py * py + pz * pz);
						const double inv_plen = (p_len > 1e-12) ? (1.0 / p_len) : 1.0;

						const auto sky_rgb = compute_sky_radiance(px * inv_plen, py * inv_plen, pz * inv_plen, params);
						accumulated_r += throughput * static_cast<double>(sky_rgb[0]);
						accumulated_g += throughput * static_cast<double>(sky_rgb[1]);
						accumulated_b += throughput * static_cast<double>(sky_rgb[2]);
					}

					const auto mapped_srgb = apply_tonemapping(
						{accumulated_r, accumulated_g, accumulated_b},
						params.tonemapping_mode,
						params.camera_exposure
					);

					output_framebuffer[pixel_idx] = GpuPixelOutput{
						.r = mapped_srgb[0],
						.g = mapped_srgb[1],
						.b = mapped_srgb[2],
						.a = 1.0f,
						.redshift = static_cast<float>(redshift_rec),
						.affine_parameter = static_cast<float>(iters * 0.05),
						.status_flags = status,
						.iterations_used = iters
					};
				}
			}
		};

		auto render_slice = [&](size_t y_start, size_t y_end) noexcept {
			render_rect(0, width, y_start, y_end);
		};

		auto render_tile_range = [&](size_t t_start, size_t t_end) noexcept {
			constexpr size_t TILE = 32;
			const size_t tiles_x = (width + TILE - 1) / TILE;
			for (size_t t = t_start; t < t_end; ++t) {
				if (cancel_flag && cancel_flag->load(std::memory_order_relaxed)) return;
				const size_t tx = (t % tiles_x) * TILE;
				const size_t ty = (t / tiles_x) * TILE;
				render_rect(tx, std::min(tx + TILE, width), ty, std::min(ty + TILE, height));
			}
		};

		const bool use_tiling = (params.render_flags & RenderFlags::USE_TILED_DISTRIBUTION) != 0U;
		constexpr size_t TILE_DIM = 32;
		const size_t total_tiles = ((width + TILE_DIM - 1) / TILE_DIM) * ((height + TILE_DIM - 1) / TILE_DIM);

		if (pool != nullptr && !(params.render_flags & RenderFlags::USE_PER_FRAME_THREADS)) {
			if (use_tiling) {
				pool->parallel_for(total_tiles, [&](size_t t_start, size_t t_end) noexcept {
					render_tile_range(t_start, t_end);
				});
			} else {
				pool->parallel_for(height, [&](size_t y_start, size_t y_end) noexcept {
					render_slice(y_start, y_end);
				});
			}
		} else {
			if (use_tiling) {
				const size_t tiles_per_thread = (total_tiles + num_threads - 1) / num_threads;
				for (size_t t = 0; t < num_threads; ++t) {
					const size_t t_start = t * tiles_per_thread;
					const size_t t_end = std::min(t_start + tiles_per_thread, total_tiles);
					if (t_start < t_end) {
						workers.emplace_back(render_tile_range, t_start, t_end);
					}
				}
			} else {
				const size_t rows_per_thread = (height + num_threads - 1) / num_threads;
				for (size_t t = 0; t < num_threads; ++t) {
					const size_t y_start = t * rows_per_thread;
					const size_t y_end = std::min(y_start + rows_per_thread, height);
					if (y_start < y_end) {
						workers.emplace_back(render_slice, y_start, y_end);
					}
				}
			}
		}
	}

	static void dispatch_fp64_simd(
		const GpuCameraPushConstants& params,
		std::span<GpuPixelOutput> output_framebuffer,
		Core::ThreadPool* pool = nullptr,
		const std::atomic<bool>* cancel_flag = nullptr
	) noexcept {
		const size_t width = params.screen_width;
		const size_t height = params.screen_height;
		const size_t total_pixels = width * height;

		if (output_framebuffer.size() < total_pixels || width == 0 || height == 0) {
			return;
		}

		const uint32_t metric_id = params.metric_type;
		const bool is_wormhole = (metric_id == 8);
		const bool is_warp = (metric_id == 9);
		const bool is_flat = (metric_id == 0);
		const bool has_accretion_disk = (metric_id == 1 || metric_id == 2 || metric_id == 3 || metric_id == 4 || metric_id == 5);
		const bool has_event_horizon = (!is_wormhole && !is_warp && !is_flat);

		const double m = std::max(params.metric_mass, 1e-4);
		const double a_spin = std::clamp(params.metric_spin, -0.999 * m, 0.999 * m);
		const double rs = 2.0 * m;
		const double rh = (std::abs(a_spin) > 1e-12) ? (m + std::sqrt(std::max(m * m - a_spin * a_spin, 0.0))) : rs;
		const double aspect = static_cast<double>(width) / static_cast<double>(height);

		const double isco = (std::abs(a_spin) > 1e-12) ? std::max(rh * 1.05, 6.0 * m - 4.0 * a_spin) : (6.0 * m);
		const double disk_outer = 24.0 * m;

		const double fwd_x = params.tetrad_e1[1], fwd_y = params.tetrad_e1[2], fwd_z = params.tetrad_e1[3];
		const double rgt_x = params.tetrad_e2[1], rgt_y = params.tetrad_e2[2], rgt_z = params.tetrad_e2[3];
		const double up_x  = params.tetrad_e3[1], up_y  = params.tetrad_e3[2], up_z  = params.tetrad_e3[3];

		const bool space_skip_enabled = (params.render_flags & RenderFlags::SPACE_SKIP_ENABLED) != 0U;
		const double effective_space_skip_radius = has_accretion_disk
			? std::max(params.space_skip_radius_scale * m, disk_outer * 1.05)
			: std::max(params.space_skip_radius_scale * m, rh * 2.0);

		const bool lod_active_simd = (params.render_flags & RenderFlags::USE_LOD_SYSTEM) != 0U && params.lod_distance_threshold > 0.0;
		const double r_obs_pre = std::max(params.observer_position[1], rh * 1.02);
		const uint32_t effective_max_steps_simd = (lod_active_simd && r_obs_pre > params.lod_distance_threshold)
			? std::min(params.max_integration_steps, params.lod_reduced_steps)
			: params.max_integration_steps;

		const double r_obs = std::max(params.observer_position[1], rh * 1.02);
		const double theta_obs = std::clamp(params.observer_position[2], 0.001, std::numbers::pi_v<double> - 0.001);
		const double phi_obs = params.observer_position[3];

		const double angular_pixel_size_simd = params.field_of_view_rad / std::max(static_cast<double>(width), 1.0);
		const double bh_angular_diameter_simd = (2.0 * rh) / r_obs;
		const double turbulence_aa_factor_simd = std::clamp(bh_angular_diameter_simd / std::max(angular_pixel_size_simd * 24.0, 1e-9), 0.0, 1.0);

		const double sin_to = std::sin(theta_obs);
		const double cos_to = std::cos(theta_obs);
		const double sin_po = std::sin(phi_obs);
		const double cos_po = std::cos(phi_obs);

		const double er_x = sin_to * cos_po, er_y = sin_to * sin_po, er_z = cos_to;
		const double eth_x = cos_to * cos_po, eth_y = cos_to * sin_po, eth_z = -sin_to;
		const double eph_x = -sin_po, eph_y = cos_po, eph_z = 0.0;

		const double factor_obs = std::max(1.0 - rs / r_obs, 1e-6);
		const double sqrt_factor_obs = std::sqrt(factor_obs);

		const auto proj_mode = static_cast<Observer::ProjectionMode>(params.projection_mode);
		const double fov_rad = params.field_of_view_rad;

		auto render_simd_rect = [&](size_t x_start, size_t x_end, size_t y_start, size_t y_end) noexcept {
			for (size_t y = y_start; y < y_end; ++y) {
				if (cancel_flag && cancel_flag->load(std::memory_order_relaxed)) return;
				const double v_norm = 1.0 - (static_cast<double>(y) + 0.5) / static_cast<double>(height) * 2.0;

				for (size_t x = x_start; x < x_end; x += 4) {
					Core::GeodesicBundle4d bundle;
					const size_t lanes = std::min(size_t{4}, x_end - x);

					std::array<double, 4> accum_r{0.0, 0.0, 0.0, 0.0};
					std::array<double, 4> accum_g{0.0, 0.0, 0.0, 0.0};
					std::array<double, 4> accum_b{0.0, 0.0, 0.0, 0.0};
					std::array<double, 4> throughput{1.0, 1.0, 1.0, 1.0};
					std::array<double, 4> redshift_rec{1.0, 1.0, 1.0, 1.0};
					std::array<uint32_t, 4> status{0, 0, 0, 0};
					std::array<uint32_t, 4> iters{0, 0, 0, 0};

					for (size_t l = 0; l < 4; ++l) {
						if (l < lanes) {
							const size_t cur_x = x + l;
							const bool is_allsky = (proj_mode == Observer::ProjectionMode::Equirectangular360 || proj_mode == Observer::ProjectionMode::HammerAitoff);
							const double u_norm = is_allsky
								? (((static_cast<double>(cur_x) + 0.5) / static_cast<double>(width)) * 2.0 - 1.0)
								: (((static_cast<double>(cur_x) + 0.5) / static_cast<double>(width) * 2.0 - 1.0) * aspect);

							const auto n_local = Observer::CameraProjector<double>::compute_ray_direction(
								proj_mode, u_norm, v_norm, fov_rad
							);

							const double ray_dir_x = n_local[0] * fwd_x + n_local[2] * rgt_x + n_local[1] * up_x;
							const double ray_dir_y = n_local[0] * fwd_y + n_local[2] * rgt_y + n_local[1] * up_y;
							const double ray_dir_z = n_local[0] * fwd_z + n_local[2] * rgt_z + n_local[1] * up_z;

							const double n_r = ray_dir_x * er_x + ray_dir_y * er_y + ray_dir_z * er_z;
							const double n_th = ray_dir_x * eth_x + ray_dir_y * eth_y + ray_dir_z * eth_z;
							const double n_ph = ray_dir_x * eph_x + ray_dir_y * eph_y + ray_dir_z * eph_z;

							bundle.x0[l] = 0.0;
							bundle.x1[l] = r_obs;
							bundle.x2[l] = theta_obs;
							bundle.x3[l] = phi_obs;

							bundle.p0[l] = 1.0 / factor_obs;
							bundle.p1[l] = n_r / sqrt_factor_obs;
							bundle.p2[l] = n_th / r_obs;
							bundle.p3[l] = n_ph / (r_obs * sin_to);
							bundle.active_mask[l] = true;
						} else {
							bundle.x0[l] = 0.0;
							bundle.x1[l] = r_obs;
							bundle.x2[l] = theta_obs;
							bundle.x3[l] = phi_obs;
							bundle.p0[l] = 1.0;
							bundle.p1[l] = 0.0;
							bundle.p2[l] = 0.0;
							bundle.p3[l] = 0.0;
							bundle.active_mask[l] = false;
						}
					}

					const uint32_t step_limit = effective_max_steps_simd;
					for (uint32_t step = 0; step < step_limit && bundle.active_mask.any(); ++step) {
						for (size_t l = 0; l < lanes; ++l) {
							if (bundle.active_mask[l]) {
								iters[l] = step + 1;
								const double delta_k = bundle.x1[l] * bundle.x1[l] - 2.0 * m * bundle.x1[l] + a_spin * a_spin;
								if (has_event_horizon && (bundle.x1[l] <= rh * 1.0001 || delta_k <= 0.0)) {
									status[l] |= PixelFlags::HORIZON_ABSORBED;
									throughput[l] = 0.0;
									bundle.active_mask[l] = false;
								} else if (bundle.x1[l] >= params.escape_radius) {
									status[l] |= PixelFlags::CELESTIAL_HIT;
									bundle.active_mask[l] = false;
								}
							}
						}

						if (!bundle.active_mask.any()) break;

						if (space_skip_enabled) {
							for (size_t l = 0; l < lanes; ++l) {
								if (bundle.active_mask[l] && bundle.x1[l] > effective_space_skip_radius) {
									static_cast<void>(attempt_analytic_space_skip(bundle.x1[l], bundle.x2[l], bundle.x3[l], bundle.p1[l], bundle.p2[l], bundle.p3[l], effective_space_skip_radius, params.escape_radius));
								}
							}
						}

						for (size_t l = 0; l < 4; ++l) {
							if (bundle.active_mask[l]) {
								const double r_scale = std::max(bundle.x1[l] - rh, 0.02 * m);
								const double smooth_dt = 0.06 * std::sqrt(bundle.x1[l] * r_scale);
								const double pole_guard = std::clamp(std::abs(std::sin(bundle.x2[l])) * 12.0, 0.15, 1.0);
								bundle.step_size[l] = -std::clamp(smooth_dt, 0.005, 4.0) * pole_guard;
							} else {
								bundle.step_size[l] = -0.01;
							}
						}

						std::array<double, 4> prev_r{bundle.x1[0], bundle.x1[1], bundle.x1[2], bundle.x1[3]};
						std::array<double, 4> prev_th{bundle.x2[0], bundle.x2[1], bundle.x2[2], bundle.x2[3]};
						std::array<double, 4> prev_phi{bundle.x3[0], bundle.x3[1], bundle.x3[2], bundle.x3[3]};

						bundle.step_rk4_schwarzschild(m, 1.0, 1.0);

						for (size_t l = 0; l < lanes; ++l) {
							if (bundle.horizon_mask[l]) {
								status[l] |= PixelFlags::HORIZON_ABSORBED;
								throughput[l] = 0.0;
							}
							if (bundle.celestial_mask[l]) {
								status[l] |= PixelFlags::CELESTIAL_HIT;
							}
						}

						const double mid_plane = std::numbers::pi_v<double> * 0.5;
						for (size_t l = 0; l < lanes; ++l) {
							if (has_accretion_disk && throughput[l] > 0.01 && ((prev_th[l] - mid_plane) * (bundle.x2[l] - mid_plane) <= 0.0)) {
								const double d_th_span = std::abs(bundle.x2[l] - prev_th[l]);
								const double s_cross = (d_th_span > 1e-12) ? std::clamp(std::abs(prev_th[l] - mid_plane) / d_th_span, 0.0, 1.0) : 0.5;
								const double r_cross = prev_r[l] + s_cross * (bundle.x1[l] - prev_r[l]);
								const double phi_cross = prev_phi[l] + s_cross * (bundle.x3[l] - prev_phi[l]);

								if (r_cross >= isco && r_cross <= disk_outer) {
									status[l] |= PixelFlags::ACCRETION_DISK_HIT;

									const double v_orb = std::sqrt(m / r_cross);
									const double omega_orb = v_orb / r_cross;
									const double gamma_orb = 1.0 / std::sqrt(std::max(1.0 - 3.0 * m / r_cross, 1e-4));

									const double Lz_val = bundle.p3[l] * (bundle.x1[l] * bundle.x1[l] * std::max(std::sin(bundle.x2[l]) * std::sin(bundle.x2[l]), 1e-6));
									const double E_val = 1.0;
									const double l_over_e = Lz_val / std::max(std::abs(E_val), 1e-12);
									const double denom_g = gamma_orb * (1.0 - omega_orb * l_over_e);
									const double g_doppler = (std::abs(denom_g) > 1e-12) ? (std::sqrt(std::max(1.0 - rs / r_cross, 1e-4)) / denom_g) : 1.0;
									redshift_rec[l] = g_doppler;

									const double t_norm = std::pow(isco / r_cross, 0.75) * std::pow(std::max(1.0 - std::sqrt(isco / r_cross), 0.0), 0.25);
									const double t_eff_k = 18000.0 * t_norm + 1200.0;
									const double t_obs = t_eff_k * g_doppler;

									const double g4 = g_doppler * g_doppler * g_doppler * g_doppler;
									const double radial_envelope = std::clamp((disk_outer - r_cross) / (1.5 * m), 0.0, 1.0) * std::clamp((r_cross - isco) / (0.8 * m), 0.0, 1.0);
									const double turbulence = 1.0 - (0.12 * turbulence_aa_factor_simd) + (0.12 * turbulence_aa_factor_simd) * std::sin(8.0 * phi_cross - 4.0 * std::log(r_cross / isco));
									const double flux_intensity = std::max(g4 * t_norm * radial_envelope * turbulence, 0.0) * 1.5;

									const auto disk_rgb = temperature_to_linear_rgb(t_obs, flux_intensity);
									const double alpha_opacity = std::clamp(radial_envelope * 0.95, 0.0, 0.98);

									accum_r[l] += throughput[l] * static_cast<double>(disk_rgb[0]);
									accum_g[l] += throughput[l] * static_cast<double>(disk_rgb[1]);
									accum_b[l] += throughput[l] * static_cast<double>(disk_rgb[2]);
									throughput[l] *= (1.0 - alpha_opacity);
								}
							}
						}
					}

					for (size_t l = 0; l < lanes; ++l) {
						if (status[l] & PixelFlags::CELESTIAL_HIT || throughput[l] > 0.01) {
							const double sin_t = std::sin(bundle.x2[l]);
							const double cos_t = std::cos(bundle.x2[l]);
							const double sin_p = std::sin(bundle.x3[l]);
							const double cos_p = std::cos(bundle.x3[l]);

							const double px = bundle.p1[l] * sin_t * cos_p + bundle.x1[l] * bundle.p2[l] * cos_t * cos_p - bundle.x1[l] * sin_t * bundle.p3[l] * sin_p;
							const double py = bundle.p1[l] * sin_t * sin_p + bundle.x1[l] * bundle.p2[l] * cos_t * sin_p + bundle.x1[l] * sin_t * bundle.p3[l] * cos_p;
							const double pz = bundle.p1[l] * cos_t - bundle.x1[l] * bundle.p2[l] * sin_t;

							const double p_len = std::sqrt(px * px + py * py + pz * pz);
							const double inv_plen = (p_len > 1e-12) ? (1.0 / p_len) : 1.0;
							const double dir_x = (p_len > 1e-12) ? (px * inv_plen) : 0.0;
							const double dir_y = (p_len > 1e-12) ? (py * inv_plen) : 1.0;
							const double dir_z = (p_len > 1e-12) ? (pz * inv_plen) : 0.0;

							const auto sky_rgb = compute_sky_radiance(dir_x, dir_y, dir_z, params);
							accum_r[l] += throughput[l] * static_cast<double>(sky_rgb[0]);
							accum_g[l] += throughput[l] * static_cast<double>(sky_rgb[1]);
							accum_b[l] += throughput[l] * static_cast<double>(sky_rgb[2]);
						}

						const auto mapped_srgb = apply_tonemapping(
							{accum_r[l], accum_g[l], accum_b[l]},
							params.tonemapping_mode,
							params.camera_exposure
						);

						const size_t out_idx = y * width + (x + l);
						if (out_idx < output_framebuffer.size()) {
							output_framebuffer[out_idx] = GpuPixelOutput{
								.r = mapped_srgb[0],
								.g = mapped_srgb[1],
								.b = mapped_srgb[2],
								.a = 1.0f,
								.redshift = static_cast<float>(redshift_rec[l]),
								.affine_parameter = static_cast<float>(iters[l] * 0.05),
								.status_flags = status[l],
								.iterations_used = iters[l]
							};
						}
					}
				}
			}
		};

		auto render_simd_slice = [&](size_t y_start, size_t y_end) noexcept {
			render_simd_rect(0, width, y_start, y_end);
		};

		auto render_simd_tile_range = [&](size_t t_start, size_t t_end) noexcept {
			constexpr size_t TILE = 32;
			const size_t tiles_x = (width + TILE - 1) / TILE;
			for (size_t t = t_start; t < t_end; ++t) {
				if (cancel_flag && cancel_flag->load(std::memory_order_relaxed)) return;
				const size_t tx = (t % tiles_x) * TILE;
				const size_t ty = (t / tiles_x) * TILE;
				render_simd_rect(tx, std::min(tx + TILE, width), ty, std::min(ty + TILE, height));
			}
		};

		const bool use_tiling = (params.render_flags & RenderFlags::USE_TILED_DISTRIBUTION) != 0U;
		constexpr size_t TILE_DIM = 32;
		const size_t total_tiles = ((width + TILE_DIM - 1) / TILE_DIM) * ((height + TILE_DIM - 1) / TILE_DIM);

		if (pool != nullptr && !(params.render_flags & RenderFlags::USE_PER_FRAME_THREADS)) {
			if (use_tiling) {
				pool->parallel_for(total_tiles, [&](size_t t_start, size_t t_end) noexcept {
					render_simd_tile_range(t_start, t_end);
				});
			} else {
				pool->parallel_for(height, [&](size_t y_start, size_t y_end) noexcept {
					render_simd_slice(y_start, y_end);
				});
			}
		} else {
			const unsigned int num_threads = std::max(1u, std::thread::hardware_concurrency());
			std::vector<std::jthread> workers;
			workers.reserve(num_threads);
			if (use_tiling) {
				const size_t tiles_per_thread = (total_tiles + num_threads - 1) / num_threads;
				for (size_t t = 0; t < num_threads; ++t) {
					const size_t t_start = t * tiles_per_thread;
					const size_t t_end = std::min(t_start + tiles_per_thread, total_tiles);
					if (t_start < t_end) {
						workers.emplace_back(render_simd_tile_range, t_start, t_end);
					}
				}
			} else {
				const size_t rows_per_thread = (height + num_threads - 1) / num_threads;
				for (size_t t = 0; t < num_threads; ++t) {
					const size_t y_start = t * rows_per_thread;
					const size_t y_end = std::min(y_start + rows_per_thread, height);
					if (y_start < y_end) {
						workers.emplace_back(render_simd_slice, y_start, y_end);
					}
				}
			}
		}
	}

	static void dispatch_fp32_scalar(
		const GpuCameraPushConstants& params,
		std::span<GpuPixelOutput> output_framebuffer,
		Core::ThreadPool* pool = nullptr,
		const std::atomic<bool>* cancel_flag = nullptr
	) noexcept {
		const size_t width = params.screen_width;
		const size_t height = params.screen_height;
		const size_t total_pixels = width * height;

		if (output_framebuffer.size() < total_pixels || width == 0 || height == 0) {
			return;
		}

		const unsigned int num_threads = std::max(1u, std::thread::hardware_concurrency());
		std::vector<std::jthread> workers;
		workers.reserve(num_threads);

		const uint32_t metric_id = params.metric_type;
		const bool is_wormhole = (metric_id == 8);
		const bool is_warp = (metric_id == 9);
		const bool is_flat = (metric_id == 0);
		const bool has_accretion_disk = (metric_id == 1 || metric_id == 2 || metric_id == 3 || metric_id == 4 || metric_id == 5);
		const bool has_event_horizon = (!is_wormhole && !is_warp && !is_flat);

		const float m = static_cast<float>(std::max(params.metric_mass, 1e-4));
		const float a_spin = static_cast<float>(std::clamp(params.metric_spin, -0.999 * params.metric_mass, 0.999 * params.metric_mass));
		const float rs = 2.0f * m;
		const float rh = (std::abs(a_spin) > 1e-6f) ? (m + std::sqrt(std::max(m * m - a_spin * a_spin, 0.0f))) : rs;
		const float aspect = static_cast<float>(width) / static_cast<float>(height);

		const float isco = (std::abs(a_spin) > 1e-6f) ? std::max(rh * 1.05f, 6.0f * m - 4.0f * a_spin) : (6.0f * m);
		const float disk_outer = 24.0f * m;

		const float fwd_x = static_cast<float>(params.tetrad_e1[1]), fwd_y = static_cast<float>(params.tetrad_e1[2]), fwd_z = static_cast<float>(params.tetrad_e1[3]);
		const float rgt_x = static_cast<float>(params.tetrad_e2[1]), rgt_y = static_cast<float>(params.tetrad_e2[2]), rgt_z = static_cast<float>(params.tetrad_e2[3]);
		const float up_x  = static_cast<float>(params.tetrad_e3[1]), up_y  = static_cast<float>(params.tetrad_e3[2]), up_z  = static_cast<float>(params.tetrad_e3[3]);

		const float pi_f = std::numbers::pi_v<float>;

		const bool space_skip_enabled = (params.render_flags & RenderFlags::SPACE_SKIP_ENABLED) != 0U;
		const float effective_space_skip_radius = has_accretion_disk
			? std::max(static_cast<float>(params.space_skip_radius_scale) * m, disk_outer * 1.05f)
			: std::max(static_cast<float>(params.space_skip_radius_scale) * m, rh * 2.0f);

		const bool use_exact_metric_path = requires_exact_metric_path(params);
		const bool lod_active = (params.render_flags & RenderFlags::USE_LOD_SYSTEM) != 0U && params.lod_distance_threshold > 0.0;
		const double r_obs_frame = std::max(params.observer_position[1], static_cast<double>(rh) * 1.02);
		const uint32_t effective_max_steps = (lod_active && r_obs_frame > params.lod_distance_threshold)
			? std::min(params.max_integration_steps, params.lod_reduced_steps)
			: params.max_integration_steps;

		const double angular_pixel_size_d = params.field_of_view_rad / std::max(static_cast<double>(width), 1.0);
		const double bh_angular_diameter_d = (2.0 * static_cast<double>(rh)) / r_obs_frame;
		const float turbulence_aa_factor = static_cast<float>(std::clamp(bh_angular_diameter_d / std::max(angular_pixel_size_d * 24.0, 1e-9), 0.0, 1.0));

		auto render_rect = [&](size_t x_start, size_t x_end, size_t y_start, size_t y_end) noexcept {
			for (size_t y = y_start; y < y_end; ++y) {
				if (cancel_flag && cancel_flag->load(std::memory_order_relaxed)) return;
				const float v_norm = 1.0f - (static_cast<float>(y) + 0.5f) / static_cast<float>(height) * 2.0f;
				for (size_t x = x_start; x < x_end; ++x) {
					const size_t pixel_idx = y * width + x;
					if (pixel_idx >= output_framebuffer.size()) continue;
					const float u_norm = ((static_cast<float>(x) + 0.5f) / static_cast<float>(width) * 2.0f - 1.0f) * aspect;

					const bool is_allsky = (params.projection_mode == 3 || params.projection_mode == 7);
					const float u_coord = is_allsky ? (((static_cast<float>(x) + 0.5f) / static_cast<float>(width)) * 2.0f - 1.0f) : u_norm;
					const auto n_local = Observer::CameraProjector<float>::compute_ray_direction(
						static_cast<Observer::ProjectionMode>(params.projection_mode),
						u_coord, v_norm, static_cast<float>(params.field_of_view_rad)
					);

					const float ray_dir_x = n_local[0] * fwd_x + n_local[2] * rgt_x + n_local[1] * up_x;
					const float ray_dir_y = n_local[0] * fwd_y + n_local[2] * rgt_y + n_local[1] * up_y;
					const float ray_dir_z = n_local[0] * fwd_z + n_local[2] * rgt_z + n_local[1] * up_z;

					const float r_obs = std::max(static_cast<float>(params.observer_position[1]), rh * 1.02f);
					const float theta_obs = std::clamp(static_cast<float>(params.observer_position[2]), 0.001f, pi_f - 0.001f);
					const float phi_obs = static_cast<float>(params.observer_position[3]);

					const float sin_to = std::sin(theta_obs);
					const float cos_to = std::cos(theta_obs);
					const float sin_po = std::sin(phi_obs);
					const float cos_po = std::cos(phi_obs);

					const float er_x = sin_to * cos_po, er_y = sin_to * sin_po, er_z = cos_to;
					const float eth_x = cos_to * cos_po, eth_y = cos_to * sin_po, eth_z = -sin_to;
					const float eph_x = -sin_po, eph_y = cos_po, eph_z = 0.0f;

					const float n_r = ray_dir_x * er_x + ray_dir_y * er_y + ray_dir_z * er_z;
					const float n_th = ray_dir_x * eth_x + ray_dir_y * eth_y + ray_dir_z * eth_z;
					const float n_ph = ray_dir_x * eph_x + ray_dir_y * eph_y + ray_dir_z * eph_z;

					if (use_exact_metric_path) {
						output_framebuffer[pixel_idx] = trace_exact_photon_dispatch(
							params.metric_type, static_cast<double>(m), static_cast<double>(a_spin), params.metric_charge,
							static_cast<double>(n_r), static_cast<double>(n_th), static_cast<double>(n_ph),
							static_cast<double>(r_obs), static_cast<double>(theta_obs), static_cast<double>(phi_obs),
							static_cast<double>(rs), params.escape_radius, effective_max_steps,
							has_accretion_disk, static_cast<double>(turbulence_aa_factor), params
						);
						continue;
					}

					const float factor_obs = std::max(1.0f - rs / r_obs, 1e-4f);
					const float sqrt_factor_obs = std::sqrt(factor_obs);

					const float p_r_init = n_r / sqrt_factor_obs;
					const float p_theta_init = n_th / r_obs;
					const float p_phi_init = n_ph / (r_obs * sin_to);

					float ray_r = r_obs;
					float ray_theta = theta_obs;
					float ray_phi = phi_obs;

					float ray_pr = p_r_init;
					float ray_ptheta = p_theta_init;
					float ray_pphi = p_phi_init;

					const float E_cons = 1.0f;
					const float Lz_cons = p_phi_init * (r_obs * r_obs * sin_to * sin_to);

					float accumulated_r = 0.0f;
					float accumulated_g = 0.0f;
					float accumulated_b = 0.0f;
					float throughput = 1.0f;
					float redshift_rec = 1.0f;

					uint32_t status = 0;
					uint32_t iters = 0;

					for (uint32_t step = 0; step < effective_max_steps && throughput > 0.01f; ++step) {
						iters = step + 1;

						const float delta_kerr_f = ray_r * ray_r - 2.0f * m * ray_r + a_spin * a_spin;
						if (has_event_horizon && (ray_r <= rh * 1.0001f || delta_kerr_f <= 0.0f)) {
							status = PixelFlags::HORIZON_ABSORBED;
							throughput = 0.0f;
							break;
						}

						if (ray_r >= static_cast<float>(params.escape_radius)) {
							status = PixelFlags::CELESTIAL_HIT;
							break;
						}

						if (space_skip_enabled && ray_r > effective_space_skip_radius &&
							attempt_analytic_space_skip(ray_r, ray_theta, ray_phi, ray_pr, ray_ptheta, ray_pphi, effective_space_skip_radius, static_cast<float>(params.escape_radius))) {
							continue;
						}

						const float r_scale = std::max(ray_r - rh, 0.02f * m);
						const float smooth_dt = 0.05f * std::sqrt(ray_r * r_scale);
						const float pole_guard = std::clamp(std::abs(std::sin(ray_theta)) * 12.0f, 0.15f, 1.0f);
						const float dt = -std::clamp(smooth_dt, 0.004f, 3.5f) * pole_guard;

						const float prev_r = ray_r;
						const float prev_theta = ray_theta;
						const float prev_phi = ray_phi;

						auto eval_acc = [&](float r_eval, float th_eval, float phi_eval,
											float pr_eval, float pth_eval, float pphi_eval,
											float& d_r, float& d_th, float& d_phi,
											float& d_pr, float& d_pth, float& d_pphi) noexcept {
							static_cast<void>(phi_eval);
							const float r = std::max(r_eval, rh * 1.0001f);
							const float th = std::clamp(th_eval, 1e-4f, pi_f - 1e-4f);
							const float sin_t = std::sin(th);
							const float cos_t = std::cos(th);
							const float sin2_t = std::max(sin_t * sin_t, 1e-8f);
							const float r2 = r * r;
							const float f = 1.0f - rs / r;
							const float safe_f = std::max(f, 1e-4f);

							d_r = pr_eval;
							d_th = pth_eval;
							d_phi = pphi_eval;

							const float p_t = E_cons / safe_f;
							const float g001 = rs / (2.0f * r2 * safe_f);
							const float g100 = (rs * safe_f) / (2.0f * r2);
							const float g111 = -g001;
							const float g122 = -r * safe_f;
							const float g133 = -r * safe_f * sin2_t;
							const float g212 = 1.0f / r;
							const float g233 = -sin_t * cos_t;
							const float g313 = 1.0f / r;
							const float safe_sin_t = (std::abs(sin_t) > 1e-5f) ? sin_t : ((sin_t >= 0.0f) ? 1e-5f : -1e-5f);
							const float g323 = cos_t / safe_sin_t;

							d_pr = -(g100 * p_t * p_t + g111 * pr_eval * pr_eval + g122 * pth_eval * pth_eval + g133 * pphi_eval * pphi_eval);
							d_pth = -(2.0f * g212 * pr_eval * pth_eval + g233 * pphi_eval * pphi_eval);
							d_pphi = -2.0f * (g313 * pr_eval * pphi_eval + g323 * pth_eval * pphi_eval);
						};

						float k1_r = 0.0f, k1_th = 0.0f, k1_phi = 0.0f, k1_pr = 0.0f, k1_pth = 0.0f, k1_pphi = 0.0f;
						eval_acc(ray_r, ray_theta, ray_phi, ray_pr, ray_ptheta, ray_pphi, k1_r, k1_th, k1_phi, k1_pr, k1_pth, k1_pphi);

						const float half_dt = 0.5f * dt;
						float k2_r = 0.0f, k2_th = 0.0f, k2_phi = 0.0f, k2_pr = 0.0f, k2_pth = 0.0f, k2_pphi = 0.0f;
						eval_acc(ray_r + half_dt * k1_r, ray_theta + half_dt * k1_th, ray_phi + half_dt * k1_phi,
								 ray_pr + half_dt * k1_pr, ray_ptheta + half_dt * k1_pth, ray_pphi + half_dt * k1_pphi,
								 k2_r, k2_th, k2_phi, k2_pr, k2_pth, k2_pphi);

						float k3_r = 0.0f, k3_th = 0.0f, k3_phi = 0.0f, k3_pr = 0.0f, k3_pth = 0.0f, k3_pphi = 0.0f;
						eval_acc(ray_r + half_dt * k2_r, ray_theta + half_dt * k2_th, ray_phi + half_dt * k2_phi,
								 ray_pr + half_dt * k2_pr, ray_ptheta + half_dt * k2_pth, ray_pphi + half_dt * k2_pphi,
								 k3_r, k3_th, k3_phi, k3_pr, k3_pth, k3_pphi);

						float k4_r = 0.0f, k4_th = 0.0f, k4_phi = 0.0f, k4_pr = 0.0f, k4_pth = 0.0f, k4_pphi = 0.0f;
						eval_acc(ray_r + dt * k3_r, ray_theta + dt * k3_th, ray_phi + dt * k3_phi,
								 ray_pr + dt * k3_pr, ray_ptheta + dt * k3_pth, ray_pphi + dt * k3_pphi,
								 k4_r, k4_th, k4_phi, k4_pr, k4_pth, k4_pphi);

						const float sixth_dt = dt * (1.0f / 6.0f);
						ray_r += sixth_dt * (k1_r + 2.0f * k2_r + 2.0f * k3_r + k4_r);
						ray_theta += sixth_dt * (k1_th + 2.0f * k2_th + 2.0f * k3_th + k4_th);
						ray_phi += sixth_dt * (k1_phi + 2.0f * k2_phi + 2.0f * k3_phi + k4_phi);
						ray_pr += sixth_dt * (k1_pr + 2.0f * k2_pr + 2.0f * k3_pr + k4_pr);
						ray_ptheta += sixth_dt * (k1_pth + 2.0f * k2_pth + 2.0f * k3_pth + k4_pth);
						ray_pphi += sixth_dt * (k1_pphi + 2.0f * k2_pphi + 2.0f * k3_pphi + k4_pphi);

						if (ray_theta < 0.0f) {
							ray_theta = -ray_theta;
							ray_phi += pi_f;
							ray_ptheta = -ray_ptheta;
						} else if (ray_theta > pi_f) {
							ray_theta = 2.0f * pi_f - ray_theta;
							ray_phi += pi_f;
							ray_ptheta = -ray_ptheta;
						}

						const float mid_plane = pi_f * 0.5f;
						if (has_accretion_disk && (prev_theta - mid_plane) * (ray_theta - mid_plane) <= 0.0f) {
							const float d_th_span = std::abs(ray_theta - prev_theta);
							const float s_cross = (d_th_span > 1e-6f) ? std::clamp(std::abs(prev_theta - mid_plane) / d_th_span, 0.0f, 1.0f) : 0.5f;
							const float r_cross = prev_r + s_cross * (ray_r - prev_r);
							const float phi_cross = prev_phi + s_cross * (ray_phi - prev_phi);

							if (r_cross >= isco && r_cross <= disk_outer) {
								status |= PixelFlags::ACCRETION_DISK_HIT;

								const float v_orb = std::sqrt(m / r_cross);
								const float omega_orb = v_orb / r_cross;
								const float gamma_orb = 1.0f / std::sqrt(std::max(1.0f - 3.0f * m / r_cross, 1e-4f));

								const float l_over_e = Lz_cons / std::max(std::abs(E_cons), 1e-8f);
								const float denom_g = gamma_orb * (1.0f - omega_orb * l_over_e);
								const float g_doppler = (std::abs(denom_g) > 1e-8f) ? (std::sqrt(std::max(1.0f - rs / r_cross, 1e-4f)) / denom_g) : 1.0f;
								redshift_rec = g_doppler;

								const float t_norm = std::pow(isco / r_cross, 0.75f) * std::pow(std::max(1.0f - std::sqrt(isco / r_cross), 0.0f), 0.25f);
								const float t_eff_k = 18000.0f * t_norm + 1200.0f;
								const float t_obs = t_eff_k * g_doppler;

								const float g4 = g_doppler * g_doppler * g_doppler * g_doppler;
								const float radial_envelope = std::clamp((disk_outer - r_cross) / (1.5f * m), 0.0f, 1.0f) * std::clamp((r_cross - isco) / (0.8f * m), 0.0f, 1.0f);
								const float turbulence = 1.0f - (0.12f * turbulence_aa_factor) + (0.12f * turbulence_aa_factor) * std::sin(8.0f * phi_cross - 4.0f * std::log(r_cross / isco));
								const float flux_intensity = std::max(g4 * t_norm * radial_envelope * turbulence, 0.0f) * 1.5f;

								const auto disk_rgb = temperature_to_linear_rgb(t_obs, flux_intensity);
								const float alpha_opacity = std::clamp(radial_envelope * 0.95f, 0.0f, 0.98f);

								accumulated_r += throughput * disk_rgb[0];
								accumulated_g += throughput * disk_rgb[1];
								accumulated_b += throughput * disk_rgb[2];
								throughput *= (1.0f - alpha_opacity);
							}
						}
					}

					if (status & PixelFlags::CELESTIAL_HIT || throughput > 0.01f) {
						const float sin_t = std::sin(ray_theta);
						const float cos_t = std::cos(ray_theta);
						const float sin_p = std::sin(ray_phi);
						const float cos_p = std::cos(ray_phi);

						const float px = ray_pr * sin_t * cos_p + ray_r * ray_ptheta * cos_t * cos_p - ray_r * sin_t * ray_pphi * sin_p;
						const float py = ray_pr * sin_t * sin_p + ray_r * ray_ptheta * cos_t * sin_p + ray_r * sin_t * ray_pphi * cos_p;
						const float pz = ray_pr * cos_t - ray_r * ray_ptheta * sin_t;

						const float p_len = std::sqrt(px * px + py * py + pz * pz);
						const float inv_plen = (p_len > 1e-6f) ? (1.0f / p_len) : 1.0f;

						const auto sky_rgb = compute_sky_radiance(
							static_cast<double>(px * inv_plen), static_cast<double>(py * inv_plen), static_cast<double>(pz * inv_plen),
							params
						);
						accumulated_r += throughput * sky_rgb[0];
						accumulated_g += throughput * sky_rgb[1];
						accumulated_b += throughput * sky_rgb[2];
					}

					const auto mapped_srgb = apply_tonemapping(
						{static_cast<double>(accumulated_r), static_cast<double>(accumulated_g), static_cast<double>(accumulated_b)},
						params.tonemapping_mode,
						params.camera_exposure
					);

					output_framebuffer[pixel_idx] = GpuPixelOutput{
						.r = mapped_srgb[0],
						.g = mapped_srgb[1],
						.b = mapped_srgb[2],
						.a = 1.0f,
						.redshift = redshift_rec,
						.affine_parameter = static_cast<float>(iters) * 0.05f,
						.status_flags = status,
						.iterations_used = iters
					};
				}
			}
		};

		auto render_slice = [&](size_t y_start, size_t y_end) noexcept {
			render_rect(0, width, y_start, y_end);
		};

		auto render_tile_range = [&](size_t t_start, size_t t_end) noexcept {
			constexpr size_t TILE = 32;
			const size_t tiles_x = (width + TILE - 1) / TILE;
			for (size_t t = t_start; t < t_end; ++t) {
				if (cancel_flag && cancel_flag->load(std::memory_order_relaxed)) return;
				const size_t tx = (t % tiles_x) * TILE;
				const size_t ty = (t / tiles_x) * TILE;
				render_rect(tx, std::min(tx + TILE, width), ty, std::min(ty + TILE, height));
			}
		};

		const bool use_tiling = (params.render_flags & RenderFlags::USE_TILED_DISTRIBUTION) != 0U;
		constexpr size_t TILE_DIM = 32;
		const size_t total_tiles = ((width + TILE_DIM - 1) / TILE_DIM) * ((height + TILE_DIM - 1) / TILE_DIM);

		if (pool != nullptr && !(params.render_flags & RenderFlags::USE_PER_FRAME_THREADS)) {
			if (use_tiling) {
				pool->parallel_for(total_tiles, [&](size_t t_start, size_t t_end) noexcept {
					render_tile_range(t_start, t_end);
				});
			} else {
				pool->parallel_for(height, [&](size_t y_start, size_t y_end) noexcept {
					render_slice(y_start, y_end);
				});
			}
		} else {
			if (use_tiling) {
				const size_t tiles_per_thread = (total_tiles + num_threads - 1) / num_threads;
				for (size_t t = 0; t < num_threads; ++t) {
					const size_t t_start = t * tiles_per_thread;
					const size_t t_end = std::min(t_start + tiles_per_thread, total_tiles);
					if (t_start < t_end) {
						workers.emplace_back(render_tile_range, t_start, t_end);
					}
				}
			} else {
				const size_t rows_per_thread = (height + num_threads - 1) / num_threads;
				for (size_t t = 0; t < num_threads; ++t) {
					const size_t y_start = t * rows_per_thread;
					const size_t y_end = std::min(y_start + rows_per_thread, height);
					if (y_start < y_end) {
						workers.emplace_back(render_slice, y_start, y_end);
					}
				}
			}
		}
	}

	static void dispatch_fp32_simd(
		const GpuCameraPushConstants& params,
		std::span<GpuPixelOutput> output_framebuffer,
		Core::ThreadPool* pool = nullptr,
		const std::atomic<bool>* cancel_flag = nullptr
	) noexcept {
		const size_t width = params.screen_width;
		const size_t height = params.screen_height;
		const size_t total_pixels = width * height;

		if (output_framebuffer.size() < total_pixels || width == 0 || height == 0) {
			return;
		}

		const uint32_t metric_id = params.metric_type;
		const bool is_wormhole = (metric_id == 8);
		const bool is_warp = (metric_id == 9);
		const bool is_flat = (metric_id == 0);
		const bool has_accretion_disk = (metric_id == 1 || metric_id == 2 || metric_id == 3 || metric_id == 4 || metric_id == 5);
		const bool has_event_horizon = (!is_wormhole && !is_warp && !is_flat);

		const float m = static_cast<float>(std::max(params.metric_mass, 1e-4));
		const float a_spin = static_cast<float>(std::clamp(params.metric_spin, -0.999 * params.metric_mass, 0.999 * params.metric_mass));
		const float rs = 2.0f * m;
		const float rh = (std::abs(a_spin) > 1e-6f) ? (m + std::sqrt(std::max(m * m - a_spin * a_spin, 0.0f))) : rs;
		const float aspect = static_cast<float>(width) / static_cast<float>(height);

		const float isco = (std::abs(a_spin) > 1e-6f) ? std::max(rh * 1.05f, 6.0f * m - 4.0f * a_spin) : (6.0f * m);
		const float disk_outer = 24.0f * m;

		const float fwd_x = static_cast<float>(params.tetrad_e1[1]), fwd_y = static_cast<float>(params.tetrad_e1[2]), fwd_z = static_cast<float>(params.tetrad_e1[3]);
		const float rgt_x = static_cast<float>(params.tetrad_e2[1]), rgt_y = static_cast<float>(params.tetrad_e2[2]), rgt_z = static_cast<float>(params.tetrad_e2[3]);
		const float up_x  = static_cast<float>(params.tetrad_e3[1]), up_y  = static_cast<float>(params.tetrad_e3[2]), up_z  = static_cast<float>(params.tetrad_e3[3]);

		const bool space_skip_enabled = (params.render_flags & RenderFlags::SPACE_SKIP_ENABLED) != 0U;
		const float effective_space_skip_radius = has_accretion_disk
			? std::max(static_cast<float>(params.space_skip_radius_scale) * m, disk_outer * 1.05f)
			: std::max(static_cast<float>(params.space_skip_radius_scale) * m, rh * 2.0f);

		const bool lod_active_simd = (params.render_flags & RenderFlags::USE_LOD_SYSTEM) != 0U && params.lod_distance_threshold > 0.0;
		const double r_obs_pre_f = std::max(params.observer_position[1], static_cast<double>(rh) * 1.02);
		const uint32_t effective_max_steps_simd = (lod_active_simd && r_obs_pre_f > params.lod_distance_threshold)
			? std::min(params.max_integration_steps, params.lod_reduced_steps)
			: params.max_integration_steps;

		const float r_obs = std::max(static_cast<float>(params.observer_position[1]), rh * 1.02f);
		const float theta_obs = std::clamp(static_cast<float>(params.observer_position[2]), 0.001f, std::numbers::pi_v<float> - 0.001f);
		const float phi_obs = static_cast<float>(params.observer_position[3]);

		const double angular_pixel_size_simd_f = params.field_of_view_rad / std::max(static_cast<double>(width), 1.0);
		const double bh_angular_diameter_simd_f = (2.0 * static_cast<double>(rh)) / static_cast<double>(r_obs);
		const float turbulence_aa_factor_simd_f = static_cast<float>(std::clamp(bh_angular_diameter_simd_f / std::max(angular_pixel_size_simd_f * 24.0, 1e-9), 0.0, 1.0));

		const float sin_to = std::sin(theta_obs);
		const float cos_to = std::cos(theta_obs);
		const float sin_po = std::sin(phi_obs);
		const float cos_po = std::cos(phi_obs);

		const float er_x = sin_to * cos_po, er_y = sin_to * sin_po, er_z = cos_to;
		const float eth_x = cos_to * cos_po, eth_y = cos_to * sin_po, eth_z = -sin_to;
		const float eph_x = -sin_po, eph_y = cos_po, eph_z = 0.0f;

		const float factor_obs = std::max(1.0f - rs / r_obs, 1e-4f);
		const float sqrt_factor_obs = std::sqrt(factor_obs);

		const auto proj_mode = static_cast<Observer::ProjectionMode>(params.projection_mode);
		const float fov_rad = static_cast<float>(params.field_of_view_rad);

		auto render_simd_rect = [&](size_t x_start, size_t x_end, size_t y_start, size_t y_end) noexcept {
			for (size_t y = y_start; y < y_end; ++y) {
				if (cancel_flag && cancel_flag->load(std::memory_order_relaxed)) return;
				const float v_norm = 1.0f - (static_cast<float>(y) + 0.5f) / static_cast<float>(height) * 2.0f;

				for (size_t x = x_start; x < x_end; x += 8) {
					Core::GeodesicBundle8f bundle;
					const size_t lanes = std::min(size_t{8}, x_end - x);

					std::array<float, 8> accum_r{};
					std::array<float, 8> accum_g{};
					std::array<float, 8> accum_b{};
					std::array<float, 8> throughput{};
					std::array<float, 8> redshift_rec{};
					throughput.fill(1.0f);
					redshift_rec.fill(1.0f);
					std::array<uint32_t, 8> status{};
					std::array<uint32_t, 8> iters{};

					for (size_t l = 0; l < 8; ++l) {
						if (l < lanes) {
							const size_t cur_x = x + l;
							const bool is_allsky = (proj_mode == Observer::ProjectionMode::Equirectangular360 || proj_mode == Observer::ProjectionMode::HammerAitoff);
							const float u_norm = is_allsky
								? (((static_cast<float>(cur_x) + 0.5f) / static_cast<float>(width)) * 2.0f - 1.0f)
								: (((static_cast<float>(cur_x) + 0.5f) / static_cast<float>(width) * 2.0f - 1.0f) * aspect);

							const auto n_local = Observer::CameraProjector<float>::compute_ray_direction(
								proj_mode, u_norm, v_norm, fov_rad
							);

							const float ray_dir_x = n_local[0] * fwd_x + n_local[2] * rgt_x + n_local[1] * up_x;
							const float ray_dir_y = n_local[0] * fwd_y + n_local[2] * rgt_y + n_local[1] * up_y;
							const float ray_dir_z = n_local[0] * fwd_z + n_local[2] * rgt_z + n_local[1] * up_z;

							const float n_r = ray_dir_x * er_x + ray_dir_y * er_y + ray_dir_z * er_z;
							const float n_th = ray_dir_x * eth_x + ray_dir_y * eth_y + ray_dir_z * eth_z;
							const float n_ph = ray_dir_x * eph_x + ray_dir_y * eph_y + ray_dir_z * eph_z;

							bundle.x0[l] = 0.0f;
							bundle.x1[l] = r_obs;
							bundle.x2[l] = theta_obs;
							bundle.x3[l] = phi_obs;

							bundle.p0[l] = 1.0f / factor_obs;
							bundle.p1[l] = n_r / sqrt_factor_obs;
							bundle.p2[l] = n_th / r_obs;
							bundle.p3[l] = n_ph / (r_obs * sin_to);
							bundle.active_mask[l] = true;
						} else {
							bundle.x0[l] = 0.0f;
							bundle.x1[l] = r_obs;
							bundle.x2[l] = theta_obs;
							bundle.x3[l] = phi_obs;
							bundle.p0[l] = 1.0f;
							bundle.p1[l] = 0.0f;
							bundle.p2[l] = 0.0f;
							bundle.p3[l] = 0.0f;
							bundle.active_mask[l] = false;
						}
					}

					const uint32_t step_limit = effective_max_steps_simd;
					for (uint32_t step = 0; step < step_limit && bundle.active_mask.any(); ++step) {
						for (size_t l = 0; l < lanes; ++l) {
							if (bundle.active_mask[l]) {
								iters[l] = step + 1;
								const float delta_k = bundle.x1[l] * bundle.x1[l] - 2.0f * m * bundle.x1[l] + a_spin * a_spin;
								if (has_event_horizon && (bundle.x1[l] <= rh * 1.0001f || delta_k <= 0.0f)) {
									status[l] |= PixelFlags::HORIZON_ABSORBED;
									throughput[l] = 0.0f;
									bundle.active_mask[l] = false;
								} else if (bundle.x1[l] >= static_cast<float>(params.escape_radius)) {
									status[l] |= PixelFlags::CELESTIAL_HIT;
									bundle.active_mask[l] = false;
								}
							}
						}

						if (!bundle.active_mask.any()) break;

						if (space_skip_enabled) {
							for (size_t l = 0; l < lanes; ++l) {
								if (bundle.active_mask[l] && bundle.x1[l] > effective_space_skip_radius) {
									static_cast<void>(attempt_analytic_space_skip(bundle.x1[l], bundle.x2[l], bundle.x3[l], bundle.p1[l], bundle.p2[l], bundle.p3[l], effective_space_skip_radius, static_cast<float>(params.escape_radius)));
								}
							}
						}

						for (size_t l = 0; l < 8; ++l) {
							if (bundle.active_mask[l]) {
								const float r_scale = std::max(bundle.x1[l] - rh, 0.02f * m);
								const float smooth_dt = 0.06f * std::sqrt(bundle.x1[l] * r_scale);
								const float pole_guard = std::clamp(std::abs(std::sin(bundle.x2[l])) * 12.0f, 0.15f, 1.0f);
								bundle.step_size[l] = -std::clamp(smooth_dt, 0.005f, 4.0f) * pole_guard;
							} else {
								bundle.step_size[l] = -0.01f;
							}
						}

						std::array<float, 8> prev_r{bundle.x1[0], bundle.x1[1], bundle.x1[2], bundle.x1[3], bundle.x1[4], bundle.x1[5], bundle.x1[6], bundle.x1[7]};
						std::array<float, 8> prev_th{bundle.x2[0], bundle.x2[1], bundle.x2[2], bundle.x2[3], bundle.x2[4], bundle.x2[5], bundle.x2[6], bundle.x2[7]};
						std::array<float, 8> prev_phi{bundle.x3[0], bundle.x3[1], bundle.x3[2], bundle.x3[3], bundle.x3[4], bundle.x3[5], bundle.x3[6], bundle.x3[7]};

						bundle.step_rk4_schwarzschild(m, 1.0f, 1.0f);

						for (size_t l = 0; l < lanes; ++l) {
							if (bundle.horizon_mask[l]) {
								status[l] |= PixelFlags::HORIZON_ABSORBED;
								throughput[l] = 0.0f;
							}
							if (bundle.celestial_mask[l]) {
								status[l] |= PixelFlags::CELESTIAL_HIT;
							}
						}

						const float mid_plane = std::numbers::pi_v<float> * 0.5f;
						for (size_t l = 0; l < lanes; ++l) {
							if (has_accretion_disk && throughput[l] > 0.01f && ((prev_th[l] - mid_plane) * (bundle.x2[l] - mid_plane) <= 0.0f)) {
								const float d_th_span = std::abs(bundle.x2[l] - prev_th[l]);
								const float s_cross = (d_th_span > 1e-6f) ? std::clamp(std::abs(prev_th[l] - mid_plane) / d_th_span, 0.0f, 1.0f) : 0.5f;
								const float r_cross = prev_r[l] + s_cross * (bundle.x1[l] - prev_r[l]);
								const float phi_cross = prev_phi[l] + s_cross * (bundle.x3[l] - prev_phi[l]);

								if (r_cross >= isco && r_cross <= disk_outer) {
									status[l] |= PixelFlags::ACCRETION_DISK_HIT;

									const float v_orb = std::sqrt(m / r_cross);
									const float omega_orb = v_orb / r_cross;
									const float gamma_orb = 1.0f / std::sqrt(std::max(1.0f - 3.0f * m / r_cross, 1e-4f));

									const float sin_x_l = std::sin(bundle.x2[l]);
									const float Lz_val = bundle.p3[l] * (bundle.x1[l] * bundle.x1[l] * std::max(sin_x_l * sin_x_l, 1e-6f));
									const float E_val = 1.0f;
									const float l_over_e = Lz_val / std::max(std::abs(E_val), 1e-8f);
									const float denom_g = gamma_orb * (1.0f - omega_orb * l_over_e);
									const float g_doppler = (std::abs(denom_g) > 1e-8f) ? (std::sqrt(std::max(1.0f - rs / r_cross, 1e-4f)) / denom_g) : 1.0f;
									redshift_rec[l] = g_doppler;

									const float t_norm = std::pow(isco / r_cross, 0.75f) * std::pow(std::max(1.0f - std::sqrt(isco / r_cross), 0.0f), 0.25f);
									const float t_eff_k = 18000.0f * t_norm + 1200.0f;
									const float t_obs = t_eff_k * g_doppler;

									const float g4 = g_doppler * g_doppler * g_doppler * g_doppler;
									const float radial_envelope = std::clamp((disk_outer - r_cross) / (1.5f * m), 0.0f, 1.0f) * std::clamp((r_cross - isco) / (0.8f * m), 0.0f, 1.0f);
									const float turbulence = 1.0f - (0.12f * turbulence_aa_factor_simd_f) + (0.12f * turbulence_aa_factor_simd_f) * std::sin(8.0f * phi_cross - 4.0f * std::log(r_cross / isco));
									const float flux_intensity = std::max(g4 * t_norm * radial_envelope * turbulence, 0.0f) * 1.5f;

									const auto disk_rgb = temperature_to_linear_rgb(t_obs, flux_intensity);
									const float alpha_opacity = std::clamp(radial_envelope * 0.95f, 0.0f, 0.98f);

									accum_r[l] += throughput[l] * disk_rgb[0];
									accum_g[l] += throughput[l] * disk_rgb[1];
									accum_b[l] += throughput[l] * disk_rgb[2];
									throughput[l] *= (1.0f - alpha_opacity);
								}
							}
						}
					}

					for (size_t l = 0; l < lanes; ++l) {
						if (status[l] & PixelFlags::CELESTIAL_HIT || throughput[l] > 0.01f) {
							const float sin_t = std::sin(bundle.x2[l]);
							const float cos_t = std::cos(bundle.x2[l]);
							const float sin_p = std::sin(bundle.x3[l]);
							const float cos_p = std::cos(bundle.x3[l]);

							const float px = bundle.p1[l] * sin_t * cos_p + bundle.x1[l] * bundle.p2[l] * cos_t * cos_p - bundle.x1[l] * sin_t * bundle.p3[l] * sin_p;
							const float py = bundle.p1[l] * sin_t * sin_p + bundle.x1[l] * bundle.p2[l] * cos_t * sin_p + bundle.x1[l] * sin_t * bundle.p3[l] * cos_p;
							const float pz = bundle.p1[l] * cos_t - bundle.x1[l] * bundle.p2[l] * sin_t;

							const float p_len = std::sqrt(px * px + py * py + pz * pz);
							const float inv_plen = (p_len > 1e-6f) ? (1.0f / p_len) : 1.0f;
							const float dir_x = (p_len > 1e-6f) ? (px * inv_plen) : 0.0f;
							const float dir_y = (p_len > 1e-6f) ? (py * inv_plen) : 1.0f;
							const float dir_z = (p_len > 1e-6f) ? (pz * inv_plen) : 0.0f;

							const auto sky_rgb = compute_sky_radiance(static_cast<double>(dir_x), static_cast<double>(dir_y), static_cast<double>(dir_z), params);
							accum_r[l] += throughput[l] * sky_rgb[0];
							accum_g[l] += throughput[l] * sky_rgb[1];
							accum_b[l] += throughput[l] * sky_rgb[2];
						}

						const auto mapped_srgb = apply_tonemapping(
							{static_cast<double>(accum_r[l]), static_cast<double>(accum_g[l]), static_cast<double>(accum_b[l])},
							params.tonemapping_mode,
							params.camera_exposure
						);

						const size_t out_idx = y * width + (x + l);
						if (out_idx < output_framebuffer.size()) {
							output_framebuffer[out_idx] = GpuPixelOutput{
								.r = mapped_srgb[0],
								.g = mapped_srgb[1],
								.b = mapped_srgb[2],
								.a = 1.0f,
								.redshift = redshift_rec[l],
								.affine_parameter = static_cast<float>(iters[l]) * 0.05f,
								.status_flags = status[l],
								.iterations_used = iters[l]
							};
						}
					}
				}
			}
		};

		auto render_simd_slice = [&](size_t y_start, size_t y_end) noexcept {
			render_simd_rect(0, width, y_start, y_end);
		};

		auto render_simd_tile_range = [&](size_t t_start, size_t t_end) noexcept {
			constexpr size_t TILE = 32;
			const size_t tiles_x = (width + TILE - 1) / TILE;
			for (size_t t = t_start; t < t_end; ++t) {
				if (cancel_flag && cancel_flag->load(std::memory_order_relaxed)) return;
				const size_t tx = (t % tiles_x) * TILE;
				const size_t ty = (t / tiles_x) * TILE;
				render_simd_rect(tx, std::min(tx + TILE, width), ty, std::min(ty + TILE, height));
			}
		};

		const bool use_tiling = (params.render_flags & RenderFlags::USE_TILED_DISTRIBUTION) != 0U;
		constexpr size_t TILE_DIM = 32;
		const size_t total_tiles = ((width + TILE_DIM - 1) / TILE_DIM) * ((height + TILE_DIM - 1) / TILE_DIM);

		if (pool != nullptr && !(params.render_flags & RenderFlags::USE_PER_FRAME_THREADS)) {
			if (use_tiling) {
				pool->parallel_for(total_tiles, [&](size_t t_start, size_t t_end) noexcept {
					render_simd_tile_range(t_start, t_end);
				});
			} else {
				pool->parallel_for(height, [&](size_t y_start, size_t y_end) noexcept {
					render_simd_slice(y_start, y_end);
				});
			}
		} else {
			const unsigned int num_threads = std::max(1u, std::thread::hardware_concurrency());
			std::vector<std::jthread> workers;
			workers.reserve(num_threads);
			if (use_tiling) {
				const size_t tiles_per_thread = (total_tiles + num_threads - 1) / num_threads;
				for (size_t t = 0; t < num_threads; ++t) {
					const size_t t_start = t * tiles_per_thread;
					const size_t t_end = std::min(t_start + tiles_per_thread, total_tiles);
					if (t_start < t_end) {
						workers.emplace_back(render_simd_tile_range, t_start, t_end);
					}
				}
			} else {
				const size_t rows_per_thread = (height + num_threads - 1) / num_threads;
				for (size_t t = 0; t < num_threads; ++t) {
					const size_t y_start = t * rows_per_thread;
					const size_t y_end = std::min(y_start + rows_per_thread, height);
					if (y_start < y_end) {
						workers.emplace_back(render_simd_slice, y_start, y_end);
					}
				}
			}
		}
	}

	static void dispatch_fp32(
		const GpuCameraPushConstants& params,
		std::span<GpuPixelOutput> output_framebuffer,
		Core::ThreadPool* pool = nullptr,
		const std::atomic<bool>* cancel_flag = nullptr
	) noexcept {
		const bool requires_exact_kerr = requires_exact_metric_path(params);
		if (requires_exact_kerr || (params.render_flags & RenderFlags::USE_SCALAR_PIPELINE)) {
			dispatch_fp32_scalar(params, output_framebuffer, pool, cancel_flag);
		} else {
			dispatch_fp32_simd(params, output_framebuffer, pool, cancel_flag);
		}
	}

	static void dispatch_fp64(
		const GpuCameraPushConstants& params,
		std::span<GpuPixelOutput> output_framebuffer,
		Core::ThreadPool* pool = nullptr,
		const std::atomic<bool>* cancel_flag = nullptr
	) noexcept {
		const bool requires_exact_kerr = requires_exact_metric_path(params);
		if (requires_exact_kerr || (params.render_flags & RenderFlags::USE_SCALAR_PIPELINE)) {
			dispatch_fp64_scalar(params, output_framebuffer, pool, cancel_flag);
		} else {
			dispatch_fp64_simd(params, output_framebuffer, pool, cancel_flag);
		}
	}

	static void dispatch_double_single(
		const GpuCameraPushConstants& params,
		std::span<GpuPixelOutput> output_framebuffer,
		Core::ThreadPool* pool = nullptr,
		const std::atomic<bool>* cancel_flag = nullptr
	) noexcept {
		dispatch_fp32(params, output_framebuffer, pool, cancel_flag);
	}
};

}
