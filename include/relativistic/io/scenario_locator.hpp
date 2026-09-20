#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <numbers>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include "relativistic/io/scenario_serializer.hpp"

namespace Relativistic::IO {

struct StartupScenarioResolution {
	std::string path{};
	bool used_fallback{false};
};

class ScenarioLocator {
public:
	static constexpr std::string_view kDirectoryName = "scenarios";
	static constexpr std::string_view kBuiltInStartupFileName = "schwarzschild_accretion.yaml";
	static constexpr std::string_view kBuiltInStartupScenario = "scenarios/schwarzschild_accretion.yaml";
	static constexpr size_t kMaxAncestorDepth = 4;

private:
	[[nodiscard]] static std::filesystem::path ancestor_prefix(size_t depth) {
		std::filesystem::path prefix;
		for (size_t level = 0; level < depth; ++level) {
			prefix /= "..";
		}
		return prefix;
	}

	[[nodiscard]] static bool is_regular_file_quiet(const std::filesystem::path& candidate) noexcept {
		std::error_code ec;
		return std::filesystem::is_regular_file(candidate, ec) && !ec;
	}

public:
	[[nodiscard]] static std::optional<std::filesystem::path> find_directory() {
		std::error_code ec;
		for (size_t depth = 0; depth <= kMaxAncestorDepth; ++depth) {
			const std::filesystem::path candidate = (ancestor_prefix(depth) / std::filesystem::path(kDirectoryName)).lexically_normal();
			if (std::filesystem::is_directory(candidate, ec) && !ec) {
				return candidate;
			}
			ec.clear();
		}
		return std::nullopt;
	}

	[[nodiscard]] static std::optional<std::filesystem::path> resolve_file(std::string_view file_path) {
		if (file_path.empty()) {
			return std::nullopt;
		}

		const std::filesystem::path requested{std::string(file_path)};
		if (is_regular_file_quiet(requested)) {
			return requested;
		}

		if (requested.is_relative()) {
			for (size_t depth = 1; depth <= kMaxAncestorDepth; ++depth) {
				const std::filesystem::path candidate = (ancestor_prefix(depth) / requested).lexically_normal();
				if (is_regular_file_quiet(candidate)) {
					return candidate;
				}
			}
		}

		if (requested.has_filename() && requested.parent_path().filename().string() == kDirectoryName) {
			if (const auto directory = find_directory(); directory.has_value()) {
				const std::filesystem::path candidate = *directory / requested.filename();
				if (is_regular_file_quiet(candidate)) {
					return candidate;
				}
			}
		}

		return std::nullopt;
	}

	[[nodiscard]] static bool same_file(std::string_view lhs, std::string_view rhs) {
		if (lhs.empty() || rhs.empty()) {
			return false;
		}

		const auto resolved_lhs = resolve_file(lhs);
		const auto resolved_rhs = resolve_file(rhs);
		if (resolved_lhs.has_value() && resolved_rhs.has_value()) {
			std::error_code ec;
			const bool is_same = std::filesystem::equivalent(*resolved_lhs, *resolved_rhs, ec);
			return !ec && is_same;
		}

		return std::filesystem::path(std::string(lhs)).lexically_normal() == std::filesystem::path(std::string(rhs)).lexically_normal();
	}

	[[nodiscard]] static std::string portable_path(std::string_view file_path) {
		const std::filesystem::path file{std::string(file_path)};
		if (file.has_filename() && !file.parent_path().empty()) {
			const auto directory = find_directory();
			std::error_code ec;
			if (directory.has_value() && std::filesystem::equivalent(file.parent_path(), *directory, ec) && !ec) {
				return (std::filesystem::path(kDirectoryName) / file.filename()).generic_string();
			}
		}
		return file.generic_string();
	}

	[[nodiscard]] static bool is_loadable(const std::filesystem::path& file) {
		std::ifstream stream(file);
		if (!stream.is_open()) {
			return false;
		}

		std::ostringstream buffer;
		buffer << stream.rdbuf();
		const auto scenario = ScenarioSerializer::from_yaml(buffer.str());
		return scenario.has_value() && ScenarioSerializer::validate(*scenario).is_valid;
	}

	[[nodiscard]] static std::optional<StartupScenarioResolution> resolve_startup_scenario(std::string_view configured_path) {
		if (!configured_path.empty()) {
			const auto configured = resolve_file(configured_path);
			if (configured.has_value() && is_loadable(*configured)) {
				return StartupScenarioResolution{configured->generic_string(), false};
			}
		}

		const auto built_in = resolve_file(kBuiltInStartupScenario);
		if (built_in.has_value() && is_loadable(*built_in)) {
			return StartupScenarioResolution{built_in->generic_string(), true};
		}

		return std::nullopt;
	}
};

}
