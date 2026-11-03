#include "relativistic/distributed/compression_codecs.hpp"
#include "relativistic/core/simd.hpp"
#include <lz4.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace Relativistic::Distributed {

namespace {

constexpr size_t kQoiWorstCaseBytesPerPixel = 25;
constexpr size_t kQoiStreamSlackBytes = 16;
constexpr size_t kQoiShortRunLimit = 64;
constexpr size_t kQoiLongRunBias = 65;
constexpr size_t kQoiLongRunLimit = kQoiLongRunBias + 16383;
constexpr size_t kQoiTableSize = 64;

constexpr uint32_t kTagMask = 0xC0U;
constexpr uint32_t kTagIndex = 0x00U;
constexpr uint32_t kTagRun = 0x40U;
constexpr uint32_t kTagPartial = 0x80U;
constexpr uint32_t kTagLongRun = 0xC0U;
constexpr uint32_t kPayloadMask = 0x3FU;

constexpr uint32_t kGroupRgb = 1U << 0U;
constexpr uint32_t kGroupAlpha = 1U << 1U;
constexpr uint32_t kGroupRedshift = 1U << 2U;
constexpr uint32_t kGroupAffine = 1U << 3U;
constexpr uint32_t kGroupStatus = 1U << 4U;
constexpr uint32_t kGroupIterations = 1U << 5U;

constexpr size_t kSanitizeWidth = 8;

constexpr uint32_t kKnownStatusMask =
	Render::PixelFlags::HORIZON_ABSORBED
	| Render::PixelFlags::CELESTIAL_HIT
	| Render::PixelFlags::ACCRETION_DISK_HIT
	| Render::PixelFlags::PHOTON_SPHERE_PROXIMITY
	| Render::PixelFlags::BODY_SURFACE_HIT
	| Render::PixelFlags::JET_EMISSION_HIT;

struct QuantizedPixel {
	uint16_t red{0};
	uint16_t green{0};
	uint16_t blue{0};
	uint16_t alpha{0};
	uint32_t redshift_bits{0};
	uint32_t affine_bits{0};
	uint32_t status{0};
	uint32_t iterations{0};

	[[nodiscard]] constexpr bool operator==(const QuantizedPixel&) const noexcept = default;
};

[[nodiscard]] constexpr QuantizedPixel initial_pixel() noexcept {
	return QuantizedPixel{0, 0, 0, HalfFloat::to_bits(1.0f), std::bit_cast<uint32_t>(1.0f), 0U, 0U, 0U};
}

[[nodiscard]] QuantizedPixel quantize(const Render::GpuPixelOutput& pixel) noexcept {
	return QuantizedPixel{
		HalfFloat::to_bits(pixel.r),
		HalfFloat::to_bits(pixel.g),
		HalfFloat::to_bits(pixel.b),
		HalfFloat::to_bits(pixel.a),
		std::bit_cast<uint32_t>(pixel.redshift),
		std::bit_cast<uint32_t>(pixel.affine_parameter),
		pixel.status_flags,
		pixel.iterations_used
	};
}

[[nodiscard]] Render::GpuPixelOutput dequantize(const QuantizedPixel& quantized) noexcept {
	Render::GpuPixelOutput pixel{};
	pixel.r = HalfFloat::from_bits(quantized.red);
	pixel.g = HalfFloat::from_bits(quantized.green);
	pixel.b = HalfFloat::from_bits(quantized.blue);
	pixel.a = HalfFloat::from_bits(quantized.alpha);
	pixel.redshift = std::bit_cast<float>(quantized.redshift_bits);
	pixel.affine_parameter = std::bit_cast<float>(quantized.affine_bits);
	pixel.status_flags = quantized.status;
	pixel.iterations_used = quantized.iterations;
	return pixel;
}

[[nodiscard]] constexpr size_t hash_pixel(const QuantizedPixel& pixel) noexcept {
	uint32_t hash = static_cast<uint32_t>(pixel.red) * 3U
		+ static_cast<uint32_t>(pixel.green) * 5U
		+ static_cast<uint32_t>(pixel.blue) * 7U
		+ static_cast<uint32_t>(pixel.alpha) * 11U
		+ pixel.redshift_bits * 13U
		+ pixel.affine_bits * 17U
		+ pixel.status * 19U
		+ pixel.iterations * 23U;
	hash ^= hash >> 15U;
	hash *= 0x2C1B3C6DU;
	hash ^= hash >> 12U;
	return static_cast<size_t>(hash & (kQoiTableSize - 1U));
}

[[nodiscard]] constexpr uint32_t changed_groups(const QuantizedPixel& current, const QuantizedPixel& previous) noexcept {
	uint32_t mask = 0U;
	if (current.red != previous.red || current.green != previous.green || current.blue != previous.blue) {
		mask |= kGroupRgb;
	}
	if (current.alpha != previous.alpha) {
		mask |= kGroupAlpha;
	}
	if (current.redshift_bits != previous.redshift_bits) {
		mask |= kGroupRedshift;
	}
	if (current.affine_bits != previous.affine_bits) {
		mask |= kGroupAffine;
	}
	if (current.status != previous.status) {
		mask |= kGroupStatus;
	}
	if (current.iterations != previous.iterations) {
		mask |= kGroupIterations;
	}
	return mask;
}

class ByteWriter {
private:
	uint8_t* data_;
	size_t capacity_;
	size_t size_{0};
	bool overflow_{false};

public:
	ByteWriter(uint8_t* data, size_t capacity) noexcept : data_(data), capacity_(capacity) {}

