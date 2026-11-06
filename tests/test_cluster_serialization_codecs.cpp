#include "support/test_harness.hpp"
#include "relativistic/core/pcg64.hpp"
#include "relativistic/distributed/compression_codecs.hpp"
#include "relativistic/distributed/task_payloads.hpp"
#include "relativistic/render/gpu_types.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>

using namespace Relativistic;
using namespace Relativistic::Distributed;
using namespace Relativistic::Render;
using namespace Relativistic::Testing;

int main() {
	TestSuite suite(
		"cluster_serialization_codecs",
		"Validates lossless and near-lossless tile codecs (DirectRaw, QOI-HDR RLE, LZ4Block), SIMD vector sanitization of corrupted radiances, and binary wire serialization contracts."
	);

	suite.section("DirectRaw Mode Exact Identity");
	{
		std::array<GpuPixelOutput, 1024> original{};
		Core::PCG64Engine rng(0xA1B2C3D4E5F60718ULL, 0x9E3779B97F4A7C15ULL);
		for (size_t i = 0; i < original.size(); ++i) {
			original[i].r = static_cast<float>(rng() & 0xFFFFU) * 0.05f;
			original[i].g = static_cast<float>(rng() & 0xFFFFU) * 0.05f;
			original[i].b = static_cast<float>(rng() & 0xFFFFU) * 0.05f;
			original[i].a = 1.0f;
			original[i].redshift = 0.5f + static_cast<float>(rng() & 0x7FFU) * 0.001f;
			original[i].affine_parameter = static_cast<float>(rng() & 0x7FFU) * 0.01f;
			original[i].status_flags = (rng() & 1U) ? PixelFlags::HORIZON_ABSORBED : PixelFlags::CELESTIAL_HIT;
			original[i].iterations_used = static_cast<uint32_t>(rng() % 500U);
		}

		std::vector<uint8_t> wire(TileCodec::max_compressed_size(TileCompressionMode::DirectRaw, original.size()));
		TileCodecScratch& scratch = TileCodec::thread_scratch();
		CompressionParameters params{};

		const CompressionResult comp = TileCodec::compress(TileCompressionMode::DirectRaw, original, wire, scratch, params);
		suite.check(comp.status == CodecStatus::Ok, "direct raw compression returns Ok");
		suite.check(comp.mode_used == TileCompressionMode::DirectRaw, "direct raw mode used is DirectRaw");
		suite.check(comp.compressed_bytes == original.size() * sizeof(GpuPixelOutput), "direct raw byte length equals raw size");

		std::array<GpuPixelOutput, 1024> restored{};
		const CodecStatus dec_status = TileCodec::decompress(TileCompressionMode::DirectRaw, std::span<const uint8_t>(wire.data(), comp.compressed_bytes), restored, scratch);
		suite.check(dec_status == CodecStatus::Ok, "direct raw decompression returns Ok");
		suite.check(std::memcmp(original.data(), restored.data(), sizeof(original)) == 0, "direct raw payload restored with byte-exact identity");
	}

	suite.section("LZ4Block Mode Compression & Throughput");
	{
		std::array<GpuPixelOutput, 1024> patterned{};
		for (size_t i = 0; i < patterned.size(); ++i) {
			const float base = static_cast<float>(i / 64) * 0.1f;
			patterned[i].r = base;
			patterned[i].g = base * 0.8f;
			patterned[i].b = base * 0.4f;
			patterned[i].a = 1.0f;
			patterned[i].redshift = 1.0f;
			patterned[i].affine_parameter = static_cast<float>(i) * 0.01f;
			patterned[i].status_flags = (i < 256) ? PixelFlags::HORIZON_ABSORBED : PixelFlags::ACCRETION_DISK_HIT;
			patterned[i].iterations_used = static_cast<uint32_t>(i % 32);
		}

		std::vector<uint8_t> compressed(TileCodec::max_compressed_size(TileCompressionMode::Lz4Block, patterned.size()));
		TileCodecScratch& scratch = TileCodec::thread_scratch();
		CompressionParameters params{};
		params.lz4_acceleration = 1;

		const CompressionResult comp = TileCodec::compress(TileCompressionMode::Lz4Block, patterned, compressed, scratch, params);
		suite.check(comp.status == CodecStatus::Ok, "lz4 compression returns Ok");
		suite.check(comp.compressed_bytes < patterned.size() * sizeof(GpuPixelOutput), "lz4 compression yields smaller byte footprint");

		std::array<GpuPixelOutput, 1024> restored{};
		const CodecStatus dec = TileCodec::decompress(TileCompressionMode::Lz4Block, std::span<const uint8_t>(compressed.data(), comp.compressed_bytes), restored, scratch);
		suite.check(dec == CodecStatus::Ok, "lz4 decompression returns Ok");
		suite.check(std::memcmp(patterned.data(), restored.data(), sizeof(patterned)) == 0, "lz4 decompression restores exact bit identity");

		const auto t0 = std::chrono::high_resolution_clock::now();
		constexpr size_t kIterations = 2000;
		for (size_t iter = 0; iter < kIterations; ++iter) {
			static_cast<void>(TileCodec::decompress(TileCompressionMode::Lz4Block, std::span<const uint8_t>(compressed.data(), comp.compressed_bytes), restored, scratch));
		}
		const auto t1 = std::chrono::high_resolution_clock::now();
		const double elapsed_sec = std::chrono::duration<double>(t1 - t0).count();
		const double mb_decompressed = static_cast<double>(kIterations * patterned.size() * sizeof(GpuPixelOutput)) / (1024.0 * 1024.0);
		const double mb_per_sec = mb_decompressed / std::max(elapsed_sec, 1.0e-9);

		suite.measure("lz4_decompress_throughput", mb_per_sec, "MB/s");
		suite.check(mb_per_sec > 1000.0, "lz4 decompression throughput exceeds 1000 MB/s");
	}

	suite.section("QOI-HDR RLE Spectral Precision");
	{
		std::array<GpuPixelOutput, 1024> hdr_tile{};
		Core::PCG64Engine rng(0x13579BDF2468ACE0ULL, 0);
		for (size_t i = 0; i < hdr_tile.size(); ++i) {
			const float exp_val = -3.0f + static_cast<float>(rng() & 0x3FFU) * (6.0f / 1023.0f);
			const float val = std::pow(10.0f, exp_val);
			hdr_tile[i].r = val;
			hdr_tile[i].g = val * 0.95f;
			hdr_tile[i].b = val * 1.1f;
			hdr_tile[i].a = 1.0f;
			hdr_tile[i].redshift = 0.85f;
			hdr_tile[i].affine_parameter = 1.25f;
			hdr_tile[i].status_flags = PixelFlags::ACCRETION_DISK_HIT;
			hdr_tile[i].iterations_used = 128;
		}

		std::vector<uint8_t> compressed(TileCodec::max_compressed_size(TileCompressionMode::QoiHdrRle, hdr_tile.size()));
		TileCodecScratch& scratch = TileCodec::thread_scratch();
		CompressionParameters params{};
		params.allow_raw_fallback = false;

		const CompressionResult comp = TileCodec::compress(TileCompressionMode::QoiHdrRle, hdr_tile, compressed, scratch, params);
		suite.check(comp.status == CodecStatus::Ok, "qoi hdr compression returns Ok");

		std::array<GpuPixelOutput, 1024> restored{};
		const CodecStatus dec = TileCodec::decompress(TileCompressionMode::QoiHdrRle, std::span<const uint8_t>(compressed.data(), comp.compressed_bytes), restored, scratch);
		suite.check(dec == CodecStatus::Ok, "qoi hdr decompression returns Ok");

		double max_relative_error = 0.0;
		for (size_t i = 0; i < hdr_tile.size(); ++i) {
			const double orig = hdr_tile[i].r;
			const double rest = restored[i].r;
			const double rel = std::abs(orig - rest) / std::max(std::abs(orig), 1.0e-4);
			max_relative_error = std::max(max_relative_error, rel);
			suite.check(hdr_tile[i].status_flags == restored[i].status_flags, "qoi hdr preserves pixel status flags");
			suite.check(hdr_tile[i].iterations_used == restored[i].iterations_used, "qoi hdr preserves iteration count");
		}
		suite.measure("qoi_max_relative_error", max_relative_error, "relative");
		suite.check_at_most(max_relative_error, 1.0e-3, "fp16 quantization relative error bounded under 1e-3");
	}

	suite.section("SIMD Vector Sanitization");
	{
		std::array<GpuPixelOutput, 64> corrupted{};
		for (auto& px : corrupted) {
			px.r = 1.0f;
			px.g = 1.0f;
			px.b = 1.0f;
			px.a = 1.0f;
			px.redshift = 1.0f;
		}

		corrupted[3].r = std::numeric_limits<float>::quiet_NaN();
		corrupted[7].g = std::numeric_limits<float>::infinity();
		corrupted[11].b = -2.5f;
		corrupted[15].r = 1.0e12f;
		corrupted[19].a = std::numeric_limits<float>::quiet_NaN();
		corrupted[23].redshift = std::numeric_limits<float>::infinity();

		const SanitizationReport report1 = TileSanitizer::sanitize(corrupted);
		suite.check(!report1.clean(), "first sanitization pass flags injected anomalies");
		suite.check(report1.pixels_neutralized == 3, "three nan/inf/negative pixels neutralized");
		suite.check(report1.pixels_clamped == 1, "excessive radiance pixel clamped to ceiling");
		suite.check(report1.metadata_repaired >= 2, "corrupted alpha and redshift repaired");

		for (const auto& px : corrupted) {
			suite.check_finite(px.r, "red radiance is finite post-sanitization");
			suite.check_finite(px.g, "green radiance is finite post-sanitization");
			suite.check_finite(px.b, "blue radiance is finite post-sanitization");
			suite.check(px.r >= 0.0f && px.r <= TileSanitizer::kMaximumRadiance, "red radiance in safe physical bounds");
		}

		const SanitizationReport report2 = TileSanitizer::sanitize(corrupted);
		suite.check(report2.clean(), "second sanitization pass confirms clean state");
	}

	suite.section("Wire Envelope Encoding & Parsing");
	{
		RenderTileTaskPayload task{};
		task.camera_constants.screen_width = 1920;
		task.camera_constants.screen_height = 1080;
		task.camera_constants.max_integration_steps = 1024;
		task.camera_constants.field_of_view_rad = 1.0;
		task.camera_constants.metric_mass = 1.0;
		task.camera_constants.escape_radius = 50.0;
		task.camera_constants.observer_position = {0.0, 10.0, 1.57, 0.0};
		task.camera_constants.tetrad_e1 = {0.0, 1.0, 0.0, 0.0};
		task.camera_constants.tetrad_e2 = {0.0, 0.0, 1.0, 0.0};
		task.camera_constants.tetrad_e3 = {0.0, 0.0, 0.0, 1.0};
		task.dispatch_timestamp = 123.456;

		const auto header = make_render_tile_task_header(42, 1920, 1080, 0, 0, TileCompressionMode::Lz4Block);
		suite.check(header.has_value(), "valid tile task header created");
		task.header = *header;

		std::vector<uint8_t> buffer(kRenderTileTaskPayloadBytes);
		const size_t written = serialize_render_tile_task(task, buffer);
		suite.check(written == kRenderTileTaskPayloadBytes, "task payload serializes exactly expected byte size");

		RenderTileTaskPayload parsed{};
		const bool ok = deserialize_render_tile_task(buffer, parsed);
		suite.check(ok, "task payload deserializes successfully");
		suite.check(parsed.header.tile_index == 42, "parsed tile index matches");
		suite.check(parsed.camera_constants.screen_width == 1920, "parsed screen width matches");

		std::vector<uint8_t> truncated(buffer.begin(), buffer.begin() + 100);
		RenderTileTaskPayload failed_payload{};
		suite.check(!deserialize_render_tile_task(truncated, failed_payload), "truncated buffer fails deserialization gracefully");
	}

	return suite.finish();
}
