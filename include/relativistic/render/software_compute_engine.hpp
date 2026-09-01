#pragma once

#include "relativistic/render/gpu_types.hpp"
#include "relativistic/render/double_single.hpp"
#include "relativistic/observer/observer_tetrad.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/metrics/kerr.hpp"
#include "relativistic/metrics/kerr_schild.hpp"
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

	[[nodiscard]] static std::array<float, 3> sample_celestial_starfield(double dir_x, double dir_y, double dir_z) noexcept {
		const double theta = std::acos(std::clamp(dir_z, -1.0, 1.0));
		const double phi = std::atan2(dir_y, dir_x);

		const double u = (phi + std::numbers::pi_v<double>) * (1.0 / (2.0 * std::numbers::pi_v<double>));
		const double v = theta * (1.0 / std::numbers::pi_v<double>);

		const double gal_lat = 0.5 - v;
		const double gal_band = std::exp(-(gal_lat * gal_lat) / 0.015);
		const double gal_core = std::exp(-(gal_lat * gal_lat) / 0.008) * std::exp(-std::pow(u - 0.5, 2.0) / 0.05);

		float r_bg = static_cast<float>(0.015 * gal_band + 0.12 * gal_core);
		float g_bg = static_cast<float>(0.020 * gal_band + 0.09 * gal_core);
		float b_bg = static_cast<float>(0.045 * gal_band + 0.06 * gal_core);

		const double u_grid = u * 720.0;
		const double v_grid = v * 360.0;
		const int iu = static_cast<int>(std::floor(u_grid));
		const int iv = static_cast<int>(std::floor(v_grid));

		const uint32_t cell_seed = static_cast<uint32_t>(iu * 73856093 ^ iv * 19349663);
		const float star_prob = hash_to_float(cell_seed);

		if (star_prob > 0.88f) {
			const float star_offset_u = hash_to_float(cell_seed + 101U);
			const float star_offset_v = hash_to_float(cell_seed + 202U);
			const double du = (u_grid - static_cast<double>(iu)) - static_cast<double>(star_offset_u);
			const double dv = (v_grid - static_cast<double>(iv)) - static_cast<double>(star_offset_v);
			const double dist_sq = du * du + dv * dv;

			if (dist_sq < 0.35) {
				const float star_brightness = std::pow(hash_to_float(cell_seed + 303U), 6.0f) * 4.5f;
				const float star_falloff = static_cast<float>(std::exp(-dist_sq * 10.0));
				const float color_temp = hash_to_float(cell_seed + 404U);

				float sr = 1.0f, sg = 1.0f, sb = 1.0f;
				if (color_temp < 0.3f) {
					sr = 0.8f; sg = 0.85f; sb = 1.3f;
				} else if (color_temp > 0.7f) {
					sr = 1.3f; sg = 0.85f; sb = 0.6f;
				}

				const float lum = star_brightness * star_falloff;
				r_bg += sr * lum;
				g_bg += sg * lum;
				b_bg += sb * lum;
			}
		}

		return {r_bg, g_bg, b_bg};
	}

	[[nodiscard]] static std::array<float, 3> temperature_to_linear_rgb(double t_kelvin, double intensity) noexcept {
		const double t = std::clamp(t_kelvin, 1000.0, 40000.0);
		double r = 0.0, g = 0.0, b = 0.0;

		if (t <= 6600.0) {
			r = 1.0;
			g = std::clamp(0.39008 * std::log(t / 100.0) - 0.63184, 0.0, 1.0);
			if (t <= 1900.0) {
				b = 0.0;
			} else {
				b = std::clamp(0.54320 * std::log(t / 100.0 - 10.0) - 1.19625, 0.0, 1.0);
			}
		} else {
			r = std::clamp(1.29293 * std::pow(t / 100.0 - 60.0, -0.13320), 0.0, 1.0);
			g = std::clamp(1.12989 * std::pow(t / 100.0 - 60.0, -0.07551), 0.0, 1.0);
			b = 1.0;
		}

		const float scale = static_cast<float>(intensity);
		return {static_cast<float>(r) * scale, static_cast<float>(g) * scale, static_cast<float>(b) * scale};
	}

