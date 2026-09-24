#pragma once

#include "relativistic/optics/sky_panorama_catalog.hpp"
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
#include <vector>

namespace Relativistic::Optics {

struct DecodedPanorama {
	uint32_t width{0};
	uint32_t height{0};
	std::vector<float> pixels{};

	[[nodiscard]] std::array<float, 3> sample_bilinear(double u, double v) const noexcept {
		if (width == 0 || height == 0 || pixels.empty()) {
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

		auto fetch = [&](int64_t x, int64_t y) noexcept -> std::array<float, 3> {
			const size_t idx = (static_cast<size_t>(y) * width + static_cast<size_t>(x)) * 3;
			return {pixels[idx], pixels[idx + 1], pixels[idx + 2]};
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

	[[nodiscard]] inline std::optional<DecodedPanorama> decode_jpeg(std::span<const uint8_t> data) noexcept {
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
				size_t p = seg_start;
				++p;
				image_height = (static_cast<uint32_t>(data[p]) << 8) | data[p + 1];
				p += 2;
				image_width = (static_cast<uint32_t>(data[p]) << 8) | data[p + 1];
				p += 2;
				component_count = data[p++];
				if (component_count == 0 || component_count > 4) return std::nullopt;
				for (uint32_t c = 0; c < component_count; ++c) {
					components[c].id = data[p++];
					const uint8_t hv = data[p++];
					components[c].h = static_cast<uint8_t>(hv >> 4);
					components[c].v = static_cast<uint8_t>(hv & 0x0FU);
					components[c].quant_table_id = data[p++];
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
				size_t p = seg_start;
				const uint8_t scan_components = data[p++];
				for (uint8_t s = 0; s < scan_components; ++s) {
					const uint8_t comp_id = data[p++];
					const uint8_t td_ta = data[p++];
					for (uint32_t c = 0; c < component_count; ++c) {
						if (components[c].id == comp_id) {
							components[c].dc_table_id = static_cast<uint8_t>(td_ta >> 4);
							components[c].ac_table_id = static_cast<uint8_t>(td_ta & 0x0FU);
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
				result.pixels.assign(static_cast<size_t>(image_width) * image_height * 3, 0.0f);

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

						const size_t idx = (static_cast<size_t>(y) * image_width + x) * 3;
						result.pixels[idx + 0] = std::clamp(r, 0.0f, 255.0f) / 255.0f;
						result.pixels[idx + 1] = std::clamp(g, 0.0f, 255.0f) / 255.0f;
						result.pixels[idx + 2] = std::clamp(b, 0.0f, 255.0f) / 255.0f;
					}
				}

				return result;
			}

			pos = seg_end;
		}

		return std::nullopt;
	}

	[[nodiscard]] inline std::optional<DecodedPanorama> decode_tiff(std::span<const uint8_t> data) noexcept {
		if (data.size() < 8) return std::nullopt;
		bool little_endian = false;
		if (data[0] == 'I' && data[1] == 'I') little_endian = true;
		else if (data[0] == 'M' && data[1] == 'M') little_endian = false;
		else return std::nullopt;

		auto read_u16 = [&](size_t off) noexcept -> uint16_t {
			if (little_endian) return static_cast<uint16_t>(data[off] | (static_cast<uint16_t>(data[off + 1]) << 8));
			return static_cast<uint16_t>((static_cast<uint16_t>(data[off]) << 8) | data[off + 1]);
		};
		auto read_u32 = [&](size_t off) noexcept -> uint32_t {
			if (little_endian) {
				return static_cast<uint32_t>(data[off]) | (static_cast<uint32_t>(data[off + 1]) << 8) | (static_cast<uint32_t>(data[off + 2]) << 16) | (static_cast<uint32_t>(data[off + 3]) << 24);
			}
			return (static_cast<uint32_t>(data[off]) << 24) | (static_cast<uint32_t>(data[off + 1]) << 16) | (static_cast<uint32_t>(data[off + 2]) << 8) | static_cast<uint32_t>(data[off + 3]);
		};

		if (read_u16(2) != 42) return std::nullopt;
		const uint32_t ifd_offset = read_u32(4);
		if (static_cast<size_t>(ifd_offset) + 2 > data.size()) return std::nullopt;

		uint32_t image_width = 0, image_height = 0;
		uint32_t bits_per_sample = 8;
		uint32_t samples_per_pixel = 1;
		uint32_t compression = 1;
		uint32_t photometric = 1;
		uint32_t planar_config = 1;
		uint32_t rows_per_strip = 0;
		std::vector<uint32_t> strip_offsets;
		std::vector<uint32_t> strip_byte_counts;

		auto read_tag_value_u32 = [&](uint16_t type, uint32_t count, uint32_t value_or_offset) noexcept -> uint32_t {
			if (type == 3 && count == 1) {
				return little_endian ? (value_or_offset & 0xFFFFU) : (value_or_offset >> 16);
			}
			return value_or_offset;
		};

		const uint16_t entry_count = read_u16(ifd_offset);
		size_t entry_pos = static_cast<size_t>(ifd_offset) + 2;

		for (uint16_t i = 0; i < entry_count; ++i) {
			if (entry_pos + 12 > data.size()) break;
			const uint16_t tag = read_u16(entry_pos);
			const uint16_t type = read_u16(entry_pos + 2);
			const uint32_t count = read_u32(entry_pos + 4);
			const uint32_t value_offset = read_u32(entry_pos + 8);

			switch (tag) {
				case 256: image_width = read_tag_value_u32(type, count, value_offset); break;
				case 257: image_height = read_tag_value_u32(type, count, value_offset); break;
				case 258: bits_per_sample = read_tag_value_u32(type, count, value_offset); break;
				case 259: compression = read_tag_value_u32(type, count, value_offset); break;
				case 262: photometric = read_tag_value_u32(type, count, value_offset); break;
				case 277: samples_per_pixel = read_tag_value_u32(type, count, value_offset); break;
				case 278: rows_per_strip = read_tag_value_u32(type, count, value_offset); break;
				case 284: planar_config = read_tag_value_u32(type, count, value_offset); break;
				case 273: {
					strip_offsets.resize(count);
					if (count == 1) {
						strip_offsets[0] = value_offset;
					} else if (static_cast<size_t>(value_offset) + static_cast<size_t>(count) * 4 <= data.size()) {
						for (uint32_t s = 0; s < count; ++s) strip_offsets[s] = read_u32(value_offset + s * 4);
					}
					break;
				}
				case 279: {
					strip_byte_counts.resize(count);
					if (count == 1) {
						strip_byte_counts[0] = value_offset;
					} else if (type == 3 && static_cast<size_t>(value_offset) + static_cast<size_t>(count) * 2 <= data.size()) {
						for (uint32_t s = 0; s < count; ++s) strip_byte_counts[s] = read_u16(value_offset + s * 2);
					} else if (static_cast<size_t>(value_offset) + static_cast<size_t>(count) * 4 <= data.size()) {
						for (uint32_t s = 0; s < count; ++s) strip_byte_counts[s] = read_u32(value_offset + s * 4);
					}
					break;
				}
				default: break;
			}
			entry_pos += 12;
		}

		if (image_width == 0 || image_height == 0 || strip_offsets.empty() || compression != 1 || planar_config != 1) {
			return std::nullopt;
		}
		if (bits_per_sample != 8 || (samples_per_pixel != 3 && samples_per_pixel != 4)) {
			return std::nullopt;
		}
		if (photometric != 2 && photometric != 0 && photometric != 1) {
			return std::nullopt;
		}
		if (rows_per_strip == 0) rows_per_strip = image_height;

		DecodedPanorama result;
		result.width = image_width;
		result.height = image_height;
		result.pixels.assign(static_cast<size_t>(image_width) * image_height * 3, 0.0f);

		uint32_t row_cursor = 0;
		for (size_t strip = 0; strip < strip_offsets.size() && row_cursor < image_height; ++strip) {
			const uint32_t offset = strip_offsets[strip];
			const uint32_t rows_in_strip = std::min(rows_per_strip, image_height - row_cursor);
			const size_t bytes_needed = static_cast<size_t>(rows_in_strip) * image_width * samples_per_pixel;
			if (static_cast<size_t>(offset) + bytes_needed > data.size()) break;

			for (uint32_t r = 0; r < rows_in_strip; ++r) {
				const uint32_t y = row_cursor + r;
				for (uint32_t x = 0; x < image_width; ++x) {
					const size_t src = static_cast<size_t>(offset) + (static_cast<size_t>(r) * image_width + x) * samples_per_pixel;
					const size_t dst = (static_cast<size_t>(y) * image_width + x) * 3;
					if (photometric == 0 || photometric == 1) {
						const float gray = static_cast<float>(data[src]) / 255.0f;
						const float g_final = (photometric == 0) ? (1.0f - gray) : gray;
						result.pixels[dst + 0] = g_final;
						result.pixels[dst + 1] = g_final;
						result.pixels[dst + 2] = g_final;
					} else {
						result.pixels[dst + 0] = static_cast<float>(data[src + 0]) / 255.0f;
						result.pixels[dst + 1] = static_cast<float>(data[src + 1]) / 255.0f;
						result.pixels[dst + 2] = static_cast<float>(data[src + 2]) / 255.0f;
					}
				}
			}
			row_cursor += rows_in_strip;
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

	[[nodiscard]] inline std::optional<DecodedPanorama> decode_panorama_file(std::string_view relative_path) {
		const auto bytes = read_asset_file(relative_path);
		if (!bytes.has_value()) return std::nullopt;

		const auto dot = relative_path.find_last_of('.');
		if (dot == std::string_view::npos) return std::nullopt;
		std::string ext(relative_path.substr(dot + 1));
		for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

		if (ext == "jpg" || ext == "jpeg") {
			return decode_jpeg(std::span<const uint8_t>(*bytes));
		}
		if (ext == "tif" || ext == "tiff") {
			return decode_tiff(std::span<const uint8_t>(*bytes));
		}
		return std::nullopt;
	}

}

class SkyPanoramaLoader {
private:
	SkyPanoramaId loaded_id_{SkyPanoramaId::NightSkyHDRI001};
	SkyPanoramaQuality loaded_quality_{SkyPanoramaQuality::Q2K};
	bool has_loaded_{false};
	DecodedPanorama image_{};
	std::mutex mutex_{};

	SkyPanoramaLoader() = default;

	void ensure_loaded(SkyPanoramaId id, SkyPanoramaQuality quality) noexcept {
		if (has_loaded_ && id == loaded_id_ && quality == loaded_quality_) {
			return;
		}
		const auto path = sky_panorama_relative_path(id, quality);
		auto decoded = PanoramaDecodeDetail::decode_panorama_file(path);
		if (decoded.has_value()) {
			image_ = std::move(*decoded);
			has_loaded_ = true;
		} else {
			has_loaded_ = false;
		}
		loaded_id_ = id;
		loaded_quality_ = quality;
	}

public:
	static SkyPanoramaLoader& instance() noexcept {
		static SkyPanoramaLoader loader;
		return loader;
	}

	[[nodiscard]] std::array<float, 3> sample_direction(
		SkyPanoramaId id,
		SkyPanoramaQuality quality,
		double dir_x,
		double dir_y,
		double dir_z,
		double rotation_rad
	) noexcept {
		std::lock_guard<std::mutex> lock(mutex_);
		ensure_loaded(id, quality);
		if (!has_loaded_) {
			return {0.0f, 0.0f, 0.0f};
		}

		const double cos_r = std::cos(rotation_rad);
		const double sin_r = std::sin(rotation_rad);
		const double rx = dir_x * cos_r - dir_y * sin_r;
		const double ry = dir_x * sin_r + dir_y * cos_r;
		const double rz = dir_z;

		const double len = std::sqrt(rx * rx + ry * ry + rz * rz);
		if (len < 1e-12) return {0.0f, 0.0f, 0.0f};
		const double nz = std::clamp(rz / len, -1.0, 1.0);
		const double theta = std::acos(nz);
		const double phi = std::atan2(ry, rx);

		const double u = (phi + std::numbers::pi_v<double>) * (1.0 / (2.0 * std::numbers::pi_v<double>));
		const double v = theta / std::numbers::pi_v<double>;

		return image_.sample_bilinear(u, v);
	}
};

}
