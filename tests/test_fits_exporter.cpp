#include "support/test_harness.hpp"
#include "relativistic/io/fits_exporter.hpp"
#include <cstddef>
#include <string_view>
#include <vector>

using namespace Relativistic::IO;
using Relativistic::Testing::TestSuite;

namespace {

[[nodiscard]] bool contains(std::string_view text, std::string_view token) noexcept {
	return text.find(token) != std::string_view::npos;
}

}

int main() {
	TestSuite suite(
		"fits_exporter",
		"Verifies the FITS exporter: logical records of 2880 bytes, mandatory keywords and WCS object name of a two-dimensional image, and the three-dimensional axis keywords of a spectral cube."
	);

	constexpr size_t width = 64;
	constexpr size_t height = 32;
	std::vector<double> image(width * height);
	for (size_t y = 0; y < height; ++y) {
		for (size_t x = 0; x < width; ++x) {
			image[y * width + x] = static_cast<double>(x + y * width);
		}
	}
	FitsWcsMetadata wcs;
	wcs.object_name = "SagittariusA_Star";
	wcs.crpix1 = 32.5;
	wcs.crpix2 = 16.5;
	wcs.crval1 = 266.4168;
	wcs.crval2 = -29.0078;
	wcs.cdelt1 = -0.0001;
	wcs.cdelt2 = 0.0001;

	suite.section("Two-dimensional image");
	const auto bytes = FitsExporter::export_image_2d(image, width, height, wcs);
	suite.check(!bytes.empty(), "the image export is not empty");
	suite.check(bytes.size() % 2880 == 0, "the image export is a whole number of 2880-byte records", "size=" + std::to_string(bytes.size()));
	if (bytes.size() >= 2880) {
		const std::string_view header(reinterpret_cast<const char*>(bytes.data()), 2880);
		suite.check(contains(header, "SIMPLE  =                    T"), "the SIMPLE keyword is true");
		suite.check(contains(header, "BITPIX  =                  -64"), "BITPIX declares 64-bit floating point");
		suite.check(contains(header, "NAXIS   =                    2"), "NAXIS equals 2");
		suite.check(contains(header, "NAXIS1  =                   64"), "NAXIS1 equals the width");
		suite.check(contains(header, "NAXIS2  =                   32"), "NAXIS2 equals the height");
		suite.check(contains(header, "OBJECT  = 'SagittariusA_Star'"), "the OBJECT keyword carries the target name");
		suite.check(contains(header, "END"), "the header is terminated by END");
	}

	suite.section("Three-dimensional spectral cube");
	constexpr size_t wavelengths = 4;
	const std::vector<double> cube(width * height * wavelengths, 1.2345);
	const auto cube_bytes = FitsExporter::export_spectral_cube_3d(cube, width, height, wavelengths, wcs);
	suite.check(cube_bytes.size() % 2880 == 0, "the cube export is a whole number of 2880-byte records", "size=" + std::to_string(cube_bytes.size()));
	if (cube_bytes.size() >= 2880) {
		const std::string_view header(reinterpret_cast<const char*>(cube_bytes.data()), 2880);
		suite.check(contains(header, "NAXIS   =                    3"), "NAXIS equals 3 for a cube");
		suite.check(contains(header, "NAXIS3  =                    4"), "NAXIS3 equals the number of wavelength channels");
	}
	return suite.finish();
}
