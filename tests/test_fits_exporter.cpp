#include "relativistic/io/fits_exporter.hpp"
#include <iostream>
#include <vector>
#include <cassert>

int main() {
	using namespace Relativistic::IO;

	const size_t width = 64;
	const size_t height = 32;
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

	const auto fits_bytes = FitsExporter::export_image_2d(image, width, height, wcs);
	assert(!fits_bytes.empty());
	assert(fits_bytes.size() % 2880 == 0);

	const std::string_view header_str(reinterpret_cast<const char*>(fits_bytes.data()), 2880);
	assert(header_str.find("SIMPLE  =                    T") != std::string_view::npos);
	assert(header_str.find("BITPIX  =                  -64") != std::string_view::npos);
	assert(header_str.find("NAXIS   =                    2") != std::string_view::npos);
	assert(header_str.find("NAXIS1  =                   64") != std::string_view::npos);
	assert(header_str.find("NAXIS2  =                   32") != std::string_view::npos);
	assert(header_str.find("OBJECT  = 'SagittariusA_Star'") != std::string_view::npos);
	assert(header_str.find("END") != std::string_view::npos);

	const size_t wavelengths = 4;
	std::vector<double> cube(width * height * wavelengths, 1.2345);
	const auto fits_cube = FitsExporter::export_spectral_cube_3d(cube, width, height, wavelengths, wcs);
	assert(fits_cube.size() % 2880 == 0);

	const std::string_view cube_header(reinterpret_cast<const char*>(fits_cube.data()), 2880);
	assert(cube_header.find("NAXIS   =                    3") != std::string_view::npos);
	assert(cube_header.find("NAXIS3  =                    4") != std::string_view::npos);

	return 0;
}
