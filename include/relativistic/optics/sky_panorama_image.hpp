#pragma once

#include "relativistic/optics/sky_panorama_catalog.hpp"
#include "relativistic/optics/sky_panorama_codecs.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <numbers>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace Relativistic::Optics {

inline constexpr uint64_t kMaxPanoramaPixels = 268435456ULL;

namespace PanoramaColor {

inline const std::array<float, 256> kSrgbToLinear = [] {
	std::array<float, 256> table{};
	for (size_t i = 0; i < table.size(); ++i) {
		const double encoded = static_cast<double>(i) / 255.0;
		table[i] = static_cast<float>((encoded <= 0.04045) ? (encoded / 12.92) : std::pow((encoded + 0.055) / 1.055, 2.4));
	}
	return table;
}();

}

struct DecodedPanorama {
	uint32_t width{0};
	uint32_t height{0};
	std::vector<uint32_t> texels{};

	[[nodiscard]] static constexpr uint32_t pack_texel(uint8_t r, uint8_t g, uint8_t b) noexcept {
		return static_cast<uint32_t>(r) | (static_cast<uint32_t>(g) << 8) | (static_cast<uint32_t>(b) << 16) | 0xFF000000U;
	}

	[[nodiscard]] bool is_valid() const noexcept {
		return width > 0 && height > 0 && texels.size() == static_cast<size_t>(width) * static_cast<size_t>(height);
	}

	[[nodiscard]] std::array<float, 3> sample_bilinear(double u, double v) const noexcept {
		if (!is_valid()) {
			return {0.0f, 0.0f, 0.0f};
		}

		const double uu = u - std::floor(u);
		const double vv = std::clamp(v, 0.0, 1.0);

		const double fx = uu * static_cast<double>(width) - 0.5;
		const double fy = vv * static_cast<double>(height) - 0.5;

		int64_t x0 = static_cast<int64_t>(std::floor(fx));
		int64_t y0 = static_cast<int64_t>(std::floor(fy));
		const double tx = fx - static_cast<double>(x0);
		const double ty = fy - static_cast<double>(y0);

		auto wrap_x = [&](int64_t x) noexcept -> int64_t {
			const int64_t w = static_cast<int64_t>(width);
			int64_t r = x % w;
			if (r < 0) r += w;
			return r;
		};
		auto clamp_y = [&](int64_t y) noexcept -> int64_t {
			return std::clamp<int64_t>(y, 0, static_cast<int64_t>(height) - 1);
		};

		const int64_t x1 = wrap_x(x0 + 1);
		x0 = wrap_x(x0);
		const int64_t y1 = clamp_y(y0 + 1);
		y0 = clamp_y(y0);

		const auto& linear_table = PanoramaColor::kSrgbToLinear;
		auto fetch = [&](int64_t x, int64_t y) noexcept -> std::array<float, 3> {
			const uint32_t texel = texels[static_cast<size_t>(y) * width + static_cast<size_t>(x)];
			return {linear_table[texel & 0xFFU], linear_table[(texel >> 8) & 0xFFU], linear_table[(texel >> 16) & 0xFFU]};
		};

		const auto c00 = fetch(x0, y0);
		const auto c10 = fetch(x1, y0);
		const auto c01 = fetch(x0, y1);
		const auto c11 = fetch(x1, y1);

		const float ftx = static_cast<float>(tx);
		const float fty = static_cast<float>(ty);
		const float one_minus_tx = 1.0f - ftx;
		const float one_minus_ty = 1.0f - fty;

		std::array<float, 3> result{};
		for (size_t i = 0; i < 3; ++i) {
			const float top = c00[i] * one_minus_tx + c10[i] * ftx;
			const float bottom = c01[i] * one_minus_tx + c11[i] * ftx;
			result[i] = top * one_minus_ty + bottom * fty;
		}
		return result;
	}
};

namespace PanoramaDecodeDetail {

	inline constexpr std::array<uint8_t, 64> kZigZag{
		0, 1, 8, 16, 9, 2, 3, 10,
		17, 24, 32, 25, 18, 11, 4, 5,
		12, 19, 26, 33, 40, 48, 41, 34,
		27, 20, 13, 6, 7, 14, 21, 28,
		35, 42, 49, 56, 57, 50, 43, 36,
		29, 22, 15, 23, 30, 37, 44, 51,
		58, 59, 52, 45, 38, 31, 39, 46,
		53, 60, 61, 54, 47, 55, 62, 63
	};

	inline void idct_8x8(const float* in, float* out) noexcept {
		static const std::array<std::array<float, 8>, 8> cos_table = [] {
			std::array<std::array<float, 8>, 8> t{};
			for (int x = 0; x < 8; ++x) {
				for (int u = 0; u < 8; ++u) {
					t[static_cast<size_t>(x)][static_cast<size_t>(u)] = std::cos((2.0f * static_cast<float>(x) + 1.0f) * static_cast<float>(u) * 3.14159265358979323846f / 16.0f);
				}
			}
			return t;
		}();

		std::array<float, 64> tmp{};
		for (int y = 0; y < 8; ++y) {
			for (int x = 0; x < 8; ++x) {
				float sum = 0.0f;
				for (int u = 0; u < 8; ++u) {
					const float cu = (u == 0) ? 0.70710678f : 1.0f;
					sum += cu * in[y * 8 + u] * cos_table[static_cast<size_t>(x)][static_cast<size_t>(u)];
				}
				tmp[static_cast<size_t>(y * 8 + x)] = sum * 0.5f;
			}
		}
		for (int x = 0; x < 8; ++x) {
			for (int y = 0; y < 8; ++y) {
				float sum = 0.0f;
				for (int v = 0; v < 8; ++v) {
					const float cv = (v == 0) ? 0.70710678f : 1.0f;
					sum += cv * tmp[static_cast<size_t>(v * 8 + x)] * cos_table[static_cast<size_t>(y)][static_cast<size_t>(v)];
				}
				out[y * 8 + x] = sum * 0.5f;
			}
		}
	}

