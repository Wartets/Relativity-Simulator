#pragma once

#include <cstdint>
#include <limits>
#include <concepts>
#include <cmath>
#include <numbers>
#include <utility>

namespace Relativistic::Core {

struct UInt128 {
	uint64_t hi;
	uint64_t lo;

	constexpr UInt128() noexcept : hi(0), lo(0) {}
	constexpr UInt128(uint64_t low) noexcept : hi(0), lo(low) {}
	constexpr UInt128(uint64_t high, uint64_t low) noexcept : hi(high), lo(low) {}

	[[nodiscard]] constexpr explicit operator bool() const noexcept {
		return (hi != 0) || (lo != 0);
	}

	[[nodiscard]] constexpr explicit operator uint64_t() const noexcept {
		return lo;
	}

	[[nodiscard]] constexpr UInt128 operator~() const noexcept {
		return UInt128(~hi, ~lo);
	}

	[[nodiscard]] constexpr UInt128 operator^(const UInt128& other) const noexcept {
		return UInt128(hi ^ other.hi, lo ^ other.lo);
	}

	[[nodiscard]] constexpr UInt128 operator|(const UInt128& other) const noexcept {
		return UInt128(hi | other.hi, lo | other.lo);
	}

	[[nodiscard]] constexpr UInt128 operator&(const UInt128& other) const noexcept {
		return UInt128(hi & other.hi, lo & other.lo);
	}

	constexpr UInt128& operator^=(const UInt128& other) noexcept {
		hi ^= other.hi;
		lo ^= other.lo;
		return *this;
	}

	constexpr UInt128& operator|=(const UInt128& other) noexcept {
		hi |= other.hi;
		lo |= other.lo;
		return *this;
	}

	constexpr UInt128& operator&=(const UInt128& other) noexcept {
		hi &= other.hi;
		lo &= other.lo;
		return *this;
	}

	[[nodiscard]] constexpr UInt128 operator<<(unsigned int shift) const noexcept {
		if (shift == 0) {
			return *this;
		}
		if (shift >= 128) {
			return UInt128(0, 0);
		}
		if (shift >= 64) {
			return UInt128(lo << (shift - 64), 0);
		}
		return UInt128((hi << shift) | (lo >> (64 - shift)), lo << shift);
	}

	[[nodiscard]] constexpr UInt128 operator>>(unsigned int shift) const noexcept {
		if (shift == 0) {
			return *this;
		}
		if (shift >= 128) {
			return UInt128(0, 0);
		}
		if (shift >= 64) {
			return UInt128(0, hi >> (shift - 64));
		}
		return UInt128(hi >> shift, (lo >> shift) | (hi << (64 - shift)));
	}

	constexpr UInt128& operator<<=(unsigned int shift) noexcept {
		*this = *this << shift;
		return *this;
	}

	constexpr UInt128& operator>>=(unsigned int shift) noexcept {
		*this = *this >> shift;
		return *this;
	}

	[[nodiscard]] constexpr UInt128 operator+(const UInt128& other) const noexcept {
		const uint64_t res_lo = lo + other.lo;
		const uint64_t carry = (res_lo < lo) ? 1ULL : 0ULL;
		const uint64_t res_hi = hi + other.hi + carry;
		return UInt128(res_hi, res_lo);
	}

	constexpr UInt128& operator+=(const UInt128& other) noexcept {
		*this = *this + other;
		return *this;
	}

	[[nodiscard]] constexpr UInt128 operator*(const UInt128& other) const noexcept {
		const uint64_t u0 = lo & 0xFFFFFFFFULL;
		const uint64_t u1 = lo >> 32;
		const uint64_t v0 = other.lo & 0xFFFFFFFFULL;
		const uint64_t v1 = other.lo >> 32;

		const uint64_t w0 = u0 * v0;
		const uint64_t t = u1 * v0 + (w0 >> 32);
		const uint64_t w1 = t & 0xFFFFFFFFULL;
		const uint64_t w2 = t >> 32;
		const uint64_t w1_prime = w1 + u0 * v1;

		const uint64_t res_lo = (w1_prime << 32) | (w0 & 0xFFFFFFFFULL);
		const uint64_t cross_carry = u1 * v1 + w2 + (w1_prime >> 32);
		const uint64_t res_hi = hi * other.lo + lo * other.hi + cross_carry;

		return UInt128(res_hi, res_lo);
	}

	constexpr UInt128& operator*=(const UInt128& other) noexcept {
		*this = *this * other;
		return *this;
	}

	[[nodiscard]] constexpr bool operator==(const UInt128& other) const noexcept {
		return hi == other.hi && lo == other.lo;
	}

	[[nodiscard]] constexpr bool operator!=(const UInt128& other) const noexcept {
		return !(*this == other);
	}
};

class PCG64Engine {
public:
	using result_type = uint64_t;

private:
	static constexpr UInt128 DEFAULT_MULTIPLIER{0x2360ED051FC65DA4ULL, 0x4385DF649FCCF645ULL};
	static constexpr UInt128 DEFAULT_STREAM{0x5851F42D4C957F2DULL, 0x14057B7EF767814FULL};

	UInt128 state_;
	UInt128 inc_;

	static constexpr uint64_t rotr64(uint64_t value, unsigned int rot) noexcept {
		return (value >> rot) | (value << ((-rot) & 63));
	}

	static constexpr uint64_t output_xsl_rr(const UInt128& state) noexcept {
		const uint64_t xorshifted = state.hi ^ state.lo;
		const unsigned int rot = static_cast<unsigned int>(state.hi >> 58);
		return rotr64(xorshifted, rot);
	}

public:
	constexpr PCG64Engine() noexcept
		: state_(0x979FD55DA1F2D3F4ULL, 0xDDA1300E4611A0B6ULL),
		  inc_(DEFAULT_STREAM) {}

