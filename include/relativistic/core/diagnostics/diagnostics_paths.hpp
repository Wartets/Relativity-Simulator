#pragma once

#include "relativistic/core/diagnostics/diagnostics_registry.hpp"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace Relativistic::Core::Diagnostics {

struct DiagnosticsPaths {
	std::filesystem::path root_directory{};
	std::filesystem::path sessions_directory{};
	std::filesystem::path crashes_directory{};
	std::filesystem::path session_log_file{};
	std::filesystem::path crash_marker_file{};
	std::string session_id{};
	bool relocated{false};
};

class DiagnosticsLocator {
public:
	static constexpr size_t kRetainedSessionLogs = 40;
	static constexpr size_t kRetainedCrashFolders = 25;

	[[nodiscard]] static DiagnosticsPaths resolve(std::time_t start_time) {
		DiagnosticsPaths paths;
		std::error_code ec;
		bool first_candidate = true;
		for (const std::filesystem::path& candidate : candidate_roots()) {
			if (is_writable_directory(candidate)) {
				paths.root_directory = std::filesystem::absolute(candidate, ec);
				if (ec) {
					paths.root_directory = candidate;
				}
				paths.relocated = !first_candidate;
				break;
			}
			first_candidate = false;
		}
		if (paths.root_directory.empty()) {
			return paths;
		}

		char stamp[48];
		format_wall_clock(stamp, sizeof(stamp), "%Y-%m-%d_%H-%M-%S", start_time);
		paths.session_id = std::string(stamp) + "_pid" + std::to_string(current_process_id());
		paths.sessions_directory = paths.root_directory / "sessions";
		paths.crashes_directory = paths.root_directory / "crashes";
		std::filesystem::create_directories(paths.sessions_directory, ec);
		std::filesystem::create_directories(paths.crashes_directory, ec);
		prune(paths.sessions_directory, kRetainedSessionLogs);
		prune(paths.crashes_directory, kRetainedCrashFolders);
		paths.session_log_file = paths.sessions_directory / ("session_" + paths.session_id + ".log");
		paths.crash_marker_file = paths.root_directory / "last_crash.txt";
		return paths;
	}

private:
	[[nodiscard]] static std::vector<std::filesystem::path> candidate_roots() {
		std::vector<std::filesystem::path> roots;
		std::error_code ec;
		const std::filesystem::path working = std::filesystem::current_path(ec);
		if (!ec) {
			roots.push_back(working / "logs");
		}
#if defined(_WIN32)
		if (const char* base = std::getenv("LOCALAPPDATA"); base != nullptr && base[0] != '\0') {
			roots.push_back(std::filesystem::path(base) / "RelativisticEngine" / "logs");
		}
#else
		if (const char* state = std::getenv("XDG_STATE_HOME"); state != nullptr && state[0] != '\0') {
			roots.push_back(std::filesystem::path(state) / "relativistic-engine" / "logs");
		} else if (const char* home = std::getenv("HOME"); home != nullptr && home[0] != '\0') {
			roots.push_back(std::filesystem::path(home) / ".local" / "state" / "relativistic-engine" / "logs");
		}
#endif
		const std::filesystem::path temporary = std::filesystem::temp_directory_path(ec);
		if (!ec) {
			roots.push_back(temporary / "relativistic-engine-logs");
		}
		return roots;
	}

	[[nodiscard]] static bool is_writable_directory(const std::filesystem::path& directory) {
		std::error_code ec;
		std::filesystem::create_directories(directory, ec);
		if (ec) {
			return false;
		}
		const std::filesystem::path probe = directory / (".write_probe_" + std::to_string(current_process_id()));
		{
			std::ofstream stream(probe, std::ios::trunc);
			if (!stream.is_open()) {
				return false;
			}
			stream << "probe";
		}
		std::filesystem::remove(probe, ec);
		return true;
	}

	static void prune(const std::filesystem::path& directory, size_t retained) {
		std::error_code ec;
		std::vector<std::filesystem::path> entries;
		for (std::filesystem::directory_iterator it(directory, ec), end; !ec && it != end; it.increment(ec)) {
			entries.push_back(it->path());
		}
		if (entries.size() <= retained) {
			return;
		}
		std::vector<std::pair<std::filesystem::file_time_type, std::filesystem::path>> dated;
		dated.reserve(entries.size());
		for (const std::filesystem::path& entry : entries) {
			const auto modified = std::filesystem::last_write_time(entry, ec);
			dated.emplace_back(ec ? std::filesystem::file_time_type::min() : modified, entry);
			ec.clear();
		}
		std::sort(dated.begin(), dated.end(), [](const auto& left, const auto& right) {
			return left.first < right.first;
		});
		const size_t excess = dated.size() - retained;
		for (size_t index = 0; index < excess; ++index) {
			std::filesystem::remove_all(dated[index].second, ec);
		}
	}
};

}
