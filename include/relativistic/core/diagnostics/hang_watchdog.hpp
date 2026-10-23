#pragma once

#include "relativistic/core/diagnostics/crash_reporter.hpp"
#include "relativistic/core/diagnostics/diagnostics_registry.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <stop_token>
#include <string>
#include <thread>

namespace Relativistic::Core::Diagnostics {

struct HangWatchdogConfig {
	double interface_stall_seconds{20.0};
	double simulation_stall_seconds{45.0};
	uint32_t poll_interval_milliseconds{500};
	uint32_t maximum_reports{3};
};

class HangWatchdog {
private:
	std::jthread worker_{};
	HangWatchdogConfig config_{};
	std::atomic<uint32_t> reports_{0};

	HangWatchdog() = default;

	[[nodiscard]] static bool interface_thread_in_system_loop() noexcept {
#if defined(_WIN32)
		const DWORD id = static_cast<DWORD>(g_registry.ui_thread_id.load(std::memory_order_acquire));
		if (id == 0) {
			return false;
		}
		GUITHREADINFO info{};
		info.cbSize = sizeof(info);
		if (GetGUIThreadInfo(id, &info) == FALSE) {
			return false;
		}
		return (info.flags & (GUI_INMOVESIZE | GUI_INMENUMODE | GUI_POPUPMENUMODE | GUI_SYSTEMMENUMODE)) != 0;
#else
		return false;
#endif
	}

	void report(const char* reason, const char* detail, uint64_t thread_id) noexcept {
		if (reports_.load(std::memory_order_acquire) >= config_.maximum_reports) {
			return;
		}
		reports_.fetch_add(1, std::memory_order_acq_rel);
		record_problem(true, detail);
		const std::string folder = CrashReporter::report_hang(reason, detail, thread_id);
		std::fprintf(stderr, "[FATAL] %s. Diagnostic report: %s\n", detail, folder.empty() ? "(not written)" : folder.c_str());
		char milestone[256];
		std::snprintf(milestone, sizeof(milestone), "watchdog report written: %s", folder.empty() ? "(not written)" : folder.c_str());
		record_milestone(milestone);
	}

	void run(std::stop_token token) noexcept {
		set_thread_name("watchdog");
		AppPhase observed_phase = current_phase();
		int64_t running_since = monotonic_nanoseconds();
		int64_t previous_tick = running_since;
		int64_t grace_until = 0;
		bool interface_flagged = false;
		bool simulation_flagged = false;

		while (!token.stop_requested()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(config_.poll_interval_milliseconds));
			const int64_t now = monotonic_nanoseconds();
			const bool resumed = (now - previous_tick) > 5000000000LL;
			previous_tick = now;
			if (resumed) {
				grace_until = now + 3000000000LL;
				interface_flagged = false;
				simulation_flagged = false;
			}
			const AppPhase phase = current_phase();
			if (phase != observed_phase) {
				observed_phase = phase;
				running_since = now;
				interface_flagged = false;
				simulation_flagged = false;
			}
			const bool interface_expected = (phase == AppPhase::Running);
			const bool simulation_expected = (phase == AppPhase::Running || phase == AppPhase::Headless);
			if (!simulation_expected || now < grace_until) {
				continue;
			}

			const auto evaluate = [&](int64_t reference_ns, double threshold, bool& flagged, const char* what, const char* reason, uint64_t thread_id, bool allowed) noexcept {
				const double stalled = static_cast<double>(now - std::max(reference_ns, running_since)) * 1.0e-9;
				if (stalled < threshold) {
					if (flagged) {
						flagged = false;
						char text[160];
						std::snprintf(text, sizeof(text), "%s recovered after a stall", what);
						record_milestone(text);
					}
					return;
				}
				if (flagged || !allowed) {
					return;
				}
				flagged = true;
				char detail[192];
				std::snprintf(detail, sizeof(detail), "%s made no progress for %.1f s", what, stalled);
				report(reason, detail, thread_id);
			};

			if (interface_expected) {
				evaluate(g_registry.last_frame_ns.load(std::memory_order_relaxed), config_.interface_stall_seconds, interface_flagged, "the interface thread", "interface thread stalled", g_registry.ui_thread_id.load(std::memory_order_acquire), !interface_thread_in_system_loop());
			}
			evaluate(g_registry.simulation_heartbeat_ns.load(std::memory_order_relaxed), config_.simulation_stall_seconds, simulation_flagged, "the simulation thread", "simulation thread stalled", g_registry.simulation_thread_id.load(std::memory_order_acquire), true);
		}
	}

public:
	HangWatchdog(const HangWatchdog&) = delete;
	HangWatchdog& operator=(const HangWatchdog&) = delete;

	[[nodiscard]] static HangWatchdog& instance() noexcept {
		static HangWatchdog watchdog;
		return watchdog;
	}

	void start(const HangWatchdogConfig& config = {}) noexcept {
		if (worker_.joinable()) {
			return;
		}
		config_ = config;
		try {
			worker_ = std::jthread([this](std::stop_token token) {
				run(token);
			});
		} catch (...) {
		}
	}

	void stop() noexcept {
		if (worker_.joinable()) {
			worker_.request_stop();
			worker_.join();
		}
	}

	[[nodiscard]] uint32_t reports_written() const noexcept {
		return reports_.load(std::memory_order_acquire);
	}
};

}