	struct HuffmanTable {
		std::array<int32_t, 17> mincode{};
		std::array<int32_t, 17> maxcode{};
		std::array<int32_t, 17> valptr{};
		std::array<uint8_t, 256> values{};
		uint32_t num_values{0};

		void build(const std::array<uint8_t, 16>& bits, const uint8_t* vals, size_t val_count) noexcept {
			num_values = static_cast<uint32_t>(val_count);
			for (size_t i = 0; i < val_count && i < values.size(); ++i) values[i] = vals[i];

			std::array<int32_t, 257> huffsize{};
			int32_t k = 0;
			for (int32_t l = 1; l <= 16; ++l) {
				for (int32_t i = 0; i < bits[static_cast<size_t>(l - 1)]; ++i) {
					huffsize[static_cast<size_t>(k)] = l;
					++k;
				}
			}
			const int32_t count = k;

			std::array<int32_t, 257> huffcode{};
			int32_t code = 0;
			int32_t si = (count > 0) ? huffsize[0] : 0;
			k = 0;
			while (k < count) {
				while (k < count && huffsize[static_cast<size_t>(k)] == si) {
					huffcode[static_cast<size_t>(k)] = code;
					++code;
					++k;
				}
				code <<= 1;
				++si;
			}

			int32_t p = 0;
			for (int32_t l = 1; l <= 16; ++l) {
				if (bits[static_cast<size_t>(l - 1)] > 0) {
					valptr[static_cast<size_t>(l)] = p;
					mincode[static_cast<size_t>(l)] = huffcode[static_cast<size_t>(p)];
					p += bits[static_cast<size_t>(l - 1)];
					maxcode[static_cast<size_t>(l)] = huffcode[static_cast<size_t>(p - 1)];
				} else {
					maxcode[static_cast<size_t>(l)] = -1;
				}
			}
		}
	};

	struct JpegBitReader {
		std::span<const uint8_t> data;
		size_t pos{0};
		uint32_t bit_buffer{0};
		int32_t bits_available{0};
		bool hit_marker{false};

		explicit JpegBitReader(std::span<const uint8_t> d) noexcept : data(d) {}

		[[nodiscard]] int32_t next_bit() noexcept {
			if (bits_available == 0) {
				if (hit_marker || pos >= data.size()) {
					bit_buffer = 0;
					bits_available = 8;
				} else {
					const uint8_t b = data[pos];
					if (b == 0xFF) {
						if (pos + 1 < data.size() && data[pos + 1] == 0x00) {
							pos += 2;
							bit_buffer = 0xFF;
						} else {
							hit_marker = true;
							bit_buffer = 0;
						}
					} else {
						++pos;
						bit_buffer = b;
					}
					bits_available = 8;
				}
			}
			--bits_available;
			return static_cast<int32_t>((bit_buffer >> bits_available) & 1U);
		}

		[[nodiscard]] int32_t receive(int32_t n) noexcept {
			int32_t v = 0;
			for (int32_t i = 0; i < n; ++i) v = (v << 1) | next_bit();
			return v;
		}
	};

	[[nodiscard]] inline int32_t decode_huffman(JpegBitReader& reader, const HuffmanTable& table) noexcept {
		int32_t code = reader.next_bit();
		int32_t length = 1;
		while (length <= 16) {
			if (table.maxcode[static_cast<size_t>(length)] >= 0 && code <= table.maxcode[static_cast<size_t>(length)]) {
				const int32_t index = table.valptr[static_cast<size_t>(length)] + (code - table.mincode[static_cast<size_t>(length)]);
				if (index < 0 || static_cast<uint32_t>(index) >= table.num_values) return -1;
				return table.values[static_cast<size_t>(index)];
			}
			code = (code << 1) | reader.next_bit();
			++length;
		}
		return -1;
	}

	[[nodiscard]] inline int32_t extend(int32_t value, int32_t size) noexcept {
		if (size == 0) return 0;
		const int32_t vt = 1 << (size - 1);
		return (value < vt) ? (value - (1 << size) + 1) : value;
	}

	struct JpegComponent {
		uint8_t id{0};
		uint8_t h{1};
		uint8_t v{1};
		uint8_t quant_table_id{0};
		uint8_t dc_table_id{0};
		uint8_t ac_table_id{0};
		int32_t dc_pred{0};
	};

