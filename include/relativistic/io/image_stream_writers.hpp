#pragma once

#include "relativistic/io/image_codecs.hpp"
#include "relativistic/io/image_format.hpp"
#include "relativistic/render/gpu_types.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Relativistic::IO {

namespace ImageStreamDetail {

[[nodiscard]] inline uint8_t quantize8(float value) noexcept {
	return static_cast<uint8_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
}

[[nodiscard]] inline uint16_t quantize16(float value) noexcept {
	return static_cast<uint16_t>(std::clamp(value, 0.0f, 1.0f) * 65535.0f + 0.5f);
}

template <typename T>
inline void append_le(std::vector<uint8_t>& buffer, T value) {
	for (size_t i = 0; i < sizeof(T); ++i) {
		buffer.push_back(static_cast<uint8_t>((static_cast<uint64_t>(value) >> (8U * i)) & 0xFFU));
	}
}

template <typename T>
inline void append_be(std::vector<uint8_t>& buffer, T value) {
	for (size_t i = sizeof(T); i > 0; --i) {
		buffer.push_back(static_cast<uint8_t>((static_cast<uint64_t>(value) >> (8U * (i - 1))) & 0xFFU));
	}
}

inline void append_text(std::vector<uint8_t>& buffer, std::string_view text) {
	buffer.insert(buffer.end(), text.begin(), text.end());
}

}

class ImageStreamWriter {
private:
	std::filesystem::path path_{};
	uint32_t rows_written_{0};

	void remove_partial() noexcept {
		std::error_code ec;
		std::filesystem::remove(path_, ec);
	}

protected:
	std::ofstream out_{};
	uint32_t width_{0};
	uint32_t height_{0};
	std::string comment_{};
	std::vector<uint8_t> scratch_{};

	[[nodiscard]] bool write_bytes(const void* data, size_t size) {
		out_.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
		return out_.good();
	}

	[[nodiscard]] bool write_buffer(const std::vector<uint8_t>& buffer) {
		return write_bytes(buffer.data(), buffer.size());
	}

	virtual bool write_header() = 0;
	virtual bool emit_rows(std::span<const Render::GpuPixelOutput> pixels, uint32_t first_row, uint32_t row_count) = 0;

	virtual bool write_trailer() {
		return true;
	}

public:
	virtual ~ImageStreamWriter() = default;

	[[nodiscard]] bool begin(const std::filesystem::path& path, uint32_t width, uint32_t height, const std::string& comment) {
		path_ = path;
		width_ = width;
		height_ = height;
		comment_ = comment;
		rows_written_ = 0;
		std::error_code ec;
		if (path.has_parent_path()) {
			std::filesystem::create_directories(path.parent_path(), ec);
		}
		out_.open(path, std::ios::binary | std::ios::trunc);
		if (!out_.is_open()) {
			return false;
		}
		if (!write_header() || !out_.good()) {
			abort();
			return false;
		}
		return true;
	}

	[[nodiscard]] bool write_rows(std::span<const Render::GpuPixelOutput> pixels) {
		if (width_ == 0U || !out_.is_open()) {
			return false;
		}
		const size_t rows = pixels.size() / width_;
		if (rows == 0U || rows_written_ + rows > height_) {
			return false;
		}
		const uint32_t row_count = static_cast<uint32_t>(rows);
		if (!emit_rows(pixels.first(rows * width_), rows_written_, row_count) || !out_.good()) {
			return false;
		}
		rows_written_ += row_count;
		return true;
	}

	[[nodiscard]] bool finish() {
		if (!out_.is_open() || rows_written_ != height_) {
			abort();
			return false;
		}
		const bool trailer_ok = write_trailer();
		out_.flush();
		const bool good = trailer_ok && out_.good();
		out_.close();
		if (!good) {
			remove_partial();
		}
		return good;
	}

	void abort() noexcept {
		if (out_.is_open()) {
			out_.close();
		}
		remove_partial();
	}
};

class PpmStreamWriter final : public ImageStreamWriter {
protected:
	bool write_header() override {
		const std::string header = "P6\n" + std::to_string(width_) + " " + std::to_string(height_) + "\n255\n";
		return write_bytes(header.data(), header.size());
	}

	bool emit_rows(std::span<const Render::GpuPixelOutput> pixels, uint32_t, uint32_t) override {
		scratch_.resize(pixels.size() * 3U);
		size_t offset = 0;
		for (const auto& px : pixels) {
			scratch_[offset++] = ImageStreamDetail::quantize8(px.r);
			scratch_[offset++] = ImageStreamDetail::quantize8(px.g);
			scratch_[offset++] = ImageStreamDetail::quantize8(px.b);
		}
		return write_buffer(scratch_);
	}
};

