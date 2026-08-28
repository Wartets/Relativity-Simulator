#include "relativistic/core/simd.hpp"
#include "relativistic/core/geodesic_bundle.hpp"
#include <chrono>
#include <iostream>
#include <vector>
#include <cmath>
#include <numbers>

struct ScalarGeodesicRay {
	double x[4];
	double p[4];
	double affine;
	double step;
	bool active;
	bool horizon;
	bool celestial;
};

void scalar_rk4_step(
	ScalarGeodesicRay& ray,
	double mass,
	double c = 1.0,
	double G = 1.0
) noexcept {
	if (!ray.active) return;

	const double r_s = 2.0 * G * mass / (c * c);
	const double dt = ray.step;

	const auto eval = [&](const double rx[4], const double rp[4], double dx[4], double dp[4]) noexcept {
		dx[0] = rp[0];
		dx[1] = rp[1];
		dx[2] = rp[2];
		dx[3] = rp[3];

		const double r = std::max(rx[1], r_s * 1.0001);
		const double theta = rx[2];
		const double sin_t = std::sin(theta);
		const double cos_t = std::cos(theta);
		const double sin2_t = sin_t * sin_t;
		const double factor = 1.0 - r_s / r;
		const double safe_f = std::max(factor, 1e-9);
		const double r2 = r * r;

		const double g001 = r_s / (2.0 * r2 * safe_f);
		const double g100 = (c * c * r_s * safe_f) / (2.0 * r2);
		const double g111 = -g001;
		const double g122 = -r * safe_f;
		const double g133 = -r * safe_f * sin2_t;
		const double g212 = 1.0 / r;
		const double g233 = -sin_t * cos_t;
		const double g313 = 1.0 / r;
		const double g323 = cos_t / (std::abs(sin_t) < 1e-9 ? 1e-9 : sin_t);

		dp[0] = -2.0 * g001 * rp[0] * rp[1];
		dp[1] = -(g100 * rp[0] * rp[0] + g111 * rp[1] * rp[1] + g122 * rp[2] * rp[2] + g133 * rp[3] * rp[3]);
		dp[2] = -(2.0 * g212 * rp[1] * rp[2] + g233 * rp[3] * rp[3]);
		dp[3] = -2.0 * (g313 * rp[1] * rp[3] + g323 * rp[2] * rp[3]);
	};

	double k1_dx[4], k1_dp[4];
	eval(ray.x, ray.p, k1_dx, k1_dp);

	double s2_x[4], s2_p[4];
	for (int i = 0; i < 4; ++i) {
		s2_x[i] = ray.x[i] + 0.5 * dt * k1_dx[i];
		s2_p[i] = ray.p[i] + 0.5 * dt * k1_dp[i];
	}
	double k2_dx[4], k2_dp[4];
	eval(s2_x, s2_p, k2_dx, k2_dp);

	double s3_x[4], s3_p[4];
	for (int i = 0; i < 4; ++i) {
		s3_x[i] = ray.x[i] + 0.5 * dt * k2_dx[i];
		s3_p[i] = ray.p[i] + 0.5 * dt * k2_dp[i];
	}
	double k3_dx[4], k3_dp[4];
	eval(s3_x, s3_p, k3_dx, k3_dp);

	double s4_x[4], s4_p[4];
	for (int i = 0; i < 4; ++i) {
		s4_x[i] = ray.x[i] + dt * k3_dx[i];
		s4_p[i] = ray.p[i] + dt * k3_dp[i];
	}
	double k4_dx[4], k4_dp[4];
	eval(s4_x, s4_p, k4_dx, k4_dp);

	for (int i = 0; i < 4; ++i) {
		ray.x[i] += (dt / 6.0) * (k1_dx[i] + 2.0 * k2_dx[i] + 2.0 * k3_dx[i] + k4_dx[i]);
		ray.p[i] += (dt / 6.0) * (k1_dp[i] + 2.0 * k2_dp[i] + 2.0 * k3_dp[i] + k4_dp[i]);
	}
	ray.affine += dt;

	if (ray.x[1] <= r_s * 1.001) {
		ray.horizon = true;
		ray.active = false;
	} else if (ray.x[1] >= 100.0 * r_s) {
		ray.celestial = true;
		ray.active = false;
	}
}