	[[nodiscard]] inline std::optional<DecodedPanorama> decode_jpeg(std::span<const uint8_t> data) {
		if (data.size() < 4 || data[0] != 0xFF || data[1] != 0xD8) return std::nullopt;

		std::array<std::array<uint16_t, 64>, 4> quant_tables{};
		std::array<HuffmanTable, 4> dc_tables{};
		std::array<HuffmanTable, 4> ac_tables{};
		std::array<JpegComponent, 4> components{};
		uint32_t component_count = 0;
		uint32_t image_width = 0;
		uint32_t image_height = 0;
		uint32_t restart_interval = 0;
		bool sof_seen = false;

		size_t pos = 2;
		while (pos + 4 <= data.size()) {
			if (data[pos] != 0xFF) { ++pos; continue; }
			const uint8_t marker = data[pos + 1];
			pos += 2;
			if (marker == 0xD8 || marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7)) continue;
			if (marker == 0xD9) break;
			if (pos + 2 > data.size()) break;
			const uint32_t seg_len = (static_cast<uint32_t>(data[pos]) << 8) | data[pos + 1];
			if (seg_len < 2 || pos + seg_len > data.size()) break;
			const size_t seg_start = pos + 2;
			const size_t seg_end = pos + seg_len;

			if (marker == 0xDB) {
				size_t p = seg_start;
				while (p < seg_end) {
					const uint8_t pq_tq = data[p++];
					const uint32_t precision = static_cast<uint32_t>(pq_tq) >> 4;
					const uint32_t table_id = pq_tq & 0x0FU;
					if (table_id >= 4) break;
					for (size_t i = 0; i < 64; ++i) {
						uint16_t val = 0;
						if (precision == 0) {
							if (p >= seg_end) break;
							val = data[p++];
						} else {
							if (p + 1 >= seg_end) break;
							val = static_cast<uint16_t>((static_cast<uint16_t>(data[p]) << 8) | data[p + 1]);
							p += 2;
						}
						quant_tables[table_id][kZigZag[i]] = val;
					}
				}
			} else if (marker == 0xC0 || marker == 0xC1) {
				if (seg_len < 8 || data[seg_start] != 8) return std::nullopt;
				size_t p = seg_start;
				++p;
				image_height = (static_cast<uint32_t>(data[p]) << 8) | data[p + 1];
				p += 2;
				image_width = (static_cast<uint32_t>(data[p]) << 8) | data[p + 1];
				p += 2;
				component_count = data[p++];
				if (component_count == 0 || component_count > 4) return std::nullopt;
				if (seg_len < 8 + 3 * component_count) return std::nullopt;
				for (uint32_t c = 0; c < component_count; ++c) {
					components[c].id = data[p++];
					const uint8_t hv = data[p++];
					components[c].h = static_cast<uint8_t>(hv >> 4);
					components[c].v = static_cast<uint8_t>(hv & 0x0FU);
					components[c].quant_table_id = data[p++];
					if (components[c].h == 0 || components[c].h > 4 || components[c].v == 0 || components[c].v > 4 || components[c].quant_table_id >= 4) {
						return std::nullopt;
					}
				}
				sof_seen = true;
			} else if (marker == 0xC2 || marker == 0xC3 || (marker >= 0xC5 && marker <= 0xC7) || (marker >= 0xC9 && marker <= 0xCB) || (marker >= 0xCD && marker <= 0xCF)) {
				return std::nullopt;
			} else if (marker == 0xC4) {
				size_t p = seg_start;
				while (p < seg_end) {
					const uint8_t tc_th = data[p++];
					const uint32_t table_class = static_cast<uint32_t>(tc_th) >> 4;
					const uint32_t table_id = tc_th & 0x0FU;
					if (table_id >= 4) break;
					std::array<uint8_t, 16> bits{};
					uint32_t total = 0;
					for (size_t i = 0; i < 16; ++i) {
						bits[i] = data[p++];
						total += bits[i];
					}
					if (p + total > seg_end) break;
					if (table_class == 0) {
						dc_tables[table_id].build(bits, &data[p], total);
					} else {
						ac_tables[table_id].build(bits, &data[p], total);
					}
					p += total;
				}
			} else if (marker == 0xDD) {
				restart_interval = (static_cast<uint32_t>(data[seg_start]) << 8) | data[seg_start + 1];
			} else if (marker == 0xDA) {
				if (!sof_seen || component_count == 0 || image_width == 0 || image_height == 0) return std::nullopt;
				if (static_cast<uint64_t>(image_width) * image_height > kMaxPanoramaPixels) return std::nullopt;
				size_t p = seg_start;
				const uint8_t scan_components = data[p++];
				for (uint8_t s = 0; s < scan_components; ++s) {
					const uint8_t comp_id = data[p++];
					const uint8_t td_ta = data[p++];
					for (uint32_t c = 0; c < component_count; ++c) {
						if (components[c].id == comp_id) {
							const uint8_t dc_id = static_cast<uint8_t>(td_ta >> 4);
							const uint8_t ac_id = static_cast<uint8_t>(td_ta & 0x0FU);
							if (dc_id >= 4 || ac_id >= 4) return std::nullopt;
							components[c].dc_table_id = dc_id;
							components[c].ac_table_id = ac_id;
						}
					}
				}
				p += 3;

				uint32_t max_h = 1;
				uint32_t max_v = 1;
				for (uint32_t c = 0; c < component_count; ++c) {
					max_h = std::max<uint32_t>(max_h, components[c].h);
					max_v = std::max<uint32_t>(max_v, components[c].v);
				}

				const uint32_t mcu_w = 8 * max_h;
				const uint32_t mcu_h = 8 * max_v;
				const uint32_t mcus_x = (image_width + mcu_w - 1) / mcu_w;
				const uint32_t mcus_y = (image_height + mcu_h - 1) / mcu_h;

				std::array<std::vector<float>, 4> plane_data{};
				std::array<uint32_t, 4> plane_w{};
				std::array<uint32_t, 4> plane_h{};
				for (uint32_t c = 0; c < component_count; ++c) {
					plane_w[c] = mcus_x * components[c].h * 8;
					plane_h[c] = mcus_y * components[c].v * 8;
					plane_data[c].assign(static_cast<size_t>(plane_w[c]) * plane_h[c], 0.0f);
					components[c].dc_pred = 0;
				}

				JpegBitReader reader(data.subspan(p));
				uint32_t mcus_since_restart = 0;

				for (uint32_t my = 0; my < mcus_y; ++my) {
					for (uint32_t mx = 0; mx < mcus_x; ++mx) {
						for (uint32_t c = 0; c < component_count; ++c) {
							auto& comp = components[c];
							for (uint32_t by = 0; by < comp.v; ++by) {
								for (uint32_t bx = 0; bx < comp.h; ++bx) {
									std::array<float, 64> coeffs{};
									const int32_t s = decode_huffman(reader, dc_tables[comp.dc_table_id]);
									if (s < 0) return std::nullopt;
									const int32_t diff = (s == 0) ? 0 : extend(reader.receive(s), s);
									comp.dc_pred += diff;
									coeffs[0] = static_cast<float>(comp.dc_pred) * static_cast<float>(quant_tables[comp.quant_table_id][0]);

									uint32_t k = 1;
									while (k < 64) {
										const int32_t rs = decode_huffman(reader, ac_tables[comp.ac_table_id]);
										if (rs < 0) return std::nullopt;
										const uint32_t run = static_cast<uint32_t>(rs) >> 4;
										const uint32_t size = static_cast<uint32_t>(rs) & 0x0FU;
										if (size == 0) {
											if (run == 15) { k += 16; continue; }
											break;
										}
										k += run;
										if (k >= 64) break;
										const int32_t value = extend(reader.receive(static_cast<int32_t>(size)), static_cast<int32_t>(size));
										coeffs[kZigZag[k]] = static_cast<float>(value) * static_cast<float>(quant_tables[comp.quant_table_id][kZigZag[k]]);
										++k;
									}

									std::array<float, 64> block{};
									idct_8x8(coeffs.data(), block.data());

									const uint32_t block_x0 = (mx * comp.h + bx) * 8;
									const uint32_t block_y0 = (my * comp.v + by) * 8;
									for (uint32_t yy = 0; yy < 8; ++yy) {
										for (uint32_t xx = 0; xx < 8; ++xx) {
											const uint32_t px = block_x0 + xx;
											const uint32_t py = block_y0 + yy;
											if (px < plane_w[c] && py < plane_h[c]) {
												plane_data[c][static_cast<size_t>(py) * plane_w[c] + px] = block[yy * 8 + xx] + 128.0f;
											}
										}
									}
								}
							}
						}

						++mcus_since_restart;
						if (restart_interval > 0 && mcus_since_restart == restart_interval && !(my == mcus_y - 1 && mx == mcus_x - 1)) {
							mcus_since_restart = 0;
							reader.bits_available = 0;
							const size_t rp = p + reader.pos;
							if (rp + 1 < data.size() && data[rp] == 0xFF && data[rp + 1] >= 0xD0 && data[rp + 1] <= 0xD7) {
								reader.pos += 2;
							}
							reader.hit_marker = false;
							for (uint32_t c = 0; c < component_count; ++c) {
								components[c].dc_pred = 0;
							}
						}
					}
				}

				DecodedPanorama result;
				result.width = image_width;
				result.height = image_height;
				result.texels.assign(static_cast<size_t>(image_width) * image_height, 0U);

				for (uint32_t y = 0; y < image_height; ++y) {
					for (uint32_t x = 0; x < image_width; ++x) {
						float yv = 0.0f, cb = 128.0f, cr = 128.0f;
						if (component_count >= 3) {
							const uint32_t sy0 = (x * components[0].h) / max_h;
							const uint32_t sy1 = (y * components[0].v) / max_v;
							yv = plane_data[0][static_cast<size_t>(sy1) * plane_w[0] + sy0];
							const uint32_t cx0 = (x * components[1].h) / max_h;
							const uint32_t cy0 = (y * components[1].v) / max_v;
							cb = plane_data[1][static_cast<size_t>(cy0) * plane_w[1] + cx0];
							const uint32_t crx0 = (x * components[2].h) / max_h;
							const uint32_t cry0 = (y * components[2].v) / max_v;
							cr = plane_data[2][static_cast<size_t>(cry0) * plane_w[2] + crx0];
						} else {
							yv = plane_data[0][static_cast<size_t>(y) * plane_w[0] + x];
						}

						float r, g, b;
						if (component_count >= 3) {
							r = yv + 1.402f * (cr - 128.0f);
							g = yv - 0.344136f * (cb - 128.0f) - 0.714136f * (cr - 128.0f);
							b = yv + 1.772f * (cb - 128.0f);
						} else {
							r = g = b = yv;
						}

						result.texels[static_cast<size_t>(y) * image_width + x] = DecodedPanorama::pack_texel(
							static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f) + 0.5f),
							static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f) + 0.5f),
							static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f) + 0.5f)
						);
					}
				}

				return result;
			}

			pos = seg_end;
		}

		return std::nullopt;
	}

	inline void undo_horizontal_predictor(uint8_t* row, size_t pixel_count, size_t samples, size_t bytes_per_sample, bool little_endian) noexcept {
		const size_t total = pixel_count * samples;
		if (bytes_per_sample == 1) {
			for (size_t i = samples; i < total; ++i) {
				row[i] = static_cast<uint8_t>(row[i] + row[i - samples]);
			}
			return;
		}
		auto load = [&](size_t index) noexcept -> uint16_t {
			const uint8_t* p = row + index * 2;
			return little_endian
				? static_cast<uint16_t>(p[0] | (p[1] << 8))
				: static_cast<uint16_t>((p[0] << 8) | p[1]);
		};
		auto store = [&](size_t index, uint16_t value) noexcept {
			uint8_t* p = row + index * 2;
			if (little_endian) {
				p[0] = static_cast<uint8_t>(value & 0xFFU);
				p[1] = static_cast<uint8_t>(value >> 8);
			} else {
				p[0] = static_cast<uint8_t>(value >> 8);
				p[1] = static_cast<uint8_t>(value & 0xFFU);
			}
		};
		for (size_t i = samples; i < total; ++i) {
			store(i, static_cast<uint16_t>(load(i) + load(i - samples)));
		}
	}

	[[nodiscard]] inline std::optional<DecodedPanorama> decode_tiff(std::span<const uint8_t> data) {
		if (data.size() < 8) return std::nullopt;
		bool little_endian = false;
		if (data[0] == 'I' && data[1] == 'I') {
			little_endian = true;
		} else if (!(data[0] == 'M' && data[1] == 'M')) {
			return std::nullopt;
		}

		auto read_u16 = [&](size_t off) noexcept -> uint16_t {
			if (off + 2 > data.size()) return 0;
			if (little_endian) return static_cast<uint16_t>(data[off] | (static_cast<uint16_t>(data[off + 1]) << 8));
			return static_cast<uint16_t>((static_cast<uint16_t>(data[off]) << 8) | data[off + 1]);
		};
		auto read_u32 = [&](size_t off) noexcept -> uint32_t {
			if (off + 4 > data.size()) return 0;
			if (little_endian) {
				return static_cast<uint32_t>(data[off]) | (static_cast<uint32_t>(data[off + 1]) << 8) | (static_cast<uint32_t>(data[off + 2]) << 16) | (static_cast<uint32_t>(data[off + 3]) << 24);
			}
			return (static_cast<uint32_t>(data[off]) << 24) | (static_cast<uint32_t>(data[off + 1]) << 16) | (static_cast<uint32_t>(data[off + 2]) << 8) | static_cast<uint32_t>(data[off + 3]);
		};

		if (read_u16(2) != 42) return std::nullopt;
		const size_t ifd_offset = read_u32(4);
		if (ifd_offset + 2 > data.size()) return std::nullopt;

		auto read_values = [&](uint16_t type, uint32_t count, size_t field_position) -> std::vector<uint32_t> {
			std::vector<uint32_t> values;
			const size_t element_size = (type == 1) ? 1 : (type == 3) ? 2 : (type == 4) ? 4 : 0;
			if (element_size == 0 || count == 0) return values;
			const size_t total_bytes = element_size * static_cast<size_t>(count);
			const size_t base = (total_bytes <= 4) ? field_position : static_cast<size_t>(read_u32(field_position));
			if (base > data.size() || total_bytes > data.size() - base) return values;
			values.reserve(count);
			for (size_t i = 0; i < count; ++i) {
				const size_t off = base + i * element_size;
				if (type == 1) values.push_back(data[off]);
				else if (type == 3) values.push_back(read_u16(off));
				else values.push_back(read_u32(off));
			}
			return values;
		};

		uint32_t image_width = 0;
		uint32_t image_height = 0;
		uint32_t compression = 1;
		uint32_t photometric = 2;
		uint32_t samples_per_pixel = 1;
		uint32_t planar_config = 1;
		uint32_t predictor = 1;
		uint32_t rows_per_strip = 0;
		uint32_t tile_width = 0;
		uint32_t tile_length = 0;
		uint32_t sample_format = 1;
		std::vector<uint32_t> bits_per_sample{8U};
		std::vector<uint32_t> strip_offsets;
		std::vector<uint32_t> strip_byte_counts;
		std::vector<uint32_t> tile_offsets;
		std::vector<uint32_t> tile_byte_counts;

		const uint16_t entry_count = read_u16(ifd_offset);
		for (size_t i = 0; i < entry_count; ++i) {
			const size_t entry_pos = ifd_offset + 2 + i * 12;
			if (entry_pos + 12 > data.size()) break;
			const uint16_t tag = read_u16(entry_pos);
			const uint16_t type = read_u16(entry_pos + 2);
			const uint32_t count = read_u32(entry_pos + 4);
			auto values = read_values(type, count, entry_pos + 8);
			if (values.empty()) continue;

			switch (tag) {
				case 256: image_width = values.front(); break;
				case 257: image_height = values.front(); break;
				case 258: bits_per_sample = std::move(values); break;
				case 259: compression = values.front(); break;
				case 262: photometric = values.front(); break;
				case 273: strip_offsets = std::move(values); break;
				case 277: samples_per_pixel = values.front(); break;
				case 278: rows_per_strip = values.front(); break;
				case 279: strip_byte_counts = std::move(values); break;
				case 284: planar_config = values.front(); break;
				case 317: predictor = values.front(); break;
				case 322: tile_width = values.front(); break;
				case 323: tile_length = values.front(); break;
				case 324: tile_offsets = std::move(values); break;
				case 325: tile_byte_counts = std::move(values); break;
				case 339: sample_format = values.front(); break;
				default: break;
			}
		}

		if (image_width == 0 || image_height == 0) return std::nullopt;
		if (static_cast<uint64_t>(image_width) * image_height > kMaxPanoramaPixels) return std::nullopt;
		if (planar_config != 1 || sample_format != 1 || (predictor != 1 && predictor != 2)) return std::nullopt;
		if (samples_per_pixel == 0 || samples_per_pixel > 8) return std::nullopt;
		if (compression != 1 && compression != 5 && compression != 8 && compression != 32946 && compression != 32773) return std::nullopt;

		const uint32_t sample_bits = bits_per_sample.front();
		for (const uint32_t bits : bits_per_sample) {
			if (bits != sample_bits) return std::nullopt;
		}
		if (sample_bits != 8 && sample_bits != 16) return std::nullopt;
		if (photometric == 2) {
			if (samples_per_pixel < 3) return std::nullopt;
		} else if (photometric != 0 && photometric != 1) {
			return std::nullopt;
		}

		const size_t bytes_per_sample = sample_bits / 8U;
		const size_t pixel_bytes = static_cast<size_t>(samples_per_pixel) * bytes_per_sample;
		const size_t row_bytes = static_cast<size_t>(image_width) * pixel_bytes;
		std::vector<uint8_t> raw(row_bytes * image_height, 0);
		std::vector<uint8_t> scratch;

		auto decode_block = [&](size_t offset, size_t byte_count, uint32_t block_x, uint32_t block_y, uint32_t block_width, uint32_t block_height) -> bool {
			const size_t block_row_bytes = static_cast<size_t>(block_width) * pixel_bytes;
			const size_t expected = block_row_bytes * block_height;
			if (compression == 1 && byte_count == 0) byte_count = expected;
			if (byte_count == 0 || offset > data.size() || byte_count > data.size() - offset) return false;

			scratch.assign(expected, 0);
			const std::span<const uint8_t> source = data.subspan(offset, byte_count);
			size_t produced = 0;
			switch (compression) {
				case 1:
					produced = std::min(expected, byte_count);
					std::memcpy(scratch.data(), source.data(), produced);
					break;
				case 5:
					produced = decompress_lzw(source, scratch.data(), expected);
					break;
				case 8:
				case 32946:
					produced = inflate_zlib(source, scratch.data(), expected);
					break;
				case 32773:
					produced = decompress_packbits(source, scratch.data(), expected);
					break;
				default:
					return false;
			}
			if (produced == 0) return false;

			if (predictor == 2) {
				for (uint32_t r = 0; r < block_height; ++r) {
					undo_horizontal_predictor(scratch.data() + static_cast<size_t>(r) * block_row_bytes, block_width, samples_per_pixel, bytes_per_sample, little_endian);
				}
			}

			const size_t copy_rows = std::min<size_t>(block_height, image_height - block_y);
			const size_t copy_bytes = std::min<size_t>(block_width, image_width - block_x) * pixel_bytes;
			for (size_t r = 0; r < copy_rows; ++r) {
				std::memcpy(
					raw.data() + ((static_cast<size_t>(block_y) + r) * image_width + block_x) * pixel_bytes,
					scratch.data() + r * block_row_bytes,
					copy_bytes
				);
			}
			return true;
		};

		if (tile_width > 0 && tile_length > 0 && !tile_offsets.empty()) {
			const uint32_t tiles_across = (image_width + tile_width - 1) / tile_width;
			const uint32_t tiles_down = (image_height + tile_length - 1) / tile_length;
			if (tile_offsets.size() < static_cast<size_t>(tiles_across) * tiles_down) return std::nullopt;
			for (uint32_t ty = 0; ty < tiles_down; ++ty) {
				for (uint32_t tx = 0; tx < tiles_across; ++tx) {
					const size_t index = static_cast<size_t>(ty) * tiles_across + tx;
					const size_t byte_count = (index < tile_byte_counts.size()) ? tile_byte_counts[index] : 0;
					if (!decode_block(tile_offsets[index], byte_count, tx * tile_width, ty * tile_length, tile_width, tile_length)) {
						return std::nullopt;
					}
				}
			}
		} else {
			if (strip_offsets.empty()) return std::nullopt;
			if (rows_per_strip == 0 || rows_per_strip > image_height) rows_per_strip = image_height;
			const size_t strip_count = (static_cast<size_t>(image_height) + rows_per_strip - 1) / rows_per_strip;
			if (strip_offsets.size() < strip_count) return std::nullopt;
			for (size_t s = 0; s < strip_count; ++s) {
				const uint32_t y0 = static_cast<uint32_t>(s * rows_per_strip);
				const uint32_t rows = std::min(rows_per_strip, image_height - y0);
				const size_t byte_count = (s < strip_byte_counts.size()) ? strip_byte_counts[s] : 0;
				if (!decode_block(strip_offsets[s], byte_count, 0, y0, image_width, rows)) {
					return std::nullopt;
				}
			}
		}

		DecodedPanorama result;
		result.width = image_width;
		result.height = image_height;
		result.texels.assign(static_cast<size_t>(image_width) * image_height, 0U);

		const size_t high_byte_offset = (bytes_per_sample == 2 && little_endian) ? 1 : 0;
		for (size_t y = 0; y < image_height; ++y) {
			for (size_t x = 0; x < image_width; ++x) {
				const size_t base = (y * image_width + x) * pixel_bytes + high_byte_offset;
				uint8_t r = raw[base];
				uint8_t g = r;
				uint8_t b = r;
				if (photometric == 2) {
					g = raw[base + bytes_per_sample];
					b = raw[base + 2 * bytes_per_sample];
				} else if (photometric == 0) {
					r = static_cast<uint8_t>(255U - r);
					g = r;
					b = r;
				}
				result.texels[y * image_width + x] = DecodedPanorama::pack_texel(r, g, b);
			}
		}

		return result;
	}

	[[nodiscard]] inline std::optional<std::vector<uint8_t>> read_asset_file(std::string_view relative_path) {
		namespace fs = std::filesystem;
		std::error_code ec;
		fs::path candidate{std::string(relative_path)};
		if (!fs::is_regular_file(candidate, ec)) {
			bool found = false;
			fs::path prefix;
			for (size_t depth = 1; depth <= 4 && !found; ++depth) {
				prefix /= "..";
				const fs::path attempt = (prefix / candidate).lexically_normal();
				if (fs::is_regular_file(attempt, ec)) {
					candidate = attempt;
					found = true;
				}
			}
			if (!found) return std::nullopt;
		}

		std::ifstream file(candidate, std::ios::binary | std::ios::ate);
		if (!file.is_open()) return std::nullopt;
		const std::streamsize size = file.tellg();
		if (size <= 0) return std::nullopt;
		std::vector<uint8_t> buffer(static_cast<size_t>(size));
		file.seekg(0);
		file.read(reinterpret_cast<char*>(buffer.data()), size);
		if (!file) return std::nullopt;
		return buffer;
	}

	[[nodiscard]] inline DecodedPanorama downscale_panorama_to_budget(DecodedPanorama source, uint32_t max_width) noexcept {
		if (source.width <= max_width || max_width == 0U) {
			return source;
		}
		const uint32_t new_width = max_width;
		const uint32_t new_height = std::max<uint32_t>(1U, static_cast<uint32_t>((static_cast<uint64_t>(source.height) * new_width) / source.width));
		DecodedPanorama result;
		result.width = new_width;
		result.height = new_height;
		result.texels.assign(static_cast<size_t>(new_width) * new_height, 0U);
		for (uint32_t y = 0; y < new_height; ++y) {
			const uint32_t sy0 = (y * source.height) / new_height;
			const uint32_t sy1 = std::min(source.height - 1, ((y + 1) * source.height) / new_height);
			for (uint32_t x = 0; x < new_width; ++x) {
				const uint32_t sx0 = (x * source.width) / new_width;
				const uint32_t sx1 = std::min(source.width - 1, ((x + 1) * source.width) / new_width);
				uint32_t sum_r = 0, sum_g = 0, sum_b = 0, sample_count = 0;
				for (uint32_t sy = sy0; sy <= sy1; ++sy) {
					for (uint32_t sx = sx0; sx <= sx1; ++sx) {
						const uint32_t texel = source.texels[static_cast<size_t>(sy) * source.width + sx];
						sum_r += texel & 0xFFU;
						sum_g += (texel >> 8) & 0xFFU;
						sum_b += (texel >> 16) & 0xFFU;
						++sample_count;
					}
				}
				sample_count = std::max<uint32_t>(sample_count, 1U);
				result.texels[static_cast<size_t>(y) * new_width + x] = DecodedPanorama::pack_texel(
					static_cast<uint8_t>(sum_r / sample_count),
					static_cast<uint8_t>(sum_g / sample_count),
					static_cast<uint8_t>(sum_b / sample_count)
				);
			}
		}
		return result;
	}

	[[nodiscard]] inline std::optional<DecodedPanorama> decode_panorama_file(std::string_view relative_path) {
		const auto bytes = read_asset_file(relative_path);
		if (!bytes.has_value()) return std::nullopt;

		const auto dot = relative_path.find_last_of('.');
		if (dot == std::string_view::npos) return std::nullopt;
		std::string ext(relative_path.substr(dot + 1));
		for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

		constexpr uint32_t kMaxPanoramaBudgetWidth = 2048U;
		std::optional<DecodedPanorama> decoded;
		if (ext == "jpg" || ext == "jpeg") {
			decoded = decode_jpeg(std::span<const uint8_t>(*bytes));
		} else if (ext == "tif" || ext == "tiff") {
			decoded = decode_tiff(std::span<const uint8_t>(*bytes));
		}
		if (decoded.has_value() && decoded->is_valid()) {
			return downscale_panorama_to_budget(std::move(*decoded), kMaxPanoramaBudgetWidth);
		}
		return std::nullopt;
	}

}

