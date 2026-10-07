#include "relativistic/metrics/horizon_regime.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace Relativistic::Metrics;

void test_constraint_set_parsing() {
	const auto kerr = HorizonConstraintSet::from_metric_name("Kerr Rotating Black Hole");
	assert(kerr.spin);
	assert(!kerr.charge);
	assert(!kerr.cosmological);

	const auto rn = HorizonConstraintSet::from_metric_name("Reissner-Nordstrom Charged");
	assert(!rn.spin);
	assert(rn.charge);
	assert(!rn.cosmological);

	const auto kn = HorizonConstraintSet::from_metric_name("Kerr-Newman Charged Rotating");
	assert(kn.spin);
	assert(kn.charge);
	assert(!kn.cosmological);

	const auto sds = HorizonConstraintSet::from_metric_name("Schwarzschild-de Sitter (Lambda)");
	assert(!sds.spin);
	assert(!sds.charge);
	assert(sds.cosmological);

	const auto schw = HorizonConstraintSet::from_metric_name("Schwarzschild Black Hole");
	assert(!schw.any());
}

void test_kerr_regime_classification() {
	const auto constraints = HorizonConstraintSet::from_metric_name("Kerr Rotating Black Hole");

	HorizonState state;
	state.mass = 1.0;
	state.spin = 0.5;
	assert(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::SubExtremal);
	assert(std::abs(HorizonRegimeAnalyzer::outer_horizon_radius(state, constraints) - (1.0 + std::sqrt(0.75))) < 1e-12);

	state.spin = 1.0;
	assert(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::Extremal);
	assert(std::abs(HorizonRegimeAnalyzer::outer_horizon_radius(state, constraints) - 1.0) < 1e-12);

	state.spin = 1.1;
	assert(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::SuperExtremal);
	assert(std::abs(HorizonRegimeAnalyzer::outer_horizon_radius(state, constraints) - 1.0) < 1e-12);
}

void test_kerr_newman_combined_extremality() {
	const auto constraints = HorizonConstraintSet::from_metric_name("Kerr-Newman Charged Rotating");

	HorizonState state;
	state.mass = 2.0;
	state.spin = 1.2;
	state.charge = 1.6;
	assert(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::Extremal);

	state.spin = 1.3;
	assert(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::SuperExtremal);

	const bool projected = HorizonRegimeAnalyzer::project(state, constraints);
	assert(projected);
	assert(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::SubExtremal);
	const double ratio = HorizonRegimeAnalyzer::extremality_ratio(state, constraints);
	assert(ratio <= HorizonRegimeAnalyzer::kGuaranteeFactor + 1e-9);
}

void test_cosmological_nariai_bound() {
	const auto constraints = HorizonConstraintSet::from_metric_name("Schwarzschild-de Sitter (Lambda)");

	HorizonState state;
	state.mass = 1.0;
	state.cosmological_constant = 1.0 / 18.0;
	assert(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::SubExtremal);

	state.cosmological_constant = 1.0 / 9.0;
	assert(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::Extremal);

	state.cosmological_constant = 1.0 / 6.0;
	assert(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::SuperExtremal);

	const bool projected = HorizonRegimeAnalyzer::project(state, constraints);
	assert(projected);
	assert(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::SubExtremal);
	assert(state.cosmological_constant <= (HorizonRegimeAnalyzer::kGuaranteeFactor / 9.0) + 1e-9);
}

void test_guaranteed_parameter_constraints() {
	const auto constraints = HorizonConstraintSet::from_metric_name("Kerr-Newman Charged Rotating");

	HorizonState state;
	state.mass = 2.0;
	state.spin = 0.0;
	state.charge = 0.0;

	const double max_spin = HorizonRegimeAnalyzer::guaranteed_maximum(HorizonParameter::Spin, state, constraints);
	assert(max_spin > 0.0 && max_spin < state.mass);

	const double clamped_spin = HorizonRegimeAnalyzer::constrain(HorizonParameter::Spin, 5.0, state, constraints);
	assert(std::abs(clamped_spin - max_spin) < 1e-12);

	state.spin = 1.0;
	const double max_charge = HorizonRegimeAnalyzer::guaranteed_maximum(HorizonParameter::Charge, state, constraints);
	assert(max_charge > 0.0);
	assert((state.spin * state.spin + max_charge * max_charge) <= HorizonRegimeAnalyzer::kGuaranteeFactor * state.mass * state.mass + 1e-12);
}

void test_zone_layout_geometry() {
	const auto constraints = HorizonConstraintSet::from_metric_name("Kerr Rotating Black Hole");

	HorizonState state;
	state.mass = 3.0;
	state.spin = 1.5;

	const auto layout_spin = HorizonRegimeAnalyzer::zone_layout(HorizonParameter::Spin, state, constraints);
	assert(layout_spin.valid);
	assert(layout_spin.symmetric);
	assert(std::abs(layout_spin.threshold - 3.0) < 1e-12);

	const auto layout_mass = HorizonRegimeAnalyzer::zone_layout(HorizonParameter::Mass, state, constraints);
	assert(layout_mass.valid);
	assert(!layout_mass.symmetric);
	assert(layout_mass.sub_extremal_above);
	assert(std::abs(layout_mass.threshold - 1.5) < 1e-12);
}

int main() {
	test_constraint_set_parsing();
	test_kerr_regime_classification();
	test_kerr_newman_combined_extremality();
	test_cosmological_nariai_bound();
	test_guaranteed_parameter_constraints();
	test_zone_layout_geometry();

	std::cout << "HORIZON REGIME TEST STATUS: ALL PASSED" << std::endl;
	return 0;
}
