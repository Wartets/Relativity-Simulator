#include "relativistic/distributed/crypto_channel.hpp"
#include "relativistic/core/pcg64.hpp"
#include <cstring>
#include <algorithm>
#include <vector>

namespace Relativistic::Distributed {

namespace {

[[nodiscard]] constexpr uint32_t rotl32(uint32_t x, int n) noexcept {
	return (x << n) | (x >> (32 - n));
}

[[nodiscard]] constexpr uint32_t load32_le(const uint8_t* p) noexcept {
	return static_cast<uint32_t>(p[0]) |
		(static_cast<uint32_t>(p[1]) << 8) |
		(static_cast<uint32_t>(p[2]) << 16) |
		(static_cast<uint32_t>(p[3]) << 24);
}

void store32_le(uint8_t* p, uint32_t x) noexcept {
	p[0] = static_cast<uint8_t>(x);
	p[1] = static_cast<uint8_t>(x >> 8);
	p[2] = static_cast<uint8_t>(x >> 16);
	p[3] = static_cast<uint8_t>(x >> 24);
}

void chacha_quarter_round(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) noexcept {
	a += b; d ^= a; d = rotl32(d, 16);
	c += d; b ^= c; b = rotl32(b, 12);
	a += b; d ^= a; d = rotl32(d, 8);
	c += d; b ^= c; b = rotl32(b, 7);
}

void x25519_cswap(uint32_t swap, uint32_t* a, uint32_t* b) noexcept {
	uint32_t mask = 0 - swap;
	for (size_t i = 0; i < 8; ++i) {
		uint32_t dummy = mask & (a[i] ^ b[i]);
		a[i] ^= dummy;
		b[i] ^= dummy;
	}
}

void x25519_add(uint32_t* out, const uint32_t* a, const uint32_t* b) noexcept {
	uint64_t carry = 0;
	for (size_t i = 0; i < 8; ++i) {
		carry += static_cast<uint64_t>(a[i]) + b[i];
		out[i] = static_cast<uint32_t>(carry);
		carry >>= 32;
	}
	carry = (carry * 38) + out[0];
	out[0] = static_cast<uint32_t>(carry);
	carry >>= 32;
	for (size_t i = 1; i < 8 && carry > 0; ++i) {
		carry += out[i];
		out[i] = static_cast<uint32_t>(carry);
		carry >>= 32;
	}
}

void x25519_sub(uint32_t* out, const uint32_t* a, const uint32_t* b) noexcept {
	uint64_t borrow = 0;
	for (size_t i = 0; i < 8; ++i) {
		borrow = static_cast<uint64_t>(a[i]) - b[i] - ((borrow >> 63) & 1);
		out[i] = static_cast<uint32_t>(borrow);
	}
	uint64_t carry = static_cast<uint64_t>(out[0]) - (((borrow >> 63) & 1) * 38);
	out[0] = static_cast<uint32_t>(carry);
	carry = (carry >> 63) & 1;
	for (size_t i = 1; i < 8 && carry > 0; ++i) {
		carry = static_cast<uint64_t>(out[i]) - carry;
		out[i] = static_cast<uint32_t>(carry);
		carry = (carry >> 63) & 1;
	}
}

void x25519_mul(uint32_t* out, const uint32_t* a, const uint32_t* b) noexcept {
	uint64_t product[16] = {0};
	for (size_t i = 0; i < 8; ++i) {
		for (size_t j = 0; j < 8; ++j) {
			product[i + j] += static_cast<uint64_t>(a[i]) * b[j];
		}
	}
	for (size_t i = 0; i < 15; ++i) {
		product[i + 1] += product[i] >> 32;
		product[i] &= 0xFFFFFFFFULL;
	}
	for (size_t i = 0; i < 8; ++i) {
		product[i] += product[i + 8] * 38;
	}
	for (size_t i = 0; i < 8; ++i) {
		product[i + 1] += product[i] >> 32;
		product[i] &= 0xFFFFFFFFULL;
	}
	product[0] += (product[8] * 38);
	product[1] += product[0] >> 32;
	product[0] &= 0xFFFFFFFFULL;
	for (size_t i = 0; i < 8; ++i) {
		out[i] = static_cast<uint32_t>(product[i]);
	}
}

void x25519_sqr(uint32_t* out, const uint32_t* a) noexcept {
	x25519_mul(out, a, a);
}

void x25519_inv(uint32_t* out, const uint32_t* a) noexcept {
	uint32_t t1[8], t2[8], t3[8];
	x25519_sqr(t1, a);
	x25519_sqr(t2, t1);
	x25519_mul(t2, t2, a);
	x25519_mul(t1, t1, t2);
	x25519_sqr(t3, t1);
	x25519_mul(t3, t3, t2);
	for (int i = 0; i < 4; ++i) x25519_sqr(t3, t3);
	x25519_mul(t3, t3, t1);
	for (int i = 0; i < 5; ++i) x25519_sqr(t1, t1);
	for (int i = 0; i < 5; ++i) x25519_sqr(t3, t3);
	for (int i = 0; i < 10; ++i) x25519_sqr(t1, t1);
	for (int i = 0; i < 20; ++i) x25519_sqr(t3, t3);
	for (int i = 0; i < 10; ++i) x25519_sqr(t1, t1);
	for (int i = 0; i < 50; ++i) x25519_sqr(t3, t3);
	for (int i = 0; i < 100; ++i) x25519_sqr(t1, t1);
	for (int i = 0; i < 50; ++i) x25519_sqr(t3, t3);
	for (int i = 0; i < 5; ++i) x25519_sqr(t1, t1);
	x25519_sqr(out, t1);
	x25519_sqr(out, out);
	x25519_mul(out, out, a);
}

void x25519_scalarmult(uint8_t* out, const uint8_t* scalar, const uint8_t* point) noexcept {
	uint8_t e[32];
	std::memcpy(e, scalar, 32);
	e[0] &= 248;
	e[31] &= 127;
	e[31] |= 64;

	uint32_t x1[8];
	for (size_t i = 0; i < 8; ++i) x1[i] = load32_le(point + i * 4);

	uint32_t x2[8] = {1,0,0,0,0,0,0,0}, z2[8] = {0}, x3[8], z3[8] = {1,0,0,0,0,0,0,0};
	for (size_t i = 0; i < 8; ++i) x3[i] = x1[i];

	uint32_t swap = 0;
	uint32_t a[8], b[8], c[8], d[8], e_temp[8], aa[8], bb[8], da[8], cb[8];

	for (int pos = 254; pos >= 0; --pos) {
		uint32_t b_bit = (e[pos / 8] >> (pos % 8)) & 1;
		swap ^= b_bit;
		x25519_cswap(swap, x2, x3);
		x25519_cswap(swap, z2, z3);
		swap = b_bit;

		x25519_add(a, x2, z2);
		x25519_sqr(aa, a);
		x25519_sub(b, x2, z2);
		x25519_sqr(bb, b);
		x25519_sub(e_temp, aa, bb);
		x25519_add(c, x3, z3);
		x25519_sub(d, x3, z3);
		x25519_mul(da, d, a);
		x25519_mul(cb, c, b);
		x25519_add(a, da, cb);
		x25519_sqr(x3, a);
		x25519_sub(b, da, cb);
		x25519_sqr(z3, b);
		x25519_mul(z3, z3, x1);
		x25519_mul(x2, aa, bb);
		x25519_add(c, bb, e_temp);
		x25519_add(c, c, e_temp);
		x25519_mul(c, c, e_temp);
		x25519_add(z2, z2, c);
	}

	x25519_cswap(swap, x2, x3);
	x25519_cswap(swap, z2, z3);

	uint32_t inv_z2[8];
	x25519_inv(inv_z2, z2);
	uint32_t res[8];
	x25519_mul(res, x2, inv_z2);

	for (size_t i = 0; i < 8; ++i) store32_le(out + i * 4, res[i]);
}

class InternalSHA256 {
private:
	uint32_t h[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
	uint8_t buffer[64];
	size_t buffer_len = 0;
	uint64_t total_len = 0;

	void process_block(const uint8_t* block) noexcept {
		uint32_t w[64];
		const uint32_t k[64] = {
			0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
			0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
			0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
			0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
			0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
			0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
			0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
			0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
		};
		auto right_rot = [](uint32_t v, uint32_t c) { return (v >> c) | (v << (32 - c)); };

		for (size_t j = 0; j < 16; ++j) {
			w[j] = (static_cast<uint32_t>(block[j * 4]) << 24) | (static_cast<uint32_t>(block[j * 4 + 1]) << 16) | (static_cast<uint32_t>(block[j * 4 + 2]) << 8) | static_cast<uint32_t>(block[j * 4 + 3]);
		}
		for (size_t j = 16; j < 64; ++j) {
			uint32_t s0 = right_rot(w[j - 15], 7) ^ right_rot(w[j - 15], 18) ^ (w[j - 15] >> 3);
			uint32_t s1 = right_rot(w[j - 2], 17) ^ right_rot(w[j - 2], 19) ^ (w[j - 2] >> 10);
			w[j] = w[j - 16] + s0 + w[j - 7] + s1;
		}
		uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], h_val = h[7];
		for (size_t j = 0; j < 64; ++j) {
			uint32_t S1 = right_rot(e, 6) ^ right_rot(e, 11) ^ right_rot(e, 25);
			uint32_t ch = (e & f) ^ ((~e) & g);
			uint32_t temp1 = h_val + S1 + ch + k[j] + w[j];
			uint32_t S0 = right_rot(a, 2) ^ right_rot(a, 13) ^ right_rot(a, 22);
			uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
			uint32_t temp2 = S0 + maj;
			h_val = g; g = f; f = e; e = d + temp1;
			d = c; c = b; b = a; a = temp1 + temp2;
		}
		h[0] += a; h[1] += b; h[2] += c; h[3] += d;
		h[4] += e; h[5] += f; h[6] += g; h[7] += h_val;
	}

public:
	void update(const uint8_t* data, size_t len) noexcept {
		for (size_t i = 0; i < len; ++i) {
			buffer[buffer_len++] = data[i];
			if (buffer_len == 64) {
				process_block(buffer);
				buffer_len = 0;
				total_len += 64;
			}
		}
	}