class SkyPanoramaLoader {
public:
	using ImageHandle = std::shared_ptr<const DecodedPanorama>;

private:
	static constexpr size_t kSlotCount = 2;
	static constexpr uint32_t kInvalidKey = 0xFFFFFFFFU;

	struct Slot {
		uint32_t key{kInvalidKey};
		uint64_t stamp{0};
		bool decoding{false};
		bool ready{false};
		ImageHandle image{};
	};

	std::mutex mutex_{};
	std::array<Slot, kSlotCount> slots_{};
	uint64_t stamp_counter_{0};

	SkyPanoramaLoader() = default;

	[[nodiscard]] static uint32_t make_key(SkyPanoramaId id, SkyPanoramaQuality quality) noexcept {
		const uint32_t id_value = std::min(static_cast<uint32_t>(id), static_cast<uint32_t>(SkyPanoramaId::Eso0932a));
		const uint32_t quality_value = std::min(static_cast<uint32_t>(quality), static_cast<uint32_t>(SkyPanoramaQuality::Q4K));
		const bool has_variants = sky_panorama_catalog_entry(static_cast<SkyPanoramaId>(id_value)).has_quality_variants;
		return (id_value << 4) | (has_variants ? quality_value : 0U);
	}

	[[nodiscard]] static ImageHandle decode_key(uint32_t key) noexcept {
		const auto id = static_cast<SkyPanoramaId>(key >> 4);
		const auto quality = static_cast<SkyPanoramaQuality>(key & 0x0FU);
		const std::string_view path = sky_panorama_relative_path(id, quality);
		ImageHandle image;
		try {
			auto decoded = PanoramaDecodeDetail::decode_panorama_file(path);
			if (decoded.has_value() && decoded->is_valid()) {
				image = std::make_shared<const DecodedPanorama>(std::move(*decoded));
			}
		} catch (...) {
			image.reset();
		}
		if (!image) {
			try {
				Core::log_error("Sky panorama could not be decoded, the procedural sky is used instead: " + std::string(path));
			} catch (...) {
			}
		}
		return image;
	}

public:
	static SkyPanoramaLoader& instance() noexcept {
		static SkyPanoramaLoader loader;
		return loader;
	}