int main() {
	using namespace Relativistic::Core;

	constexpr size_t NUM_RAYS = 16384;
	constexpr size_t NUM_STEPS = 100;
	constexpr double M = 1.0;
	constexpr double r_start = 25.0;
	constexpr double theta = 0.5 * std::numbers::pi_v<double>;

	std::vector<ScalarGeodesicRay> scalar_rays(NUM_RAYS);
	for (size_t i = 0; i < NUM_RAYS; ++i) {
		const double b = 3.0 + static_cast<double>(i % 500) * 0.02;
		const double f = 1.0 - 2.0 * M / r_start;
		const double pr = -std::sqrt(std::max(0.0, 1.0 - f * (b * b) / (r_start * r_start)));
		scalar_rays[i] = ScalarGeodesicRay{
			.x = {0.0, r_start, theta, 0.0},
			.p = {1.0 / f, pr, 0.0, b / (r_start * r_start)},
			.affine = 0.0,
			.step = 0.01,
			.active = true,
			.horizon = false,
			.celestial = false
		};
	}

	const auto t_scalar_start = std::chrono::high_resolution_clock::now();
	for (size_t step = 0; step < NUM_STEPS; ++step) {
		for (size_t i = 0; i < NUM_RAYS; ++i) {
			scalar_rk4_step(scalar_rays[i], M);
		}
	}
	const auto t_scalar_end = std::chrono::high_resolution_clock::now();
	const double scalar_duration_ms = std::chrono::duration<double, std::milli>(t_scalar_end - t_scalar_start).count();

	constexpr size_t BUNDLE_COUNT_8D = NUM_RAYS / 8;
	std::vector<GeodesicBundle8d> bundles8d(BUNDLE_COUNT_8D);

	for (size_t b_idx = 0; b_idx < BUNDLE_COUNT_8D; ++b_idx) {
		for (size_t lane = 0; lane < 8; ++lane) {
			const size_t ray_idx = b_idx * 8 + lane;
			const double b = 3.0 + static_cast<double>(ray_idx % 500) * 0.02;
			const double f = 1.0 - 2.0 * M / r_start;
			const double pr = -std::sqrt(std::max(0.0, 1.0 - f * (b * b) / (r_start * r_start)));
			bundles8d[b_idx].set_ray(
				lane,
				{0.0, r_start, theta, 0.0},
				{1.0 / f, pr, 0.0, b / (r_start * r_start)},
				0.01
			);
		}
	}

	const auto t_simd_start = std::chrono::high_resolution_clock::now();
	for (size_t step = 0; step < NUM_STEPS; ++step) {
		for (size_t b_idx = 0; b_idx < BUNDLE_COUNT_8D; ++b_idx) {
			bundles8d[b_idx].step_rk4_schwarzschild(M);
		}
	}
	const auto t_simd_end = std::chrono::high_resolution_clock::now();
	const double simd_duration_ms = std::chrono::duration<double, std::milli>(t_simd_end - t_simd_start).count();

	const double speedup = scalar_duration_ms / simd_duration_ms;

	std::cout << "Scalar duration: " << scalar_duration_ms << " ms\n";
	std::cout << "SIMD 8-wide duration: " << simd_duration_ms << " ms\n";
	std::cout << "Speedup ratio: " << speedup << "x\n";

	for (size_t i = 0; i < 64; ++i) {
		const size_t b_idx = i / 8;
		const size_t lane = i % 8;
		const auto pos_simd = bundles8d[b_idx].get_position(lane);
		const auto& pos_scalar = scalar_rays[i].x;

		for (size_t mu = 0; mu < 4; ++mu) {
			const double diff = std::abs(pos_simd[mu] - pos_scalar[mu]);
			if (diff > 1e-6) {
				std::cerr << "Mismatch at ray " << i << " coord " << mu << ": " << pos_simd[mu] << " vs " << pos_scalar[mu] << "\n";
				return 1;
			}
		}
	}

	return 0;
}