	void put_u8(uint32_t value) noexcept {
		if (size_ >= capacity_) {
			overflow_ = true;
			return;
		}
		data_[size_++] = static_cast<uint8_t>(value & 0xFFU);
	}

	void put_u16(uint32_t value) noexcept {
		put_u8(value);
		put_u8(value >> 8U);
	}

	void put_u32(uint32_t value) noexcept {
		put_u8(value);
		put_u8(value >> 8U);
		put_u8(value >> 16U);
		put_u8(value >> 24U);
	}

	[[nodiscard]] size_t size() const noexcept { return size_; }
	[[nodiscard]] bool overflowed() const noexcept { return overflow_; }
};

class ByteReader {
private:
	const uint8_t* data_;
	size_t size_;
	size_t position_{0};

public:
	ByteReader(const uint8_t* data, size_t size) noexcept : data_(data), size_(size) {}

	[[nodiscard]] bool read_u8(uint32_t& value) noexcept {
		if (position_ >= size_) {
			return false;
		}
		value = data_[position_++];
		return true;
	}

	[[nodiscard]] bool read_u16(uint32_t& value) noexcept {
		uint32_t low = 0;
		uint32_t high = 0;
		if (!read_u8(low) || !read_u8(high)) {
			return false;
		}
		value = low | (high << 8U);
		return true;
	}

	[[nodiscard]] bool read_u32(uint32_t& value) noexcept {
		uint32_t low = 0;
		uint32_t high = 0;
		if (!read_u16(low) || !read_u16(high)) {
			return false;
		}
		value = low | (high << 16U);
		return true;
	}

