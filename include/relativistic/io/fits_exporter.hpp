#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <span>
#include <string_view>
#include <string>
#include <array>
#include <bit>
#include <algorithm>

namespace Relativistic::IO {

struct FitsWcsMetadata {
	double crpix1{0.0};
	double crpix2{0.0};
	double crpix3{0.0};
	double crval1{0.0};
	double crval2{0.0};
	double crval3{0.0};
	double cdelt1{1.0};
	double cdelt2{1.0};
	double cdelt3{1.0};
	std::string ctype1{"RA---TAN"};
	std::string ctype2{"DEC--TAN"};
	std::string ctype3{"WAVE"};
	std::string cunit1{"deg"};
	std::string cunit2{"deg"};
	std::string cunit3{"m"};
	std::string telescope{"RelativisticEngine"};
	std::string observer{"Observer"};
	std::string object_name{"KerrBlackHole"};
	std::string date_obs{"2000-01-01T12:00:00.0"};
	std::string bunit{"W/m2/sr"};
};

class FitsExporter {
private:
	static constexpr size_t FITS_BLOCK_SIZE = 2880;
	static constexpr size_t FITS_CARD_SIZE = 80;

	static void append_card(std::vector<char>& header, std::string_view key, std::string_view val, std::string_view comment = {}) {
		char card[FITS_CARD_SIZE];
		std::memset(card, ' ', FITS_CARD_SIZE);

		const size_t k_len = std::min(key.size(), static_cast<size_t>(8));
		std::memcpy(card, key.data(), k_len);

		if (!val.empty()) {
			card[8] = '=';
			card[9] = ' ';
			if (val.front() == '\'') {
				const size_t v_len = std::min(val.size(), static_cast<size_t>(70));
				std::memcpy(card + 10, val.data(), v_len);
			} else {
				const size_t v_len = std::min(val.size(), static_cast<size_t>(20));
				const size_t start_pos = 30 - v_len;
				std::memcpy(card + start_pos, val.data(), v_len);
			}
		}

		if (!comment.empty()) {
			const size_t c_pos = 32;
			card[c_pos] = '/';
			card[c_pos + 1] = ' ';
			const size_t c_len = std::min(comment.size(), FITS_CARD_SIZE - c_pos - 2);
			std::memcpy(card + c_pos + 2, comment.data(), c_len);
		}

		header.insert(header.end(), card, card + FITS_CARD_SIZE);
	}

	static void pad_to_block_size(std::vector<char>& data, char pad_char) {
		const size_t rem = data.size() % FITS_BLOCK_SIZE;
		if (rem != 0) {
			const size_t padding = FITS_BLOCK_SIZE - rem;
			data.insert(data.end(), padding, pad_char);
		}
	}

	static constexpr uint64_t swap_endian_64(uint64_t val) noexcept {
		if constexpr (std::endian::native == std::endian::little) {
			return ((val & 0xFF00000000000000ULL) >> 56) |
			       ((val & 0x00FF000000000000ULL) >> 40) |
			       ((val & 0x0000FF0000000000ULL) >> 24) |
			       ((val & 0x000000FF00000000ULL) >> 8)  |
			       ((val & 0x00000000FF000000ULL) << 8)  |
			       ((val & 0x0000000000FF0000ULL) << 24) |
			       ((val & 0x000000000000FF00ULL) << 40) |
			       ((val & 0x00000000000000FFULL) << 56);
		}
		return val;
	}

	static constexpr uint32_t swap_endian_32(uint32_t val) noexcept {
		if constexpr (std::endian::native == std::endian::little) {
			return ((val & 0xFF000000U) >> 24) |
			       ((val & 0x00FF0000U) >> 8)  |
			       ((val & 0x0000FF00U) << 8)  |
			       ((val & 0x000000FFU) << 24);
		}
		return val;
	}

public:
	[[nodiscard]] static std::vector<uint8_t> export_image_2d(
		std::span<const double> pixels,
		size_t width,
		size_t height,
		const FitsWcsMetadata& wcs = {}
	) {
		std::vector<char> header;
		header.reserve(FITS_BLOCK_SIZE);

		append_card(header, "SIMPLE", "T", "Standard FITS format");
		append_card(header, "BITPIX", "-64", "IEEE 754 double precision");
		append_card(header, "NAXIS", "2", "2-dimensional image");
		append_card(header, "NAXIS1", std::to_string(width), "Image width");
		append_card(header, "NAXIS2", std::to_string(height), "Image height");
		append_card(header, "EXTEND", "T", "Extensions permitted");
		append_card(header, "BSCALE", "1.000000000000E+00", "Linear scale factor");
		append_card(header, "BZERO", "0.000000000000E+00", "Zero offset");
		append_card(header, "BUNIT", "'" + wcs.bunit + "'", "Physical units");

		append_card(header, "CRPIX1", std::to_string(wcs.crpix1), "Reference pixel X");
		append_card(header, "CRPIX2", std::to_string(wcs.crpix2), "Reference pixel Y");
		append_card(header, "CRVAL1", std::to_string(wcs.crval1), "Reference coordinate X");
		append_card(header, "CRVAL2", std::to_string(wcs.crval2), "Reference coordinate Y");
		append_card(header, "CDELT1", std::to_string(wcs.cdelt1), "Coordinate increment X");
		append_card(header, "CDELT2", std::to_string(wcs.cdelt2), "Coordinate increment Y");
		append_card(header, "CTYPE1", "'" + wcs.ctype1 + "'", "Coordinate type X");
		append_card(header, "CTYPE2", "'" + wcs.ctype2 + "'", "Coordinate type Y");
		append_card(header, "CUNIT1", "'" + wcs.cunit1 + "'", "Units X");
		append_card(header, "CUNIT2", "'" + wcs.cunit2 + "'", "Units Y");

		append_card(header, "OBJECT", "'" + wcs.object_name + "'", "Source object");
		append_card(header, "TELESCOP", "'" + wcs.telescope + "'", "Telescope / Engine");
		append_card(header, "OBSERVER", "'" + wcs.observer + "'", "Observer");
		append_card(header, "DATE-OBS", "'" + wcs.date_obs + "'", "Observation date UTC");
		append_card(header, "END", "");

		pad_to_block_size(header, ' ');

		std::vector<char> data_bytes;
		data_bytes.resize(pixels.size() * sizeof(double));

		for (size_t i = 0; i < pixels.size(); ++i) {
			double val = pixels[i];
			uint64_t raw;
			std::memcpy(&raw, &val, sizeof(double));
			raw = swap_endian_64(raw);
			std::memcpy(data_bytes.data() + i * sizeof(double), &raw, sizeof(double));
		}

		pad_to_block_size(data_bytes, '\0');

		std::vector<uint8_t> result;
		result.resize(header.size() + data_bytes.size());
		std::memcpy(result.data(), header.data(), header.size());
		std::memcpy(result.data() + header.size(), data_bytes.data(), data_bytes.size());

		return result;
	}