	explicit constexpr PCG64Engine(uint64_t init_state, uint64_t init_seq = 1ULL) noexcept
		: state_(0, 0), inc_((UInt128(0, init_seq) << 1) | UInt128(0, 1ULL)) {
		state_ = state_ * DEFAULT_MULTIPLIER + inc_;
		state_ += UInt128(0, init_state);
		state_ = state_ * DEFAULT_MULTIPLIER + inc_;
	}

	explicit constexpr PCG64Engine(uint64_t init_state_hi, uint64_t init_state_lo, uint64_t init_seq_hi, uint64_t init_seq_lo) noexcept
		: state_(0, 0), inc_((UInt128(init_seq_hi, init_seq_lo) << 1) | UInt128(0, 1ULL)) {
		state_ = state_ * DEFAULT_MULTIPLIER + inc_;
		state_ += UInt128(init_state_hi, init_state_lo);
		state_ = state_ * DEFAULT_MULTIPLIER + inc_;
	}

	constexpr void seed(uint64_t init_state, uint64_t init_seq = 1ULL) noexcept {
		state_ = UInt128(0, 0);
		inc_ = (UInt128(0, init_seq) << 1) | UInt128(0, 1ULL);
		state_ = state_ * DEFAULT_MULTIPLIER + inc_;
		state_ += UInt128(0, init_state);
		state_ = state_ * DEFAULT_MULTIPLIER + inc_;
	}

	constexpr uint64_t operator()() noexcept {
		const UInt128 old_state = state_;
		state_ = old_state * DEFAULT_MULTIPLIER + inc_;
		return output_xsl_rr(old_state);
	}

	constexpr uint64_t next_bounded(uint64_t bound) noexcept {
		if (bound == 0) [[unlikely]] {
			return 0;
		}
		const uint64_t threshold = static_cast<uint64_t>(-static_cast<int64_t>(bound)) % bound;
		for (;;) {
			const uint64_t r = (*this)();
			if (r >= threshold) {
				return r % bound;
			}
		}
	}

	[[nodiscard]] double next_uniform_double() noexcept {
		constexpr double factor = 1.0 / static_cast<double>(1ULL << 53);
		const uint64_t raw = (*this)() >> 11;
		return static_cast<double>(raw) * factor;
	}

	[[nodiscard]] double next_uniform_range(double min_val, double max_val) noexcept {
		return min_val + (max_val - min_val) * next_uniform_double();
	}

	[[nodiscard]] std::pair<double, double> next_gaussian_pair(double mean = 0.0, double stddev = 1.0) noexcept {
		double u1 = 0.0;
		while (u1 <= std::numeric_limits<double>::min()) {
			u1 = next_uniform_double();
		}
		const double u2 = next_uniform_double();
		const double radius = std::sqrt(-2.0 * std::log(u1));
		const double theta = 2.0 * std::numbers::pi_v<double> * u2;
		return {
			mean + stddev * (radius * std::cos(theta)),
			mean + stddev * (radius * std::sin(theta))
		};
	}

	constexpr void step_forward(uint64_t delta) noexcept {
		UInt128 cur_mult = DEFAULT_MULTIPLIER;
		UInt128 cur_plus = inc_;
		UInt128 acc_mult(0, 1ULL);
		UInt128 acc_plus(0, 0ULL);
		UInt128 steps(0, delta);

		while (static_cast<bool>(steps)) {
			if ((steps.lo & 1ULL) != 0ULL) {
				acc_mult *= cur_mult;
				acc_plus = acc_plus * cur_mult + cur_plus;
			}
			cur_plus = (cur_mult + UInt128(0, 1ULL)) * cur_plus;
			cur_mult *= cur_mult;
			steps >>= 1;
		}
		state_ = acc_mult * state_ + acc_plus;
	}

	static constexpr uint64_t min() noexcept {
		return std::numeric_limits<uint64_t>::min();
	}

	static constexpr uint64_t max() noexcept {
		return std::numeric_limits<uint64_t>::max();
	}

	[[nodiscard]] constexpr bool operator==(const PCG64Engine& other) const noexcept {
		return state_ == other.state_ && inc_ == other.inc_;
	}

	[[nodiscard]] constexpr bool operator!=(const PCG64Engine& other) const noexcept {
		return !(*this == other);
	}
};

class DeterministicRngRegistry {
private:
	PCG64Engine global_engine_;
	uint64_t master_seed_;
	uint64_t sequence_id_;

public:
	explicit constexpr DeterministicRngRegistry(uint64_t initial_seed = 0x853C49E6748FEA9BULL) noexcept
		: global_engine_(initial_seed, 1ULL),
		  master_seed_(initial_seed),
		  sequence_id_(1ULL) {}

	constexpr void initialize(uint64_t master_seed, uint64_t sequence_id = 1ULL) noexcept {
		master_seed_ = master_seed;
		sequence_id_ = sequence_id;
		global_engine_.seed(master_seed_, sequence_id_);
	}

	[[nodiscard]] PCG64Engine create_sub_engine(uint64_t stream_index) noexcept {
		const uint64_t sub_seed = global_engine_();
		return PCG64Engine(sub_seed, (stream_index << 1) | 1ULL);
	}

	[[nodiscard]] constexpr PCG64Engine& global_engine() noexcept {
		return global_engine_;
	}

	[[nodiscard]] constexpr const PCG64Engine& global_engine() const noexcept {
		return global_engine_;
	}

	[[nodiscard]] constexpr uint64_t master_seed() const noexcept {
		return master_seed_;
	}
};

}
