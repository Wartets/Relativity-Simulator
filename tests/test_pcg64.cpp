#include "support/test_harness.hpp"
#include "relativistic/core/pcg64.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>

using namespace Relativistic::Core;
using Relativistic::Testing::TestSuite;

namespace {

void check_reproducibility(TestSuite& suite) {
	suite.section("Reproducibility");
	PCG64Engine first(1337ULL, 42ULL);
	PCG64Engine second(1337ULL, 42ULL);
	size_t mismatches = 0;
	for (int i = 0; i < 10000; ++i) {
		if (first() != second()) {
			++mismatches;
		}
	}
	suite.check(mismatches == 0, "identical seed and stream produce identical sequences", "mismatches=" + std::to_string(mismatches));

	PCG64Engine stream_a(1337ULL, 1ULL);
	PCG64Engine stream_b(1337ULL, 2ULL);
	size_t matches = 0;
	for (int i = 0; i < 1000; ++i) {
		if (stream_a() == stream_b()) {
			++matches;
		}
	}
	suite.check(matches == 0, "different streams do not produce coinciding values", "matches=" + std::to_string(matches));
}

void check_ranges(TestSuite& suite) {
	suite.section("Output ranges");
	PCG64Engine rng(999ULL, 1ULL);
	bool unit_ok = true;
	for (int i = 0; i < 10000; ++i) {
		const double value = rng.next_uniform_double();
		unit_ok = unit_ok && value >= 0.0 && value < 1.0;
	}
	suite.check(unit_ok, "uniform doubles stay in [0, 1)");
	bool range_ok = true;
	for (int i = 0; i < 10000; ++i) {
		const double value = rng.next_uniform_range(-5.0, 15.0);
		range_ok = range_ok && value >= -5.0 && value < 15.0;
	}
	suite.check(range_ok, "ranged uniform doubles stay in [-5, 15)");
}

void check_gaussian(TestSuite& suite) {
	suite.section("Gaussian statistics");
	PCG64Engine rng(2024ULL, 5ULL);
	constexpr size_t kSamples = 50000;
	double sum = 0.0;
	double sum_squares = 0.0;
	for (size_t i = 0; i < kSamples / 2; ++i) {
		const auto [first, second] = rng.next_gaussian_pair(0.0, 1.0);
		sum += first + second;
		sum_squares += first * first + second * second;
	}
	const double mean = sum / static_cast<double>(kSamples);
	const double variance = sum_squares / static_cast<double>(kSamples) - mean * mean;
	suite.measure("sample_mean", mean, "dimensionless");
	suite.measure("sample_variance", variance, "dimensionless");
	suite.check_near(mean, 0.0, 0.05, "the sample mean of the standard normal distribution is close to zero");
	suite.check_near(variance, 1.0, 0.05, "the sample variance of the standard normal distribution is close to one");
}

void check_jump_and_registry(TestSuite& suite) {
	suite.section("Jump-ahead and sub-engines");
	PCG64Engine walked(54321ULL, 77ULL);
	PCG64Engine jumped = walked;
	constexpr uint64_t kSteps = 1000;
	for (uint64_t i = 0; i < kSteps; ++i) {
		static_cast<void>(walked());
	}
	jumped.step_forward(kSteps);
	suite.check(walked() == jumped(), "step_forward equals the same number of sequential draws");

	DeterministicRngRegistry registry(42ULL);
	PCG64Engine sub_one = registry.create_sub_engine(1);
	PCG64Engine sub_two = registry.create_sub_engine(2);
	suite.check(sub_one() != sub_two(), "distinct sub-engine identifiers give distinct streams");
	DeterministicRngRegistry same_registry(42ULL);
	PCG64Engine repeat = same_registry.create_sub_engine(1);
	PCG64Engine reference = registry.create_sub_engine(1);
	suite.check(repeat() == reference(), "the same registry seed and identifier reproduce the same sub-engine");
}

}

int main() {
	TestSuite suite(
		"pcg64",
		"Verifies the explicit-state PCG64 generator that underpins deterministic replay: reproducibility, stream independence, output ranges, Gaussian moments, jump-ahead and registry derived sub-engines."
	);
	check_reproducibility(suite);
	check_ranges(suite);
	check_gaussian(suite);
	check_jump_and_registry(suite);
	return suite.finish();
}
