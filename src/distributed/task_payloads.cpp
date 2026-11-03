#include "relativistic/distributed/task_payloads.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace Relativistic::Distributed {

namespace {

[[nodiscard]] bool framebuffer_geometry_fits(const TileGeometry& geometry, uint32_t screen_width, uint32_t screen_height, size_t framebuffer_size) noexcept {
	if (screen_width == 0U || screen_height == 0U || geometry.width == 0U || geometry.height == 0U) {
		return false;
	}
	if (geometry.origin_x > screen_width || geometry.width > screen_width - geometry.origin_x) {
		return false;
	}
	if (geometry.origin_y > screen_height || geometry.height > screen_height - geometry.origin_y) {
		return false;
	}
	return framebuffer_size >= static_cast<size_t>(screen_width) * static_cast<size_t>(screen_height);
}

[[nodiscard]] bool all_finite(const std::array<double, 4>& values) noexcept {
	return std::isfinite(values[0]) && std::isfinite(values[1]) && std::isfinite(values[2]) && std::isfinite(values[3]);
}

}

std::optional<TileGeometry> resolve_tile_geometry(uint32_t tile_index, uint32_t screen_width, uint32_t screen_height) noexcept {
	if (screen_width == 0U || screen_height == 0U || screen_width > kMaximumScreenWidth || screen_height > kMaximumScreenHeight) {
		return std::nullopt;
	}
	const uint32_t columns = tile_grid_columns(screen_width);
	const uint32_t rows = tile_grid_rows(screen_height);
	if (tile_index >= static_cast<uint64_t>(columns) * rows) {
		return std::nullopt;
	}
	TileGeometry geometry{};
	geometry.origin_x = (tile_index % columns) * kTileEdgePixels;
	geometry.origin_y = (tile_index / columns) * kTileEdgePixels;
	geometry.width = std::min(kTileEdgePixels, screen_width - geometry.origin_x);
	geometry.height = std::min(kTileEdgePixels, screen_height - geometry.origin_y);
	return geometry;
}

std::optional<RenderTileTaskHeader> make_render_tile_task_header(
	uint32_t tile_index,
	uint32_t screen_width,
	uint32_t screen_height,
	uint16_t lod_reduced_steps,
	uint8_t flags,
	TileCompressionMode result_mode
) noexcept {
	const auto geometry = resolve_tile_geometry(tile_index, screen_width, screen_height);
	if (!geometry.has_value() || !is_valid_compression_mode(static_cast<uint8_t>(result_mode))) {
		return std::nullopt;
	}
	RenderTileTaskHeader header{};
	header.tile_index = tile_index;
	header.tile_x = static_cast<uint16_t>(geometry->origin_x / kTileEdgePixels);
	header.tile_y = static_cast<uint16_t>(geometry->origin_y / kTileEdgePixels);
	header.tile_width = static_cast<uint16_t>(geometry->width);
	header.tile_height = static_cast<uint16_t>(geometry->height);
	header.lod_reduced_steps = lod_reduced_steps;
	header.flags = flags;
	header.compression_type = static_cast<uint8_t>(result_mode);
	return header;
}

bool validate_render_tile_task(const RenderTileTaskPayload& payload) noexcept {
	const Render::GpuCameraPushConstants& camera = payload.camera_constants;
	if (camera.screen_width == 0U || camera.screen_height == 0U) {
		return false;
	}
	if (camera.screen_width > kMaximumScreenWidth || camera.screen_height > kMaximumScreenHeight) {
		return false;
	}
	if (camera.max_integration_steps == 0U) {
		return false;
	}
	if (!all_finite(camera.observer_position) || !all_finite(camera.tetrad_e1) || !all_finite(camera.tetrad_e2) || !all_finite(camera.tetrad_e3)) {
		return false;
	}
	if (!std::isfinite(camera.field_of_view_rad) || !std::isfinite(camera.metric_mass) || !std::isfinite(camera.metric_spin) || !std::isfinite(camera.escape_radius)) {
		return false;
	}
	if (!std::isfinite(payload.dispatch_timestamp)) {
		return false;
	}
	if (!is_valid_compression_mode(payload.header.compression_type)) {
		return false;
	}
	const auto geometry = resolve_tile_geometry(payload.header.tile_index, camera.screen_width, camera.screen_height);
	if (!geometry.has_value()) {
		return false;
	}
	return payload.header.tile_x == geometry->origin_x / kTileEdgePixels
		&& payload.header.tile_y == geometry->origin_y / kTileEdgePixels
		&& payload.header.tile_width == geometry->width
		&& payload.header.tile_height == geometry->height;
}

