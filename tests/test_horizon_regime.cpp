#include "support/test_harness.hpp"
#include "relativistic/metrics/horizon_regime.hpp"
#include <cmath>

using namespace Relativistic::Metrics;
using Relativistic::Testing::TestSuite;

namespace {

void check_constraint_parsing(TestSuite& suite) {
	suite.section("Constraint set parsing");
	const auto kerr = HorizonConstraintSet::from_metric_name("Kerr Rotating Black Hole");
	suite.check(kerr.spin && !kerr.charge && !kerr.cosmological, "Kerr constrains the spin only");
	const auto reissner = HorizonConstraintSet::from_metric_name("Reissner-Nordstrom Charged");
	suite.check(!reissner.spin && reissner.charge && !reissner.cosmological, "Reissner-Nordstrom constrains the charge only");
	const auto kerr_newman = HorizonConstraintSet::from_metric_name("Kerr-Newman Charged Rotating");
	suite.check(kerr_newman.spin && kerr_newman.charge && !kerr_newman.cosmological, "Kerr-Newman constrains spin and charge");
	const auto de_sitter = HorizonConstraintSet::from_metric_name("Schwarzschild-de Sitter (Lambda)");
	suite.check(!de_sitter.spin && !de_sitter.charge && de_sitter.cosmological, "Schwarzschild-de Sitter constrains the cosmological constant only");
	suite.check(!HorizonConstraintSet::from_metric_name("Schwarzschild Black Hole").any(), "Schwarzschild has no horizon constraint");
}

void check_kerr(TestSuite& suite) {
	suite.section("Kerr regime classification");
	const auto constraints = HorizonConstraintSet::from_metric_name("Kerr Rotating Black Hole");
	HorizonState state;
	state.mass = 1.0;
	state.spin = 0.5;
	suite.check(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::SubExtremal, "a / M = 0.5 is sub-extremal");
	suite.check_near(HorizonRegimeAnalyzer::outer_horizon_radius(state, constraints), 1.0 + std::sqrt(0.75), 1e-12, "the outer horizon equals M + sqrt(M^2 - a^2)");
	state.spin = 1.0;
	suite.check(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::Extremal, "a / M = 1 is extremal");
	suite.check_near(HorizonRegimeAnalyzer::outer_horizon_radius(state, constraints), 1.0, 1e-12, "the extremal horizon sits at M");
	state.spin = 1.1;
	suite.check(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::SuperExtremal, "a / M = 1.1 is super-extremal");
	suite.check_near(HorizonRegimeAnalyzer::outer_horizon_radius(state, constraints), 1.0, 1e-12, "a super-extremal state reports the radius at M");
}

void check_kerr_newman(TestSuite& suite) {
	suite.section("Kerr-Newman combined extremality");
	const auto constraints = HorizonConstraintSet::from_metric_name("Kerr-Newman Charged Rotating");
	HorizonState state;
	state.mass = 2.0;
	state.spin = 1.2;
	state.charge = 1.6;
	suite.check(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::Extremal, "a^2 + Q^2 = M^2 is extremal");
	state.spin = 1.3;
	suite.check(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::SuperExtremal, "a^2 + Q^2 > M^2 is super-extremal");
	const bool projected = HorizonRegimeAnalyzer::project(state, constraints);
	suite.check(projected, "projecting a super-extremal state reports a change");
	suite.check(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::SubExtremal, "the projected state is sub-extremal");
	suite.check_at_most(HorizonRegimeAnalyzer::extremality_ratio(state, constraints), HorizonRegimeAnalyzer::kGuaranteeFactor + 1e-9, "the projected extremality ratio respects the guarantee factor");
}

void check_cosmological(TestSuite& suite) {
	suite.section("Nariai bound");
	const auto constraints = HorizonConstraintSet::from_metric_name("Schwarzschild-de Sitter (Lambda)");
	HorizonState state;
	state.mass = 1.0;
	state.cosmological_constant = 1.0 / 18.0;
	suite.check(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::SubExtremal, "Lambda M^2 = 1/18 is sub-extremal");
	state.cosmological_constant = 1.0 / 9.0;
	suite.check(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::Extremal, "Lambda M^2 = 1/9 is the Nariai limit");
	state.cosmological_constant = 1.0 / 6.0;
	suite.check(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::SuperExtremal, "Lambda M^2 = 1/6 exceeds the Nariai limit");
	suite.check(HorizonRegimeAnalyzer::project(state, constraints), "projecting a super-extremal cosmological state reports a change");
	suite.check(HorizonRegimeAnalyzer::classify(state, constraints) == HorizonRegime::SubExtremal, "the projected cosmological state is sub-extremal");
	suite.check_at_most(state.cosmological_constant, HorizonRegimeAnalyzer::kGuaranteeFactor / 9.0 + 1e-9, "the projected constant respects the guaranteed maximum");
}

void check_guaranteed_parameters(TestSuite& suite) {
	suite.section("Guaranteed parameter limits");
	const auto constraints = HorizonConstraintSet::from_metric_name("Kerr-Newman Charged Rotating");
	HorizonState state;
	state.mass = 2.0;
	const double max_spin = HorizonRegimeAnalyzer::guaranteed_maximum(HorizonParameter::Spin, state, constraints);
	suite.check(max_spin > 0.0 && max_spin < state.mass, "the guaranteed spin limit lies strictly between zero and the mass");
	suite.check_near(HorizonRegimeAnalyzer::constrain(HorizonParameter::Spin, 5.0, state, constraints), max_spin, 1e-12, "constraining an excessive spin returns the guaranteed limit");
	state.spin = 1.0;
	const double max_charge = HorizonRegimeAnalyzer::guaranteed_maximum(HorizonParameter::Charge, state, constraints);
	suite.check(max_charge > 0.0, "a positive charge limit remains for a moderate spin");
	suite.check_at_most(state.spin * state.spin + max_charge * max_charge, HorizonRegimeAnalyzer::kGuaranteeFactor * state.mass * state.mass + 1e-12, "the combined spin and charge limit respects the guarantee factor");
}

void check_zone_layout(TestSuite& suite) {
	suite.section("Zone layout geometry");
	const auto constraints = HorizonConstraintSet::from_metric_name("Kerr Rotating Black Hole");
	HorizonState state;
	state.mass = 3.0;
	state.spin = 1.5;
	const auto spin_layout = HorizonRegimeAnalyzer::zone_layout(HorizonParameter::Spin, state, constraints);
	suite.check(spin_layout.valid && spin_layout.symmetric, "the spin axis has a symmetric layout");
	suite.check_near(spin_layout.threshold, 3.0, 1e-12, "the spin threshold equals the mass");
	const auto mass_layout = HorizonRegimeAnalyzer::zone_layout(HorizonParameter::Mass, state, constraints);
	suite.check(mass_layout.valid && !mass_layout.symmetric && mass_layout.sub_extremal_above, "the mass axis is asymmetric with the sub-extremal zone above the threshold");
	suite.check_near(mass_layout.threshold, 1.5, 1e-12, "the mass threshold equals the spin");
}

}

int main() {
	TestSuite suite(
		"horizon_regime",
		"Verifies the horizon regime analysis: constraint detection per metric, Kerr, Kerr-Newman and Nariai extremality classification, projection onto the sub-extremal domain, guaranteed parameter limits and zone layout geometry."
	);
	check_constraint_parsing(suite);
	check_kerr(suite);
	check_kerr_newman(suite);
	check_cosmological(suite);
	check_guaranteed_parameters(suite);
	check_zone_layout(suite);
	return suite.finish();
}
