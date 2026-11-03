#pragma once

#include "relativistic/render/gpu_types.hpp"
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

namespace Relativistic::Distributed {

static_assert(std::endian::native == std::endian::little, "The cluster wire format requires a little-endian host");
static_assert(sizeof(Render::GpuPixelOutput) == 32, "GpuPixelOutput must remain a 32-byte record");
static_assert(std::is_trivially_copyable_v<Render::GpuPixelOutput>, "GpuPixelOutput must remain trivially copyable");

inline constexpr size_t kPixelStrideBytes = sizeof(Render::GpuPixelOutput);
inline constexpr size_t kTilePixelCapacity = 1024;
inline constexpr size_t kTileRawCapacityBytes = kTilePixelCapacity * kPixelStrideBytes;

enum class TileCompressionMode : uint8_t {
	DirectRaw = 0,
	QoiHdrRle = 1,
	Lz4Block = 2
};

[[nodiscard]] constexpr bool is_valid_compression_mode(uint8_t raw_value) noexcept {
	return raw_value <= static_cast<uint8_t>(TileCompressionMode::Lz4Block);
}

[[nodiscard]] constexpr const char* compression_mode_name(TileCompressionMode mode) noexcept {
	switch (mode) {
		case TileCompressionMode::DirectRaw: return "DirectRaw";
		case TileCompressionMode::QoiHdrRle: return "QoiHdrRle";
		case TileCompressionMode::Lz4Block: return "Lz4Block";
	}
	return "Unknown";
}

enum class CodecStatus : uint32_t {
	Ok = 0,
	InvalidArgument = 1,
	OutputTooSmall = 2,
	CorruptStream = 3,
	SizeMismatch = 4,
	UnsupportedMode = 5
};

[[nodiscard]] constexpr const char* codec_status_name(CodecStatus status) noexcept {
	switch (status) {
		case CodecStatus::Ok: return "Ok";
		case CodecStatus::InvalidArgument: return "InvalidArgument";
		case CodecStatus::OutputTooSmall: return "OutputTooSmall";
		case CodecStatus::CorruptStream: return "CorruptStream";
		case CodecStatus::SizeMismatch: return "SizeMismatch";
		case CodecStatus::UnsupportedMode: return "UnsupportedMode";
	}
	return "Unknown";
}

class HalfFloat {
public:
	[[nodiscard]] static constexpr uint16_t to_bits(float value) noexcept {
		const uint32_t bits = std::bit_cast<uint32_t>(value);
		const uint32_t sign = (bits >> 16U) & 0x8000U;
		const uint32_t exponent = (bits >> 23U) & 0xFFU;
		const uint32_t mantissa = bits & 0x7FFFFFU;
		if (exponent == 0xFFU) {
			const uint32_t payload = (mantissa != 0U) ? (0x200U | (mantissa >> 13U)) : 0U;
			return static_cast<uint16_t>(sign | 0x7C00U | payload);
		}
		const int32_t rebased = static_cast<int32_t>(exponent) - 127 + 15;
		if (rebased >= 31) {
			return static_cast<uint16_t>(sign | 0x7BFFU);
		}
		if (rebased <= 0) {
			if (rebased < -10) {
				return static_cast<uint16_t>(sign);
			}
			const uint32_t significand = mantissa | 0x800000U;
			const uint32_t shift = static_cast<uint32_t>(14 - rebased);
			uint32_t subnormal = significand >> shift;
			const uint32_t remainder = significand & ((1U << shift) - 1U);
			const uint32_t halfway = 1U << (shift - 1U);
			if (remainder > halfway || (remainder == halfway && (subnormal & 1U) != 0U)) {
				++subnormal;
			}
			return static_cast<uint16_t>(sign | subnormal);
		}
		uint32_t half = (static_cast<uint32_t>(rebased) << 10U) | (mantissa >> 13U);
		const uint32_t remainder = mantissa & 0x1FFFU;
		if (remainder > 0x1000U || (remainder == 0x1000U && (half & 1U) != 0U)) {
			++half;
		}
		if (half >= 0x7C00U) {
			half = 0x7BFFU;
		}
		return static_cast<uint16_t>(sign | half);
	}

	[[nodiscard]] static constexpr float from_bits(uint16_t half) noexcept {
		const uint32_t wide = static_cast<uint32_t>(half);
		const uint32_t sign = (wide & 0x8000U) << 16U;
		const uint32_t exponent = (wide >> 10U) & 0x1FU;
		const uint32_t mantissa = wide & 0x3FFU;
		if (exponent == 0U) {
			const float magnitude = static_cast<float>(mantissa) * 5.9604644775390625e-8f;
			return std::bit_cast<float>(std::bit_cast<uint32_t>(magnitude) | sign);
		}
		if (exponent == 31U) {
			return std::bit_cast<float>(sign | 0x7F800000U | (mantissa << 13U));
		}
		return std::bit_cast<float>(sign | ((exponent + 112U) << 23U) | (mantissa << 13U));
	}
};

struct alignas(64) TileCodecScratch {
	std::array<uint8_t, kTileRawCapacityBytes> shuffled{};
};

struct CompressionParameters {
	int lz4_acceleration{1};
	bool allow_raw_fallback{true};
};

struct CompressionResult {
	CodecStatus status{CodecStatus::InvalidArgument};
	TileCompressionMode mode_used{TileCompressionMode::DirectRaw};
	size_t compressed_bytes{0};
};

class TileCodec {
public:
	[[nodiscard]] static size_t max_compressed_size(TileCompressionMode mode, size_t pixel_count) noexcept;

	[[nodiscard]] static CompressionResult compress(
		TileCompressionMode mode,
		std::span<const Render::GpuPixelOutput> pixels,
		std::span<uint8_t> output,
		TileCodecScratch& scratch,
		const CompressionParameters& parameters = {}
	) noexcept;

	[[nodiscard]] static CodecStatus decompress(
		TileCompressionMode mode,
		std::span<const uint8_t> input,
		std::span<Render::GpuPixelOutput> pixels,
		TileCodecScratch& scratch
	) noexcept;

	[[nodiscard]] static TileCodecScratch& thread_scratch() noexcept;
};

struct LinkProfile {
	double bandwidth_bytes_per_second{125.0e6};
	double round_trip_seconds{0.001};
};

class TileCompressionPolicy {
public:
	static constexpr double kRawBandwidthThreshold = 100.0e6;
	static constexpr double kQoiBandwidthThreshold = 10.0e6;

	[[nodiscard]] static constexpr TileCompressionMode select(const LinkProfile& link) noexcept {
		if (!(link.bandwidth_bytes_per_second > 0.0)) {
			return TileCompressionMode::Lz4Block;
		}
		if (link.bandwidth_bytes_per_second >= kRawBandwidthThreshold) {
			return TileCompressionMode::DirectRaw;
		}
		if (link.bandwidth_bytes_per_second >= kQoiBandwidthThreshold) {
			return TileCompressionMode::QoiHdrRle;
		}
		return TileCompressionMode::Lz4Block;
	}
};

struct SanitizationReport {
	uint32_t pixels_inspected{0};
	uint32_t pixels_neutralized{0};
	uint32_t pixels_clamped{0};
	uint32_t metadata_repaired{0};

	[[nodiscard]] constexpr bool clean() const noexcept {
		return pixels_neutralized == 0U && pixels_clamped == 0U && metadata_repaired == 0U;
	}
};

class TileSanitizer {
public:
	static constexpr float kMaximumRadiance = 1.0e8f;

	static SanitizationReport sanitize(std::span<Render::GpuPixelOutput> pixels) noexcept;
};

}
