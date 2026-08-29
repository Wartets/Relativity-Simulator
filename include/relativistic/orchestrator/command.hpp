#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <optional>
#include <algorithm>

namespace Relativistic::Orchestrator {

enum class CommandType : uint32_t {
	None = 0,
	Pause = 1,
	Resume = 2,
	Step = 3,
	Warp = 4,
	Reset = 5,
	SetParam = 6,
	SetTickRate = 7,
	Status = 8,
	Shutdown = 9
};

enum class ParameterType : uint32_t {
	Unknown = 0,
	Mass = 1,
	Spin = 2,
	Charge = 3,
	CosmologicalLambda = 4,
	WormholeThroat = 5,
	WarpVelocity = 6,
	TickRate = 7,
	Custom = 8
};

struct Command {
	CommandType type{CommandType::None};
	ParameterType param_type{ParameterType::Unknown};
	uint64_t step_count{0};
	double numeric_value{0.0};
	char custom_param_name[32]{};
	uint64_t target_tick{0};

	[[nodiscard]] static constexpr Command make_pause() noexcept {
		Command cmd{};
		cmd.type = CommandType::Pause;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_resume() noexcept {
		Command cmd{};
		cmd.type = CommandType::Resume;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_step(uint64_t n = 1) noexcept {
		Command cmd{};
		cmd.type = CommandType::Step;
		cmd.step_count = n;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_warp(double factor) noexcept {
		Command cmd{};
		cmd.type = CommandType::Warp;
		cmd.numeric_value = factor;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_reset() noexcept {
		Command cmd{};
		cmd.type = CommandType::Reset;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_set_param(ParameterType param, double val) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetParam;
		cmd.param_type = param;
		cmd.numeric_value = val;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_set_tickrate(double rate_hz) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetTickRate;
		cmd.numeric_value = rate_hz;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_status() noexcept {
		Command cmd{};
		cmd.type = CommandType::Status;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_shutdown() noexcept {
		Command cmd{};
		cmd.type = CommandType::Shutdown;
		return cmd;
	}
};

struct CommandResult {
	bool success{false};
	char message[64]{};
};

class CommandParser {
public:
	[[nodiscard]] static std::optional<Command> parse(std::string_view line, CommandResult* result_out = nullptr) noexcept {
		auto trim_sv = [](std::string_view s) noexcept -> std::string_view {
			while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) {
				s.remove_prefix(1);
			}
			while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) {
				s.remove_suffix(1);
			}
			return s;
		};

		auto iequals_sv = [](std::string_view a, std::string_view b) noexcept -> bool {
			if (a.size() != b.size()) return false;
			for (size_t i = 0; i < a.size(); ++i) {
				const char ca = (a[i] >= 'A' && a[i] <= 'Z') ? static_cast<char>(a[i] + ('a' - 'A')) : a[i];
				const char cb = (b[i] >= 'A' && b[i] <= 'Z') ? static_cast<char>(b[i] + ('a' - 'A')) : b[i];
				if (ca != cb) return false;
			}
			return true;
		};

		auto parse_u64 = [](std::string_view s, uint64_t& out) noexcept -> bool {
			if (s.empty()) return false;
			uint64_t v = 0;
			for (char c : s) {
				if (c < '0' || c > '9') return false;
				v = v * 10 + static_cast<uint64_t>(c - '0');
			}
			out = v;
			return true;
		};

		auto parse_dbl = [](std::string_view s, double& out) noexcept -> bool {
			if (s.empty()) return false;
			char buf[64];
			if (s.size() >= sizeof(buf)) return false;
			std::memcpy(buf, s.data(), s.size());
			buf[s.size()] = '\0';
			char* end_ptr = nullptr;
			const double v = std::strtod(buf, &end_ptr);
			if (end_ptr != buf + s.size()) return false;
			out = v;
			return true;
		};

		auto set_msg = [](CommandResult* r, bool s, std::string_view m) noexcept {
			if (!r) return;
			r->success = s;
			const size_t len = std::min(m.size(), sizeof(r->message) - 1);
			std::memcpy(r->message, m.data(), len);
			r->message[len] = '\0';
		};

		std::string_view remaining = trim_sv(line);
		if (remaining.empty()) {
			set_msg(result_out, false, "Empty command line");
			return std::nullopt;
		}

		auto extract_token = [&](std::string_view& rem) noexcept -> std::string_view {
			rem = trim_sv(rem);
			if (rem.empty()) return {};
			size_t end = 0;
			while (end < rem.size() && rem[end] != ' ' && rem[end] != '\t' && rem[end] != '\r' && rem[end] != '\n') {
				++end;
			}
			const std::string_view token = rem.substr(0, end);
			rem = trim_sv(rem.substr(end));
			return token;
		};

		const std::string_view token1 = extract_token(remaining);

		if (iequals_sv(token1, "pause")) {
			set_msg(result_out, true, "Command parsed: pause");
			return Command::make_pause();
		}

		if (iequals_sv(token1, "resume")) {
			set_msg(result_out, true, "Command parsed: resume");
			return Command::make_resume();
		}

		if (iequals_sv(token1, "step")) {
			const std::string_view token2 = extract_token(remaining);
			uint64_t n = 1;
			if (!token2.empty()) {
				if (!parse_u64(token2, n) || n == 0) {
					set_msg(result_out, false, "Invalid step count argument");
					return std::nullopt;
				}
			}
			set_msg(result_out, true, "Command parsed: step");
			return Command::make_step(n);
		}

		if (iequals_sv(token1, "warp")) {
			const std::string_view token2 = extract_token(remaining);
			if (token2.empty()) {
				set_msg(result_out, false, "Missing warp factor argument");
				return std::nullopt;
			}
			double factor = 0.0;
			if (!parse_dbl(token2, factor) || factor <= 0.0) {
				set_msg(result_out, false, "Invalid warp factor (must be positive)");
				return std::nullopt;
			}
			set_msg(result_out, true, "Command parsed: warp");
			return Command::make_warp(factor);
		}

		if (iequals_sv(token1, "reset")) {
			set_msg(result_out, true, "Command parsed: reset");
			return Command::make_reset();
		}

		if (iequals_sv(token1, "tickrate")) {
			const std::string_view token2 = extract_token(remaining);
			if (token2.empty()) {
				set_msg(result_out, false, "Missing tickrate argument");
				return std::nullopt;
			}
			double rate = 0.0;
			if (!parse_dbl(token2, rate) || rate < 10.0 || rate > 1000.0) {
				set_msg(result_out, false, "Invalid tickrate (must be between 10.0 and 1000.0 Hz)");
				return std::nullopt;
			}
			set_msg(result_out, true, "Command parsed: tickrate");
			return Command::make_set_tickrate(rate);
		}

		if (iequals_sv(token1, "status")) {
			set_msg(result_out, true, "Command parsed: status");
			return Command::make_status();
		}

		if (iequals_sv(token1, "shutdown") || iequals_sv(token1, "quit") || iequals_sv(token1, "exit")) {
			set_msg(result_out, true, "Command parsed: shutdown");
			return Command::make_shutdown();
		}

		if (iequals_sv(token1, "set")) {
			const std::string_view token2 = extract_token(remaining);
			if (token2.empty()) {
				set_msg(result_out, false, "Missing parameter name in set command");
				return std::nullopt;
			}
			const std::string_view token3 = extract_token(remaining);
			if (token3.empty()) {
				set_msg(result_out, false, "Missing value in set command");
				return std::nullopt;
			}
			double val = 0.0;
			if (!parse_dbl(token3, val)) {
				set_msg(result_out, false, "Invalid numeric value in set command");
				return std::nullopt;
			}

			ParameterType ptype = ParameterType::Custom;
			if (iequals_sv(token2, "mass")) ptype = ParameterType::Mass;
			else if (iequals_sv(token2, "spin")) ptype = ParameterType::Spin;
			else if (iequals_sv(token2, "charge")) ptype = ParameterType::Charge;
			else if (iequals_sv(token2, "lambda")) ptype = ParameterType::CosmologicalLambda;
			else if (iequals_sv(token2, "throat")) ptype = ParameterType::WormholeThroat;
			else if (iequals_sv(token2, "warp_velocity") || iequals_sv(token2, "warp_vel")) ptype = ParameterType::WarpVelocity;
			else if (iequals_sv(token2, "tickrate")) {
				if (val < 10.0 || val > 1000.0) {
					set_msg(result_out, false, "Invalid tickrate (must be between 10.0 and 1000.0 Hz)");
					return std::nullopt;
				}
				ptype = ParameterType::TickRate;
			}

			Command cmd = Command::make_set_param(ptype, val);
			if (ptype == ParameterType::Custom) {
				const size_t len = std::min(token2.size(), sizeof(cmd.custom_param_name) - 1);
				std::memcpy(cmd.custom_param_name, token2.data(), len);
				cmd.custom_param_name[len] = '\0';
			}
			set_msg(result_out, true, "Command parsed: set");
			return cmd;
		}

		set_msg(result_out, false, "Unrecognized command");
		return std::nullopt;
	}
};

}
