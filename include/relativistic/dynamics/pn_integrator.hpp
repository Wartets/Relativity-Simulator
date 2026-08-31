#pragma once

#include "relativistic/dynamics/pn_nbody_system.hpp"
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

namespace Relativistic::Dynamics {

class RungeKutta4PNIntegrator {
public:
	static void step(PostNewtonianSystem& system, double dt) noexcept {
		auto bodies = system.bodies();
		const size_t n = bodies.size();
		if (n == 0) return;

		const auto orig_bodies = std::vector<PostNewtonianBody>(bodies.begin(), bodies.end());

		system.update_accelerations();
		std::vector<std::array<double, 3>> k1_v(n), k1_a(n);
		for (size_t i = 0; i < n; ++i) {
			k1_v[i] = bodies[i].velocity;
			k1_a[i] = bodies[i].acceleration;
		}

		const double half_dt = 0.5 * dt;
		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				bodies[i].position[c] = orig_bodies[i].position[c] + half_dt * k1_v[i][c];
				bodies[i].velocity[c] = orig_bodies[i].velocity[c] + half_dt * k1_a[i][c];
			}
		}

		system.update_accelerations();
		std::vector<std::array<double, 3>> k2_v(n), k2_a(n);
		for (size_t i = 0; i < n; ++i) {
			k2_v[i] = bodies[i].velocity;
			k2_a[i] = bodies[i].acceleration;
		}

		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				bodies[i].position[c] = orig_bodies[i].position[c] + half_dt * k2_v[i][c];
				bodies[i].velocity[c] = orig_bodies[i].velocity[c] + half_dt * k2_a[i][c];
			}
		}

		system.update_accelerations();
		std::vector<std::array<double, 3>> k3_v(n), k3_a(n);
		for (size_t i = 0; i < n; ++i) {
			k3_v[i] = bodies[i].velocity;
			k3_a[i] = bodies[i].acceleration;
		}

		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				bodies[i].position[c] = orig_bodies[i].position[c] + dt * k3_v[i][c];
				bodies[i].velocity[c] = orig_bodies[i].velocity[c] + dt * k3_a[i][c];
			}
		}

		system.update_accelerations();
		std::vector<std::array<double, 3>> k4_v(n), k4_a(n);
		for (size_t i = 0; i < n; ++i) {
			k4_v[i] = bodies[i].velocity;
			k4_a[i] = bodies[i].acceleration;
		}

		const double sixth_dt = dt / 6.0;
		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				bodies[i].position[c] = orig_bodies[i].position[c] + sixth_dt * (k1_v[i][c] + 2.0 * k2_v[i][c] + 2.0 * k3_v[i][c] + k4_v[i][c]);
				bodies[i].velocity[c] = orig_bodies[i].velocity[c] + sixth_dt * (k1_a[i][c] + 2.0 * k2_a[i][c] + 2.0 * k3_a[i][c] + k4_a[i][c]);
			}
		}

		system.update_accelerations();
		system.advance_time(dt);
	}
};

class SymplecticForestRuthPNIntegrator {
private:
	static constexpr double THETA = 1.351207191959657634047687808971;

public:
	static void step(PostNewtonianSystem& system, double dt) noexcept {
		auto bodies = system.bodies();
		const size_t n = bodies.size();
		if (n == 0) return;

		const double c1 = THETA * 0.5 * dt;
		const double c2 = (1.0 - THETA) * 0.5 * dt;
		const double c3 = c2;
		const double c4 = c1;

		const double d1 = THETA * dt;
		const double d2 = (1.0 - 2.0 * THETA) * dt;
		const double d3 = d1;

		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				bodies[i].position[c] += c1 * bodies[i].velocity[c];
			}
		}

		system.update_accelerations();
		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				bodies[i].velocity[c] += d1 * bodies[i].acceleration[c];
			}
		}

		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				bodies[i].position[c] += c2 * bodies[i].velocity[c];
			}
		}

		system.update_accelerations();
		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				bodies[i].velocity[c] += d2 * bodies[i].acceleration[c];
			}
		}

		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				bodies[i].position[c] += c3 * bodies[i].velocity[c];
			}
		}

		system.update_accelerations();
		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				bodies[i].velocity[c] += d3 * bodies[i].acceleration[c];
			}
		}

		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				bodies[i].position[c] += c4 * bodies[i].velocity[c];
			}
		}

		system.update_accelerations();
		system.advance_time(dt);
	}
};

