#pragma once

#include "relativistic/capture/motion_script.hpp"
#include "relativistic/io/capture_settings_io.hpp"
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Relativistic::Capture {

[[nodiscard]] inline std::string motion_script_to_text(const MotionScript& script) {
	std::ostringstream out;
	IO::SettingsWriter writer(out);
	script.write(writer);
	return out.str();
}

[[nodiscard]] inline std::optional<MotionScript> motion_script_from_text(std::string_view text) {
	std::unordered_map<std::string, std::string> entries;
	size_t position = 0;
	while (position < text.size()) {
		size_t line_end = text.find('\n', position);
		if (line_end == std::string_view::npos) {
			line_end = text.size();
		}
		std::string_view line = text.substr(position, line_end - position);
		position = line_end + 1;
		while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
			line.remove_suffix(1);
		}
		const size_t equals = line.find('=');
		if (equals == std::string_view::npos) {
			continue;
		}
		entries[std::string(line.substr(0, equals))] = std::string(line.substr(equals + 1));
	}
	if (entries.find("script.segments") == entries.end()) {
		return std::nullopt;
	}
	const IO::SettingsReader reader(entries);
	MotionScript script;
	script.read(reader);
	script.sanitize();
	return script;
}

[[nodiscard]] inline std::string save_motion_script(const std::filesystem::path& path, const MotionScript& script) {
	std::error_code ec;
	if (path.has_parent_path()) {
		std::filesystem::create_directories(path.parent_path(), ec);
	}
	std::ofstream out(path, std::ios::trunc);
	if (!out.is_open()) {
		return "The script file could not be written: " + path.string();
	}
	out << motion_script_to_text(script);
	out.flush();
	if (!out.good()) {
		return "Writing the script file failed: " + path.string();
	}
	return {};
}

[[nodiscard]] inline std::string load_motion_script(const std::filesystem::path& path, MotionScript& destination) {
	std::ifstream in(path);
	if (!in.is_open()) {
		return "The script file could not be opened: " + path.string();
	}
	std::ostringstream buffer;
	buffer << in.rdbuf();
	auto parsed = motion_script_from_text(buffer.str());
	if (!parsed.has_value()) {
		return "The file does not contain a valid motion script: " + path.string();
	}
	destination = std::move(*parsed);
	return {};
}

}
