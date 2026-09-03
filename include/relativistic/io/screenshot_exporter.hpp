#pragma once

#include "relativistic/render/gpu_types.hpp"
#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <fstream>
#include <thread>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace Relativistic::IO {

enum class ScreenshotFormat : uint32_t {
	PPM = 0,
	BMP = 1
};

class ScreenshotExporter {
public:
	[[nodiscard]] static std::string expand_filename_pattern(std::string_view pattern) {
		const auto now = std::chrono::system_clock::now();
		const std::time_t t = std::chrono::system_clock::to_time_t(now);
		std::tm tm_buf{};
#if defined(_WIN32)
		localtime_s(&tm_buf, &t);
#else
		localtime_r(&t, &tm_buf);
#endif
		std::ostringstream ss;
		ss << std::put_time(&tm_buf, std::string(pattern).c_str());
		return ss.str();
	}

	static void export_async(
		std::vector<Render::GpuPixelOutput> pixels,
		uint32_t width,
		uint32_t height,
		std::string output_directory,
		std::string filename_stem,
		ScreenshotFormat format
	) {
		std::thread([pixels = std::move(pixels), width, height, output_directory = std::move(output_directory), filename_stem = std::move(filename_stem), format]() mutable {
			write_to_disk(pixels, width, height, output_directory, filename_stem, format);
		}).detach();
	}

private:
	static void write_to_disk(
		const std::vector<Render::GpuPixelOutput>& pixels,
		uint32_t width,
		uint32_t height,
		const std::string& output_directory,
		const std::string& filename_stem,
		ScreenshotFormat format
	) {
		std::error_code ec;
		std::filesystem::create_directories(output_directory, ec);
		if (ec) return;

		const std::string extension = (format == ScreenshotFormat::BMP) ? ".bmp" : ".ppm";
		std::filesystem::path out_path = std::filesystem::path(output_directory) / (filename_stem + extension);

		size_t suffix = 1;
		while (std::filesystem::exists(out_path)) {
			out_path = std::filesystem::path(output_directory) / (filename_stem + "_" + std::to_string(suffix) + extension);
			++suffix;
		}

		if (format == ScreenshotFormat::BMP) {
			write_bmp(out_path, pixels, width, height);
		} else {
			write_ppm(out_path, pixels, width, height);
		}
	}

	[[nodiscard]] static uint8_t to_byte(float v) noexcept {
		return static_cast<uint8_t>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
	}

	static void write_ppm(const std::filesystem::path& path, const std::vector<Render::GpuPixelOutput>& pixels, uint32_t width, uint32_t height) {
		std::ofstream out(path, std::ios::binary);
		if (!out.is_open()) return;
		out << "P6\n" << width << " " << height << "\n255\n";
		std::vector<uint8_t> row(static_cast<size_t>(width) * 3);
		for (uint32_t y = 0; y < height; ++y) {
			for (uint32_t x = 0; x < width; ++x) {
				const size_t idx = static_cast<size_t>(y) * width + x;
				if (idx >= pixels.size()) continue;
				row[x * 3 + 0] = to_byte(pixels[idx].r);
				row[x * 3 + 1] = to_byte(pixels[idx].g);
				row[x * 3 + 2] = to_byte(pixels[idx].b);
			}
			out.write(reinterpret_cast<const char*>(row.data()), static_cast<std::streamsize>(row.size()));
		}
	}

	static void write_bmp(const std::filesystem::path& path, const std::vector<Render::GpuPixelOutput>& pixels, uint32_t width, uint32_t height) {
		std::ofstream out(path, std::ios::binary);
		if (!out.is_open()) return;

		const uint32_t row_padded = (width * 3 + 3) & ~3U;
		const uint32_t data_size = row_padded * height;
		const uint32_t file_size = 54 + data_size;

		auto write_u16 = [&](uint16_t v) { out.write(reinterpret_cast<const char*>(&v), 2); };
		auto write_u32 = [&](uint32_t v) { out.write(reinterpret_cast<const char*>(&v), 4); };
		auto write_i32 = [&](int32_t v) { out.write(reinterpret_cast<const char*>(&v), 4); };

		out.put('B'); out.put('M');
		write_u32(file_size);
		write_u32(0);
		write_u32(54);
		write_u32(40);
		write_i32(static_cast<int32_t>(width));
		write_i32(static_cast<int32_t>(height));
		write_u16(1);
		write_u16(24);
		write_u32(0);
		write_u32(data_size);
		write_i32(2835);
		write_i32(2835);
		write_u32(0);
		write_u32(0);

		std::vector<uint8_t> row(row_padded, 0);
		for (int32_t y = static_cast<int32_t>(height) - 1; y >= 0; --y) {
			for (uint32_t x = 0; x < width; ++x) {
				const size_t idx = static_cast<size_t>(y) * width + x;
				uint8_t r = 0, g = 0, b = 0;
				if (idx < pixels.size()) {
					r = to_byte(pixels[idx].r);
					g = to_byte(pixels[idx].g);
					b = to_byte(pixels[idx].b);
				}
				row[x * 3 + 0] = b;
				row[x * 3 + 1] = g;
				row[x * 3 + 2] = r;
			}
			out.write(reinterpret_cast<const char*>(row.data()), row_padded);
		}
	}
};

}
