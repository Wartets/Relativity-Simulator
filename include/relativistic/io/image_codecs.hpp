#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <array>
#include <string>
#include <cmath>
#include <algorithm>

namespace Relativistic::IO {

class ImageCodecs {
private:
	static constexpr std::array<uint32_t, 256> build_crc_table() noexcept {
		std::array<uint32_t, 256> table{};
		for (uint32_t n = 0; n < 256; ++n) {
			uint32_t c = n;
			for (int k = 0; k < 8; ++k) {
				c = (c & 1) ? (0xEDB88320U ^ (c >> 1)) : (c >> 1);
			}
			table[n] = c;
		}
		return table;
	}

	static uint32_t crc32(const uint8_t* data, size_t len) noexcept {
		static constexpr auto table = build_crc_table();
		uint32_t c = 0xFFFFFFFFU;
		for (size_t i = 0; i < len; ++i) {
			c = table[(c ^ data[i]) & 0xFFU] ^ (c >> 8);
		}
		return c ^ 0xFFFFFFFFU;
	}

	static uint32_t adler32(const uint8_t* data, size_t len) noexcept {
		uint32_t a = 1, b = 0;
		constexpr uint32_t mod_adler = 65521U;
		for (size_t i = 0; i < len; ++i) {
			a = (a + data[i]) % mod_adler;
			b = (b + a) % mod_adler;
		}
		return (b << 16) | a;
	}

	static void append_u32_be(std::vector<uint8_t>& out, uint32_t v) {
		out.push_back(static_cast<uint8_t>(v >> 24));
		out.push_back(static_cast<uint8_t>(v >> 16));
		out.push_back(static_cast<uint8_t>(v >> 8));
		out.push_back(static_cast<uint8_t>(v));
	}

	static void append_png_chunk(std::vector<uint8_t>& out, const char* type, const std::vector<uint8_t>& payload) {
		append_u32_be(out, static_cast<uint32_t>(payload.size()));
		std::vector<uint8_t> type_and_payload;
		type_and_payload.reserve(4 + payload.size());
		type_and_payload.insert(type_and_payload.end(), type, type + 4);
		type_and_payload.insert(type_and_payload.end(), payload.begin(), payload.end());
		out.insert(out.end(), type_and_payload.begin(), type_and_payload.end());
		append_u32_be(out, crc32(type_and_payload.data(), type_and_payload.size()));
	}

	static std::vector<uint8_t> deflate_stored(const std::vector<uint8_t>& raw) {
		std::vector<uint8_t> out;
		out.push_back(0x78);
		out.push_back(0x01);

		constexpr size_t max_block = 65535;
		size_t offset = 0;
		do {
			const size_t remaining = raw.size() - offset;
			const size_t block_len = std::min(remaining, max_block);
			const bool is_final = (offset + block_len >= raw.size());

			out.push_back(is_final ? 0x01 : 0x00);
			const uint16_t len16 = static_cast<uint16_t>(block_len);
			const uint16_t nlen16 = static_cast<uint16_t>(~len16 & 0xFFFFU);
			out.push_back(static_cast<uint8_t>(len16 & 0xFF));
			out.push_back(static_cast<uint8_t>(len16 >> 8));
			out.push_back(static_cast<uint8_t>(nlen16 & 0xFF));
			out.push_back(static_cast<uint8_t>(nlen16 >> 8));
			out.insert(out.end(), raw.begin() + static_cast<std::ptrdiff_t>(offset), raw.begin() + static_cast<std::ptrdiff_t>(offset + block_len));

			offset += block_len;
		} while (offset < raw.size());

		append_u32_be(out, adler32(raw.data(), raw.size()));
		return out;
	}

public:
	[[nodiscard]] static std::vector<uint8_t> encode_tga_bgr(
		const std::vector<std::array<uint8_t, 3>>& rgb_pixels,
		uint32_t width,
		uint32_t height
	) {
		std::vector<uint8_t> out;
		out.reserve(18 + rgb_pixels.size() * 3);
		out.insert(out.end(), {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0});
		out.push_back(static_cast<uint8_t>(width & 0xFF));
		out.push_back(static_cast<uint8_t>((width >> 8) & 0xFF));
		out.push_back(static_cast<uint8_t>(height & 0xFF));
		out.push_back(static_cast<uint8_t>((height >> 8) & 0xFF));
		out.push_back(24);
		out.push_back(0x00);

		for (int32_t y = static_cast<int32_t>(height) - 1; y >= 0; --y) {
			for (uint32_t x = 0; x < width; ++x) {
				const auto& px = rgb_pixels[static_cast<size_t>(y) * width + x];
				out.push_back(px[2]);
				out.push_back(px[1]);
				out.push_back(px[0]);
			}
		}
		return out;
	}

