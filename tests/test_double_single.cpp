#include "support/test_harness.hpp"
#include "relativistic/render/double_single.hpp"

using namespace Relativistic::Render;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"double_single",
		"Verifies the compensated double-single arithmetic that emulates extended precision with two single-precision floats: conversion round trip, the four arithmetic operations, square root and sine, each against native double precision."
	);

	constexpr double kA = 1.23456789012345;
	constexpr double kB = 9.87654321098765;
	constexpr double kC = 137.035999084;
	constexpr double kTolerance = 1e-13;
	const DoubleSingle a(kA);
	const DoubleSingle b(kB);

	suite.section("Conversion");
	suite.check_near(static_cast<double>(a), kA, 1e-13, "a double survives the split and recombination");
	suite.check_near(static_cast<double>(b), kB, 1e-13, "a second double survives the split and recombination");

	suite.section("Arithmetic");
	suite.check_relative(static_cast<double>(a + b), kA + kB, kTolerance, "addition matches native double precision");
	suite.check_relative(static_cast<double>(b - a), kB - kA, kTolerance, "subtraction matches native double precision");
	suite.check_relative(static_cast<double>(a * b), kA * kB, kTolerance, "multiplication matches native double precision");
	suite.check_relative(static_cast<double>(b / a), kB / kA, kTolerance, "division matches native double precision");

	suite.section("Functions");
	suite.check_relative(static_cast<double>(ds_sqrt(DoubleSingle(kC))), std::sqrt(kC), kTolerance, "square root matches native double precision");
	suite.check_relative(static_cast<double>(ds_sin(a)), std::sin(kA), kTolerance, "sine matches native double precision");
	suite.check_relative(static_cast<double>(ds_cos(a)), std::cos(kA), kTolerance, "cosine matches native double precision");

	suite.section("Ordering");
	suite.check(a < b && b > a && a <= a && a >= a && a != b && a == a, "comparison operators order values consistently");
	suite.check(static_cast<double>(ds_abs(-a)) == static_cast<double>(a), "the absolute value removes the sign");
	suite.check(ds_max(a, b) == b && ds_min(a, b) == a, "minimum and maximum select the expected operands");

	return suite.finish();
}