size_t serialize_render_tile_task(const RenderTileTaskPayload& payload, std::span<uint8_t> output) noexcept {
	if (output.size() < kRenderTileTaskPayloadBytes || !validate_render_tile_task(payload)) {
		return 0;
	}
	uint8_t* cursor = output.data();
	std::memcpy(cursor, &payload.header, sizeof(RenderTileTaskHeader));
	cursor += sizeof(RenderTileTaskHeader);
	std::memcpy(cursor, &payload.dispatch_timestamp, sizeof(double));
	cursor += sizeof(double);
	std::memcpy(cursor, &payload.camera_constants, sizeof(Render::GpuCameraPushConstants));
	return kRenderTileTaskPayloadBytes;
}

bool deserialize_render_tile_task(std::span<const uint8_t> input, RenderTileTaskPayload& payload) noexcept {
	if (input.size() != kRenderTileTaskPayloadBytes) {
		return false;
	}
	const uint8_t* cursor = input.data();
	std::memcpy(&payload.header, cursor, sizeof(RenderTileTaskHeader));
	cursor += sizeof(RenderTileTaskHeader);
	std::memcpy(&payload.dispatch_timestamp, cursor, sizeof(double));
	cursor += sizeof(double);
	std::memcpy(&payload.camera_constants, cursor, sizeof(Render::GpuCameraPushConstants));
	return validate_render_tile_task(payload);
}

EncodedTileResult encode_render_tile_result(
	uint64_t frame_index,
	uint32_t tile_index,
	std::span<const Render::GpuPixelOutput> pixels,
	TileCompressionMode requested_mode,
	uint32_t execution_time_us,
	std::span<uint8_t> output,
	TileCodecScratch& scratch,
	const CompressionParameters& parameters
) noexcept {
	const size_t count = pixels.size();
	if (count == 0U || count > kTilePixelCapacity) {
		return EncodedTileResult{CodecStatus::InvalidArgument, requested_mode, 0};
	}
	if (output.size() <= sizeof(RenderTileResultPayload)) {
		return EncodedTileResult{CodecStatus::OutputTooSmall, requested_mode, 0};
	}
	const CompressionResult compressed = TileCodec::compress(requested_mode, pixels, output.subspan(sizeof(RenderTileResultPayload)), scratch, parameters);
	if (compressed.status != CodecStatus::Ok) {
		return EncodedTileResult{compressed.status, requested_mode, 0};
	}

	RenderTileResultPayload payload{};
	payload.header.tile_index = tile_index;
	payload.header.frame_index = frame_index;
	payload.header.pixel_count = static_cast<uint16_t>(count);
	payload.header.compression_type = static_cast<uint8_t>(compressed.mode_used);
	payload.header.reserved = 0;
	payload.header.uncompressed_bytes = static_cast<uint32_t>(count * kPixelStrideBytes);
	payload.header.execution_time_us = execution_time_us;
	payload.compressed_bytes = static_cast<uint32_t>(compressed.compressed_bytes);
	std::memcpy(output.data(), &payload, sizeof(RenderTileResultPayload));

	return EncodedTileResult{CodecStatus::Ok, compressed.mode_used, sizeof(RenderTileResultPayload) + compressed.compressed_bytes};
}

