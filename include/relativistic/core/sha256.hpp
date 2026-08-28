#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <span>
#include <bit>

namespace Relativistic::Core {

class SHA256 {
private:
	std::array<uint32_t, 8> state_;
	std::array<uint8_t, 64> buffer_;
	uint64_t total_bits_;
	size_t buffer_length_;

	static constexpr std::array<uint32_t, 64> K = {
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
		0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
		0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
		0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
		0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
		0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
		0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
	};

	static constexpr uint32_t rotr(uint32_t x, uint32_t n) noexcept {
		return (x >> n) | (x << (32 - n));
	}
	static constexpr uint32_t ch(uint32_t x, uint32_t y, uint32_t z) noexcept {
		return (x & y) ^ (~x & z);
	}
	static constexpr uint32_t maj(uint32_t x, uint32_t y, uint32_t z) noexcept {
		return (x & y) ^ (x & z) ^ (y & z);
	}
	static constexpr uint32_t Sigma0(uint32_t x) noexcept {
		return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
	}
	static constexpr uint32_t Sigma1(uint32_t x) noexcept {
		return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
	}
	static constexpr uint32_t sigma0(uint32_t x) noexcept {
		return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
	}
	static constexpr uint32_t sigma1(uint32_t x) noexcept {
		return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
	}

	constexpr void process_block(const uint8_t* block) noexcept {
		std::array<uint32_t, 64> w{};
		for (size_t t = 0; t < 16; ++t) {
			w[t] = (static_cast<uint32_t>(block[t * 4]) << 24) |
				   (static_cast<uint32_t>(block[t * 4 + 1]) << 16) |
				   (static_cast<uint32_t>(block[t * 4 + 2]) << 8) |
				   (static_cast<uint32_t>(block[t * 4 + 3]));
		}
		for (size_t t = 16; t < 64; ++t) {
			w[t] = sigma1(w[t - 2]) + w[t - 7] + sigma0(w[t - 15]) + w[t - 16];
		}

		uint32_t a = state_[0];
		uint32_t b = state_[1];
		uint32_t c = state_[2];
		uint32_t d = state_[3];
		uint32_t e = state_[4];
		uint32_t f = state_[5];
		uint32_t g = state_[6];
		uint32_t h = state_[7];

		for (size_t t = 0; t < 64; ++t) {
			const uint32_t temp1 = h + Sigma1(e) + ch(e, f, g) + K[t] + w[t];
			const uint32_t temp2 = Sigma0(a) + maj(a, b, c);
			h = g;
			g = f;
			f = e;
			e = d + temp1;
			d = c;
			c = b;
			b = a;
			a = temp1 + temp2;
		}

		state_[0] += a;
		state_[1] += b;
		state_[2] += c;
		state_[3] += d;
		state_[4] += e;
		state_[5] += f;
		state_[6] += g;
		state_[7] += h;
	}

public:
	constexpr SHA256() noexcept : buffer_{}, total_bits_(0), buffer_length_(0) {
		reset();
	}

	constexpr void reset() noexcept {
		state_[0] = 0x6a09e667;
		state_[1] = 0xbb67ae85;
		state_[2] = 0x3c6ef372;
		state_[3] = 0xa54ff53a;
		state_[4] = 0x510e527f;
		state_[5] = 0x9b05688c;
		state_[6] = 0x1f83d9ab;
		state_[7] = 0x5be0cd19;
		total_bits_ = 0;
		buffer_length_ = 0;
		buffer_.fill(0);
	}

	constexpr void update(std::span<const uint8_t> data) noexcept {
		size_t i = 0;
		while (i < data.size()) {
			const size_t space = 64 - buffer_length_;
			const size_t to_copy = (data.size() - i < space) ? (data.size() - i) : space;
			for (size_t j = 0; j < to_copy; ++j) {
				buffer_[buffer_length_ + j] = data[i + j];
			}
			buffer_length_ += to_copy;
			total_bits_ += to_copy * 8;
			i += to_copy;

			if (buffer_length_ == 64) {
				process_block(buffer_.data());
				buffer_length_ = 0;
			}
		}
	}

	constexpr void update(std::span<const std::byte> data) noexcept {
		update(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
	}

	template <typename T>
	constexpr void update_value(const T& value) noexcept {
		update(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(T)));
	}

	[[nodiscard]] constexpr std::array<uint8_t, 32> finalize() noexcept {
		buffer_[buffer_length_] = 0x80;
		buffer_length_++;

		if (buffer_length_ > 56) {
			while (buffer_length_ < 64) {
				buffer_[buffer_length_++] = 0x00;
			}
			process_block(buffer_.data());
			buffer_length_ = 0;
		}

		while (buffer_length_ < 56) {
			buffer_[buffer_length_++] = 0x00;
		}

		for (size_t i = 0; i < 8; ++i) {
			buffer_[63 - i] = static_cast<uint8_t>(total_bits_ >> (i * 8));
		}

		process_block(buffer_.data());

		std::array<uint8_t, 32> hash{};
		for (size_t i = 0; i < 8; ++i) {
			hash[i * 4] = static_cast<uint8_t>(state_[i] >> 24);
			hash[i * 4 + 1] = static_cast<uint8_t>(state_[i] >> 16);
			hash[i * 4 + 2] = static_cast<uint8_t>(state_[i] >> 8);
			hash[i * 4 + 3] = static_cast<uint8_t>(state_[i]);
		}
		return hash;
	}
};

}