	[[nodiscard]] static std::vector<uint8_t> encode_png_rgba(
		const std::vector<std::array<uint8_t, 4>>& rgba_pixels,
		uint32_t width,
		uint32_t height,
		const std::string& comment_text = {}
	) {
		std::vector<uint8_t> out;
		static constexpr uint8_t signature[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
		out.insert(out.end(), signature, signature + 8);

		std::vector<uint8_t> ihdr;
		append_u32_be(ihdr, width);
		append_u32_be(ihdr, height);
		ihdr.push_back(8);
		ihdr.push_back(6);
		ihdr.push_back(0);
		ihdr.push_back(0);
		ihdr.push_back(0);
		append_png_chunk(out, "IHDR", ihdr);

		if (!comment_text.empty()) {
			std::vector<uint8_t> text_chunk;
			static const char keyword[] = "Comment";
			text_chunk.insert(text_chunk.end(), keyword, keyword + 7);
			text_chunk.push_back(0);
			text_chunk.insert(text_chunk.end(), comment_text.begin(), comment_text.end());
			append_png_chunk(out, "tEXt", text_chunk);
		}

		std::vector<uint8_t> raw;
		raw.reserve(static_cast<size_t>(height) * (1 + static_cast<size_t>(width) * 4));
		for (uint32_t y = 0; y < height; ++y) {
			raw.push_back(0);
			for (uint32_t x = 0; x < width; ++x) {
				const auto& px = rgba_pixels[static_cast<size_t>(y) * width + x];
				raw.push_back(px[0]);
				raw.push_back(px[1]);
				raw.push_back(px[2]);
				raw.push_back(px[3]);
			}
		}

		append_png_chunk(out, "IDAT", deflate_stored(raw));
		append_png_chunk(out, "IEND", {});
		return out;
	}

	[[nodiscard]] static std::vector<uint8_t> encode_radiance_hdr(
		const std::vector<std::array<float, 3>>& linear_rgb,
		uint32_t width,
		uint32_t height,
		const std::string& comment_text = {}
	) {
		std::vector<uint8_t> out;
		std::string header = "#?RADIANCE\n";
		if (!comment_text.empty()) {
			header += "# " + comment_text + "\n";
		}
		header += "FORMAT=32-bit_rle_rgbe\n\n-Y ";
		header += std::to_string(height);
		header += " +X ";
		header += std::to_string(width);
		header += "\n";
		out.insert(out.end(), header.begin(), header.end());

		for (uint32_t y = 0; y < height; ++y) {
			for (uint32_t x = 0; x < width; ++x) {
				const auto& px = linear_rgb[static_cast<size_t>(y) * width + x];
				const float max_c = std::max({px[0], px[1], px[2], 1e-32f});
				int exponent = 0;
				const float mantissa = std::frexp(max_c, &exponent);
				const float scale = mantissa * 256.0f / max_c;
				out.push_back(static_cast<uint8_t>(std::clamp(px[0] * scale, 0.0f, 255.0f)));
				out.push_back(static_cast<uint8_t>(std::clamp(px[1] * scale, 0.0f, 255.0f)));
				out.push_back(static_cast<uint8_t>(std::clamp(px[2] * scale, 0.0f, 255.0f)));
				out.push_back(static_cast<uint8_t>(exponent + 128));
			}
		}
		return out;
	}
};

}
