#include "support/test_harness.hpp"
#include "relativistic/optics/spectrum.hpp"
#include "relativistic/optics/cie_observer.hpp"
#include "relativistic/optics/tonemapping.hpp"

using namespace Relativistic::Optics;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"cie_spectral_pipeline",
		"Verifies the spectral colour pipeline: the CIE 1931 colour matching functions at characteristic wavelengths, the conversion of a solar blackbody spectrum to positive linear sRGB, and the bounds and monotonicity of the logarithmic and ACES tone mapping operators."
	);

	suite.section("Colour matching functions");
	const double luminous = CIE1931Observer::y_bar(555.0);
	suite.check(luminous > 0.95 && luminous <= 1.01, "the luminosity function peaks near 1 at 555 nm", "y_bar=" + Relativistic::Testing::format_number(luminous, 6));
	suite.check(CIE1931Observer::z_bar(450.0) > 1.0, "the blue matching function exceeds 1 at 450 nm");
	suite.check(CIE1931Observer::x_bar(600.0) > 1.0, "the red matching function exceeds 1 at 600 nm");

	suite.section("Blackbody conversion");
	const auto xyz = CIE1931Observer::integrate_spectrum(ContinuousSpectrum<double>::make_blackbody(5778.0));
	suite.check(xyz.x > 0.0 && xyz.y > 0.0 && xyz.z > 0.0, "the solar blackbody has positive tristimulus values");
	const auto rgb = CIE1931Observer::xyz_to_linear_srgb(xyz);
	suite.check(rgb.r > 0.0 && rgb.g > 0.0 && rgb.b > 0.0, "the solar blackbody converts to positive linear sRGB");

	suite.section("Tone mapping");
	const auto faint = Tonemapper::logarithmic_hdr_tonemap(ColorRGB{1e-10, 1e-10, 1e-10}, 1e-12, 1e20);
	const auto bright = Tonemapper::logarithmic_hdr_tonemap(ColorRGB{1e18, 1e18, 1e18}, 1e-12, 1e20);
	suite.check(faint.r >= 0.0 && faint.r <= 1.0, "a faint source maps into [0, 1]");
	suite.check(bright.r >= 0.0 && bright.r <= 1.0, "a quasar-bright source maps into [0, 1]");
	suite.check(bright.r > faint.r, "the logarithmic mapping preserves brightness ordering");
	const auto aces = Tonemapper::aces_tonemap(ColorRGB{5.0, 2.0, 1.0});
	suite.check(aces.r <= 1.0 && aces.g <= 1.0 && aces.b <= 1.0, "the ACES curve clamps an overexposed colour to at most 1");
	return suite.finish();
}
