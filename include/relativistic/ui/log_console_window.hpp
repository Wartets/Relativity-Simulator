#pragma once

#include "relativistic/core/engine_log.hpp"
#include "relativistic/core/system_console.hpp"
#include <imgui.h>

namespace Relativistic::UI {

class LogConsoleWindow {
private:
	bool is_open_{false};
	bool show_info_{true};
	bool show_warnings_{true};
	bool show_errors_{true};
	bool auto_scroll_{true};
	bool* system_console_visible_{nullptr};

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

		ImGui::Separator();
		ImGui::BeginChild("LogScrollRegion", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

		const auto entries = log.snapshot();
		for (const auto& entry : entries) {
			if (entry.level == Relativistic::Core::LogLevel::Info && !show_info_) continue;
			if (entry.level == Relativistic::Core::LogLevel::Warning && !show_warnings_) continue;
			if (entry.level == Relativistic::Core::LogLevel::Error && !show_errors_) continue;

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