	[[nodiscard]] static std::vector<uint8_t> export_spectral_cube_3d(
		std::span<const double> voxel_cube,
		size_t width,
		size_t height,
		size_t num_wavelengths,
		const FitsWcsMetadata& wcs = {}
	) {
		std::vector<char> header;
		header.reserve(FITS_BLOCK_SIZE);

		append_card(header, "SIMPLE", "T", "Standard FITS format");
		append_card(header, "BITPIX", "-64", "IEEE 754 double precision");
		append_card(header, "NAXIS", "3", "3-dimensional spectral data cube");
		append_card(header, "NAXIS1", std::to_string(width), "Spatial X");
		append_card(header, "NAXIS2", std::to_string(height), "Spatial Y");
		append_card(header, "NAXIS3", std::to_string(num_wavelengths), "Spectral wavelength channels");
		append_card(header, "EXTEND", "T", "Extensions permitted");
		append_card(header, "BSCALE", "1.000000000000E+00", "Linear scale factor");
		append_card(header, "BZERO", "0.000000000000E+00", "Zero offset");
		append_card(header, "BUNIT", "'" + wcs.bunit + "'", "Physical units");

		append_card(header, "CRPIX1", std::to_string(wcs.crpix1), "Reference pixel X");
		append_card(header, "CRPIX2", std::to_string(wcs.crpix2), "Reference pixel Y");
		append_card(header, "CRPIX3", std::to_string(wcs.crpix3), "Reference pixel Lambda");
		append_card(header, "CRVAL1", std::to_string(wcs.crval1), "Reference coordinate X");
		append_card(header, "CRVAL2", std::to_string(wcs.crval2), "Reference coordinate Y");
		append_card(header, "CRVAL3", std::to_string(wcs.crval3), "Reference coordinate Lambda");
		append_card(header, "CDELT1", std::to_string(wcs.cdelt1), "Increment X");
		append_card(header, "CDELT2", std::to_string(wcs.cdelt2), "Increment Y");
		append_card(header, "CDELT3", std::to_string(wcs.cdelt3), "Increment Lambda");
		append_card(header, "CTYPE1", "'" + wcs.ctype1 + "'", "RA");
		append_card(header, "CTYPE2", "'" + wcs.ctype2 + "'", "DEC");
		append_card(header, "CTYPE3", "'" + wcs.ctype3 + "'", "Wavelength");
		append_card(header, "CUNIT1", "'" + wcs.cunit1 + "'", "deg");
		append_card(header, "CUNIT2", "'" + wcs.cunit2 + "'", "deg");
		append_card(header, "CUNIT3", "'" + wcs.cunit3 + "'", "m");

		append_card(header, "OBJECT", "'" + wcs.object_name + "'", "Source object");
		append_card(header, "TELESCOP", "'" + wcs.telescope + "'", "Telescope / Engine");
		append_card(header, "OBSERVER", "'" + wcs.observer + "'", "Observer");
		append_card(header, "DATE-OBS", "'" + wcs.date_obs + "'", "Observation date UTC");
		append_card(header, "END", "");

		pad_to_block_size(header, ' ');

		std::vector<char> data_bytes;
		data_bytes.resize(voxel_cube.size() * sizeof(double));

		for (size_t i = 0; i < voxel_cube.size(); ++i) {
			double val = voxel_cube[i];
			uint64_t raw;
			std::memcpy(&raw, &val, sizeof(double));
			raw = swap_endian_64(raw);
			std::memcpy(data_bytes.data() + i * sizeof(double), &raw, sizeof(double));
		}

		pad_to_block_size(data_bytes, '\0');

		std::vector<uint8_t> result;
		result.resize(header.size() + data_bytes.size());
		std::memcpy(result.data(), header.data(), header.size());
		std::memcpy(result.data() + header.size(), data_bytes.data(), data_bytes.size());

		return result;
	}
};

}