	[[nodiscard]] bool exhausted() const noexcept { return position_ == size_; }
};

void shuffle_bytes(const uint8_t* source, uint8_t* destination, size_t pixel_count) noexcept {
	for (size_t lane = 0; lane < kPixelStrideBytes; ++lane) {
		uint8_t* plane = destination + lane * pixel_count;
		const uint8_t* column = source + lane;
		for (size_t index = 0; index < pixel_count; ++index) {
			plane[index] = column[index * kPixelStrideBytes];
		}
	}
}

void unshuffle_bytes(const uint8_t* source, uint8_t* destination, size_t pixel_count) noexcept {
	for (size_t lane = 0; lane < kPixelStrideBytes; ++lane) {
		const uint8_t* plane = source + lane * pixel_count;
		uint8_t* column = destination + lane;
		for (size_t index = 0; index < pixel_count; ++index) {
			column[index * kPixelStrideBytes] = plane[index];
		}
	}
}

[[nodiscard]] CodecStatus encode_qoi_hdr_rle(std::span<const Render::GpuPixelOutput> pixels, std::span<uint8_t> output, size_t& written) noexcept {
	ByteWriter writer(output.data(), output.size());
	std::array<QuantizedPixel, kQoiTableSize> table{};
	QuantizedPixel previous = initial_pixel();
	size_t run = 0;

	const auto flush_run = [&]() noexcept {
		if (run == 0U) {
			return;
		}
		if (run <= kQoiShortRunLimit) {
			writer.put_u8(kTagRun | static_cast<uint32_t>(run - 1U));
		} else {
			const uint32_t biased = static_cast<uint32_t>(run - kQoiLongRunBias);
			writer.put_u8(kTagLongRun | (biased >> 8U));
			writer.put_u8(biased & 0xFFU);
		}
		run = 0;
	};

	for (const Render::GpuPixelOutput& pixel : pixels) {
		const QuantizedPixel current = quantize(pixel);
		if (current == previous) {
			++run;
			if (run == kQoiLongRunLimit) {
				flush_run();
			}
			continue;
		}
		flush_run();
		const size_t slot = hash_pixel(current);
		if (table[slot] == current) {
			writer.put_u8(kTagIndex | static_cast<uint32_t>(slot));
		} else {
			const uint32_t mask = changed_groups(current, previous);
			writer.put_u8(kTagPartial | mask);
			if ((mask & kGroupRgb) != 0U) {
				writer.put_u16(current.red);
				writer.put_u16(current.green);
				writer.put_u16(current.blue);
			}
			if ((mask & kGroupAlpha) != 0U) {
				writer.put_u16(current.alpha);
			}
			if ((mask & kGroupRedshift) != 0U) {
				writer.put_u32(current.redshift_bits);
			}
			if ((mask & kGroupAffine) != 0U) {
				writer.put_u32(current.affine_bits);
			}
			if ((mask & kGroupStatus) != 0U) {
				writer.put_u32(current.status);
			}
			if ((mask & kGroupIterations) != 0U) {
				writer.put_u32(current.iterations);
			}
			table[slot] = current;
		}
		previous = current;
	}
	flush_run();

	if (writer.overflowed()) {
		return CodecStatus::OutputTooSmall;
	}
	written = writer.size();
	return CodecStatus::Ok;
}

[[nodiscard]] CodecStatus decode_qoi_hdr_rle(std::span<const uint8_t> input, std::span<Render::GpuPixelOutput> pixels) noexcept {
	ByteReader reader(input.data(), input.size());
	std::array<QuantizedPixel, kQoiTableSize> table{};
	QuantizedPixel previous = initial_pixel();
	size_t produced = 0;

	while (produced < pixels.size()) {
		uint32_t tag = 0;
		if (!reader.read_u8(tag)) {
			return CodecStatus::CorruptStream;
		}
		const uint32_t kind = tag & kTagMask;
		const uint32_t payload = tag & kPayloadMask;

		if (kind == kTagRun || kind == kTagLongRun) {
			size_t length = 0;
			if (kind == kTagRun) {
				length = static_cast<size_t>(payload) + 1U;
			} else {
				uint32_t low = 0;
				if (!reader.read_u8(low)) {
					return CodecStatus::CorruptStream;
				}
				length = static_cast<size_t>((payload << 8U) | low) + kQoiLongRunBias;
			}
			if (length > pixels.size() - produced) {
				return CodecStatus::CorruptStream;
			}
			const Render::GpuPixelOutput value = dequantize(previous);
			for (size_t index = 0; index < length; ++index) {
				pixels[produced++] = value;
			}
			continue;
		}

		QuantizedPixel current = previous;
		if (kind == kTagIndex) {
			current = table[payload];
		} else {
			if (payload == 0U) {
				return CodecStatus::CorruptStream;
			}
			uint32_t value = 0;
			if ((payload & kGroupRgb) != 0U) {
				if (!reader.read_u16(value)) { return CodecStatus::CorruptStream; }
				current.red = static_cast<uint16_t>(value);
				if (!reader.read_u16(value)) { return CodecStatus::CorruptStream; }
				current.green = static_cast<uint16_t>(value);
				if (!reader.read_u16(value)) { return CodecStatus::CorruptStream; }
				current.blue = static_cast<uint16_t>(value);
			}
			if ((payload & kGroupAlpha) != 0U) {
				if (!reader.read_u16(value)) { return CodecStatus::CorruptStream; }
				current.alpha = static_cast<uint16_t>(value);
			}
			if ((payload & kGroupRedshift) != 0U) {
				if (!reader.read_u32(current.redshift_bits)) { return CodecStatus::CorruptStream; }
			}
			if ((payload & kGroupAffine) != 0U) {
				if (!reader.read_u32(current.affine_bits)) { return CodecStatus::CorruptStream; }
			}
			if ((payload & kGroupStatus) != 0U) {
				if (!reader.read_u32(current.status)) { return CodecStatus::CorruptStream; }
			}
			if ((payload & kGroupIterations) != 0U) {
				if (!reader.read_u32(current.iterations)) { return CodecStatus::CorruptStream; }
			}
			table[hash_pixel(current)] = current;
		}
		pixels[produced++] = dequantize(current);
		previous = current;
	}

	return reader.exhausted() ? CodecStatus::Ok : CodecStatus::CorruptStream;
}

[[nodiscard]] CodecStatus encode_lz4_block(std::span<const Render::GpuPixelOutput> pixels, std::span<uint8_t> output, TileCodecScratch& scratch, int acceleration, size_t& written) noexcept {
	const size_t raw_bytes = pixels.size() * kPixelStrideBytes;
	shuffle_bytes(reinterpret_cast<const uint8_t*>(pixels.data()), scratch.shuffled.data(), pixels.size());
	const int produced = LZ4_compress_fast(
		reinterpret_cast<const char*>(scratch.shuffled.data()),
		reinterpret_cast<char*>(output.data()),
		static_cast<int>(raw_bytes),
		static_cast<int>(std::min<size_t>(output.size(), static_cast<size_t>(std::numeric_limits<int>::max()))),
		std::max(acceleration, 1)
	);
	if (produced <= 0) {
		return CodecStatus::OutputTooSmall;
	}
	written = static_cast<size_t>(produced);
	return CodecStatus::Ok;
}

[[nodiscard]] CodecStatus decode_lz4_block(std::span<const uint8_t> input, std::span<Render::GpuPixelOutput> pixels, TileCodecScratch& scratch) noexcept {
	const size_t raw_bytes = pixels.size() * kPixelStrideBytes;
	if (input.empty() || input.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
		return CodecStatus::CorruptStream;
	}
	const int produced = LZ4_decompress_safe(
		reinterpret_cast<const char*>(input.data()),
		reinterpret_cast<char*>(scratch.shuffled.data()),
		static_cast<int>(input.size()),
		static_cast<int>(raw_bytes)
	);
	if (produced < 0) {
		return CodecStatus::CorruptStream;
	}
	if (static_cast<size_t>(produced) != raw_bytes) {
		return CodecStatus::SizeMismatch;
	}
	unshuffle_bytes(scratch.shuffled.data(), reinterpret_cast<uint8_t*>(pixels.data()), pixels.size());
	return CodecStatus::Ok;
}

}