DecodedTileResult decode_render_tile_result(
	std::span<const uint8_t> input,
	std::span<Render::GpuPixelOutput> pixel_output,
	TileCodecScratch& scratch
) noexcept {
	DecodedTileResult result{};
	if (input.size() < sizeof(RenderTileResultPayload)) {
		result.status = TileDecodeStatus::Truncated;
		return result;
	}

	RenderTileResultPayload payload{};
	std::memcpy(&payload, input.data(), sizeof(RenderTileResultPayload));
	result.header = payload.header;

	const size_t pixel_count = payload.header.pixel_count;
	const uint32_t uncompressed_bytes = payload.header.uncompressed_bytes;
	const uint8_t mode_value = payload.header.compression_type;
	const uint32_t compressed_bytes = payload.compressed_bytes;

	if (pixel_count == 0U || pixel_count > kTilePixelCapacity) {
		result.status = TileDecodeStatus::InvalidHeader;
		return result;
	}
	if (uncompressed_bytes != pixel_count * kPixelStrideBytes) {
		result.status = TileDecodeStatus::SizeMismatch;
		return result;
	}
	const std::span<const uint8_t> body = input.subspan(sizeof(RenderTileResultPayload));
	if (compressed_bytes != body.size()) {
		result.status = TileDecodeStatus::SizeMismatch;
		return result;
	}
	if (!is_valid_compression_mode(mode_value)) {
		result.status = TileDecodeStatus::UnsupportedMode;
		return result;
	}
	if (pixel_output.size() < pixel_count) {
		result.status = TileDecodeStatus::OutputTooSmall;
		return result;
	}

	const std::span<Render::GpuPixelOutput> destination = pixel_output.first(pixel_count);
	const CodecStatus status = TileCodec::decompress(static_cast<TileCompressionMode>(mode_value), body, destination, scratch);
	switch (status) {
		case CodecStatus::Ok:
			break;
		case CodecStatus::SizeMismatch:
			result.status = TileDecodeStatus::SizeMismatch;
			return result;
		case CodecStatus::UnsupportedMode:
			result.status = TileDecodeStatus::UnsupportedMode;
			return result;
		default:
			result.status = TileDecodeStatus::CorruptStream;
			return result;
	}

	result.sanitization = TileSanitizer::sanitize(destination);
	result.status = TileDecodeStatus::Ok;
	return result;
}

bool store_tile_pixels(
	const TileGeometry& geometry,
	std::span<const Render::GpuPixelOutput> tile_pixels,
	std::span<Render::GpuPixelOutput> framebuffer,
	uint32_t screen_width,
	uint32_t screen_height
) noexcept {
	if (!framebuffer_geometry_fits(geometry, screen_width, screen_height, framebuffer.size()) || tile_pixels.size() < geometry.pixel_count()) {
		return false;
	}
	const size_t row_bytes = static_cast<size_t>(geometry.width) * kPixelStrideBytes;
	for (uint32_t row = 0; row < geometry.height; ++row) {
		Render::GpuPixelOutput* destination = framebuffer.data() + static_cast<size_t>(geometry.origin_y + row) * screen_width + geometry.origin_x;
		const Render::GpuPixelOutput* source = tile_pixels.data() + static_cast<size_t>(row) * geometry.width;
		std::memcpy(destination, source, row_bytes);
	}
	return true;
}

bool extract_tile_pixels(
	const TileGeometry& geometry,
	std::span<const Render::GpuPixelOutput> framebuffer,
	uint32_t screen_width,
	uint32_t screen_height,
	std::span<Render::GpuPixelOutput> tile_pixels
) noexcept {
	if (!framebuffer_geometry_fits(geometry, screen_width, screen_height, framebuffer.size()) || tile_pixels.size() < geometry.pixel_count()) {
		return false;
	}
	const size_t row_bytes = static_cast<size_t>(geometry.width) * kPixelStrideBytes;
	for (uint32_t row = 0; row < geometry.height; ++row) {
		const Render::GpuPixelOutput* source = framebuffer.data() + static_cast<size_t>(geometry.origin_y + row) * screen_width + geometry.origin_x;
		Render::GpuPixelOutput* destination = tile_pixels.data() + static_cast<size_t>(row) * geometry.width;
		std::memcpy(destination, source, row_bytes);
	}
	return true;
}

}