class PamStreamWriter final : public ImageStreamWriter {
protected:
	bool write_header() override {
		const std::string header = "P7\nWIDTH " + std::to_string(width_) + "\nHEIGHT " + std::to_string(height_) + "\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n";
		return write_bytes(header.data(), header.size());
	}

	bool emit_rows(std::span<const Render::GpuPixelOutput> pixels, uint32_t, uint32_t) override {
		scratch_.resize(pixels.size() * 4U);
		size_t offset = 0;
		for (const auto& px : pixels) {
			scratch_[offset++] = ImageStreamDetail::quantize8(px.r);
			scratch_[offset++] = ImageStreamDetail::quantize8(px.g);
			scratch_[offset++] = ImageStreamDetail::quantize8(px.b);
			scratch_[offset++] = ImageStreamDetail::quantize8(px.a);
		}
		return write_buffer(scratch_);
	}
};

class BmpStreamWriter final : public ImageStreamWriter {
private:
	[[nodiscard]] uint32_t padded_row_bytes() const noexcept {
		return (width_ * 3U + 3U) & ~3U;
	}

protected:
	bool write_header() override {
		using namespace ImageStreamDetail;
		const uint32_t data_size = padded_row_bytes() * height_;
		std::vector<uint8_t> header;
		header.reserve(54);
		header.push_back('B');
		header.push_back('M');
		append_le<uint32_t>(header, 54U + data_size);
		append_le<uint32_t>(header, 0U);
		append_le<uint32_t>(header, 54U);
		append_le<uint32_t>(header, 40U);
		append_le<int32_t>(header, static_cast<int32_t>(width_));
		append_le<int32_t>(header, -static_cast<int32_t>(height_));
		append_le<uint16_t>(header, 1U);
		append_le<uint16_t>(header, 24U);
		append_le<uint32_t>(header, 0U);
		append_le<uint32_t>(header, data_size);
		append_le<int32_t>(header, 2835);
		append_le<int32_t>(header, 2835);
		append_le<uint32_t>(header, 0U);
		append_le<uint32_t>(header, 0U);
		return write_buffer(header);
	}

	bool emit_rows(std::span<const Render::GpuPixelOutput> pixels, uint32_t, uint32_t row_count) override {
		const size_t row_bytes = padded_row_bytes();
		scratch_.assign(row_bytes * row_count, 0);
		for (uint32_t r = 0; r < row_count; ++r) {
			uint8_t* destination = scratch_.data() + static_cast<size_t>(r) * row_bytes;
			for (uint32_t x = 0; x < width_; ++x) {
				const auto& px = pixels[static_cast<size_t>(r) * width_ + x];
				destination[x * 3U + 0U] = ImageStreamDetail::quantize8(px.b);
				destination[x * 3U + 1U] = ImageStreamDetail::quantize8(px.g);
				destination[x * 3U + 2U] = ImageStreamDetail::quantize8(px.r);
			}
		}
		return write_buffer(scratch_);
	}
};

class TgaStreamWriter final : public ImageStreamWriter {
protected:
	bool write_header() override {
		using namespace ImageStreamDetail;
		std::vector<uint8_t> header;
		header.reserve(18);
		header.insert(header.end(), {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0});
		append_le<uint16_t>(header, static_cast<uint16_t>(width_));
		append_le<uint16_t>(header, static_cast<uint16_t>(height_));
		header.push_back(24);
		header.push_back(0x20);
		return write_buffer(header);
	}

	bool emit_rows(std::span<const Render::GpuPixelOutput> pixels, uint32_t, uint32_t) override {
		scratch_.resize(pixels.size() * 3U);
		size_t offset = 0;
		for (const auto& px : pixels) {
			scratch_[offset++] = ImageStreamDetail::quantize8(px.b);
			scratch_[offset++] = ImageStreamDetail::quantize8(px.g);
			scratch_[offset++] = ImageStreamDetail::quantize8(px.r);
		}
		return write_buffer(scratch_);
	}
};

class PngStreamWriter final : public ImageStreamWriter {
private:
	static constexpr size_t kBlockSize = 65535;

	bool sixteen_bit_;
	std::vector<uint8_t> block_{};
	std::vector<uint8_t> row_{};
	uint32_t adler_{1U};
	bool zlib_header_pending_{true};