	void finalize(std::span<uint8_t, 32> out) noexcept {
		total_len += buffer_len;
		buffer[buffer_len++] = 0x80;
		if (buffer_len > 56) {
			while (buffer_len < 64) buffer[buffer_len++] = 0x00;
			process_block(buffer);
			buffer_len = 0;
		}
		while (buffer_len < 56) buffer[buffer_len++] = 0x00;
		uint64_t bits = total_len * 8;
		for (int i = 7; i >= 0; --i) {
			buffer[56 + i] = static_cast<uint8_t>(bits & 0xFF);
			bits >>= 8;
		}
		process_block(buffer);
		for (size_t i = 0; i < 8; ++i) {
			out[i * 4] = static_cast<uint8_t>((h[i] >> 24) & 0xFF);
			out[i * 4 + 1] = static_cast<uint8_t>((h[i] >> 16) & 0xFF);
			out[i * 4 + 2] = static_cast<uint8_t>((h[i] >> 8) & 0xFF);
			out[i * 4 + 3] = static_cast<uint8_t>(h[i] & 0xFF);
		}
	}
};

void hmac_sha256(std::span<const uint8_t> key, std::span<const uint8_t> data, std::span<uint8_t, 32> out) noexcept {
	uint8_t k_ipad[64];
	uint8_t k_opad[64];
	uint8_t actual_key[64] = {0};

	if (key.size() > 64) {
		InternalSHA256 sha;
		sha.update(key.data(), key.size());
		sha.finalize(std::span<uint8_t, 32>(actual_key, 32));
	} else {
		std::memcpy(actual_key, key.data(), key.size());
	}

	for (size_t i = 0; i < 64; ++i) {
		k_ipad[i] = actual_key[i] ^ 0x36;
		k_opad[i] = actual_key[i] ^ 0x5C;
	}

	InternalSHA256 sha_inner;
	sha_inner.update(k_ipad, 64);
	sha_inner.update(data.data(), data.size());
	std::array<uint8_t, 32> inner_hash{};
	sha_inner.finalize(inner_hash);

	InternalSHA256 sha_outer;
	sha_outer.update(k_opad, 64);
	sha_outer.update(inner_hash.data(), 32);
	sha_outer.finalize(out);
}

void hkdf_sha256(std::span<const uint8_t> salt, std::span<const uint8_t> ikm, std::span<const uint8_t> info, std::span<uint8_t> okm) noexcept {
	std::array<uint8_t, 32> prk{};
	hmac_sha256(salt, ikm, std::span<uint8_t, 32>(prk));

	std::array<uint8_t, 32> t{};
	size_t okm_offset = 0;
	uint8_t counter = 1;

	while (okm_offset < okm.size()) {
		InternalSHA256 sha_step;
		uint8_t k_ipad[64];
		uint8_t k_opad[64];
		for (size_t i = 0; i < 32; ++i) {
			k_ipad[i] = prk[i] ^ 0x36;
			k_opad[i] = prk[i] ^ 0x5C;
		}
		for (size_t i = 32; i < 64; ++i) {
			k_ipad[i] = 0x36;
			k_opad[i] = 0x5C;
		}

		sha_step.update(k_ipad, 64);
		if (counter > 1) {
			sha_step.update(t.data(), 32);
		}
		sha_step.update(info.data(), info.size());
		sha_step.update(&counter, 1);
		std::array<uint8_t, 32> inner_hash{};
		sha_step.finalize(inner_hash);

		InternalSHA256 sha_outer;
		sha_outer.update(k_opad, 64);
		sha_outer.update(inner_hash.data(), 32);
		sha_outer.finalize(t);

		const size_t to_copy = std::min(static_cast<size_t>(32), okm.size() - okm_offset);
		std::memcpy(okm.data() + okm_offset, t.data(), to_copy);
		okm_offset += to_copy;
		++counter;
	}
}

}

void CryptoChannel::chacha20_block(
	std::span<const uint32_t, 8> key,
	uint32_t counter,
	std::span<const uint32_t, 3> nonce,
	std::span<uint8_t, 64> block_output
) noexcept {
	uint32_t state[16] = {
		0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
		key[0], key[1], key[2], key[3],
		key[4], key[5], key[6], key[7],
		counter, nonce[0], nonce[1], nonce[2]
	};
	uint32_t working_state[16];
	std::memcpy(working_state, state, sizeof(working_state));

	for (int i = 0; i < 10; ++i) {
		chacha_quarter_round(working_state[0], working_state[4], working_state[8], working_state[12]);
		chacha_quarter_round(working_state[1], working_state[5], working_state[9], working_state[13]);
		chacha_quarter_round(working_state[2], working_state[6], working_state[10], working_state[14]);
		chacha_quarter_round(working_state[3], working_state[7], working_state[11], working_state[15]);
		chacha_quarter_round(working_state[0], working_state[5], working_state[10], working_state[15]);
		chacha_quarter_round(working_state[1], working_state[6], working_state[11], working_state[12]);
		chacha_quarter_round(working_state[2], working_state[7], working_state[8], working_state[13]);
		chacha_quarter_round(working_state[3], working_state[4], working_state[9], working_state[14]);
	}

	for (int i = 0; i < 16; ++i) {
		store32_le(block_output.data() + i * 4, working_state[i] + state[i]);
	}
}

void CryptoChannel::poly1305_generate_tag(
	std::span<const uint8_t, 32> one_time_key,
	std::span<const uint8_t> associated_data,
	std::span<const uint8_t> ciphertext,
	std::span<uint8_t, 16> output_tag
) noexcept {
	uint32_t r0, r1, r2, r3, r4;
	uint32_t s1, s2, s3, s4;
	uint32_t h0 = 0, h1 = 0, h2 = 0, h3 = 0, h4 = 0;

	r0 = load32_le(one_time_key.data()) & 0x3FFFFFF;
	r1 = (load32_le(one_time_key.data() + 3) >> 2) & 0x3FFFF03;
	r2 = (load32_le(one_time_key.data() + 6) >> 4) & 0x3FFC0FF;
	r3 = (load32_le(one_time_key.data() + 9) >> 6) & 0x3F03FFF;
	r4 = (load32_le(one_time_key.data() + 12) >> 8) & 0x00FFFFF;

	s1 = r1 * 5; s2 = r2 * 5; s3 = r3 * 5; s4 = r4 * 5;

	auto process_blocks = [&](std::span<const uint8_t> data) {
		size_t offset = 0;
		while (offset < data.size()) {
			uint8_t block[16] = {0};
			const size_t len = std::min<size_t>(16, data.size() - offset);
			std::memcpy(block, data.data() + offset, len);
			block[len] = 1;

			h0 += load32_le(block) & 0x3FFFFFF;
			h1 += (load32_le(block + 3) >> 2) & 0x3FFFFFF;
			h2 += (load32_le(block + 6) >> 4) & 0x3FFFFFF;
			h3 += (load32_le(block + 9) >> 6) & 0x3FFFFFF;
			h4 += (load32_le(block + 12) >> 8) | (block[16] << 16);

			uint64_t d0 = static_cast<uint64_t>(h0) * r0 + static_cast<uint64_t>(h1) * s4 + static_cast<uint64_t>(h2) * s3 + static_cast<uint64_t>(h3) * s2 + static_cast<uint64_t>(h4) * s1;
			uint64_t d1 = static_cast<uint64_t>(h0) * r1 + static_cast<uint64_t>(h1) * r0 + static_cast<uint64_t>(h2) * s4 + static_cast<uint64_t>(h3) * s3 + static_cast<uint64_t>(h4) * s2;
			uint64_t d2 = static_cast<uint64_t>(h0) * r2 + static_cast<uint64_t>(h1) * r1 + static_cast<uint64_t>(h2) * r0 + static_cast<uint64_t>(h3) * s4 + static_cast<uint64_t>(h4) * s3;
			uint64_t d3 = static_cast<uint64_t>(h0) * r3 + static_cast<uint64_t>(h1) * r2 + static_cast<uint64_t>(h2) * r1 + static_cast<uint64_t>(h3) * r0 + static_cast<uint64_t>(h4) * s4;
			uint64_t d4 = static_cast<uint64_t>(h0) * r4 + static_cast<uint64_t>(h1) * r3 + static_cast<uint64_t>(h2) * r2 + static_cast<uint64_t>(h3) * r1 + static_cast<uint64_t>(h4) * r0;

			h0 = static_cast<uint32_t>(d0) & 0x3FFFFFF; d1 += d0 >> 26;
			h1 = static_cast<uint32_t>(d1) & 0x3FFFFFF; d2 += d1 >> 26;
			h2 = static_cast<uint32_t>(d2) & 0x3FFFFFF; d3 += d2 >> 26;
			h3 = static_cast<uint32_t>(d3) & 0x3FFFFFF; d4 += d3 >> 26;
			h4 = static_cast<uint32_t>(d4) & 0x3FFFFFF; h0 += static_cast<uint32_t>(d4 >> 26) * 5; h1 += h0 >> 26; h0 &= 0x3FFFFFF;

			offset += len;
		}
	};

	process_blocks(associated_data);
	process_blocks(ciphertext);

	uint8_t len_block[16] = {0};
	store32_le(len_block, static_cast<uint32_t>(associated_data.size()));
	store32_le(len_block + 8, static_cast<uint32_t>(ciphertext.size()));
	process_blocks(std::span<const uint8_t>(len_block, 16));

	uint32_t c = h1 >> 26; h1 &= 0x3FFFFFF;
	h2 += c; c = h2 >> 26; h2 &= 0x3FFFFFF;
	h3 += c; c = h3 >> 26; h3 &= 0x3FFFFFF;
	h4 += c; c = h4 >> 26; h4 &= 0x3FFFFFF;
	h0 += c * 5; c = h0 >> 26; h0 &= 0x3FFFFFF;
	h1 += c;

	uint32_t g0 = h0 + 5; c = g0 >> 26; g0 &= 0x3FFFFFF;
	uint32_t g1 = h1 + c; c = g1 >> 26; g1 &= 0x3FFFFFF;
	uint32_t g2 = h2 + c; c = g2 >> 26; g2 &= 0x3FFFFFF;
	uint32_t g3 = h3 + c; c = g3 >> 26; g3 &= 0x3FFFFFF;
	uint32_t g4 = h4 + c - (1 << 26);

	uint32_t mask = (g4 >> 31) - 1;
	g0 = (g0 & mask) | (h0 & ~mask);
	g1 = (g1 & mask) | (h1 & ~mask);
	g2 = (g2 & mask) | (h2 & ~mask);
	g3 = (g3 & mask) | (h3 & ~mask);
	g4 = (g4 & mask) | (h4 & ~mask);

	h0 = g0 | (g1 << 26);
	h1 = (g1 >> 6) | (g2 << 20);
	h2 = (g2 >> 12) | (g3 << 14);
	h3 = (g3 >> 18) | (g4 << 8);

	uint64_t f0 = static_cast<uint64_t>(h0) + load32_le(one_time_key.data() + 16);
	uint64_t f1 = static_cast<uint64_t>(h1) + load32_le(one_time_key.data() + 20) + (f0 >> 32);
	uint64_t f2 = static_cast<uint64_t>(h2) + load32_le(one_time_key.data() + 24) + (f1 >> 32);
	uint64_t f3 = static_cast<uint64_t>(h3) + load32_le(one_time_key.data() + 28) + (f2 >> 32);

	store32_le(output_tag.data(), static_cast<uint32_t>(f0));
	store32_le(output_tag.data() + 4, static_cast<uint32_t>(f1));
	store32_le(output_tag.data() + 8, static_cast<uint32_t>(f2));
	store32_le(output_tag.data() + 12, static_cast<uint32_t>(f3));
}

void CryptoChannel::derive_keys_hkdf_sha256(
	std::span<const uint8_t, 32> shared_secret,
	std::span<const uint8_t, 32> pre_shared_token,
	bool is_server,
	std::span<uint8_t, 32> out_tx_key,
	std::span<uint8_t, 32> out_rx_key
) noexcept {
	std::array<uint8_t, 64> key_material{};
	hkdf_sha256(pre_shared_token, shared_secret, std::span<const uint8_t>(reinterpret_cast<const uint8_t*>("RelativisticEngineV1"), 20), key_material);
	if (is_server) {
		std::memcpy(out_tx_key.data(), key_material.data(), 32);
		std::memcpy(out_rx_key.data(), key_material.data() + 32, 32);
	} else {
		std::memcpy(out_rx_key.data(), key_material.data(), 32);
		std::memcpy(out_tx_key.data(), key_material.data() + 32, 32);
	}
}

CryptoKeyPair CryptoChannel::generate_node_identity_keys(uint64_t seed_value) noexcept {
	CryptoKeyPair pair;
	Core::PCG64Engine rng(seed_value, 0x123456789ABCDEF0ULL);
	for (size_t i = 0; i < 32; i += 8) {
		const uint64_t v = rng();
		std::memcpy(pair.private_key.data() + i, &v, 8);
	}
	pair.private_key[0] &= 248;
	pair.private_key[31] &= 127;
	pair.private_key[31] |= 64;

	const uint8_t base_point[32] = {9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	x25519_scalarmult(pair.public_key.data(), pair.private_key.data(), base_point);
	return pair;
}

bool CryptoChannel::perform_handshake_server(
	std::span<const uint8_t, 32> client_public_key,
	std::span<const uint8_t, 32> server_private_key,
	std::span<const uint8_t, 32> pre_shared_token
) noexcept {
	x25519_scalarmult(context_.shared_secret.data(), server_private_key.data(), client_public_key.data());
	derive_keys_hkdf_sha256(context_.shared_secret, pre_shared_token, true, context_.transmission_key, context_.reception_key);
	context_.transmit_sequence = 0;
	context_.receive_sequence = 0;
	context_.replay_window_mask = 0;
	context_.authenticated = true;
	return true;
}

bool CryptoChannel::perform_handshake_client(
	std::span<const uint8_t, 32> server_public_key,
	std::span<const uint8_t, 32> client_private_key,
	std::span<const uint8_t, 32> pre_shared_token
) noexcept {
	x25519_scalarmult(context_.shared_secret.data(), client_private_key.data(), server_public_key.data());
	derive_keys_hkdf_sha256(context_.shared_secret, pre_shared_token, false, context_.transmission_key, context_.reception_key);
	context_.transmit_sequence = 0;
	context_.receive_sequence = 0;
	context_.replay_window_mask = 0;
	context_.authenticated = true;
	return true;
}

bool CryptoChannel::encrypt_and_authenticate_packet(
	std::span<const uint8_t> associated_header,
	std::span<const uint8_t> plaintext_payload,
	std::span<uint8_t> output_ciphertext,
	std::span<uint8_t, 16> output_tag
) noexcept {
	if (!context_.authenticated || output_ciphertext.size() < plaintext_payload.size()) {
		return false;
	}

	uint32_t key_u32[8];
	for (size_t i = 0; i < 8; ++i) key_u32[i] = load32_le(context_.transmission_key.data() + i * 4);

	uint32_t nonce_u32[3];
	nonce_u32[0] = context_.session_epoch;
	nonce_u32[1] = static_cast<uint32_t>(context_.transmit_sequence & 0xFFFFFFFF);
	nonce_u32[2] = static_cast<uint32_t>(context_.transmit_sequence >> 32);
	context_.transmit_sequence++;

	std::array<uint8_t, 64> poly_key_block{};
	chacha20_block(key_u32, 0, nonce_u32, poly_key_block);

	size_t offset = 0;
	uint32_t counter = 1;
	while (offset < plaintext_payload.size()) {
		std::array<uint8_t, 64> block{};
		chacha20_block(key_u32, counter++, nonce_u32, block);
		const size_t len = std::min<size_t>(64, plaintext_payload.size() - offset);
		for (size_t i = 0; i < len; ++i) {
			output_ciphertext[offset + i] = plaintext_payload[offset + i] ^ block[i];
		}
		offset += len;
	}

	poly1305_generate_tag(std::span<const uint8_t, 32>(poly_key_block.data(), 32), associated_header, output_ciphertext.subspan(0, plaintext_payload.size()), output_tag);
	return true;
}

bool CryptoChannel::check_and_update_replay_window(uint64_t sequence) noexcept {
	if (sequence > context_.receive_sequence) {
		const uint64_t diff = sequence - context_.receive_sequence;
		if (diff < 64) {
			context_.replay_window_mask <<= diff;
			context_.replay_window_mask |= 1ULL;
		} else {
			context_.replay_window_mask = 1ULL;
		}
		context_.receive_sequence = sequence;
		return true;
	}
	if (sequence + 64 <= context_.receive_sequence) {
		return false;
	}
	const uint64_t diff = context_.receive_sequence - sequence;
	if ((context_.replay_window_mask & (1ULL << diff)) != 0) {
		return false;
	}
	context_.replay_window_mask |= (1ULL << diff);
	return true;
}

bool CryptoChannel::verify_and_decrypt_packet(
	std::span<const uint8_t> associated_header,
	uint64_t packet_sequence,
	std::span<const uint8_t> ciphertext_payload,
	std::span<const uint8_t, 16> authentication_tag,
	std::span<uint8_t> output_plaintext
) noexcept {
	if (!context_.authenticated || output_plaintext.size() < ciphertext_payload.size()) {
		return false;
	}

	uint32_t key_u32[8];
	for (size_t i = 0; i < 8; ++i) key_u32[i] = load32_le(context_.reception_key.data() + i * 4);

	uint32_t nonce_u32[3];
	nonce_u32[0] = context_.session_epoch;
	nonce_u32[1] = static_cast<uint32_t>(packet_sequence & 0xFFFFFFFF);
	nonce_u32[2] = static_cast<uint32_t>(packet_sequence >> 32);

	std::array<uint8_t, 64> poly_key_block{};
	chacha20_block(key_u32, 0, nonce_u32, poly_key_block);

	std::array<uint8_t, 16> expected_tag{};
	poly1305_generate_tag(std::span<const uint8_t, 32>(poly_key_block.data(), 32), associated_header, ciphertext_payload, expected_tag);

	uint8_t diff = 0;
	for (size_t i = 0; i < 16; ++i) {
		diff |= static_cast<uint8_t>(expected_tag[i] ^ authentication_tag[i]);
	}
	if (diff != 0) {
		return false;
	}

	if (!check_and_update_replay_window(packet_sequence)) {
		return false;
	}

	size_t offset = 0;
	uint32_t counter = 1;
	while (offset < ciphertext_payload.size()) {
		std::array<uint8_t, 64> block{};
		chacha20_block(key_u32, counter++, nonce_u32, block);
		const size_t len = std::min<size_t>(64, ciphertext_payload.size() - offset);
		for (size_t i = 0; i < len; ++i) {
			output_plaintext[offset + i] = ciphertext_payload[offset + i] ^ block[i];
		}
		offset += len;
	}

	return true;
}

}