class AdaptiveRungeKuttaPNIntegrator {
private:
	static constexpr double A21 = 1.0 / 5.0;
	static constexpr double A31 = 3.0 / 40.0, A32 = 9.0 / 40.0;
	static constexpr double A41 = 44.0 / 45.0, A42 = -56.0 / 15.0, A43 = 32.0 / 9.0;
	static constexpr double A51 = 19372.0 / 6561.0, A52 = -25360.0 / 2187.0, A53 = 64448.0 / 6561.0, A54 = -212.0 / 729.0;
	static constexpr double A61 = 9017.0 / 3168.0, A62 = -355.0 / 33.0, A63 = 46732.0 / 5247.0, A64 = 49.0 / 176.0, A65 = -5103.0 / 18656.0;

	static constexpr double B1 = 35.0 / 384.0, B3 = 500.0 / 1113.0, B4 = 125.0 / 192.0, B5 = -2187.0 / 6784.0, B6 = 11.0 / 84.0;
	static constexpr double E1 = 71.0 / 57600.0, E3 = -71.0 / 16695.0, E4 = 71.0 / 1920.0, E5 = -17253.0 / 339200.0, E6 = 22.0 / 525.0, E7 = -1.0 / 40.0;

public:
	static bool step(
		PostNewtonianSystem& system,
		double& dt,
		double rtol = 1e-10,
		double atol = 1e-14,
		double min_step = 1e-8,
		double max_step = 1e6
	) noexcept {
		auto bodies = system.bodies();
		const size_t n = bodies.size();
		if (n == 0) return false;

		const auto orig_bodies = std::vector<PostNewtonianBody>(bodies.begin(), bodies.end());

		system.update_accelerations();
		std::vector<std::array<double, 3>> k1_v(n), k1_a(n);
		for (size_t i = 0; i < n; ++i) {
			k1_v[i] = bodies[i].velocity;
			k1_a[i] = bodies[i].acceleration;
		}

		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				bodies[i].position[c] = orig_bodies[i].position[c] + dt * A21 * k1_v[i][c];
				bodies[i].velocity[c] = orig_bodies[i].velocity[c] + dt * A21 * k1_a[i][c];
			}
		}
		system.update_accelerations();
		std::vector<std::array<double, 3>> k2_v(n), k2_a(n);
		for (size_t i = 0; i < n; ++i) {
			k2_v[i] = bodies[i].velocity;
			k2_a[i] = bodies[i].acceleration;
		}

		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				bodies[i].position[c] = orig_bodies[i].position[c] + dt * (A31 * k1_v[i][c] + A32 * k2_v[i][c]);
				bodies[i].velocity[c] = orig_bodies[i].velocity[c] + dt * (A31 * k1_a[i][c] + A32 * k2_a[i][c]);
			}
		}
		system.update_accelerations();
		std::vector<std::array<double, 3>> k3_v(n), k3_a(n);
		for (size_t i = 0; i < n; ++i) {
			k3_v[i] = bodies[i].velocity;
			k3_a[i] = bodies[i].acceleration;
		}

		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				bodies[i].position[c] = orig_bodies[i].position[c] + dt * (A41 * k1_v[i][c] + A42 * k2_v[i][c] + A43 * k3_v[i][c]);
				bodies[i].velocity[c] = orig_bodies[i].velocity[c] + dt * (A41 * k1_a[i][c] + A42 * k2_a[i][c] + A43 * k3_a[i][c]);
			}
		}
		system.update_accelerations();
		std::vector<std::array<double, 3>> k4_v(n), k4_a(n);
		for (size_t i = 0; i < n; ++i) {
			k4_v[i] = bodies[i].velocity;
			k4_a[i] = bodies[i].acceleration;
		}

		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				bodies[i].position[c] = orig_bodies[i].position[c] + dt * (A51 * k1_v[i][c] + A52 * k2_v[i][c] + A53 * k3_v[i][c] + A54 * k4_v[i][c]);
				bodies[i].velocity[c] = orig_bodies[i].velocity[c] + dt * (A51 * k1_a[i][c] + A52 * k2_a[i][c] + A53 * k3_a[i][c] + A54 * k4_a[i][c]);
			}
		}
		system.update_accelerations();
		std::vector<std::array<double, 3>> k5_v(n), k5_a(n);
		for (size_t i = 0; i < n; ++i) {
			k5_v[i] = bodies[i].velocity;
			k5_a[i] = bodies[i].acceleration;
		}

		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				bodies[i].position[c] = orig_bodies[i].position[c] + dt * (A61 * k1_v[i][c] + A62 * k2_v[i][c] + A63 * k3_v[i][c] + A64 * k4_v[i][c] + A65 * k5_v[i][c]);
				bodies[i].velocity[c] = orig_bodies[i].velocity[c] + dt * (A61 * k1_a[i][c] + A62 * k2_a[i][c] + A63 * k3_a[i][c] + A64 * k4_a[i][c] + A65 * k5_a[i][c]);
			}
		}
		system.update_accelerations();
		std::vector<std::array<double, 3>> k6_v(n), k6_a(n);
		for (size_t i = 0; i < n; ++i) {
			k6_v[i] = bodies[i].velocity;
			k6_a[i] = bodies[i].acceleration;
		}

		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				bodies[i].position[c] = orig_bodies[i].position[c] + dt * (B1 * k1_v[i][c] + B3 * k3_v[i][c] + B4 * k4_v[i][c] + B5 * k5_v[i][c] + B6 * k6_v[i][c]);
				bodies[i].velocity[c] = orig_bodies[i].velocity[c] + dt * (B1 * k1_a[i][c] + B3 * k3_a[i][c] + B4 * k4_a[i][c] + B5 * k5_a[i][c] + B6 * k6_a[i][c]);
			}
		}
		system.update_accelerations();
		std::vector<std::array<double, 3>> k7_v(n), k7_a(n);
		for (size_t i = 0; i < n; ++i) {
			k7_v[i] = bodies[i].velocity;
			k7_a[i] = bodies[i].acceleration;
		}

		double max_err = 0.0;
		for (size_t i = 0; i < n; ++i) {
			for (size_t c = 0; c < 3; ++c) {
				const double err_x = dt * std::abs(E1 * k1_v[i][c] + E3 * k3_v[i][c] + E4 * k4_v[i][c] + E5 * k5_v[i][c] + E6 * k6_v[i][c] + E7 * k7_v[i][c]);
				const double err_v = dt * std::abs(E1 * k1_a[i][c] + E3 * k3_a[i][c] + E4 * k4_a[i][c] + E5 * k5_a[i][c] + E6 * k6_a[i][c] + E7 * k7_a[i][c]);

				const double scale_x = rtol * std::max(std::abs(orig_bodies[i].position[c]), std::abs(bodies[i].position[c])) + atol;
				const double scale_v = rtol * std::max(std::abs(orig_bodies[i].velocity[c]), std::abs(bodies[i].velocity[c])) + atol;

				max_err = std::max({max_err, err_x / scale_x, err_v / scale_v});
			}
		}

		if (max_err <= 1.0) {
			system.advance_time(dt);
			const double factor = (max_err == 0.0) ? 5.0 : std::pow(max_err, -0.2);
			dt = std::clamp(dt * std::clamp(0.9 * factor, 0.2, 5.0), min_step, max_step);
			return true;
		} else {
			for (size_t i = 0; i < n; ++i) {
				bodies[i] = orig_bodies[i];
			}
			const double factor = std::pow(max_err, -0.25);
			dt = std::clamp(dt * std::clamp(0.9 * factor, 0.1, 0.9), min_step, max_step);
			return false;
		}
	}
};

}