size_t TileCodec::max_compressed_size(TileCompressionMode mode, size_t pixel_count) noexcept {
	const size_t raw_bytes = pixel_count * kPixelStrideBytes;
	switch (mode) {
		case TileCompressionMode::QoiHdrRle:
			return std::max(raw_bytes, pixel_count * kQoiWorstCaseBytesPerPixel + kQoiStreamSlackBytes);
		case TileCompressionMode::Lz4Block: {
			const int bound = LZ4_compressBound(static_cast<int>(raw_bytes));
			return std::max(raw_bytes, (bound > 0) ? static_cast<size_t>(bound) : raw_bytes);
		}
		case TileCompressionMode::DirectRaw:
		default:
			return raw_bytes;
	}
}

CompressionResult TileCodec::compress(
	TileCompressionMode mode,
	std::span<const Render::GpuPixelOutput> pixels,
	std::span<uint8_t> output,
	TileCodecScratch& scratch,
	const CompressionParameters& parameters
) noexcept {
	const size_t count = pixels.size();
	if (count == 0U || count > kTilePixelCapacity) {
		return CompressionResult{CodecStatus::InvalidArgument, TileCompressionMode::DirectRaw, 0};
	}
	if (!is_valid_compression_mode(static_cast<uint8_t>(mode))) {
		return CompressionResult{CodecStatus::UnsupportedMode, TileCompressionMode::DirectRaw, 0};
	}
	const size_t raw_bytes = count * kPixelStrideBytes;

	if (mode != TileCompressionMode::DirectRaw) {
		size_t written = 0;
		const CodecStatus status = (mode == TileCompressionMode::QoiHdrRle)
			? encode_qoi_hdr_rle(pixels, output, written)
			: encode_lz4_block(pixels, output, scratch, parameters.lz4_acceleration, written);
		if (!parameters.allow_raw_fallback) {
			return CompressionResult{status, mode, (status == CodecStatus::Ok) ? written : 0U};
		}
		if (status == CodecStatus::Ok && written < raw_bytes) {
			return CompressionResult{CodecStatus::Ok, mode, written};
		}
	}

	if (output.size() < raw_bytes) {
		return CompressionResult{CodecStatus::OutputTooSmall, TileCompressionMode::DirectRaw, 0};
	}
	std::memcpy(output.data(), pixels.data(), raw_bytes);
	return CompressionResult{CodecStatus::Ok, TileCompressionMode::DirectRaw, raw_bytes};
}

