#pragma once

#include "relativistic/distributed/compression_codecs.hpp"
#include "relativistic/distributed/packet_protocol.hpp"
#include "relativistic/render/gpu_types.hpp"
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>

namespace Relativistic::Distributed {

static_assert(std::endian::native == std::endian::little, "The cluster wire format requires a little-endian host");

inline constexpr uint32_t kTileEdgePixels = 32;
inline constexpr uint32_t kMaximumScreenWidth = 7680;
inline constexpr uint32_t kMaximumScreenHeight = 4320;

struct RenderTileTaskPayload {
	RenderTileTaskHeader header{};
	double dispatch_timestamp{0.0};
	Render::GpuCameraPushConstants camera_constants{};
};

inline constexpr size_t kRenderTileTaskPrefixBytes = sizeof(RenderTileTaskHeader) + sizeof(double);
inline constexpr size_t kRenderTileTaskPayloadBytes = kRenderTileTaskPrefixBytes + sizeof(Render::GpuCameraPushConstants);

#pragma pack(push, 1)

struct RenderTileResultPayload {
	RenderTileResultHeader header{};
	uint32_t compressed_bytes{0};
};

#pragma pack(pop)

static_assert(sizeof(RenderTileResultPayload) == 28);
static_assert(DistributedPayload<RenderTileResultPayload>);
static_assert(DistributedPayload<Render::GpuCameraPushConstants>);
static_assert(kRenderTileTaskPayloadBytes <= 65536);

struct TileGeometry {
	uint32_t origin_x{0};
	uint32_t origin_y{0};
	uint32_t width{0};
	uint32_t height{0};

	[[nodiscard]] constexpr size_t pixel_count() const noexcept {
		return static_cast<size_t>(width) * static_cast<size_t>(height);
	}
};

[[nodiscard]] constexpr uint32_t tile_grid_columns(uint32_t screen_width) noexcept {
	return (screen_width + kTileEdgePixels - 1U) / kTileEdgePixels;
}

[[nodiscard]] constexpr uint32_t tile_grid_rows(uint32_t screen_height) noexcept {
	return (screen_height + kTileEdgePixels - 1U) / kTileEdgePixels;
}

[[nodiscard]] constexpr size_t max_render_tile_result_bytes(size_t pixel_count, TileCompressionMode mode) noexcept {
	const size_t raw_bytes = pixel_count * kPixelStrideBytes;
	const size_t worst_case = (mode == TileCompressionMode::DirectRaw) ? raw_bytes : (raw_bytes + raw_bytes / 255U + 64U);
	return sizeof(RenderTileResultPayload) + worst_case;
}

[[nodiscard]] std::optional<TileGeometry> resolve_tile_geometry(uint32_t tile_index, uint32_t screen_width, uint32_t screen_height) noexcept;

[[nodiscard]] std::optional<RenderTileTaskHeader> make_render_tile_task_header(
	uint32_t tile_index,
	uint32_t screen_width,
	uint32_t screen_height,
	uint16_t lod_reduced_steps,
	uint8_t flags,
	TileCompressionMode result_mode
) noexcept;

[[nodiscard]] bool validate_render_tile_task(const RenderTileTaskPayload& payload) noexcept;

[[nodiscard]] size_t serialize_render_tile_task(const RenderTileTaskPayload& payload, std::span<uint8_t> output) noexcept;

[[nodiscard]] bool deserialize_render_tile_task(std::span<const uint8_t> input, RenderTileTaskPayload& payload) noexcept;

struct EncodedTileResult {
	CodecStatus status{CodecStatus::InvalidArgument};
	TileCompressionMode mode_used{TileCompressionMode::DirectRaw};
	size_t bytes{0};
};

[[nodiscard]] EncodedTileResult encode_render_tile_result(
	uint64_t frame_index,
	uint32_t tile_index,
	std::span<const Render::GpuPixelOutput> pixels,
	TileCompressionMode requested_mode,
	uint32_t execution_time_us,
	std::span<uint8_t> output,
	TileCodecScratch& scratch,
	const CompressionParameters& parameters = {}
) noexcept;

enum class TileDecodeStatus : uint32_t {
	Ok = 0,
	Truncated = 1,
	InvalidHeader = 2,
	SizeMismatch = 3,
	UnsupportedMode = 4,
	CorruptStream = 5,
	OutputTooSmall = 6
};

[[nodiscard]] constexpr const char* tile_decode_status_name(TileDecodeStatus status) noexcept {
	switch (status) {
		case TileDecodeStatus::Ok: return "Ok";
		case TileDecodeStatus::Truncated: return "Truncated";
		case TileDecodeStatus::InvalidHeader: return "InvalidHeader";
		case TileDecodeStatus::SizeMismatch: return "SizeMismatch";
		case TileDecodeStatus::UnsupportedMode: return "UnsupportedMode";
		case TileDecodeStatus::CorruptStream: return "CorruptStream";
		case TileDecodeStatus::OutputTooSmall: return "OutputTooSmall";
	}
	return "Unknown";
}

struct DecodedTileResult {
	TileDecodeStatus status{TileDecodeStatus::Truncated};
	RenderTileResultHeader header{};
	SanitizationReport sanitization{};
};

[[nodiscard]] DecodedTileResult decode_render_tile_result(
	std::span<const uint8_t> input,
	std::span<Render::GpuPixelOutput> pixel_output,
	TileCodecScratch& scratch
) noexcept;

[[nodiscard]] bool store_tile_pixels(
	const TileGeometry& geometry,
	std::span<const Render::GpuPixelOutput> tile_pixels,
	std::span<Render::GpuPixelOutput> framebuffer,
	uint32_t screen_width,
	uint32_t screen_height
) noexcept;

[[nodiscard]] bool extract_tile_pixels(
	const TileGeometry& geometry,
	std::span<const Render::GpuPixelOutput> framebuffer,
	uint32_t screen_width,
	uint32_t screen_height,
	std::span<Render::GpuPixelOutput> tile_pixels
) noexcept;

}
