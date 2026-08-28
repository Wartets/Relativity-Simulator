#include "relativistic/core/pcg64.hpp"
#include <cassert>
#include <cmath>
#include <array>

int main() {
	using namespace Relativistic::Core;

	{
		PCG64Engine rng1(1337ULL, 42ULL);
		PCG64Engine rng2(1337ULL, 42ULL);

		for (int i = 0; i < 10000; ++i) {
			const uint64_t v1 = rng1();
			const uint64_t v2 = rng2();
			assert(v1 == v2);
		}
	}

	{
		PCG64Engine rng_a(1337ULL, 1ULL);
		PCG64Engine rng_b(1337ULL, 2ULL);
		size_t identical_matches = 0;
		for (int i = 0; i < 1000; ++i) {
			if (rng_a() == rng_b()) {
				++identical_matches;
			}
		}
		assert(identical_matches == 0);
	}

	{
		PCG64Engine rng(999ULL, 1ULL);
		for (int i = 0; i < 10000; ++i) {
			const double val = rng.next_uniform_double();
			assert(val >= 0.0);
			assert(val < 1.0);
		}

		for (int i = 0; i < 10000; ++i) {
			const double val = rng.next_uniform_range(-5.0, 15.0);
			assert(val >= -5.0);
			assert(val < 15.0);
		}
	}

	{
		PCG64Engine rng(2024ULL, 5ULL);
		constexpr size_t num_samples = 50000;
		double sum = 0.0;
		double sum_sq = 0.0;

		for (size_t i = 0; i < num_samples / 2; ++i) {
			const auto [g1, g2] = rng.next_gaussian_pair(0.0, 1.0);
			sum += g1 + g2;
			sum_sq += g1 * g1 + g2 * g2;
		}

		const double mean = sum / static_cast<double>(num_samples);
		const double variance = (sum_sq / static_cast<double>(num_samples)) - (mean * mean);

		assert(std::abs(mean) < 0.05);
		assert(std::abs(variance - 1.0) < 0.05);
	}

	{
		PCG64Engine rng1(54321ULL, 77ULL);
		PCG64Engine rng2 = rng1;

		constexpr uint64_t jump_steps = 1000;
		for (uint64_t i = 0; i < jump_steps; ++i) {
			rng1();
		}

		rng2.step_forward(jump_steps);

		assert(rng1() == rng2());
	}

	{
		DeterministicRngRegistry registry(42ULL);
		PCG64Engine sub1 = registry.create_sub_engine(1);
		PCG64Engine sub2 = registry.create_sub_engine(2);

		assert(sub1() != sub2());
	}

	return 0;
}
