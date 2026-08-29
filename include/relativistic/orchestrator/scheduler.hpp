#pragma once

#include <cstdint>
#include <cstddef>
#include <chrono>
#include <algorithm>
#include <cmath>

namespace Relativistic::Orchestrator {

template <typename Scalar = double>
struct SchedulerConfig {
	Scalar min_tick_rate_hz = static_cast<Scalar>(10.0);
	Scalar max_tick_rate_hz = static_cast<Scalar>(1000.0);
	Scalar default_tick_rate_hz = static_cast<Scalar>(60.0);
	Scalar max_accumulated_seconds = static_cast<Scalar>(0.25);
};

template <typename Scalar = double>
struct SchedulerSnapshot {
	uint64_t tick_index{0};
	Scalar logical_time{0.0};
	Scalar tick_rate_hz{60.0};
	Scalar tick_dt{1.0 / 60.0};
	Scalar warp_factor{1.0};
	bool is_paused{false};
	uint64_t remaining_steps{0};
};

template <typename Scalar = double>
class Scheduler {
private:
	SchedulerConfig<Scalar> config_;
	Scalar tick_rate_hz_;
	Scalar tick_dt_;
	Scalar warp_factor_;
	bool is_paused_;
	uint64_t remaining_steps_;
	uint64_t current_tick_;
	Scalar logical_time_;
	int64_t accumulator_ns_;
	int64_t tick_interval_ns_;
	int64_t max_accumulator_ns_;

	void recalculate_intervals() noexcept {
		tick_dt_ = static_cast<Scalar>(1.0) / tick_rate_hz_;
		const double ns_per_tick = 1e9 / static_cast<double>(tick_rate_hz_);
		tick_interval_ns_ = static_cast<int64_t>(ns_per_tick);
		const double max_acc_ns = static_cast<double>(config_.max_accumulated_seconds) * 1e9;
		max_accumulator_ns_ = static_cast<int64_t>(max_acc_ns);
	}

public:
	explicit constexpr Scheduler(const SchedulerConfig<Scalar>& config = {}) noexcept
		: config_(config),
		  tick_rate_hz_(config.default_tick_rate_hz),
		  tick_dt_(static_cast<Scalar>(1.0) / config.default_tick_rate_hz),
		  warp_factor_(static_cast<Scalar>(1.0)),
		  is_paused_(false),
		  remaining_steps_(0),
		  current_tick_(0),
		  logical_time_(static_cast<Scalar>(0.0)),
		  accumulator_ns_(0),
		  tick_interval_ns_(0),
		  max_accumulator_ns_(0) {
		recalculate_intervals();
	}

	void set_tick_rate(Scalar hz) noexcept {
		tick_rate_hz_ = std::clamp(hz, config_.min_tick_rate_hz, config_.max_tick_rate_hz);
		recalculate_intervals();
	}

	[[nodiscard]] constexpr Scalar tick_rate() const noexcept {
		return tick_rate_hz_;
	}

	[[nodiscard]] constexpr Scalar tick_dt() const noexcept {
		return tick_dt_;
	}

	void set_warp_factor(Scalar warp) noexcept {
		if (warp > static_cast<Scalar>(0.0)) {
			warp_factor_ = warp;
		}
	}

	[[nodiscard]] constexpr Scalar warp_factor() const noexcept {
		return warp_factor_;
	}

	void pause() noexcept {
		is_paused_ = true;
		remaining_steps_ = 0;
	}

	void resume() noexcept {
		is_paused_ = false;
	}

	[[nodiscard]] constexpr bool is_paused() const noexcept {
		return is_paused_;
	}

	void request_steps(uint64_t n) noexcept {
		remaining_steps_ += n;
		is_paused_ = false;
	}

	[[nodiscard]] constexpr uint64_t remaining_steps() const noexcept {
		return remaining_steps_;
	}

	void reset() noexcept {
		current_tick_ = 0;
		logical_time_ = static_cast<Scalar>(0.0);
		accumulator_ns_ = 0;
		remaining_steps_ = 0;
		is_paused_ = false;
		warp_factor_ = static_cast<Scalar>(1.0);
		tick_rate_hz_ = config_.default_tick_rate_hz;
		recalculate_intervals();
	}

	[[nodiscard]] constexpr uint64_t current_tick() const noexcept {
		return current_tick_;
	}

	[[nodiscard]] constexpr Scalar logical_time() const noexcept {
		return logical_time_;
	}

	void add_real_time_nanoseconds(int64_t ns) noexcept {
		if (ns <= 0) return;
		accumulator_ns_ += ns;
		if (accumulator_ns_ > max_accumulator_ns_) {
			accumulator_ns_ = max_accumulator_ns_;
		}
	}

	[[nodiscard]] bool can_advance_tick() const noexcept {
		if (is_paused_ && remaining_steps_ == 0) {
			return false;
		}
		if (remaining_steps_ > 0) {
			return true;
		}
		return accumulator_ns_ >= tick_interval_ns_;
	}

	bool advance_tick() noexcept {
		if (is_paused_ && remaining_steps_ == 0) {
			return false;
		}

		if (remaining_steps_ > 0) {
			--remaining_steps_;
			if (remaining_steps_ == 0) {
				is_paused_ = true;
			}
			if (accumulator_ns_ >= tick_interval_ns_) {
				accumulator_ns_ -= tick_interval_ns_;
			}
		} else {
			if (accumulator_ns_ < tick_interval_ns_) {
				return false;
			}
			accumulator_ns_ -= tick_interval_ns_;
		}

		++current_tick_;
		logical_time_ += tick_dt_ * warp_factor_;
		return true;
	}

	[[nodiscard]] Scalar alpha() const noexcept {
		if (tick_interval_ns_ <= 0) return static_cast<Scalar>(0.0);
		const double frac = static_cast<double>(accumulator_ns_) / static_cast<double>(tick_interval_ns_);
		return static_cast<Scalar>(std::clamp(frac, 0.0, 1.0));
	}

	[[nodiscard]] SchedulerSnapshot<Scalar> snapshot() const noexcept {
		return SchedulerSnapshot<Scalar>{
			.tick_index = current_tick_,
			.logical_time = logical_time_,
			.tick_rate_hz = tick_rate_hz_,
			.tick_dt = tick_dt_,
			.warp_factor = warp_factor_,
			.is_paused = is_paused_,
			.remaining_steps = remaining_steps_
		};
	}
};

}