	[[nodiscard]] bool write_chunk(const char* type, const std::vector<uint8_t>& payload) {
		std::vector<uint8_t> length_field;
		ImageStreamDetail::append_be<uint32_t>(length_field, static_cast<uint32_t>(payload.size()));
		if (!write_buffer(length_field) || !write_bytes(type, 4)) {
			return false;
		}
		uint32_t crc = ImageCodecs::crc32_extend(0U, reinterpret_cast<const uint8_t*>(type), 4);
		crc = ImageCodecs::crc32_extend(crc, payload.data(), payload.size());
		if (!write_buffer(payload)) {
			return false;
		}
		std::vector<uint8_t> crc_field;
		ImageStreamDetail::append_be<uint32_t>(crc_field, crc);
		return write_buffer(crc_field);
	}

	[[nodiscard]] bool flush_block(bool final_block) {
		using namespace ImageStreamDetail;
		std::vector<uint8_t> payload;
		payload.reserve(block_.size() + 11U);
		if (zlib_header_pending_) {
			payload.push_back(0x78);
			payload.push_back(0x01);
			zlib_header_pending_ = false;
		}
		payload.push_back(final_block ? 1U : 0U);
		const uint16_t length = static_cast<uint16_t>(block_.size());
		append_le<uint16_t>(payload, length);
		append_le<uint16_t>(payload, static_cast<uint16_t>(~length & 0xFFFFU));
		payload.insert(payload.end(), block_.begin(), block_.end());
		if (final_block) {
			append_be<uint32_t>(payload, adler_);
		}
		block_.clear();
		return write_chunk("IDAT", payload);
	}

	[[nodiscard]] bool append_raw(const uint8_t* data, size_t size) {
		adler_ = ImageCodecs::adler32_extend(adler_, data, size);
		while (size > 0) {
			const size_t take = std::min(size, kBlockSize - block_.size());
			block_.insert(block_.end(), data, data + take);
			data += take;
			size -= take;
			if (block_.size() == kBlockSize && !flush_block(false)) {
				return false;
			}
		}
		return true;
	}

public:
	explicit PngStreamWriter(bool sixteen_bit) noexcept : sixteen_bit_(sixteen_bit) {
		block_.reserve(kBlockSize);
	}

protected:
	bool write_header() override {
		using namespace ImageStreamDetail;
		static constexpr uint8_t signature[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
		if (!write_bytes(signature, sizeof(signature))) {
			return false;
		}
		std::vector<uint8_t> ihdr;
		append_be<uint32_t>(ihdr, width_);
		append_be<uint32_t>(ihdr, height_);
		ihdr.push_back(sixteen_bit_ ? 16 : 8);
		ihdr.push_back(6);
		ihdr.push_back(0);
		ihdr.push_back(0);
		ihdr.push_back(0);
		if (!write_chunk("IHDR", ihdr)) {
			return false;
		}
		if (!comment_.empty()) {
			std::vector<uint8_t> text;
			append_text(text, "Comment");
			text.push_back(0);
			append_text(text, comment_);
			return write_chunk("tEXt", text);
		}
		return true;
	}

	bool emit_rows(std::span<const Render::GpuPixelOutput> pixels, uint32_t, uint32_t row_count) override {
		const size_t bytes_per_pixel = sixteen_bit_ ? 8U : 4U;
		row_.assign(1U + static_cast<size_t>(width_) * bytes_per_pixel, 0);
		for (uint32_t r = 0; r < row_count; ++r) {
			size_t offset = 1;
			for (uint32_t x = 0; x < width_; ++x) {
				const auto& px = pixels[static_cast<size_t>(r) * width_ + x];
				if (sixteen_bit_) {
					const std::array<uint16_t, 4> samples{
						ImageStreamDetail::quantize16(px.r), ImageStreamDetail::quantize16(px.g),
						ImageStreamDetail::quantize16(px.b), ImageStreamDetail::quantize16(px.a)
					};
					for (const uint16_t sample : samples) {
						row_[offset++] = static_cast<uint8_t>(sample >> 8);
						row_[offset++] = static_cast<uint8_t>(sample & 0xFFU);
					}
				} else {
					row_[offset++] = ImageStreamDetail::quantize8(px.r);
					row_[offset++] = ImageStreamDetail::quantize8(px.g);
					row_[offset++] = ImageStreamDetail::quantize8(px.b);
					row_[offset++] = ImageStreamDetail::quantize8(px.a);
				}
			}
			if (!append_raw(row_.data(), row_.size())) {
				return false;
			}
		}
		return true;
	}

	bool write_trailer() override {
		if (!flush_block(true)) {
			return false;
		}
		return write_chunk("IEND", {});
	}
};

class HdrStreamWriter final : public ImageStreamWriter {
protected:
	bool write_header() override {
		std::string header = "#?RADIANCE\n";
		if (!comment_.empty()) {
			header += "# " + comment_ + "\n";
		}
		header += "FORMAT=32-bit_rle_rgbe\n\n-Y " + std::to_string(height_) + " +X " + std::to_string(width_) + "\n";
		return write_bytes(header.data(), header.size());
	}