CodecStatus TileCodec::decompress(
	TileCompressionMode mode,
	std::span<const uint8_t> input,
	std::span<Render::GpuPixelOutput> pixels,
	TileCodecScratch& scratch
) noexcept {
	if (pixels.empty() || pixels.size() > kTilePixelCapacity) {
		return CodecStatus::InvalidArgument;
	}
	switch (mode) {
		case TileCompressionMode::DirectRaw: {
			const size_t raw_bytes = pixels.size() * kPixelStrideBytes;
			if (input.size() != raw_bytes) {
				return CodecStatus::SizeMismatch;
			}
			std::memcpy(pixels.data(), input.data(), raw_bytes);
			return CodecStatus::Ok;
		}
		case TileCompressionMode::QoiHdrRle:
			return decode_qoi_hdr_rle(input, pixels);
		case TileCompressionMode::Lz4Block:
			return decode_lz4_block(input, pixels, scratch);
	}
	return CodecStatus::UnsupportedMode;
}

TileCodecScratch& TileCodec::thread_scratch() noexcept {
	thread_local TileCodecScratch scratch;
	return scratch;
}

SanitizationReport TileSanitizer::sanitize(std::span<Render::GpuPixelOutput> pixels) noexcept {
	using Lane = Core::SimdVec<float, kSanitizeWidth>;

	SanitizationReport report{};
	report.pixels_inspected = static_cast<uint32_t>(pixels.size());

	const Lane zero(0.0f);
	const Lane radiance_limit(kMaximumRadiance);
	const Lane finite_limit(std::numeric_limits<float>::max());

	for (size_t base = 0; base < pixels.size(); base += kSanitizeWidth) {
		const size_t lanes = std::min(kSanitizeWidth, pixels.size() - base);
		std::array<float, kSanitizeWidth> red{};
		std::array<float, kSanitizeWidth> green{};
		std::array<float, kSanitizeWidth> blue{};
		for (size_t lane = 0; lane < lanes; ++lane) {
			red[lane] = pixels[base + lane].r;
			green[lane] = pixels[base + lane].g;
			blue[lane] = pixels[base + lane].b;
		}

		const Lane r(red);
		const Lane g(green);
		const Lane b(blue);

		const auto finite = (r == r) && (Core::abs(r) <= finite_limit)
			&& (g == g) && (Core::abs(g) <= finite_limit)
			&& (b == b) && (Core::abs(b) <= finite_limit);
		const auto invalid = (!finite) || (r < zero) || (g < zero) || (b < zero);

		const Lane clamped_r = Core::min(r, radiance_limit);
		const Lane clamped_g = Core::min(g, radiance_limit);
		const Lane clamped_b = Core::min(b, radiance_limit);

		for (size_t lane = 0; lane < lanes; ++lane) {
			Render::GpuPixelOutput& pixel = pixels[base + lane];
			if (invalid[lane]) {
				pixel.r = 0.0f;
				pixel.g = 0.0f;
				pixel.b = 0.0f;
				pixel.a = 1.0f;
				pixel.status_flags = Render::PixelFlags::CELESTIAL_HIT;
				++report.pixels_neutralized;
			} else {
				const bool changed = (clamped_r[lane] != pixel.r) || (clamped_g[lane] != pixel.g) || (clamped_b[lane] != pixel.b);
				pixel.r = clamped_r[lane];
				pixel.g = clamped_g[lane];
				pixel.b = clamped_b[lane];
				if (changed) {
					++report.pixels_clamped;
				}
				if (!std::isfinite(pixel.a)) {
					pixel.a = 1.0f;
					++report.metadata_repaired;
				} else {
					pixel.a = std::clamp(pixel.a, 0.0f, 1.0f);
				}
				pixel.status_flags &= kKnownStatusMask;
			}
			if (!std::isfinite(pixel.redshift)) {
				pixel.redshift = 1.0f;
				++report.metadata_repaired;
			}
			if (!std::isfinite(pixel.affine_parameter)) {
				pixel.affine_parameter = 0.0f;
				++report.metadata_repaired;
			}
		}
	}

	return report;
}

}
