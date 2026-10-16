#pragma once

#include <algorithm>
#include <atomic>
#include <exception>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>

namespace Relativistic::Startup {

struct StartupRange {
	float begin{0.0f};
	float end{1.0f};

	[[nodiscard]] constexpr float at(float local_fraction) const noexcept {
		const float clamped = local_fraction < 0.0f ? 0.0f : (local_fraction > 1.0f ? 1.0f : local_fraction);
		return begin + (end - begin) * clamped;
	}
};

namespace StartupStage {

inline constexpr StartupRange Runtime{0.00f, 0.05f};
inline constexpr StartupRange Settings{0.05f, 0.12f};
inline constexpr StartupRange Simulation{0.12f, 0.22f};
inline constexpr StartupRange WindowSystem{0.22f, 0.28f};
inline constexpr StartupRange GraphicsContext{0.28f, 0.36f};
inline constexpr StartupRange RenderBackend{0.36f, 0.70f};
inline constexpr StartupRange Interface{0.70f, 0.84f};
inline constexpr StartupRange Content{0.84f, 0.96f};
inline constexpr StartupRange Finalize{0.96f, 1.00f};

}

enum class StartupPhase : uint32_t {
	Running = 0,
	Completed = 1,
	Failed = 2
};

struct StartupSnapshot {
	float fraction{0.0f};
	std::string stage{};
	std::string detail{};
	std::string error{};
	StartupPhase phase{StartupPhase::Running};
	bool cancel_requested{false};
};

class StartupCancelled final : public std::exception {
public:
	[[nodiscard]] const char* what() const noexcept override {
		return "Application startup was cancelled.";
	}
};

class StartupProgress {
private:
	mutable std::mutex mutex_{};
	std::function<void()> listener_{};
	std::string stage_{};
	std::string detail_{};
	std::string error_{};
	float fraction_{0.0f};
	float creep_limit_{0.0f};
	StartupPhase phase_{StartupPhase::Running};
	std::atomic<bool> cancel_requested_{false};

	template <typename Mutator>
	void update(Mutator&& mutator) {
		std::function<void()> listener;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			mutator();
			listener = listener_;
		}
		if (listener) {
			listener();
		}
	}

public:
	void set_listener(std::function<void()> listener) {
		std::lock_guard<std::mutex> lock(mutex_);
		listener_ = std::move(listener);
	}

	void report(StartupRange range, float local_fraction, std::string_view stage, std::string_view detail) {
		update([&]() {
			fraction_ = std::max(fraction_, range.at(local_fraction));
			creep_limit_ = range.end;
			stage_.assign(stage);
			detail_.assign(detail);
		});
	}

	void note(std::string_view detail) {
		update([&]() {
			const float remaining = std::max(creep_limit_ - fraction_, 0.0f);
			fraction_ += remaining * 0.16f;
			detail_.assign(detail);
		});
	}

	void complete() {
		update([&]() {
			fraction_ = 1.0f;
			creep_limit_ = 1.0f;
			phase_ = StartupPhase::Completed;
			stage_.assign("Ready");
			detail_.assign("Workspace ready");
		});
	}

	void fail(std::string_view message) {
		update([&]() {
			phase_ = StartupPhase::Failed;
			error_.assign(message);
		});
	}

	void request_cancel() {
		cancel_requested_.store(true, std::memory_order_release);
		update([]() {});
	}

	[[nodiscard]] bool cancel_requested() const noexcept {
		return cancel_requested_.load(std::memory_order_acquire);
	}

	void throw_if_cancelled() const {
		if (cancel_requested()) {
			throw StartupCancelled{};
		}
	}

	[[nodiscard]] StartupSnapshot snapshot() const {
		StartupSnapshot result;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			result.fraction = fraction_;
			result.stage = stage_;
			result.detail = detail_;
			result.error = error_;
			result.phase = phase_;
		}
		result.cancel_requested = cancel_requested();
		return result;
	}
};

namespace Detail {

[[nodiscard]] inline std::atomic<StartupProgress*>& active_progress_slot() noexcept {
	static std::atomic<StartupProgress*> slot{nullptr};
	return slot;
}

}

inline void set_active_progress(StartupProgress* progress) noexcept {
	Detail::active_progress_slot().store(progress, std::memory_order_release);
}

[[nodiscard]] inline StartupProgress* active_progress() noexcept {
	return Detail::active_progress_slot().load(std::memory_order_acquire);
}

inline void note(std::string_view detail) noexcept {
	StartupProgress* progress = active_progress();
	if (progress == nullptr) {
		return;
	}
	try {
		progress->note(detail);
	} catch (...) {
	}
}

}
