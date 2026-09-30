#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace Relativistic::IO {

enum class ScreenshotFormat : uint32_t {
	PPM = 0,
	BMP = 1,
	PNG = 2,
	TGA = 3,
	HDR = 4,
	PNG16 = 5,
	QOI = 6,
	PFM = 7,
	TIFF = 8,
	PAM = 9
};

inline constexpr uint32_t kScreenshotFormatCount = 10;

struct ImageFormatDescriptor {
	ScreenshotFormat format;
	std::string_view display_name;
	std::string_view extension;
	bool supports_comment;
	bool preserves_alpha;
	bool stores_float_samples;
	uint64_t max_dimension;
	uint64_t max_pixels;
	uint64_t max_file_bytes;
	uint32_t bytes_per_pixel;
};

[[nodiscard]] inline const ImageFormatDescriptor& image_format_descriptor(ScreenshotFormat format) noexcept {
	static constexpr std::array<ImageFormatDescriptor, kScreenshotFormatCount> table{{
		{ScreenshotFormat::PPM, "PPM (Lossless, 8-bit RGB)", "ppm", false, false, false, 0xFFFFFFFFULL, 0, 0, 3},
		{ScreenshotFormat::BMP, "BMP (Lossless, 8-bit RGB)", "bmp", false, false, false, 0x7FFFFFFFULL, 0, 4290000000ULL, 3},
		{ScreenshotFormat::PNG, "PNG (Lossless, 8-bit RGBA, Metadata)", "png", true, true, false, 0x7FFFFFFFULL, 0, 0, 4},
		{ScreenshotFormat::TGA, "TGA (Lossless, 8-bit RGB)", "tga", false, false, false, 65535ULL, 0, 0, 3},
		{ScreenshotFormat::HDR, "HDR (Radiance RGBE, Linear Float)", "hdr", true, false, true, 0xFFFFFFFFULL, 0, 0, 4},
		{ScreenshotFormat::PNG16, "PNG 16-bit (Lossless, High Precision, Metadata)", "png", true, true, false, 0x7FFFFFFFULL, 0, 0, 8},
		{ScreenshotFormat::QOI, "QOI (Lossless, Fast Compressed, 8-bit RGBA)", "qoi", false, true, false, 0xFFFFFFFFULL, 400000000ULL, 0, 4},
		{ScreenshotFormat::PFM, "PFM (Portable Float Map, 32-bit Float RGB)", "pfm", false, false, true, 0xFFFFFFFFULL, 0, 0, 12},
		{ScreenshotFormat::TIFF, "TIFF (Lossless, 8-bit RGB, Metadata)", "tif", true, false, false, 0xFFFFFFFFULL, 0, 4290000000ULL, 3},
		{ScreenshotFormat::PAM, "PAM (Lossless, 8-bit RGBA)", "pam", false, true, false, 0xFFFFFFFFULL, 0, 0, 4}
	}};
	const size_t index = static_cast<size_t>(format);
	return table[index < table.size() ? index : 0];
}

[[nodiscard]] inline uint64_t estimate_image_file_bytes(ScreenshotFormat format, uint64_t width, uint64_t height) noexcept {
	return width * height * static_cast<uint64_t>(image_format_descriptor(format).bytes_per_pixel);
}

[[nodiscard]] inline std::optional<std::string> validate_image_dimensions(ScreenshotFormat format, uint64_t width, uint64_t height) {
	const auto& descriptor = image_format_descriptor(format);
	if (width == 0 || height == 0) {
		return std::string("Image dimensions must be strictly positive.");
	}
	if (width > descriptor.max_dimension || height > descriptor.max_dimension) {
		return std::string(descriptor.display_name) + " supports at most " + std::to_string(descriptor.max_dimension) + " pixels per side.";
	}
	if (descriptor.max_pixels != 0 && width * height > descriptor.max_pixels) {
		return std::string(descriptor.display_name) + " supports at most " + std::to_string(descriptor.max_pixels) + " pixels in total.";
	}
	if (descriptor.max_file_bytes != 0 && estimate_image_file_bytes(format, width, height) > descriptor.max_file_bytes) {
		return std::string(descriptor.display_name) + " cannot store an image of this size because the container is limited to 4 GiB.";
	}
	return std::nullopt;
}

}