	[[nodiscard]] ImageHandle try_acquire(SkyPanoramaId id, SkyPanoramaQuality quality) noexcept {
		const uint32_t key = make_key(id, quality);
		Slot* target = nullptr;
		bool need_decode = false;

		{
			std::lock_guard<std::mutex> lock(mutex_);
			++stamp_counter_;
			for (Slot& slot : slots_) {
				if (slot.key == key) {
					slot.stamp = stamp_counter_;
					target = &slot;
					break;
				}
			}
			if (target == nullptr) {
				Slot* victim = nullptr;
				for (Slot& slot : slots_) {
					if (slot.decoding) continue;
					if (victim == nullptr || slot.stamp < victim->stamp) victim = &slot;
				}
				if (victim == nullptr) {
					return nullptr;
				}
				victim->key = key;
				victim->stamp = stamp_counter_;
				victim->ready = false;
				victim->decoding = true;
				victim->image.reset();
				target = victim;
				need_decode = true;
			}
		}

		if (need_decode) {
			std::thread([this, target, key]() {
				auto decoded = decode_key(key);
				std::lock_guard<std::mutex> lock(mutex_);
				target->image = decoded;
				target->ready = true;
				target->decoding = false;
			}).detach();
			return nullptr;
		}

		std::lock_guard<std::mutex> lock(mutex_);
		return target->ready ? target->image : nullptr;
	}

