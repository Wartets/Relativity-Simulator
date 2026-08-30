#include "relativistic/optics/spectrum.hpp"
#include "relativistic/optics/cie_observer.hpp"
#include "relativistic/optics/tonemapping.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

void test_cie_color_matching_functions() {
	using namespace Relativistic::Optics;

	const double y_555 = CIE1931Observer::y_bar(555.0);
	assert(y_555 > 0.95 && y_555 <= 1.01);

	const double z_450 = CIE1931Observer::z_bar(450.0);
	assert(z_450 > 1.0);

	const double x_600 = CIE1931Observer::x_bar(600.0);
	assert(x_600 > 1.0);
}

void test_blackbody_spectrum_to_srgb() {
	using namespace Relativistic::Optics;

	const auto spec_sun = ContinuousSpectrum<double>::make_blackbody(5778.0);
	const auto xyz = CIE1931Observer::integrate_spectrum(spec_sun);
	assert(xyz.x > 0.0 && xyz.y > 0.0 && xyz.z > 0.0);

	const auto linear_rgb = CIE1931Observer::xyz_to_linear_srgb(xyz);
	assert(linear_rgb.r > 0.0 && linear_rgb.g > 0.0 && linear_rgb.b > 0.0);
}

void test_hdr_tonemapping_extremes() {
	using namespace Relativistic::Optics;

	const ColorRGB faint_source{1e-10, 1e-10, 1e-10};
	const auto mapped_faint = Tonemapper::logarithmic_hdr_tonemap(faint_source, 1e-12, 1e20);
	assert(mapped_faint.r >= 0.0 && mapped_faint.r <= 1.0);

	const ColorRGB extreme_quasar{1e18, 1e18, 1e18};
	const auto mapped_quasar = Tonemapper::logarithmic_hdr_tonemap(extreme_quasar, 1e-12, 1e20);
	assert(mapped_quasar.r >= 0.0 && mapped_quasar.r <= 1.0);
	assert(mapped_quasar.r > mapped_faint.r);

	const ColorRGB overexposed{5.0, 2.0, 1.0};
	const auto aces_mapped = Tonemapper::aces_tonemap(overexposed);
	assert(aces_mapped.r <= 1.0 && aces_mapped.g <= 1.0 && aces_mapped.b <= 1.0);
}

int main() {
	test_cie_color_matching_functions();
	test_blackbody_spectrum_to_srgb();
	test_hdr_tonemapping_extremes();
	std::cout << "CIE 1931 spectral and tonemapping pipeline validation passed.\n";
	return 0;
}