	bool emit_rows(std::span<const Render::GpuPixelOutput> pixels, uint32_t, uint32_t) override {
		scratch_.resize(pixels.size() * 4U);
		size_t offset = 0;
		for (const auto& px : pixels) {
			const float r = std::max(px.r, 0.0f);
			const float g = std::max(px.g, 0.0f);
			const float b = std::max(px.b, 0.0f);
			const float peak = std::max({r, g, b});
			if (peak < 1e-32f) {
				scratch_[offset++] = 0;
				scratch_[offset++] = 0;
				scratch_[offset++] = 0;
				scratch_[offset++] = 0;
				continue;
			}
			int exponent = 0;
			const float mantissa = std::frexp(peak, &exponent);
			const float scale = mantissa * 256.0f / peak;
			scratch_[offset++] = static_cast<uint8_t>(std::clamp(r * scale, 0.0f, 255.0f));
			scratch_[offset++] = static_cast<uint8_t>(std::clamp(g * scale, 0.0f, 255.0f));
			scratch_[offset++] = static_cast<uint8_t>(std::clamp(b * scale, 0.0f, 255.0f));
			scratch_[offset++] = static_cast<uint8_t>(std::clamp(exponent + 128, 0, 255));
		}
		return write_buffer(scratch_);
	}
};

class QoiStreamWriter final : public ImageStreamWriter {
private:
	struct Pixel {
		uint8_t r{0};
		uint8_t g{0};
		uint8_t b{0};
		uint8_t a{255};

		[[nodiscard]] bool operator==(const Pixel&) const noexcept = default;
	};

	std::array<Pixel, 64> index_{};
	Pixel previous_{};
	uint32_t run_{0};

	void encode(const Pixel& p) {
		if (p == previous_) {
			if (++run_ == 62U) {
				scratch_.push_back(static_cast<uint8_t>(0xC0U | 61U));
				run_ = 0;
			}
			return;
		}
		if (run_ > 0U) {
			scratch_.push_back(static_cast<uint8_t>(0xC0U | (run_ - 1U)));
			run_ = 0;
		}
		const size_t slot = (static_cast<size_t>(p.r) * 3U + static_cast<size_t>(p.g) * 5U + static_cast<size_t>(p.b) * 7U + static_cast<size_t>(p.a) * 11U) % 64U;
		if (index_[slot] == p) {
			scratch_.push_back(static_cast<uint8_t>(slot));
		} else {
			index_[slot] = p;
			if (p.a == previous_.a) {
				const int vr = static_cast<int8_t>(static_cast<uint8_t>(p.r - previous_.r));
				const int vg = static_cast<int8_t>(static_cast<uint8_t>(p.g - previous_.g));
				const int vb = static_cast<int8_t>(static_cast<uint8_t>(p.b - previous_.b));
				const int vg_r = vr - vg;
				const int vg_b = vb - vg;
				if (vr > -3 && vr < 2 && vg > -3 && vg < 2 && vb > -3 && vb < 2) {
					scratch_.push_back(static_cast<uint8_t>(0x40 | ((vr + 2) << 4) | ((vg + 2) << 2) | (vb + 2)));
				} else if (vg_r > -9 && vg_r < 8 && vg > -33 && vg < 32 && vg_b > -9 && vg_b < 8) {
					scratch_.push_back(static_cast<uint8_t>(0x80 | (vg + 32)));
					scratch_.push_back(static_cast<uint8_t>(((vg_r + 8) << 4) | (vg_b + 8)));
				} else {
					scratch_.push_back(0xFE);
					scratch_.push_back(p.r);
					scratch_.push_back(p.g);
					scratch_.push_back(p.b);
				}
			} else {
				scratch_.push_back(0xFF);
				scratch_.push_back(p.r);
				scratch_.push_back(p.g);
				scratch_.push_back(p.b);
				scratch_.push_back(p.a);
			}
		}
		previous_ = p;
	}

protected:
	bool write_header() override {
		using namespace ImageStreamDetail;
		std::vector<uint8_t> header;
		append_text(header, "qoif");
		append_be<uint32_t>(header, width_);
		append_be<uint32_t>(header, height_);
		header.push_back(4);
		header.push_back(0);
		return write_buffer(header);
	}

