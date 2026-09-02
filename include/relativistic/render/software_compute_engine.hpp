#pragma once

#include "relativistic/render/gpu_types.hpp"
#include "relativistic/render/double_single.hpp"
#include "relativistic/observer/observer_tetrad.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/metrics/kerr.hpp"
#include "relativistic/metrics/kerr_schild.hpp"
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

	[[nodiscard]] static std::array<float, 3> sample_celestial_grid_sphere(double dir_x, double dir_y, double dir_z) noexcept {
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
		if (on_major_lat || on_major_lon) {
			return {0.14f, 0.20f, 0.28f};
		}
		if (on_minor_line) {
			return {0.05f, 0.07f, 0.10f};
		}
		return {0.008f, 0.009f, 0.012f};
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

		auto render_slice = [&](size_t y_start, size_t y_end) noexcept {
			for (size_t y = y_start; y < y_end; ++y) {
				if (cancel_flag && cancel_flag->load(std::memory_order_relaxed)) return;
				const double v_norm = 1.0 - (static_cast<double>(y) + 0.5) / static_cast<double>(height) * 2.0;
				for (size_t x = 0; x < width; ++x) {
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

						std::array<float, 3> sky_rgb{0.0f, 0.0f, 0.0f};
						const uint32_t sky_mode = params.render_flags & RenderFlags::SKYBOX_MODE_MASK;
						if (sky_mode == RenderFlags::SKYBOX_GRID || (params.render_flags & RenderFlags::USE_GRID_SKYBOX)) {
							sky_rgb = sample_celestial_grid_sphere(px * inv_plen, py * inv_plen, pz * inv_plen);
						} else if (sky_mode == RenderFlags::SKYBOX_COMPOSITE) {
							const auto stars = sample_celestial_starfield(px * inv_plen, py * inv_plen, pz * inv_plen);
							const auto grid = sample_celestial_grid_sphere(px * inv_plen, py * inv_plen, pz * inv_plen);
							sky_rgb = {stars[0] + grid[0] * 0.7f, stars[1] + grid[1] * 0.7f, stars[2] + grid[2] * 0.7f};
						} else if (sky_mode == RenderFlags::SKYBOX_STARS) {
							sky_rgb = sample_celestial_starfield(px * inv_plen, py * inv_plen, pz * inv_plen);
						}
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

		if (pool != nullptr && !(params.render_flags & RenderFlags::USE_PER_FRAME_THREADS)) {
			pool->parallel_for(height, [&](size_t y_start, size_t y_end) noexcept {
				render_slice(y_start, y_end);
			});
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

		const double factor_obs = std::max(1.0 - rs / r_obs, 1e-6);
		const double sqrt_factor_obs = std::sqrt(factor_obs);

		const auto proj_mode = static_cast<Observer::ProjectionMode>(params.projection_mode);
		const double fov_rad = params.field_of_view_rad;

		auto render_simd_slice = [&](size_t y_start, size_t y_end) noexcept {
			for (size_t y = y_start; y < y_end; ++y) {
				if (cancel_flag && cancel_flag->load(std::memory_order_relaxed)) return;
				const double v_norm = 1.0 - (static_cast<double>(y) + 0.5) / static_cast<double>(height) * 2.0;

				for (size_t x = 0; x < width; x += 4) {
					Core::GeodesicBundle4d bundle;
					const size_t lanes = std::min(size_t{4}, width - x);

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

					const uint32_t step_limit = params.max_integration_steps;
					for (uint32_t step = 0; step < step_limit && bundle.active_mask.any(); ++step) {
						for (size_t l = 0; l < lanes; ++l) {
							if (bundle.active_mask[l]) {
								iters[l] = step + 1;
								if (bundle.x1[l] <= rh * 1.002) {
									status[l] |= PixelFlags::HORIZON_ABSORBED;
									throughput[l] = 0.0;
									bundle.active_mask[l] = false;
								} else if (bundle.x1[l] >= params.escape_radius || (step > 2 && bundle.x1[l] >= 25.0 * m && bundle.p1[l] > 0.0)) {
									status[l] |= PixelFlags::CELESTIAL_HIT;
									bundle.active_mask[l] = false;
								}
							}
						}

						if (!bundle.active_mask.any()) break;

						for (size_t l = 0; l < 4; ++l) {
							if (bundle.active_mask[l]) {
								double dt_step = 0.08;
								if (bundle.x1[l] > 15.0 * m) {
									dt_step = (bundle.p1[l] > 0.0) ? (0.40 * bundle.x1[l]) : (0.20 * bundle.x1[l]);
								} else if (bundle.x1[l] > 5.0 * m) {
									dt_step = 0.12 * bundle.x1[l];
								} else {
									dt_step = std::max(0.06 * (bundle.x1[l] - rh), 0.01);
								}
								bundle.step_size[l] = -std::min(dt_step, 6.0);
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
							if (throughput[l] > 0.01 && ((prev_th[l] - mid_plane) * (bundle.x2[l] - mid_plane) <= 0.0)) {
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
									const double turbulence = 0.88 + 0.12 * std::sin(8.0 * phi_cross - 4.0 * std::log(r_cross / isco));
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

							std::array<float, 3> sky_rgb{0.0f, 0.0f, 0.0f};
							const uint32_t sky_mode = params.render_flags & RenderFlags::SKYBOX_MODE_MASK;
							if (sky_mode == RenderFlags::SKYBOX_GRID || (params.render_flags & RenderFlags::USE_GRID_SKYBOX)) {
								sky_rgb = sample_celestial_grid_sphere(dir_x, dir_y, dir_z);
							} else if (sky_mode == RenderFlags::SKYBOX_COMPOSITE) {
								const auto stars = sample_celestial_starfield(dir_x, dir_y, dir_z);
								const auto grid = sample_celestial_grid_sphere(dir_x, dir_y, dir_z);
								sky_rgb = {stars[0] + grid[0] * 0.7f, stars[1] + grid[1] * 0.7f, stars[2] + grid[2] * 0.7f};
							} else if (sky_mode == RenderFlags::SKYBOX_STARS) {
								sky_rgb = sample_celestial_starfield(dir_x, dir_y, dir_z);
							}
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

		if (pool != nullptr && !(params.render_flags & RenderFlags::USE_PER_FRAME_THREADS)) {
			pool->parallel_for(height, [&](size_t y_start, size_t y_end) noexcept {
				render_simd_slice(y_start, y_end);
			});
		} else {
			const unsigned int num_threads = std::max(1u, std::thread::hardware_concurrency());
			std::vector<std::jthread> workers;
			workers.reserve(num_threads);
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

	static void dispatch_fp64(
		const GpuCameraPushConstants& params,
		std::span<GpuPixelOutput> output_framebuffer,
		Core::ThreadPool* pool = nullptr,
		const std::atomic<bool>* cancel_flag = nullptr
	) noexcept {
		if (params.render_flags & RenderFlags::USE_SCALAR_PIPELINE) {
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
		dispatch_fp64(params, output_framebuffer, pool, cancel_flag);
	}
};

}
