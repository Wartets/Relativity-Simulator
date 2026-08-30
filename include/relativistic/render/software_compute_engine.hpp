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

namespace Relativistic::Render {

class SoftwareComputeEngine {
public:
	static void dispatch_fp64(
		const GpuCameraPushConstants& params,
		std::span<GpuPixelOutput> output_framebuffer
	) noexcept {
		const size_t width = params.screen_width;
		const size_t height = params.screen_height;
		const size_t total_pixels = width * height;

		if (output_framebuffer.size() < total_pixels) {
			return;
		}

		const unsigned int num_threads = std::max(1u, std::thread::hardware_concurrency());
		std::vector<std::jthread> workers;
		workers.reserve(num_threads);

		const double rs = params.horizon_radius;
		const double c2 = params.speed_of_light * params.speed_of_light;
		const double aspect = static_cast<double>(width) / static_cast<double>(height);

		auto render_slice = [&](size_t y_start, size_t y_end) noexcept {
			for (size_t y = y_start; y < y_end; ++y) {
				const double v_norm = 1.0 - (static_cast<double>(y) + 0.5) / static_cast<double>(height) * 2.0;
				for (size_t x = 0; x < width; ++x) {
					const size_t pixel_idx = y * width + x;
					const double u_norm = ((static_cast<double>(x) + 0.5) / static_cast<double>(width) * 2.0 - 1.0) * aspect;

					const auto n = Observer::CameraProjector<double>::compute_ray_direction(
						static_cast<Observer::ProjectionMode>(params.projection_mode),
						(params.projection_mode == 3) ? (((static_cast<double>(x) + 0.5) / static_cast<double>(width)) * 2.0 - 1.0) : u_norm,
						v_norm,
						params.field_of_view_rad
					);
					const double n1 = n[0];
					const double n2 = n[1];
					const double n3 = n[2];

					Core::FourVector<double> ray_x(
						params.observer_position[0],
						params.observer_position[1],
						params.observer_position[2],
						params.observer_position[3]
					);

					Core::FourVector<double> ray_u;
					for (size_t mu = 0; mu < 4; ++mu) {
						ray_u(mu) = params.tetrad_e0[mu] + n1 * params.tetrad_e1[mu] + n2 * params.tetrad_e2[mu] + n3 * params.tetrad_e3[mu];
					}

					double total_lambda = 0.0;
					uint32_t status = 0;
					uint32_t iters = 0;

					for (uint32_t step = 0; step < params.max_integration_steps; ++step) {
						iters = step + 1;
						const double current_r = ray_x(1);

						if (current_r <= rs * 1.05 && ray_u(1) > 0.0) {
							status = PixelFlags::HORIZON_ABSORBED;
							break;
						}

						if (current_r <= rs * 1.0001) {
							status = PixelFlags::HORIZON_ABSORBED;
							break;
						}

						if (current_r >= params.escape_radius || (step > 4 && current_r >= params.observer_position[1] && ray_u(1) < 0.0)) {
							status = PixelFlags::CELESTIAL_HIT;
							break;
						}

						const double dt = -std::clamp(0.05 * current_r, 0.005, 0.5);

						auto compute_acc = [&](const Core::FourVector<double>& rx, const Core::FourVector<double>& ru) noexcept -> Core::FourVector<double> {
							const double r = std::max(rx(1), rs * 1.000000001);
							const double theta = rx(2);
							const double sin_t = std::sin(theta);
							const double cos_t = std::cos(theta);
							const double r_diff = std::max(r - rs, 0.02 * rs);
							const double r2 = r * r;
							const double r3 = r2 * r;

							const double g001 = rs / (2.0 * r * r_diff);
							const double g100 = (c2 * rs * r_diff) / (2.0 * r3);
							const double g111 = -g001;
							const double g122 = -r_diff;
							const double g133 = -r_diff * sin_t * sin_t;
							const double g212 = 1.0 / r;
							const double g233 = -sin_t * cos_t;
							const double g313 = 1.0 / r;
							const double g323 = (std::abs(sin_t) > 1e-15) ? (cos_t / sin_t) : 0.0;

							Core::FourVector<double> a;
							a(0) = -2.0 * g001 * ru(0) * ru(1);
							a(1) = -(g100 * ru(0) * ru(0) + g111 * ru(1) * ru(1) + g122 * ru(2) * ru(2) + g133 * ru(3) * ru(3));
							a(2) = -(2.0 * g212 * ru(1) * ru(2) + g233 * ru(3) * ru(3));
							a(3) = -2.0 * (g313 * ru(1) * ru(3) + g323 * ru(2) * ru(3));
							return a;
						};

						const auto k1_x = ray_u;
						const auto k1_u = compute_acc(ray_x, ray_u);

						Core::FourVector<double> s2_x, s2_u;
						for (size_t i = 0; i < 4; ++i) {
							s2_x(i) = ray_x(i) + (0.5 * dt) * k1_x(i);
							s2_u(i) = ray_u(i) + (0.5 * dt) * k1_u(i);
						}
						const auto k2_x = s2_u;
						const auto k2_u = compute_acc(s2_x, s2_u);

						Core::FourVector<double> s3_x, s3_u;
						for (size_t i = 0; i < 4; ++i) {
							s3_x(i) = ray_x(i) + (0.5 * dt) * k2_x(i);
							s3_u(i) = ray_u(i) + (0.5 * dt) * k2_u(i);
						}
						const auto k3_x = s3_u;
						const auto k3_u = compute_acc(s3_x, s3_u);

						Core::FourVector<double> s4_x, s4_u;
						for (size_t i = 0; i < 4; ++i) {
							s4_x(i) = ray_x(i) + dt * k3_x(i);
							s4_u(i) = ray_u(i) + dt * k3_u(i);
						}
						const auto k4_x = s4_u;
						const auto k4_u = compute_acc(s4_x, s4_u);

						const double sixth_dt = dt * (1.0 / 6.0);
						for (size_t i = 0; i < 4; ++i) {
							ray_x(i) += sixth_dt * (k1_x(i) + 2.0 * k2_x(i) + 2.0 * k3_x(i) + k4_x(i));
							ray_u(i) += sixth_dt * (k1_u(i) + 2.0 * k2_u(i) + 2.0 * k3_u(i) + k4_u(i));
						}

						total_lambda += std::abs(dt);
					}

					if (status == 0) {
						if (ray_x(1) > rs * 1.5 && ray_u(1) < 0.0) {
							status = PixelFlags::CELESTIAL_HIT;
						} else {
							status = PixelFlags::HORIZON_ABSORBED;
						}
					}

					float r_col = 0.0f;
					float g_col = 0.0f;
					float b_col = 0.0f;
					float redshift = 1.0f;

					if (status == PixelFlags::CELESTIAL_HIT) {
						double px_dir = ray_u(1) * std::sin(ray_u(2)) * std::cos(ray_u(3));
						double py_dir = ray_u(1) * std::sin(ray_u(2)) * std::sin(ray_u(3));
						double pz_dir = ray_u(1) * std::cos(ray_u(2));
						const double len = std::sqrt(px_dir * px_dir + py_dir * py_dir + pz_dir * pz_dir);
						if (len > 0.0) {
							px_dir /= len;
							py_dir /= len;
							pz_dir /= len;
						}

						const double phi = std::atan2(py_dir, px_dir);
						const double theta = std::acos(std::clamp(pz_dir, -1.0, 1.0));
						const double u_coord = (phi + std::numbers::pi_v<double>) / (2.0 * std::numbers::pi_v<double>);
						const double v_coord = theta / std::numbers::pi_v<double>;

						const double grid_u = std::abs(u_coord * 24.0 - std::floor(u_coord * 24.0) - 0.5);
						const double grid_v = std::abs(v_coord * 12.0 - std::floor(v_coord * 12.0) - 0.5);
						const float grid = (grid_u < 0.02 || grid_v < 0.04) ? 0.8f : 0.1f;

						r_col = static_cast<float>(0.5 + 0.5 * std::sin(u_coord * 6.283185307179586));
						g_col = static_cast<float>(0.5 + 0.5 * std::cos(v_coord * 6.283185307179586));
						b_col = grid;

						const double factor_obs = 1.0 - rs / params.observer_position[1];
						const double factor_emit = 1.0 - rs / ray_x(1);
						if (factor_obs > 0.0 && factor_emit > 0.0) {
							redshift = static_cast<float>(std::sqrt(factor_emit / factor_obs));
						}
					}

					output_framebuffer[pixel_idx] = GpuPixelOutput{
						.r = r_col,
						.g = g_col,
						.b = b_col,
						.a = 1.0f,
						.redshift = redshift,
						.affine_parameter = static_cast<float>(total_lambda),
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
		const size_t width = params.screen_width;
		const size_t height = params.screen_height;
		const size_t total_pixels = width * height;

		if (output_framebuffer.size() < total_pixels) {
			return;
		}

		const unsigned int num_threads = std::max(1u, std::thread::hardware_concurrency());
		std::vector<std::jthread> workers;
		workers.reserve(num_threads);

		const DoubleSingle rs(params.horizon_radius);
		const DoubleSingle c2(params.speed_of_light * params.speed_of_light);
		const DoubleSingle aspect(static_cast<double>(width) / static_cast<double>(height));

		auto render_slice = [&](size_t y_start, size_t y_end) noexcept {
			for (size_t y = y_start; y < y_end; ++y) {
				const DoubleSingle v_norm(1.0 - (static_cast<double>(y) + 0.5) / static_cast<double>(height) * 2.0);
				for (size_t x = 0; x < width; ++x) {
					const size_t pixel_idx = y * width + x;
					const DoubleSingle u_norm = DoubleSingle((static_cast<double>(x) + 0.5) / static_cast<double>(width) * 2.0 - 1.0) * aspect;

					const auto n_dir = Observer::CameraProjector<double>::compute_ray_direction(
						static_cast<Observer::ProjectionMode>(params.projection_mode),
						(params.projection_mode == 3) ? (((static_cast<double>(x) + 0.5) / static_cast<double>(width)) * 2.0 - 1.0) : static_cast<double>(u_norm),
						static_cast<double>(v_norm),
						params.field_of_view_rad
					);
					DoubleSingle n1(n_dir[0]);
					DoubleSingle n2(n_dir[1]);
					DoubleSingle n3(n_dir[2]);

					std::array<DoubleSingle, 4> ray_x{
						DoubleSingle(params.observer_position[0]),
						DoubleSingle(params.observer_position[1]),
						DoubleSingle(params.observer_position[2]),
						DoubleSingle(params.observer_position[3])
					};

					std::array<DoubleSingle, 4> ray_u{};
					for (size_t mu = 0; mu < 4; ++mu) {
						ray_u[mu] = DoubleSingle(params.tetrad_e0[mu]) + n1 * DoubleSingle(params.tetrad_e1[mu]) + n2 * DoubleSingle(params.tetrad_e2[mu]) + n3 * DoubleSingle(params.tetrad_e3[mu]);
					}

					DoubleSingle total_lambda(0.0);
					uint32_t status = 0;
					uint32_t iters = 0;

					for (uint32_t step = 0; step < params.max_integration_steps; ++step) {
						iters = step + 1;
						const DoubleSingle current_r = ray_x[1];

						if (current_r <= rs * DoubleSingle(1.05) && ray_u[1] > DoubleSingle(0.0)) {
							status = PixelFlags::HORIZON_ABSORBED;
							break;
						}

						if (current_r <= rs * DoubleSingle(1.0001)) {
							status = PixelFlags::HORIZON_ABSORBED;
							break;
						}

						if (current_r >= DoubleSingle(params.escape_radius) || (step > 4 && current_r >= DoubleSingle(params.observer_position[1]) && ray_u[1] < DoubleSingle(0.0))) {
							status = PixelFlags::CELESTIAL_HIT;
							break;
						}

						const DoubleSingle dt = -ds_clamp(DoubleSingle(0.05) * current_r, DoubleSingle(0.005), DoubleSingle(0.5));

						auto compute_ds_acc = [&](const std::array<DoubleSingle, 4>& rx, const std::array<DoubleSingle, 4>& ru) noexcept -> std::array<DoubleSingle, 4> {
							const DoubleSingle r = ds_max(rx[1], rs * DoubleSingle(1.000000001));
							const DoubleSingle theta = rx[2];
							const DoubleSingle sin_t = ds_sin(theta);
							const DoubleSingle cos_t = ds_cos(theta);
							const DoubleSingle r_diff = ds_max(r - rs, DoubleSingle(0.02) * rs);
							const DoubleSingle r2 = r * r;
							const DoubleSingle r3 = r2 * r;

							const DoubleSingle g001 = rs / (DoubleSingle(2.0) * r * r_diff);
							const DoubleSingle g100 = (c2 * rs * r_diff) / (DoubleSingle(2.0) * r3);
							const DoubleSingle g111 = -g001;
							const DoubleSingle g122 = -r_diff;
							const DoubleSingle g133 = -r_diff * sin_t * sin_t;
							const DoubleSingle g212 = DoubleSingle(1.0) / r;
							const DoubleSingle g233 = -sin_t * cos_t;
							const DoubleSingle g313 = DoubleSingle(1.0) / r;
							const DoubleSingle g323 = (ds_abs(sin_t) > DoubleSingle(1e-6)) ? (cos_t / sin_t) : DoubleSingle(0.0);

							std::array<DoubleSingle, 4> a{};
							a[0] = -DoubleSingle(2.0) * g001 * ru[0] * ru[1];
							a[1] = -(g100 * ru[0] * ru[0] + g111 * ru[1] * ru[1] + g122 * ru[2] * ru[2] + g133 * ru[3] * ru[3]);
							a[2] = -(DoubleSingle(2.0) * g212 * ru[1] * ru[2] + g233 * ru[3] * ru[3]);
							a[3] = -DoubleSingle(2.0) * (g313 * ru[1] * ru[3] + g323 * ru[2] * ru[3]);
							return a;
						};

						const auto k1_x = ray_u;
						const auto k1_u = compute_ds_acc(ray_x, ray_u);

						std::array<DoubleSingle, 4> s2_x{}, s2_u{};
						const DoubleSingle half_dt = dt * DoubleSingle(0.5);
						for (size_t i = 0; i < 4; ++i) {
							s2_x[i] = ray_x[i] + half_dt * k1_x[i];
							s2_u[i] = ray_u[i] + half_dt * k1_u[i];
						}
						const auto k2_x = s2_u;
						const auto k2_u = compute_ds_acc(s2_x, s2_u);

						std::array<DoubleSingle, 4> s3_x{}, s3_u{};
						for (size_t i = 0; i < 4; ++i) {
							s3_x[i] = ray_x[i] + half_dt * k2_x[i];
							s3_u[i] = ray_u[i] + half_dt * k2_u[i];
						}
						const auto k3_x = s3_u;
						const auto k3_u = compute_ds_acc(s3_x, s3_u);

						std::array<DoubleSingle, 4> s4_x{}, s4_u{};
						for (size_t i = 0; i < 4; ++i) {
							s4_x[i] = ray_x[i] + dt * k3_x[i];
							s4_u[i] = ray_u[i] + dt * k3_u[i];
						}
						const auto k4_x = s4_u;
						const auto k4_u = compute_ds_acc(s4_x, s4_u);

						const DoubleSingle sixth_dt = dt / DoubleSingle(6.0);
						const DoubleSingle two_ds(2.0);
						for (size_t i = 0; i < 4; ++i) {
							ray_x[i] += sixth_dt * (k1_x[i] + two_ds * k2_x[i] + two_ds * k3_x[i] + k4_x[i]);
							ray_u[i] += sixth_dt * (k1_u[i] + two_ds * k2_u[i] + two_ds * k3_u[i] + k4_u[i]);
						}

						total_lambda += ds_abs(dt);
					}

					if (status == 0) {
						if (ray_x[1] > rs * DoubleSingle(1.5) && ray_u[1] < DoubleSingle(0.0)) {
							status = PixelFlags::CELESTIAL_HIT;
						} else {
							status = PixelFlags::HORIZON_ABSORBED;
						}
					}

					float r_col = 0.0f;
					float g_col = 0.0f;
					float b_col = 0.0f;
					float redshift = 1.0f;

					if (status == PixelFlags::CELESTIAL_HIT) {
						double px_dir = static_cast<double>(ray_u[1]) * std::sin(static_cast<double>(ray_u[2])) * std::cos(static_cast<double>(ray_u[3]));
						double py_dir = static_cast<double>(ray_u[1]) * std::sin(static_cast<double>(ray_u[2])) * std::sin(static_cast<double>(ray_u[3]));
						double pz_dir = static_cast<double>(ray_u[1]) * std::cos(static_cast<double>(ray_u[2]));
						const double len = std::sqrt(px_dir * px_dir + py_dir * py_dir + pz_dir * pz_dir);
						if (len > 0.0) {
							px_dir /= len;
							py_dir /= len;
							pz_dir /= len;
						}

						const double phi = std::atan2(py_dir, px_dir);
						const double theta = std::acos(std::clamp(pz_dir, -1.0, 1.0));
						const double u_coord = (phi + std::numbers::pi_v<double>) / (2.0 * std::numbers::pi_v<double>);
						const double v_coord = theta / std::numbers::pi_v<double>;

						const double grid_u = std::abs(u_coord * 24.0 - std::floor(u_coord * 24.0) - 0.5);
						const double grid_v = std::abs(v_coord * 12.0 - std::floor(v_coord * 12.0) - 0.5);
						const float grid = (grid_u < 0.02 || grid_v < 0.04) ? 0.8f : 0.1f;

						r_col = static_cast<float>(0.5 + 0.5 * std::sin(u_coord * 6.283185307179586));
						g_col = static_cast<float>(0.5 + 0.5 * std::cos(v_coord * 6.283185307179586));
						b_col = grid;

						const double factor_obs = 1.0 - static_cast<double>(rs) / params.observer_position[1];
						const double factor_emit = 1.0 - static_cast<double>(rs) / static_cast<double>(ray_x[1]);
						if (factor_obs > 0.0 && factor_emit > 0.0) {
							redshift = static_cast<float>(std::sqrt(factor_emit / factor_obs));
						}
					}

					output_framebuffer[pixel_idx] = GpuPixelOutput{
						.r = r_col,
						.g = g_col,
						.b = b_col,
						.a = 1.0f,
						.redshift = redshift,
						.affine_parameter = static_cast<float>(static_cast<double>(total_lambda)),
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
};

}