	bool emit_rows(std::span<const Render::GpuPixelOutput> pixels, uint32_t, uint32_t) override {
		scratch_.clear();
		scratch_.reserve(pixels.size() * 2U);
		for (const auto& px : pixels) {
			encode(Pixel{
				ImageStreamDetail::quantize8(px.r), ImageStreamDetail::quantize8(px.g),
				ImageStreamDetail::quantize8(px.b), ImageStreamDetail::quantize8(px.a)
			});
		}
		return write_buffer(scratch_);
	}

	bool write_trailer() override {
		scratch_.clear();
		if (run_ > 0U) {
			scratch_.push_back(static_cast<uint8_t>(0xC0U | (run_ - 1U)));
			run_ = 0;
		}
		scratch_.insert(scratch_.end(), {0, 0, 0, 0, 0, 0, 0, 1});
		return write_buffer(scratch_);
	}
};

class PfmStreamWriter final : public ImageStreamWriter {
private:
	std::streamoff header_size_{0};

protected:
	bool write_header() override {
		const std::string header = "PF\n" + std::to_string(width_) + " " + std::to_string(height_) + "\n-1.0\n";
		header_size_ = static_cast<std::streamoff>(header.size());
		return write_bytes(header.data(), header.size());
	}

	bool emit_rows(std::span<const Render::GpuPixelOutput> pixels, uint32_t first_row, uint32_t row_count) override {
		const size_t row_bytes = static_cast<size_t>(width_) * 12U;
		for (uint32_t r = 0; r < row_count; ++r) {
			scratch_.clear();
			scratch_.reserve(row_bytes);
			for (uint32_t x = 0; x < width_; ++x) {
				const auto& px = pixels[static_cast<size_t>(r) * width_ + x];
				ImageStreamDetail::append_le<uint32_t>(scratch_, std::bit_cast<uint32_t>(px.r));
				ImageStreamDetail::append_le<uint32_t>(scratch_, std::bit_cast<uint32_t>(px.g));
				ImageStreamDetail::append_le<uint32_t>(scratch_, std::bit_cast<uint32_t>(px.b));
			}
			const uint64_t file_row = static_cast<uint64_t>(height_) - 1ULL - (static_cast<uint64_t>(first_row) + r);
			out_.seekp(header_size_ + static_cast<std::streamoff>(file_row * row_bytes));
			if (!write_buffer(scratch_)) {
				return false;
			}
		}
		return true;
	}
};

class TiffStreamWriter final : public ImageStreamWriter {
protected:
	bool write_header() override {
		using namespace ImageStreamDetail;
		const uint64_t row_bytes = static_cast<uint64_t>(width_) * 3ULL;
		const uint32_t rows_per_strip = static_cast<uint32_t>(std::clamp<uint64_t>(1048576ULL / std::max<uint64_t>(row_bytes, 1ULL), 1ULL, height_));
		const uint32_t strips = (height_ + rows_per_strip - 1U) / rows_per_strip;
		const bool has_comment = !comment_.empty();
		const uint32_t entry_count = has_comment ? 13U : 12U;
		const uint32_t ifd_size = 2U + entry_count * 12U + 4U;
		uint32_t cursor = 8U + ifd_size;
		const uint32_t bits_offset = cursor;
		cursor += 6U;
		const uint32_t x_resolution_offset = cursor;
		cursor += 8U;
		const uint32_t y_resolution_offset = cursor;
		cursor += 8U;
		const uint32_t description_offset = cursor;
		const uint32_t description_size = has_comment ? static_cast<uint32_t>((comment_.size() + 2U) & ~size_t{1}) : 0U;
		cursor += description_size;
		const uint32_t offsets_array = cursor;
		if (strips > 1U) {
			cursor += 4U * strips;
		}
		const uint32_t counts_array = cursor;
		if (strips > 1U) {
			cursor += 4U * strips;
		}
		const uint32_t pixel_offset = cursor;
		const uint32_t first_strip_bytes = static_cast<uint32_t>(std::min<uint64_t>(rows_per_strip, height_) * row_bytes);

		std::vector<uint8_t> header;
		header.push_back('I');
		header.push_back('I');
		append_le<uint16_t>(header, 42U);
		append_le<uint32_t>(header, 8U);
		append_le<uint16_t>(header, static_cast<uint16_t>(entry_count));
		const auto entry = [&header](uint16_t tag, uint16_t type, uint32_t count, uint32_t value) {
			append_le<uint16_t>(header, tag);
			append_le<uint16_t>(header, type);
			append_le<uint32_t>(header, count);
			append_le<uint32_t>(header, value);
		};
		entry(256, 4, 1, width_);
		entry(257, 4, 1, height_);
		entry(258, 3, 3, bits_offset);
		entry(259, 3, 1, 1);
		entry(262, 3, 1, 2);
		if (has_comment) {
			entry(270, 2, static_cast<uint32_t>(comment_.size() + 1U), description_offset);
		}
		entry(273, 4, strips, strips > 1U ? offsets_array : pixel_offset);
		entry(277, 3, 1, 3);
		entry(278, 4, 1, rows_per_strip);
		entry(279, 4, strips, strips > 1U ? counts_array : first_strip_bytes);
		entry(282, 5, 1, x_resolution_offset);
		entry(283, 5, 1, y_resolution_offset);
		entry(296, 3, 1, 2);
		append_le<uint32_t>(header, 0U);
		append_le<uint16_t>(header, 8U);
		append_le<uint16_t>(header, 8U);
		append_le<uint16_t>(header, 8U);
		append_le<uint32_t>(header, 72U);
		append_le<uint32_t>(header, 1U);
		append_le<uint32_t>(header, 72U);
		append_le<uint32_t>(header, 1U);
		if (has_comment) {
			append_text(header, comment_);
			header.resize(header.size() + (description_size - comment_.size()), 0);
		}
		if (strips > 1U) {
			for (uint32_t s = 0; s < strips; ++s) {
				append_le<uint32_t>(header, static_cast<uint32_t>(pixel_offset + static_cast<uint64_t>(s) * rows_per_strip * row_bytes));
			}
			for (uint32_t s = 0; s < strips; ++s) {
				const uint32_t rows = std::min(rows_per_strip, height_ - s * rows_per_strip);
				append_le<uint32_t>(header, static_cast<uint32_t>(static_cast<uint64_t>(rows) * row_bytes));
			}
		}
		return write_buffer(header);
	}

