#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <mutex>
#include <string>
#include <string_view>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__linux__)
#include <sys/syscall.h>
#include <unistd.h>
#else
#include <functional>
#include <thread>
#include <unistd.h>
#endif

namespace Relativistic::Core::Diagnostics {

inline constexpr size_t kEventLineCapacity = 320;
inline constexpr size_t kEventRingSize = 512;
inline constexpr size_t kEnvironmentEntryCount = 96;
inline constexpr size_t kEnvironmentKeyCapacity = 48;
inline constexpr size_t kEnvironmentValueCapacity = 256;
inline constexpr size_t kActivitySlotCount = 64;
inline constexpr size_t kActivityLabelCapacity = 96;
inline constexpr size_t kThreadNameCapacity = 32;
inline constexpr size_t kSnapshotCapacity = 16384;

enum class AppPhase : uint32_t {
	Launching = 0,
	Startup = 1,
	Running = 2,
	Headless = 3,
	ShuttingDown = 4
};

[[nodiscard]] constexpr const char* app_phase_name(AppPhase phase) noexcept {
	switch (phase) {
		case AppPhase::Launching: return "Launching";
		case AppPhase::Startup: return "Startup";
		case AppPhase::Running: return "Running";
		case AppPhase::Headless: return "Headless";
		case AppPhase::ShuttingDown: return "ShuttingDown";
	}
	return "Unknown";
}

inline void copy_text(char* destination, size_t capacity, std::string_view source) noexcept {
	if (capacity == 0) {
		return;
	}
	const size_t length = (source.size() < capacity - 1) ? source.size() : (capacity - 1);
	if (length > 0) {
		std::memcpy(destination, source.data(), length);
	}
	destination[length] = '\0';
}

#if defined(__clang__)
__attribute__((format(printf, 2, 3)))
#elif defined(__GNUC__)
__attribute__((format(gnu_printf, 2, 3)))
#endif
inline void append_format(std::string& output, const char* format, ...) {
	char buffer[1024];
	va_list arguments;
	va_start(arguments, format);
	const int written = std::vsnprintf(buffer, sizeof(buffer), format, arguments);
	va_end(arguments);
	if (written <= 0) {
		return;
	}
	const size_t length = (static_cast<size_t>(written) < sizeof(buffer)) ? static_cast<size_t>(written) : (sizeof(buffer) - 1);
	output.append(buffer, length);
}

[[nodiscard]] inline uint64_t current_thread_id() noexcept {
#if defined(_WIN32)
	return static_cast<uint64_t>(GetCurrentThreadId());
#elif defined(__linux__)
	return static_cast<uint64_t>(::syscall(SYS_gettid));
#else
	return static_cast<uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
#endif
}

[[nodiscard]] inline uint64_t current_process_id() noexcept {
#if defined(_WIN32)
	return static_cast<uint64_t>(GetCurrentProcessId());
#else
	return static_cast<uint64_t>(::getpid());
#endif
}

[[nodiscard]] inline int64_t monotonic_nanoseconds() noexcept {
	return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

inline size_t format_wall_clock(char* buffer, size_t capacity, const char* pattern, std::time_t value) noexcept {
	if (capacity == 0) {
		return 0;
	}
	buffer[0] = '\0';
	std::tm broken{};
#if defined(_MSC_VER)
	localtime_s(&broken, &value);
#elif defined(_WIN32)
	const std::tm* shared = std::localtime(&value);
	if (shared == nullptr) {
		return 0;
	}
	broken = *shared;
#else
	localtime_r(&value, &broken);
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
	const size_t written = std::strftime(buffer, capacity, pattern, &broken);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
	return written;
}

inline void wall_clock_text(char* buffer, size_t capacity) noexcept {
	const auto now = std::chrono::system_clock::now();
	const std::time_t seconds = std::chrono::system_clock::to_time_t(now);
	const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
	char head[40];
	format_wall_clock(head, sizeof(head), "%Y-%m-%d %H:%M:%S", seconds);
	std::snprintf(buffer, capacity, "%s.%03d", head, static_cast<int>(milliseconds));
}

struct EventSlot {
	std::atomic<uint64_t> sequence{0};
	std::array<char, kEventLineCapacity> text{};
};

struct EnvironmentEntry {
	std::atomic<bool> used{false};
	std::array<char, kEnvironmentKeyCapacity> key{};
	std::array<char, kEnvironmentValueCapacity> value{};
};

struct ActivitySlot {
	std::atomic<uint64_t> thread_id{0};
	std::atomic<int64_t> since_ns{0};
	std::array<char, kThreadNameCapacity> thread_name{};
	std::array<char, kActivityLabelCapacity> label{};
};

struct SnapshotBuffer {
	std::array<std::array<char, kSnapshotCapacity>, 2> storage{};
	std::atomic<uint32_t> active{0};
	std::atomic<int64_t> stamp_ns{0};
	std::atomic<bool> published{false};
};

struct Registry {
	std::array<EventSlot, kEventRingSize> events{};
	std::atomic<uint64_t> event_cursor{0};
	std::array<EnvironmentEntry, kEnvironmentEntryCount> environment{};
	std::mutex environment_mutex{};
	std::array<ActivitySlot, kActivitySlotCount> activities{};
	SnapshotBuffer scene{};
	SnapshotBuffer render{};
	SnapshotBuffer startup{};
	std::atomic<uint32_t> phase{0};
	std::atomic<int64_t> process_start_ns{0};
	std::atomic<uint64_t> frame_counter{0};
	std::atomic<uint64_t> command_counter{0};
};

inline Registry g_registry{};

inline void initialize_registry() noexcept {
	g_registry.process_start_ns.store(monotonic_nanoseconds(), std::memory_order_release);
}

[[nodiscard]] inline double uptime_seconds() noexcept {
	const int64_t start = g_registry.process_start_ns.load(std::memory_order_acquire);
	if (start == 0) {
		return 0.0;
	}
	return static_cast<double>(monotonic_nanoseconds() - start) * 1.0e-9;
}

inline void set_phase(AppPhase phase) noexcept {
	g_registry.phase.store(static_cast<uint32_t>(phase), std::memory_order_release);
}

[[nodiscard]] inline AppPhase current_phase() noexcept {
	return static_cast<AppPhase>(g_registry.phase.load(std::memory_order_acquire));
}

inline void count_frame() noexcept {
	g_registry.frame_counter.fetch_add(1, std::memory_order_relaxed);
}

inline void count_command() noexcept {
	g_registry.command_counter.fetch_add(1, std::memory_order_relaxed);
}

inline void record_event(std::string_view text) noexcept {
	const uint64_t index = g_registry.event_cursor.fetch_add(1, std::memory_order_acq_rel);
	EventSlot& slot = g_registry.events[static_cast<size_t>(index % kEventRingSize)];
	const size_t limit = kEventLineCapacity - 48;
	const int length = static_cast<int>((text.size() < limit) ? text.size() : limit);
	char line[kEventLineCapacity];
	std::snprintf(line, sizeof(line), "[%10.3f] [T%05llu] %.*s", uptime_seconds(), static_cast<unsigned long long>(current_thread_id()), length, text.data());
	for (char& character : line) {
		if (character == '\0') {
			break;
		}
		if (character == '\n' || character == '\r') {
			character = ' ';
		}
	}
	copy_text(slot.text.data(), kEventLineCapacity, line);
	slot.sequence.store(index + 1, std::memory_order_release);
}

inline void write_recent_events(std::FILE* out, size_t maximum) noexcept {
	const uint64_t cursor = g_registry.event_cursor.load(std::memory_order_acquire);
	uint64_t first = (cursor > kEventRingSize) ? (cursor - kEventRingSize) : 0;
	if (cursor - first > maximum) {
		first = cursor - maximum;
	}
	for (uint64_t index = first; index < cursor; ++index) {
		const EventSlot& slot = g_registry.events[static_cast<size_t>(index % kEventRingSize)];
		if (slot.sequence.load(std::memory_order_acquire) != index + 1) {
			continue;
		}
		std::fprintf(out, "  %s\n", slot.text.data());
	}
}

inline void set_environment(std::string_view key, std::string_view value) noexcept {
	std::lock_guard<std::mutex> lock(g_registry.environment_mutex);
	EnvironmentEntry* target = nullptr;
	for (EnvironmentEntry& entry : g_registry.environment) {
		if (!entry.used.load(std::memory_order_acquire)) {
			if (target == nullptr) {
				target = &entry;
			}
			continue;
		}
		if (key == std::string_view(entry.key.data())) {
			target = &entry;
			break;
		}
	}
	if (target == nullptr) {
		return;
	}
	copy_text(target->key.data(), kEnvironmentKeyCapacity, key);
	copy_text(target->value.data(), kEnvironmentValueCapacity, value);
	target->used.store(true, std::memory_order_release);
}

inline void write_environment(std::FILE* out) noexcept {
	bool any = false;
	for (const EnvironmentEntry& entry : g_registry.environment) {
		if (!entry.used.load(std::memory_order_acquire)) {
			continue;
		}
		any = true;
		std::fprintf(out, "  %-28s %s\n", entry.key.data(), entry.value.data());
	}
	if (!any) {
		std::fprintf(out, "  (no entries recorded)\n");
	}
}

struct ActivityHandle {
	ActivitySlot* slot{nullptr};

	ActivityHandle() noexcept {
		const uint64_t id = current_thread_id();
		for (ActivitySlot& candidate : g_registry.activities) {
			uint64_t expected = 0;
			if (candidate.thread_id.compare_exchange_strong(expected, id, std::memory_order_acq_rel)) {
				candidate.label[0] = '\0';
				candidate.thread_name[0] = '\0';
				candidate.since_ns.store(monotonic_nanoseconds(), std::memory_order_relaxed);
				slot = &candidate;
				return;
			}
		}
	}

	~ActivityHandle() {
		if (slot != nullptr) {
			slot->label[0] = '\0';
			slot->thread_name[0] = '\0';
			slot->thread_id.store(0, std::memory_order_release);
		}
	}

	ActivityHandle(const ActivityHandle&) = delete;
	ActivityHandle& operator=(const ActivityHandle&) = delete;
};

[[nodiscard]] inline ActivitySlot* current_activity_slot() noexcept {
	thread_local ActivityHandle handle;
	return handle.slot;
}

inline void set_thread_name(std::string_view name) noexcept {
	ActivitySlot* slot = current_activity_slot();
	if (slot != nullptr) {
		copy_text(slot->thread_name.data(), kThreadNameCapacity, name);
	}
}

inline void set_activity(std::string_view label) noexcept {
	ActivitySlot* slot = current_activity_slot();
	if (slot != nullptr) {
		copy_text(slot->label.data(), kActivityLabelCapacity, label);
		slot->since_ns.store(monotonic_nanoseconds(), std::memory_order_relaxed);
	}
}

class ActivityScope {
private:
	ActivitySlot* slot_;
	std::array<char, kActivityLabelCapacity> previous_{};

public:
	explicit ActivityScope(std::string_view label) noexcept : slot_(current_activity_slot()) {
		if (slot_ != nullptr) {
			previous_ = slot_->label;
			copy_text(slot_->label.data(), kActivityLabelCapacity, label);
			slot_->since_ns.store(monotonic_nanoseconds(), std::memory_order_relaxed);
		}
	}

	~ActivityScope() {
		if (slot_ != nullptr) {
			slot_->label = previous_;
			slot_->since_ns.store(monotonic_nanoseconds(), std::memory_order_relaxed);
		}
	}

	ActivityScope(const ActivityScope&) = delete;
	ActivityScope& operator=(const ActivityScope&) = delete;
};

[[nodiscard]] inline const ActivitySlot* find_activity_slot(uint64_t thread_id) noexcept {
	for (const ActivitySlot& slot : g_registry.activities) {
		if (slot.thread_id.load(std::memory_order_acquire) == thread_id) {
			return &slot;
		}
	}
	return nullptr;
}

inline void write_activity_table(std::FILE* out, uint64_t faulting_thread) noexcept {
	const int64_t now = monotonic_nanoseconds();
	bool any = false;
	for (const ActivitySlot& slot : g_registry.activities) {
		const uint64_t id = slot.thread_id.load(std::memory_order_acquire);
		if (id == 0) {
			continue;
		}
		any = true;
		const double seconds = static_cast<double>(now - slot.since_ns.load(std::memory_order_relaxed)) * 1.0e-9;
		std::fprintf(
			out, "  thread %llu%s  name=%s  activity=%s  (for %.3f s)\n",
			static_cast<unsigned long long>(id), (id == faulting_thread) ? " [FAULTING THREAD]" : "",
			(slot.thread_name[0] != '\0') ? slot.thread_name.data() : "-",
			(slot.label[0] != '\0') ? slot.label.data() : "-", seconds
		);
	}
	if (!any) {
		std::fprintf(out, "  (no instrumented threads)\n");
	}
}

inline void publish_snapshot(SnapshotBuffer& buffer, std::string_view text) noexcept {
	const uint32_t target = 1U - buffer.active.load(std::memory_order_acquire);
	copy_text(buffer.storage[target].data(), kSnapshotCapacity, text);
	buffer.stamp_ns.store(monotonic_nanoseconds(), std::memory_order_relaxed);
	buffer.active.store(target, std::memory_order_release);
	buffer.published.store(true, std::memory_order_release);
}

inline void publish_scene_snapshot(std::string_view text) noexcept {
	publish_snapshot(g_registry.scene, text);
}

inline void publish_render_snapshot(std::string_view text) noexcept {
	publish_snapshot(g_registry.render, text);
}

inline void publish_startup_snapshot(std::string_view text) noexcept {
	publish_snapshot(g_registry.startup, text);
}

inline void write_snapshot(std::FILE* out, const SnapshotBuffer& buffer, const char* empty_message) noexcept {
	if (!buffer.published.load(std::memory_order_acquire)) {
		std::fprintf(out, "  %s\n", empty_message);
		return;
	}
	const double age = static_cast<double>(monotonic_nanoseconds() - buffer.stamp_ns.load(std::memory_order_relaxed)) * 1.0e-9;
	std::fprintf(out, "  (captured %.3f s before this report)\n%s\n", age, buffer.storage[buffer.active.load(std::memory_order_acquire)].data());
}

}
