#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace Relativistic::Optics::PanoramaDecodeDetail {

struct DeflateBitReader {
	std::span<const uint8_t> data;
	size_t pos{0};
	uint32_t bit_buffer{0};
	uint32_t bit_count{0};
	bool exhausted{false};

	explicit DeflateBitReader(std::span<const uint8_t> input) noexcept : data(input) {}

	[[nodiscard]] uint32_t read_bits(uint32_t count) noexcept {
		while (bit_count < count) {
			if (pos >= data.size()) {
				exhausted = true;
				return 0;
			}
			bit_buffer |= static_cast<uint32_t>(data[pos++]) << bit_count;
			bit_count += 8;
		}
		const uint32_t value = bit_buffer & ((1U << count) - 1U);
		bit_buffer >>= count;
		bit_count -= count;
		return value;
	}

	void align_to_byte() noexcept {
		bit_buffer = 0;
		bit_count = 0;
	}
};

struct DeflateHuffman {
	std::array<uint16_t, 16> counts{};
	std::array<uint16_t, 288> symbols{};

	[[nodiscard]] bool build(const uint8_t* lengths, size_t symbol_count) noexcept {
		counts.fill(0);
		for (size_t i = 0; i < symbol_count; ++i) {
			++counts[lengths[i]];
		}
		if (counts[0] == symbol_count) {
			return true;
		}
		int left = 1;
		for (size_t len = 1; len < counts.size(); ++len) {
			left <<= 1;
			left -= static_cast<int>(counts[len]);
			if (left < 0) {
				return false;
			}
		}
		std::array<uint16_t, 16> offsets{};
		for (size_t len = 1; len < 15; ++len) {
			offsets[len + 1] = static_cast<uint16_t>(offsets[len] + counts[len]);
		}
		for (size_t i = 0; i < symbol_count; ++i) {
			if (lengths[i] != 0) {
				symbols[offsets[lengths[i]]++] = static_cast<uint16_t>(i);
			}
		}
		return true;
	}

	[[nodiscard]] int decode(DeflateBitReader& reader) const noexcept {
		int code = 0;
		int first = 0;
		int index = 0;
		for (size_t len = 1; len < counts.size(); ++len) {
			code |= static_cast<int>(reader.read_bits(1));
			const int count = static_cast<int>(counts[len]);
			if (code - count < first) {
				return static_cast<int>(symbols[static_cast<size_t>(index + (code - first))]);
			}
			index += count;
			first += count;
			first <<= 1;
			code <<= 1;
		}
		return -1;
	}
};

