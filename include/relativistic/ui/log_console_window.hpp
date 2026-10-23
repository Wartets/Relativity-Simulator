#pragma once

#include "relativistic/core/engine_log.hpp"
#include "relativistic/core/system_console.hpp"
#include "relativistic/core/diagnostics/diagnostics_session.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>

namespace Relativistic::UI {

class LogConsoleWindow {
private:
	bool is_open_{false};
	bool show_info_{true};
	bool show_warnings_{true};
	bool show_errors_{true};
	bool auto_scroll_{true};
	bool* system_console_visible_{nullptr};
	char filter_text_[128]{};
	std::string snapshot_status_{};

	[[nodiscard]] static const char* level_tag(Relativistic::Core::LogLevel level) noexcept {
		return (level == Relativistic::Core::LogLevel::Error) ? "ERROR" : ((level == Relativistic::Core::LogLevel::Warning) ? "WARN" : "INFO");
	}

	[[nodiscard]] bool entry_matches_filter(const Relativistic::Core::LogEntry& entry) const {
		if (filter_text_[0] == '\0') {
			return true;
		}
		std::string haystack = entry.message;
		std::string needle = filter_text_;
		std::transform(haystack.begin(), haystack.end(), haystack.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		std::transform(needle.begin(), needle.end(), needle.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return haystack.find(needle) != std::string::npos;
	}

public:
	[[nodiscard]] bool& open_state() noexcept { return is_open_; }

	void attach_system_console_flag(bool& flag) noexcept {
		system_console_visible_ = &flag;
	}

	void render() {
		if (!is_open_) return;

		ImGui::SetNextWindowPos(ImVec2(200.0f, 500.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(720.0f, 360.0f), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin("Engine Log Console", &is_open_)) {
			ImGui::End();
			return;
		}

		auto& log = Relativistic::Core::EngineLog::instance();

		if (system_console_visible_ != nullptr && Relativistic::Core::SystemConsole::is_supported()) {
			if (ImGui::Checkbox("Show System Console Window", system_console_visible_)) {
				Relativistic::Core::SystemConsole::set_visible(*system_console_visible_);
			}
			ImGui::SameLine();
		}

		ImGui::Checkbox("Info", &show_info_);
		ImGui::SameLine();
		ImGui::Checkbox("Warnings", &show_warnings_);
		ImGui::SameLine();
		ImGui::Checkbox("Errors", &show_errors_);
		ImGui::SameLine();
		ImGui::Checkbox("Auto-Scroll", &auto_scroll_);
		ImGui::SameLine();
		if (ImGui::SmallButton("Clear")) {
			log.clear();
		}

		ImGui::SetNextItemWidth(220.0f);
		ImGui::InputTextWithHint("##LogFilter", "Filter text...", filter_text_, sizeof(filter_text_));
		render_setting_tooltip("Shows only the log lines containing this text, ignoring case.");
		ImGui::SameLine();
		if (ImGui::SmallButton("Copy Visible")) {
			std::string text;
			for (const auto& entry : log.snapshot()) {
				if (entry.level == Relativistic::Core::LogLevel::Info && !show_info_) continue;
				if (entry.level == Relativistic::Core::LogLevel::Warning && !show_warnings_) continue;
				if (entry.level == Relativistic::Core::LogLevel::Error && !show_errors_) continue;
				if (!entry_matches_filter(entry)) continue;
				char prefix[48];
				std::snprintf(prefix, sizeof(prefix), "[%8.2fs] [%s] ", entry.timestamp_seconds, level_tag(entry.level));
				text += prefix;
				text += entry.message;
				text += '\n';
			}
			ImGui::SetClipboardText(text.c_str());
		}
		render_setting_tooltip("Copies every line currently visible with the active level toggles and filter to the clipboard.");
		ImGui::SameLine();
		if (ImGui::SmallButton("Capture Diagnostic Snapshot")) {
			const std::string folder = Relativistic::Core::Diagnostics::CrashReporter::capture_snapshot("manual snapshot requested from the log console");
			snapshot_status_ = folder.empty() ? std::string("The diagnostic snapshot could not be written.") : ("Diagnostic snapshot written to " + folder);
			Relativistic::Core::log_info(snapshot_status_);
		}
		render_setting_tooltip("Writes a complete live diagnostic report (system, graphics, command journal, milestones, thread activity, session log and active scenario) without stopping the application.");
		ImGui::TextDisabled("Warnings: %llu | Errors: %llu | Session log: %s",
			static_cast<unsigned long long>(Relativistic::Core::Diagnostics::g_registry.warning_counter.load(std::memory_order_relaxed)),
			static_cast<unsigned long long>(Relativistic::Core::Diagnostics::g_registry.error_counter.load(std::memory_order_relaxed)),
			Relativistic::Core::Diagnostics::DiagnosticsSession::paths().session_log_file.generic_string().c_str());
		if (!snapshot_status_.empty()) {
			ImGui::TextWrapped("%s", snapshot_status_.c_str());
		}

		ImGui::Separator();
		ImGui::BeginChild("LogScrollRegion", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

		const auto entries = log.snapshot();
		for (const auto& entry : entries) {
			if (entry.level == Relativistic::Core::LogLevel::Info && !show_info_) continue;
			if (entry.level == Relativistic::Core::LogLevel::Warning && !show_warnings_) continue;
			if (entry.level == Relativistic::Core::LogLevel::Error && !show_errors_) continue;
			if (!entry_matches_filter(entry)) continue;

			ImVec4 color(0.85f, 0.85f, 0.85f, 1.0f);
			const char* tag = "INFO";
			if (entry.level == Relativistic::Core::LogLevel::Warning) {
				color = ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
				tag = "WARN";
			} else if (entry.level == Relativistic::Core::LogLevel::Error) {
				color = ImVec4(1.0f, 0.4f, 0.35f, 1.0f);
				tag = "ERROR";
			}

			ImGui::TextColored(color, "[%8.2fs] [%s] %s", entry.timestamp_seconds, tag, entry.message.c_str());
		}

		if (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.0f) {
			ImGui::SetScrollHereY(1.0f);
		}

		ImGui::EndChild();
		ImGui::End();
	}
};

}
