#pragma once

#include <string>
#include <string_view>
#include <deque>
#include <mutex>
#include <chrono>
#include <cstdio>

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

class EngineLog {
private:
	static constexpr size_t kMaxEntries = 4096;

	std::deque<LogEntry> entries_{};
	mutable std::mutex mutex_{};
	std::chrono::steady_clock::time_point start_time_{std::chrono::steady_clock::now()};

	EngineLog() = default;

public:
	static EngineLog& instance() noexcept {
		static EngineLog log;
		return log;
	}

	void log(LogLevel level, std::string_view message) {
		const double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time_).count();
		{
			std::lock_guard<std::mutex> lock(mutex_);
			entries_.push_back(LogEntry{level, std::string(message), t});
			while (entries_.size() > kMaxEntries) {
				entries_.pop_front();
			}
		}
		const char* prefix = (level == LogLevel::Error) ? "[ERROR] " : (level == LogLevel::Warning) ? "[WARN] " : "[INFO] ";
		std::fprintf(level == LogLevel::Error ? stderr : stdout, "%s%s\n", prefix, std::string(message).c_str());
	}

	[[nodiscard]] std::deque<LogEntry> snapshot() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return entries_;
	}

	void clear() noexcept {
		std::lock_guard<std::mutex> lock(mutex_);
		entries_.clear();
	}
};

inline void log_info(std::string_view msg) { EngineLog::instance().log(LogLevel::Info, msg); }
inline void log_warning(std::string_view msg) { EngineLog::instance().log(LogLevel::Warning, msg); }
inline void log_error(std::string_view msg) { EngineLog::instance().log(LogLevel::Error, msg); }

}
