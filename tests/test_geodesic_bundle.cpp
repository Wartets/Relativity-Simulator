#include "relativistic/core/geodesic_bundle.hpp"
#include "relativistic/core/four_vector_bundle.hpp"
#include <cassert>
#include <cmath>
#include <numbers>
#include <atomic>
#include <cstdlib>
#include <new>

static std::atomic<size_t> g_dynamic_allocations_count{0};
static std::atomic<bool> g_trap_dynamic_allocations{false};

void* operator new(size_t size) {
	g_dynamic_allocations_count.fetch_add(1, std::memory_order_relaxed);
	if (g_trap_dynamic_allocations.load(std::memory_order_relaxed)) {
		std::abort();
	}
	return std::malloc(size);
}

void operator delete(void* ptr) noexcept {
	std::free(ptr);
}

void operator delete(void* ptr, size_t) noexcept {
	std::free(ptr);
}

int main() {
	using namespace Relativistic::Core;

	g_trap_dynamic_allocations.store(true, std::memory_order_seq_cst);

	{
		Double4 u0(1.0, 2.0, 3.0, 4.0);
		Double4 u1(0.0, 1.0, 0.0, 0.0);
		Double4 u2(0.0, 0.0, 1.0, 0.0);
		Double4 u3(0.0, 0.0, 0.0, 1.0);

		Double4 v0(1.0, 1.0, 1.0, 1.0);
		Double4 v1(0.0, 0.5, 0.0, 0.0);
		Double4 v2(0.0, 0.0, 0.5, 0.0);
		Double4 v3(0.0, 0.0, 0.0, 0.5);

		const auto dot = minkowski_dot(u0, u1, u2, u3, v0, v1, v2, v3, 1.0);

		assert(std::abs(dot[0] - (-1.0)) < 1e-15);
		assert(std::abs(dot[1] - (-2.0 + 0.5)) < 1e-15);
		assert(std::abs(dot[2] - (-3.0 + 0.5)) < 1e-15);
		assert(std::abs(dot[3] - (-4.0 + 0.5)) < 1e-15);
	}

	{
		Double4 v1(0.6, 0.0, 0.8, 0.0);
		Double4 v2(0.0, 0.6, 0.0, 0.0);
		Double4 v3(0.0, 0.0, 0.0, 0.0);

		const auto gamma = lorentz_factor(v1, v2, v3, 1.0);
		assert(std::abs(gamma[0] - 1.25) < 1e-14);
		assert(std::abs(gamma[1] - 1.25) < 1e-14);
		assert(std::abs(gamma[2] - (1.0 / 0.6)) < 1e-14);
		assert(std::abs(gamma[3] - 1.0) < 1e-14);
	}

	{
		Double4 u1(0.5, 0.5, 0.0, 0.0);
		Double4 u2(0.0, 0.0, 0.0, 0.0);
		Double4 u3(0.0, 0.0, 0.0, 0.0);

		Double4 v1(0.5, -0.5, 0.0, 0.0);
		Double4 v2(0.0, 0.0, 0.0, 0.0);
		Double4 v3(0.0, 0.0, 0.0, 0.0);

		const auto added = relativistic_velocity_add(u1, u2, u3, v1, v2, v3, 1.0);
		assert(std::abs(added.v1[0] - (1.0 / 1.25)) < 1e-14);
		assert(std::abs(added.v1[1] - 0.0) < 1e-14);
	}

	{
		GeodesicBundle8d bundle;
		assert(bundle.is_all_active());
		assert(bundle.active_count() == 8);

		constexpr double M = 1.0;
		constexpr double r_start = 20.0;
		constexpr double theta = 0.5 * std::numbers::pi_v<double>;

		for (size_t i = 0; i < 8; ++i) {
			const double impact_param = 4.0 + static_cast<double>(i) * 0.5;
			const double p_phi = impact_param;
			const double p_t = 1.0;
			const double f = 1.0 - 2.0 * M / r_start;
			const double pr_sq = (p_t * p_t - f * (p_phi * p_phi) / (r_start * r_start));
			const double p_r = -std::sqrt(std::max(0.0, pr_sq));

			bundle.set_ray(
				i,
				{0.0, r_start, theta, 0.0},
				{p_t / f, p_r, 0.0, p_phi / (r_start * r_start)},
				0.01
			);
		}

		for (int step = 0; step < 500 && bundle.is_any_active(); ++step) {
			bundle.step_rk4_schwarzschild(M, 1.0, 1.0);
		}

		for (size_t i = 0; i < 8; ++i) {
			const auto pos = bundle.get_position(i);
			assert(pos[1] > 0.0);
		}
	}

	g_trap_dynamic_allocations.store(false, std::memory_order_seq_cst);

	assert(g_dynamic_allocations_count.load(std::memory_order_relaxed) == 0);

	return 0;
}