	bool emit_rows(std::span<const Render::GpuPixelOutput> pixels, uint32_t, uint32_t) override {
		scratch_.resize(pixels.size() * 3U);
		size_t offset = 0;
		for (const auto& px : pixels) {
			scratch_[offset++] = ImageStreamDetail::quantize8(px.r);
			scratch_[offset++] = ImageStreamDetail::quantize8(px.g);
			scratch_[offset++] = ImageStreamDetail::quantize8(px.b);
		}
		return write_buffer(scratch_);
	}
};

[[nodiscard]] inline std::unique_ptr<ImageStreamWriter> make_image_stream_writer(ScreenshotFormat format) {
	switch (format) {
		case ScreenshotFormat::BMP: return std::make_unique<BmpStreamWriter>();
		case ScreenshotFormat::PNG: return std::make_unique<PngStreamWriter>(false);
		case ScreenshotFormat::TGA: return std::make_unique<TgaStreamWriter>();
		case ScreenshotFormat::HDR: return std::make_unique<HdrStreamWriter>();
		case ScreenshotFormat::PNG16: return std::make_unique<PngStreamWriter>(true);
		case ScreenshotFormat::QOI: return std::make_unique<QoiStreamWriter>();
		case ScreenshotFormat::PFM: return std::make_unique<PfmStreamWriter>();
		case ScreenshotFormat::TIFF: return std::make_unique<TiffStreamWriter>();
		case ScreenshotFormat::PAM: return std::make_unique<PamStreamWriter>();
		case ScreenshotFormat::PPM:
		default: return std::make_unique<PpmStreamWriter>();
	}
}

[[nodiscard]] inline bool write_image_file(
	const std::filesystem::path& path,
	std::span<const Render::GpuPixelOutput> pixels,
	uint32_t width,
	uint32_t height,
	ScreenshotFormat format,
	const std::string& comment
) {
	auto writer = make_image_stream_writer(format);
	if (!writer->begin(path, width, height, comment)) {
		return false;
	}
	if (!writer->write_rows(pixels)) {
		writer->abort();
		return false;
	}
	return writer->finish();
}

}