[[nodiscard]] inline size_t inflate_raw(std::span<const uint8_t> input, uint8_t* output, size_t capacity) noexcept {
	static constexpr std::array<uint16_t, 29> kLengthBase{3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
	static constexpr std::array<uint16_t, 29> kLengthExtra{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
	static constexpr std::array<uint16_t, 30> kDistanceBase{1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
	static constexpr std::array<uint16_t, 30> kDistanceExtra{0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
	static constexpr std::array<uint8_t, 19> kCodeLengthOrder{16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

	DeflateBitReader reader(input);
	size_t written = 0;
	bool final_block = false;

	while (!final_block) {
		final_block = reader.read_bits(1) != 0U;
		const uint32_t block_type = reader.read_bits(2);
		if (reader.exhausted) {
			return written;
		}

		if (block_type == 0U) {
			reader.align_to_byte();
			if (reader.pos + 4 > input.size()) {
				return written;
			}
			const size_t length = static_cast<size_t>(input[reader.pos]) | (static_cast<size_t>(input[reader.pos + 1]) << 8);
			reader.pos += 4;
			if (reader.pos + length > input.size()) {
				return written;
			}
			const size_t copy_count = std::min(length, capacity - written);
			std::memcpy(output + written, input.data() + reader.pos, copy_count);
			written += copy_count;
			reader.pos += length;
			if (copy_count < length) {
				return written;
			}
			continue;
		}

		if (block_type == 3U) {
			return written;
		}

		DeflateHuffman literal_table;
		DeflateHuffman distance_table;

		if (block_type == 1U) {
			std::array<uint8_t, 288> lengths{};
			for (size_t i = 0; i < 144; ++i) lengths[i] = 8;
			for (size_t i = 144; i < 256; ++i) lengths[i] = 9;
			for (size_t i = 256; i < 280; ++i) lengths[i] = 7;
			for (size_t i = 280; i < 288; ++i) lengths[i] = 8;
			static_cast<void>(literal_table.build(lengths.data(), 288));
			std::array<uint8_t, 30> distance_lengths{};
			distance_lengths.fill(5);
			static_cast<void>(distance_table.build(distance_lengths.data(), 30));
		} else {
			const size_t literal_count = static_cast<size_t>(reader.read_bits(5)) + 257;
			const size_t distance_count = static_cast<size_t>(reader.read_bits(5)) + 1;
			const size_t code_length_count = static_cast<size_t>(reader.read_bits(4)) + 4;
			if (reader.exhausted || literal_count > 286 || distance_count > 30) {
				return written;
			}

			std::array<uint8_t, 19> code_length_lengths{};
			for (size_t i = 0; i < code_length_count; ++i) {
				code_length_lengths[kCodeLengthOrder[i]] = static_cast<uint8_t>(reader.read_bits(3));
			}
			DeflateHuffman code_length_table;
			if (!code_length_table.build(code_length_lengths.data(), 19)) {
				return written;
			}

			std::array<uint8_t, 320> lengths{};
			const size_t total = literal_count + distance_count;
			size_t index = 0;
			while (index < total) {
				const int symbol = code_length_table.decode(reader);
				if (symbol < 0 || reader.exhausted) {
					return written;
				}
				if (symbol < 16) {
					lengths[index++] = static_cast<uint8_t>(symbol);
					continue;
				}
				uint8_t previous = 0;
				size_t repeat = 0;
				if (symbol == 16) {
					if (index == 0) {
						return written;
					}
					previous = lengths[index - 1];
					repeat = 3 + static_cast<size_t>(reader.read_bits(2));
				} else if (symbol == 17) {
					repeat = 3 + static_cast<size_t>(reader.read_bits(3));
				} else {
					repeat = 11 + static_cast<size_t>(reader.read_bits(7));
				}
				if (index + repeat > total) {
					return written;
				}
				for (size_t i = 0; i < repeat; ++i) {
					lengths[index++] = previous;
				}
			}

			if (!literal_table.build(lengths.data(), literal_count) || !distance_table.build(lengths.data() + literal_count, distance_count)) {
				return written;
			}
		}

		for (;;) {
			const int symbol = literal_table.decode(reader);
			if (symbol < 0 || reader.exhausted) {
				return written;
			}
			if (symbol < 256) {
				if (written >= capacity) {
					return written;
				}
				output[written++] = static_cast<uint8_t>(symbol);
				continue;
			}
			if (symbol == 256) {
				break;
			}

			const size_t length_index = static_cast<size_t>(symbol - 257);
			if (length_index >= kLengthBase.size()) {
				return written;
			}
			size_t length = static_cast<size_t>(kLengthBase[length_index]) + reader.read_bits(kLengthExtra[length_index]);
			const int distance_symbol = distance_table.decode(reader);
			if (distance_symbol < 0 || static_cast<size_t>(distance_symbol) >= kDistanceBase.size() || reader.exhausted) {
				return written;
			}
			const size_t distance_index = static_cast<size_t>(distance_symbol);
			const size_t distance = static_cast<size_t>(kDistanceBase[distance_index]) + reader.read_bits(kDistanceExtra[distance_index]);
			if (distance > written) {
				return written;
			}
			const bool truncated = written + length > capacity;
			if (truncated) {
				length = capacity - written;
			}
			for (size_t i = 0; i < length; ++i) {
				output[written + i] = output[written + i - distance];
			}
			written += length;
			if (truncated) {
				return written;
			}
		}
	}

	return written;
}

[[nodiscard]] inline size_t inflate_zlib(std::span<const uint8_t> input, uint8_t* output, size_t capacity) noexcept {
	if (input.size() < 2) {
		return 0;
	}
	const bool valid_header = ((input[0] & 0x0FU) == 8U) && ((((static_cast<uint32_t>(input[0]) << 8) | input[1]) % 31U) == 0U);
	const size_t skip = valid_header ? (((input[1] & 0x20U) != 0U) ? size_t{6} : size_t{2}) : size_t{0};
	if (skip >= input.size()) {
		return 0;
	}
	return inflate_raw(input.subspan(skip), output, capacity);
}

[[nodiscard]] inline size_t decompress_lzw(std::span<const uint8_t> input, uint8_t* output, size_t capacity) noexcept {
	constexpr uint32_t kClearCode = 256;
	constexpr uint32_t kEndCode = 257;
	constexpr uint32_t kFirstFree = 258;
	constexpr uint32_t kMaxCodes = 4096;
	constexpr uint32_t kNoPrefix = 0xFFFFU;

	std::array<uint16_t, kMaxCodes> prefix{};
	std::array<uint8_t, kMaxCodes> suffix{};
	std::array<uint8_t, kMaxCodes> first_byte{};
	std::array<uint32_t, kMaxCodes> length{};
	for (uint32_t i = 0; i < 256; ++i) {
		prefix[i] = static_cast<uint16_t>(kNoPrefix);
		suffix[i] = static_cast<uint8_t>(i);
		first_byte[i] = static_cast<uint8_t>(i);
		length[i] = 1;
	}

	uint32_t next_code = kFirstFree;
	uint32_t width = 9;
	int64_t previous = -1;
	uint64_t bit_buffer = 0;
	uint32_t bit_count = 0;
	size_t input_pos = 0;
	size_t output_pos = 0;

	auto emit = [&](uint32_t code) noexcept {
		const size_t code_length = length[code];
		size_t index = output_pos + code_length;
		uint32_t current = code;
		while (current != kNoPrefix) {
			--index;
			if (index < capacity) {
				output[index] = suffix[current];
			}
			current = prefix[current];
		}
		output_pos += code_length;
	};

	auto add_entry = [&](uint32_t parent, uint8_t byte) noexcept {
		prefix[next_code] = static_cast<uint16_t>(parent);
		suffix[next_code] = byte;
		first_byte[next_code] = first_byte[parent];
		length[next_code] = length[parent] + 1;
		++next_code;
	};

	while (output_pos < capacity) {
		while (bit_count < width && input_pos < input.size()) {
			bit_buffer = (bit_buffer << 8) | input[input_pos++];
			bit_count += 8;
		}
		if (bit_count < width) {
			break;
		}
		const uint32_t code = static_cast<uint32_t>((bit_buffer >> (bit_count - width)) & ((1ULL << width) - 1ULL));
		bit_count -= width;

		if (code == kEndCode) {
			break;
		}
		if (code == kClearCode) {
			next_code = kFirstFree;
			width = 9;
			previous = -1;
			continue;
		}
		if (previous < 0) {
			if (code >= kClearCode) {
				break;
			}
			emit(code);
			previous = static_cast<int64_t>(code);
			continue;
		}

		const uint32_t parent = static_cast<uint32_t>(previous);
		if (code < next_code) {
			emit(code);
			if (next_code < kMaxCodes) {
				add_entry(parent, first_byte[code]);
			}
		} else if (code == next_code && next_code < kMaxCodes) {
			add_entry(parent, first_byte[parent]);
			emit(code);
		} else {
			break;
		}
		previous = static_cast<int64_t>(code);
		if (next_code + 1 >= (1U << width) && width < 12) {
			++width;
		}
	}

	return std::min(output_pos, capacity);
}

[[nodiscard]] inline size_t decompress_packbits(std::span<const uint8_t> input, uint8_t* output, size_t capacity) noexcept {
	size_t input_pos = 0;
	size_t output_pos = 0;
	while (input_pos < input.size() && output_pos < capacity) {
		const int header = static_cast<int8_t>(input[input_pos++]);
		if (header >= 0) {
			const size_t run = static_cast<size_t>(header) + 1;
			const size_t copy_count = std::min({run, input.size() - input_pos, capacity - output_pos});
			std::memcpy(output + output_pos, input.data() + input_pos, copy_count);
			input_pos += run;
			output_pos += copy_count;
		} else if (header != -128) {
			if (input_pos >= input.size()) {
				break;
			}
			const size_t run = static_cast<size_t>(1 - header);
			const uint8_t value = input[input_pos++];
			const size_t fill_count = std::min(run, capacity - output_pos);
			std::memset(output + output_pos, value, fill_count);
			output_pos += fill_count;
		}
	}
	return output_pos;
}

}