	[[nodiscard]] std::optional<std::array<float, 3>> sample_direction(
		SkyPanoramaId id,
		SkyPanoramaQuality quality,
		double dir_x,
		double dir_y,
		double dir_z,
		double rotation_rad
	) noexcept {
		struct ThreadCache {
			uint32_t key{kInvalidKey};
			ImageHandle image{};
		};
		thread_local ThreadCache cache;

		const uint32_t key = make_key(id, quality);
		if (cache.key != key || !cache.image) {
			auto acquired = try_acquire(id, quality);
			if (acquired) {
				cache.image = std::move(acquired);
			}
			cache.key = key;
		}
		if (!cache.image) {
			return std::nullopt;
		}

		const double cos_r = std::cos(rotation_rad);
		const double sin_r = std::sin(rotation_rad);
		const double rx = dir_x * cos_r - dir_y * sin_r;
		const double ry = dir_x * sin_r + dir_y * cos_r;
		const double rz = dir_z;

		const double len = std::sqrt(rx * rx + ry * ry + rz * rz);
		if (len < 1e-12) return std::array<float, 3>{0.0f, 0.0f, 0.0f};
		const double nz = std::clamp(rz / len, -1.0, 1.0);
		const double theta = std::acos(nz);
		const double phi = std::atan2(ry, rx);

		const double u = (phi + std::numbers::pi_v<double>) * (1.0 / (2.0 * std::numbers::pi_v<double>));
		const double v = theta / std::numbers::pi_v<double>;

		return cache.image->sample_bilinear(u, v);
	}
};

}
