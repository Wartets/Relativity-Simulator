#pragma once

#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <ostream>
#include <string>
#include <unordered_map>

namespace Relativistic::IO {

class SettingsReader {
private:
	const std::unordered_map<std::string, std::string>& entries_;

public:
	explicit SettingsReader(const std::unordered_map<std::string, std::string>& entries) noexcept : entries_(entries) {}

	[[nodiscard]] double real(const std::string& key, double fallback) const {
		const auto it = entries_.find(key);
		return (it != entries_.end()) ? std::strtod(it->second.c_str(), nullptr) : fallback;
	}

	[[nodiscard]] uint32_t unsigned_value(const std::string& key, uint32_t fallback) const {
		const auto it = entries_.find(key);
		return (it != entries_.end()) ? static_cast<uint32_t>(std::strtoul(it->second.c_str(), nullptr, 10)) : fallback;
	}

	[[nodiscard]] uint64_t wide_value(const std::string& key, uint64_t fallback) const {
		const auto it = entries_.find(key);
		return (it != entries_.end()) ? static_cast<uint64_t>(std::strtoull(it->second.c_str(), nullptr, 10)) : fallback;
	}

	[[nodiscard]] int32_t signed_value(const std::string& key, int32_t fallback) const {
		const auto it = entries_.find(key);
		return (it != entries_.end()) ? static_cast<int32_t>(std::strtol(it->second.c_str(), nullptr, 10)) : fallback;
	}

	[[nodiscard]] bool flag(const std::string& key, bool fallback) const {
		const auto it = entries_.find(key);
		return (it != entries_.end()) ? (std::strtoul(it->second.c_str(), nullptr, 10) != 0UL) : fallback;
	}

	[[nodiscard]] std::string text(const std::string& key, const std::string& fallback) const {
		const auto it = entries_.find(key);
		return (it != entries_.end()) ? it->second : fallback;
	}

	template <typename Enum>
	[[nodiscard]] Enum enumeration(const std::string& key, Enum fallback, Enum last) const {
		const uint32_t value = unsigned_value(key, static_cast<uint32_t>(fallback));
		return (value <= static_cast<uint32_t>(last)) ? static_cast<Enum>(value) : fallback;
	}
};

class SettingsWriter {
private:
	std::ostream& out_;

public:
	explicit SettingsWriter(std::ostream& out) noexcept : out_(out) {}

	void real(const std::string& key, double value) {
		out_ << key << '=' << std::setprecision(17) << value << '\n';
	}

	void unsigned_value(const std::string& key, uint64_t value) {
		out_ << key << '=' << value << '\n';
	}

	void signed_value(const std::string& key, int64_t value) {
		out_ << key << '=' << value << '\n';
	}

	void flag(const std::string& key, bool value) {
		out_ << key << '=' << (value ? 1 : 0) << '\n';
	}

	void text(const std::string& key, const std::string& value) {
		out_ << key << '=' << value << '\n';
	}

	template <typename Enum>
	void enumeration(const std::string& key, Enum value) {
		out_ << key << '=' << static_cast<uint32_t>(value) << '\n';
	}
};

}
