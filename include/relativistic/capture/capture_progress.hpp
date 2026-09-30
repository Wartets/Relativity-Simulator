#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <utility>

namespace Relativistic::Capture {

enum class CapturePhase : uint32_t {
	Idle = 0,
	Rendering = 1,
	Assembling = 2,
	Completed = 3,
	Cancelled = 4,
	Failed = 5
};

[[nodiscard]] constexpr const char* capture_phase_name(CapturePhase phase) noexcept {
	switch (phase) {
		case CapturePhase::Rendering: return "Capturing";
		case CapturePhase::Assembling: return "Assembling Video";
		case CapturePhase::Completed: return "Completed";
		case CapturePhase::Cancelled: return "Cancelled";
		case CapturePhase::Failed: return "Failed";
		case CapturePhase::Idle:
		default: return "Idle";
	}
}

class CaptureProgress {
private:
	std::atomic<CapturePhase> phase_{CapturePhase::Idle};
	std::atomic<uint64_t> frames_total_{0};
	std::atomic<uint64_t> frames_done_{0};
	std::atomic<uint64_t> frames_dropped_{0};
	std::atomic<uint64_t> frames_duplicated_{0};
	std::atomic<uint32_t> bands_total_{0};
	std::atomic<uint32_t> bands_done_{0};
	std::atomic<bool> cancel_requested_{false};
	std::atomic<int64_t> start_ns_{0};
	std::atomic<int64_t> end_ns_{0};
	mutable std::mutex text_mutex_;
	std::string label_;
	std::string message_;

	[[nodiscard]] static int64_t now_ns() noexcept {
		return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

public:
	void begin(std::string label, uint64_t total_frames) {
		{
			std::lock_guard<std::mutex> lock(text_mutex_);
			label_ = std::move(label);
			message_.clear();
		}
		frames_total_.store(total_frames, std::memory_order_relaxed);
		frames_done_.store(0, std::memory_order_relaxed);
		frames_dropped_.store(0, std::memory_order_relaxed);
		frames_duplicated_.store(0, std::memory_order_relaxed);
		bands_total_.store(0, std::memory_order_relaxed);
		bands_done_.store(0, std::memory_order_relaxed);
		cancel_requested_.store(false, std::memory_order_relaxed);
		end_ns_.store(0, std::memory_order_relaxed);
		start_ns_.store(now_ns(), std::memory_order_relaxed);
		phase_.store(CapturePhase::Rendering, std::memory_order_release);
	}

	void set_phase(CapturePhase phase) noexcept {
		phase_.store(phase, std::memory_order_release);
	}

	void finish(CapturePhase phase, std::string message) {
		{
			std::lock_guard<std::mutex> lock(text_mutex_);
			message_ = std::move(message);
		}
		end_ns_.store(now_ns(), std::memory_order_relaxed);
		phase_.store(phase, std::memory_order_release);
	}

	void set_message(std::string message) {
		std::lock_guard<std::mutex> lock(text_mutex_);
		message_ = std::move(message);
	}

	void set_total_frames(uint64_t total) noexcept { frames_total_.store(total, std::memory_order_relaxed); }
	void add_frame() noexcept { frames_done_.fetch_add(1, std::memory_order_relaxed); }
	void add_dropped(uint64_t count) noexcept { frames_dropped_.fetch_add(count, std::memory_order_relaxed); }
	void add_duplicated(uint64_t count) noexcept { frames_duplicated_.fetch_add(count, std::memory_order_relaxed); }

	void set_bands(uint32_t done, uint32_t total) noexcept {
		bands_total_.store(total, std::memory_order_relaxed);
		bands_done_.store(done, std::memory_order_relaxed);
	}

	void request_cancel() noexcept { cancel_requested_.store(true, std::memory_order_release); }
	[[nodiscard]] bool cancel_requested() const noexcept { return cancel_requested_.load(std::memory_order_acquire); }
	[[nodiscard]] const std::atomic<bool>* cancel_flag() const noexcept { return &cancel_requested_; }

	[[nodiscard]] CapturePhase phase() const noexcept { return phase_.load(std::memory_order_acquire); }
	[[nodiscard]] uint64_t frames_total() const noexcept { return frames_total_.load(std::memory_order_relaxed); }
	[[nodiscard]] uint64_t frames_done() const noexcept { return frames_done_.load(std::memory_order_relaxed); }
	[[nodiscard]] uint64_t frames_dropped() const noexcept { return frames_dropped_.load(std::memory_order_relaxed); }
	[[nodiscard]] uint64_t frames_duplicated() const noexcept { return frames_duplicated_.load(std::memory_order_relaxed); }
	[[nodiscard]] uint32_t bands_total() const noexcept { return bands_total_.load(std::memory_order_relaxed); }
	[[nodiscard]] uint32_t bands_done() const noexcept { return bands_done_.load(std::memory_order_relaxed); }

	[[nodiscard]] std::string label() const {
		std::lock_guard<std::mutex> lock(text_mutex_);
		return label_;
	}

	[[nodiscard]] std::string message() const {
		std::lock_guard<std::mutex> lock(text_mutex_);
		return message_;
	}

	[[nodiscard]] double fraction() const noexcept {
		const CapturePhase current = phase();
		if (current == CapturePhase::Completed) {
			return 1.0;
		}
		const uint64_t total = frames_total();
		if (total == 0) {
			return 0.0;
		}
		const uint64_t done = frames_done();
		const uint32_t band_total = bands_total();
		const double band_fraction = (band_total > 0 && done < total) ? (static_cast<double>(bands_done()) / static_cast<double>(band_total)) : 0.0;
		return std::clamp((static_cast<double>(done) + band_fraction) / static_cast<double>(total), 0.0, 1.0);
	}

	[[nodiscard]] double elapsed_seconds() const noexcept {
		const int64_t start = start_ns_.load(std::memory_order_relaxed);
		if (start == 0) {
			return 0.0;
		}
		const int64_t end = end_ns_.load(std::memory_order_relaxed);
		const int64_t reference = (end != 0) ? end : now_ns();
		return static_cast<double>(reference - start) * 1.0e-9;
	}

	[[nodiscard]] double eta_seconds() const noexcept {
		const double progress = fraction();
		if (progress < 1.0e-3 || progress >= 1.0) {
			return 0.0;
		}
		const double elapsed = elapsed_seconds();
		return elapsed / progress - elapsed;
	}
};

}
