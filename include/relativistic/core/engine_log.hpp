#pragma once

#include <string>
#include <string_view>
#include <deque>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>

namespace Relativistic::Core {

enum class LogLevel : uint32_t {
	Info = 0,
	Warning = 1,
	Error = 2
};

struct LogEntry {
	LogLevel level{LogLevel::Info};
	std::string message{};
	double timestamp_seconds{0.0};
};

using EventSink = void (*)(std::string_view line) noexcept;

class EngineLog {
private:
	static constexpr size_t kMaxEntries = 4096;

	std::deque<LogEntry> entries_{};
	mutable std::mutex mutex_{};
	std::chrono::steady_clock::time_point start_time_{std::chrono::steady_clock::now()};

	std::FILE* session_file_{nullptr};
	std::string session_path_{};
	std::time_t start_wall_{std::time(nullptr)};
	std::atomic<EventSink> event_sink_{nullptr};
	static inline std::atomic<uint32_t> thread_counter_{1};

	EngineLog() = default;

	[[nodiscard]] static uint32_t thread_ordinal() noexcept {
		thread_local const uint32_t ordinal = thread_counter_.fetch_add(1, std::memory_order_relaxed);
		return ordinal;
	}

	[[nodiscard]] static const char* tag_for(LogLevel level) noexcept {
		return (level == LogLevel::Error) ? "ERROR" : (level == LogLevel::Warning) ? "WARN " : "INFO ";
	}

	void write_session_line(const char* tag, double uptime, uint32_t thread, std::string_view message) {
		if (session_file_ == nullptr) {
			return;
		}
		const std::time_t wall = start_wall_ + static_cast<std::time_t>(uptime);
		char stamp[32]{};
		if (const std::tm* local = std::localtime(&wall)) {
			std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", local);
		}
		std::fprintf(session_file_, "[%s] [%9.3fs] [T%02u] [%s] %.*s\n", stamp, uptime, thread, tag, static_cast<int>(message.size()), message.data());
		std::fflush(session_file_);
	}

	void emit(LogLevel level, std::string_view message, bool trace_only) {
		const double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time_).count();
		const uint32_t thread = thread_ordinal();
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (!trace_only) {
				entries_.push_back(LogEntry{level, std::string(message), t});
				while (entries_.size() > kMaxEntries) {
					entries_.pop_front();
				}
			}
			write_session_line(trace_only ? "TRACE" : tag_for(level), t, thread, message);
		}
		if (!trace_only) {
			FILE* stream = stream_for(level);
			std::fprintf(stream, "%s%s\n", prefix_for(level), std::string(message).c_str());
			std::fflush(stream);
		}
		if (const EventSink sink = event_sink_.load(std::memory_order_acquire)) {
			std::string line(trace_only ? "[TRACE] " : prefix_for(level));
			line.append(message);
			sink(line);
		}
	}

public:
	static EngineLog& instance() noexcept {
		static EngineLog log;
		return log;
	}

	~EngineLog() {
		close_session_file();
	}

	void log(LogLevel level, std::string_view message) {
		emit(level, message, false);
	}

	void trace(std::string_view message) {
		emit(LogLevel::Info, message, true);
	}

	[[nodiscard]] bool open_session_file(const std::string& path) {
		std::FILE* file = std::fopen(path.c_str(), "ab");
		if (file == nullptr) {
			return false;
		}
		std::lock_guard<std::mutex> lock(mutex_);
		if (session_file_ != nullptr) {
			std::fclose(session_file_);
		}
		session_file_ = file;
		session_path_ = path;
		for (const LogEntry& entry : entries_) {
			write_session_line(tag_for(entry.level), entry.timestamp_seconds, 0U, entry.message);
		}
		return true;
	}

	void close_session_file() noexcept {
		std::lock_guard<std::mutex> lock(mutex_);
		if (session_file_ != nullptr) {
			std::fflush(session_file_);
			std::fclose(session_file_);
			session_file_ = nullptr;
		}
	}

	void set_event_sink(EventSink sink) noexcept {
		event_sink_.store(sink, std::memory_order_release);
	}

	[[nodiscard]] std::deque<LogEntry> snapshot() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return entries_;
	}

	[[nodiscard]] static const char* prefix_for(LogLevel level) noexcept {
		return (level == LogLevel::Error) ? "[ERROR] " : (level == LogLevel::Warning) ? "[WARN] " : "[INFO] ";
	}

	[[nodiscard]] static FILE* stream_for(LogLevel level) noexcept {
		return (level == LogLevel::Error) ? stderr : stdout;
	}

	void replay_to_console() const {
		const std::deque<LogEntry> entries = snapshot();
		for (const auto& entry : entries) {
			std::fprintf(stream_for(entry.level), "%s%s\n", prefix_for(entry.level), entry.message.c_str());
		}
		std::fflush(stdout);
		std::fflush(stderr);
	}

	void clear() noexcept {
		std::lock_guard<std::mutex> lock(mutex_);
		entries_.clear();
	}
};

inline void log_info(std::string_view msg) { EngineLog::instance().log(LogLevel::Info, msg); }
inline void log_warning(std::string_view msg) { EngineLog::instance().log(LogLevel::Warning, msg); }
inline void log_error(std::string_view msg) { EngineLog::instance().log(LogLevel::Error, msg); }
inline void log_trace(std::string_view msg) { EngineLog::instance().trace(msg); }

}