public:
	static void dispatch_fp64(
		const GpuCameraPushConstants& params,
		std::span<GpuPixelOutput> output_framebuffer
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

		const double m = std::max(params.metric_mass, 1e-4);
		const double a_spin = std::clamp(params.metric_spin, -0.999 * m, 0.999 * m);
		const double rs = 2.0 * m;
		const double rh = (std::abs(a_spin) > 1e-12) ? (m + std::sqrt(std::max(m * m - a_spin * a_spin, 0.0))) : rs;
		const double aspect = static_cast<double>(width) / static_cast<double>(height);

		const double isco = (std::abs(a_spin) > 1e-12) ? std::max(rh * 1.05, 6.0 * m - 4.0 * a_spin) : (6.0 * m);
		const double disk_outer = 24.0 * m;

		auto render_slice = [&](size_t y_start, size_t y_end) noexcept {
			for (size_t y = y_start; y < y_end; ++y) {
				const double v_norm = 1.0 - (static_cast<double>(y) + 0.5) / static_cast<double>(height) * 2.0;
				for (size_t x = 0; x < width; ++x) {
					const size_t pixel_idx = y * width + x;
					const double u_norm = ((static_cast<double>(x) + 0.5) / static_cast<double>(width) * 2.0 - 1.0) * aspect;

					const auto n_dir = Observer::CameraProjector<double>::compute_ray_direction(
						static_cast<Observer::ProjectionMode>(params.projection_mode),
						(params.projection_mode == 3) ? (((static_cast<double>(x) + 0.5) / static_cast<double>(width)) * 2.0 - 1.0) : u_norm,
						v_norm,
						params.field_of_view_rad
					);

					const double n1 = n_dir[0];
					const double n2 = n_dir[1];
					const double n3 = n_dir[2];

					const double r_obs = std::max(params.observer_position[1], rh * 1.02);
					const double theta_obs = std::clamp(params.observer_position[2], 0.01, std::numbers::pi_v<double> - 0.01);
					const double phi_obs = params.observer_position[3];

					const double sin_to = std::sin(theta_obs);
					const double factor_obs = std::max(1.0 - rs / r_obs, 1e-6);
					const double sqrt_factor_obs = std::sqrt(factor_obs);

					const double e0_t = 1.0 / sqrt_factor_obs;
					const double e1_r = sqrt_factor_obs;
					const double e2_theta = 1.0 / r_obs;
					const double e3_phi = 1.0 / (r_obs * sin_to);

					const double p_t_init = -e0_t;
					const double p_r_init = n1 * e1_r;
					const double p_theta_init = n2 * e2_theta;
					const double p_phi_init = n3 * e3_phi;

					double ray_r = r_obs;
					double ray_theta = theta_obs;
					double ray_phi = phi_obs;

					double ray_pr = p_r_init;
					double ray_ptheta = p_theta_init;
					double ray_pphi = p_phi_init;

					const double E_cons = -p_t_init * factor_obs;
					const double Lz_cons = p_phi_init * (r_obs * r_obs * sin_to * sin_to);

					double accumulated_r = 0.0;
					double accumulated_g = 0.0;
					double accumulated_b = 0.0;
					double throughput = 1.0;
					double redshift_rec = 1.0;

					uint32_t status = 0;
					uint32_t iters = 0;

					for (uint32_t step = 0; step < params.max_integration_steps && throughput > 0.01; ++step) {
						iters = step + 1;

						if (ray_r <= rh * 1.002) {
							status = PixelFlags::HORIZON_ABSORBED;
							throughput = 0.0;
							break;
						}

						if (ray_r >= params.escape_radius || (step > 4 && ray_r >= 30.0 * m && ray_pr > 0.0)) {
							status = PixelFlags::CELESTIAL_HIT;
							break;
						}

						double dt_step = 0.05;
						if (ray_r > 15.0 * m) {
							dt_step = (ray_pr > 0.0) ? (0.25 * ray_r) : (0.12 * ray_r);
						} else if (ray_r > 5.0 * m) {
							dt_step = 0.08 * ray_r;
						} else {
							dt_step = std::max(0.04 * (ray_r - rh), 0.005);
						}
						const double dt = -std::min(dt_step, 4.0);

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
							const double g323 = cos_t / sin_t;

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

						const double mid_plane = std::numbers::pi_v<double> * 0.5;
						if ((prev_theta - mid_plane) * (ray_theta - mid_plane) <= 0.0) {
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
								const double turbulence = 0.88 + 0.12 * std::sin(8.0 * phi_cross - 4.0 * std::log(r_cross / isco));
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

						const auto sky_rgb = sample_celestial_starfield(px * inv_plen, py * inv_plen, pz * inv_plen);
						accumulated_r += throughput * static_cast<double>(sky_rgb[0]);
						accumulated_g += throughput * static_cast<double>(sky_rgb[1]);
						accumulated_b += throughput * static_cast<double>(sky_rgb[2]);
					}

					output_framebuffer[pixel_idx] = GpuPixelOutput{
						.r = static_cast<float>(accumulated_r),
						.g = static_cast<float>(accumulated_g),
						.b = static_cast<float>(accumulated_b),
						.a = 1.0f,
						.redshift = static_cast<float>(redshift_rec),
						.affine_parameter = static_cast<float>(iters * 0.05),
						.status_flags = status,
						.iterations_used = iters
					};
				}
			}
		};

		const size_t rows_per_thread = (height + num_threads - 1) / num_threads;
		for (size_t t = 0; t < num_threads; ++t) {
			const size_t y_start = t * rows_per_thread;
			const size_t y_end = std::min(y_start + rows_per_thread, height);
			if (y_start < y_end) {
				workers.emplace_back(render_slice, y_start, y_end);
			}
		}
	}

	static void dispatch_double_single(
		const GpuCameraPushConstants& params,
		std::span<GpuPixelOutput> output_framebuffer
	) noexcept {
		dispatch_fp64(params, output_framebuffer);
	}
};

}
