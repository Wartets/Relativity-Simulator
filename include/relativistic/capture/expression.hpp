#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <numbers>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Relativistic::Capture {

inline constexpr size_t kExpressionVariableCount = 8;
inline constexpr size_t kExpressionStackLimit = 32;

using ExpressionVariables = std::array<double, kExpressionVariableCount>;

namespace ExpressionSlot {
	inline constexpr size_t LocalTime = 0;
	inline constexpr size_t Progress = 1;
	inline constexpr size_t Duration = 2;
	inline constexpr size_t GlobalTime = 3;
	inline constexpr size_t ParameterA = 4;
	inline constexpr size_t ParameterB = 5;
	inline constexpr size_t ParameterC = 6;
	inline constexpr size_t ParameterK = 7;
}

inline constexpr std::array<std::string_view, kExpressionVariableCount> kExpressionVariableNames{"t", "u", "d", "g", "a", "b", "c", "k"};

[[nodiscard]] inline double hashed_noise_value(int64_t lattice) noexcept {
	uint64_t x = static_cast<uint64_t>(lattice) * 0x9E3779B97F4A7C15ULL;
	x ^= x >> 30;
	x *= 0xBF58476D1CE4E5B9ULL;
	x ^= x >> 27;
	x *= 0x94D049BB133111EBULL;
	x ^= x >> 31;
	return static_cast<double>(x >> 11) * (1.0 / 9007199254740992.0) * 2.0 - 1.0;
}

[[nodiscard]] inline double smooth_value_noise(double x) noexcept {
	const double cell = std::floor(x);
	const double f = x - cell;
	const double s = f * f * (3.0 - 2.0 * f);
	const int64_t index = static_cast<int64_t>(cell);
	const double a = hashed_noise_value(index);
	const double b = hashed_noise_value(index + 1);
	return a + (b - a) * s;
}

enum class ExpressionOp : uint8_t {
	Push,
	Variable,
	Add, Sub, Mul, Div, Mod, Pow, Less, Greater, LessEqual, GreaterEqual, Equal, NotEqual, Atan2, Min, Max, Hypot, Step,
	Negate, Sin, Cos, Tan, Asin, Acos, Atan, Sinh, Cosh, Tanh, Exp, Log, Log2, Log10, Sqrt, Cbrt, Abs, Floor, Ceil, Round, Fract, Sign, Radians, Degrees, Saturate, Smooth, Noise, Triangle, Sawtooth, Square,
	Clamp, Mix, Select, SmoothRange
};

struct ExpressionInstruction {
	ExpressionOp op{ExpressionOp::Push};
	double value{0.0};
	uint8_t index{0};
};

[[nodiscard]] constexpr bool is_binary_op(ExpressionOp op) noexcept {
	return op >= ExpressionOp::Add && op <= ExpressionOp::Step;
}

[[nodiscard]] constexpr bool is_unary_op(ExpressionOp op) noexcept {
	return op >= ExpressionOp::Negate && op <= ExpressionOp::Square;
}

[[nodiscard]] constexpr bool is_ternary_op(ExpressionOp op) noexcept {
	return op >= ExpressionOp::Clamp && op <= ExpressionOp::SmoothRange;
}

[[nodiscard]] inline double evaluate_binary_op(ExpressionOp op, double a, double b) noexcept {
	switch (op) {
		case ExpressionOp::Add: return a + b;
		case ExpressionOp::Sub: return a - b;
		case ExpressionOp::Mul: return a * b;
		case ExpressionOp::Div: return (b == 0.0) ? 0.0 : a / b;
		case ExpressionOp::Mod: return (b == 0.0) ? 0.0 : a - b * std::floor(a / b);
		case ExpressionOp::Pow: return std::pow(a, b);
		case ExpressionOp::Less: return (a < b) ? 1.0 : 0.0;
		case ExpressionOp::Greater: return (a > b) ? 1.0 : 0.0;
		case ExpressionOp::LessEqual: return (a <= b) ? 1.0 : 0.0;
		case ExpressionOp::GreaterEqual: return (a >= b) ? 1.0 : 0.0;
		case ExpressionOp::Equal: return (std::abs(a - b) < 1e-12) ? 1.0 : 0.0;
		case ExpressionOp::NotEqual: return (std::abs(a - b) >= 1e-12) ? 1.0 : 0.0;
		case ExpressionOp::Atan2: return std::atan2(a, b);
		case ExpressionOp::Min: return std::min(a, b);
		case ExpressionOp::Max: return std::max(a, b);
		case ExpressionOp::Hypot: return std::hypot(a, b);
		case ExpressionOp::Step: return (b >= a) ? 1.0 : 0.0;
		default: return 0.0;
	}
}

[[nodiscard]] inline double evaluate_unary_op(ExpressionOp op, double x) noexcept {
	constexpr double pi = std::numbers::pi_v<double>;
	switch (op) {
		case ExpressionOp::Negate: return -x;
		case ExpressionOp::Sin: return std::sin(x);
		case ExpressionOp::Cos: return std::cos(x);
		case ExpressionOp::Tan: return std::tan(x);
		case ExpressionOp::Asin: return std::asin(std::clamp(x, -1.0, 1.0));
		case ExpressionOp::Acos: return std::acos(std::clamp(x, -1.0, 1.0));
		case ExpressionOp::Atan: return std::atan(x);
		case ExpressionOp::Sinh: return std::sinh(std::clamp(x, -700.0, 700.0));
		case ExpressionOp::Cosh: return std::cosh(std::clamp(x, -700.0, 700.0));
		case ExpressionOp::Tanh: return std::tanh(x);
		case ExpressionOp::Exp: return std::exp(std::min(x, 700.0));
		case ExpressionOp::Log: return std::log(std::max(x, 1e-300));
		case ExpressionOp::Log2: return std::log2(std::max(x, 1e-300));
		case ExpressionOp::Log10: return std::log10(std::max(x, 1e-300));
		case ExpressionOp::Sqrt: return std::sqrt(std::max(x, 0.0));
		case ExpressionOp::Cbrt: return std::cbrt(x);
		case ExpressionOp::Abs: return std::abs(x);
		case ExpressionOp::Floor: return std::floor(x);
		case ExpressionOp::Ceil: return std::ceil(x);
		case ExpressionOp::Round: return std::round(x);
		case ExpressionOp::Fract: return x - std::floor(x);
		case ExpressionOp::Sign: return (x > 0.0) ? 1.0 : ((x < 0.0) ? -1.0 : 0.0);
		case ExpressionOp::Radians: return x * pi / 180.0;
		case ExpressionOp::Degrees: return x * 180.0 / pi;
		case ExpressionOp::Saturate: return std::clamp(x, 0.0, 1.0);
		case ExpressionOp::Smooth: {
			const double s = std::clamp(x, 0.0, 1.0);
			return s * s * (3.0 - 2.0 * s);
		}
		case ExpressionOp::Noise: return smooth_value_noise(x);
		case ExpressionOp::Triangle: return std::asin(std::sin(2.0 * pi * x)) * 2.0 / pi;
		case ExpressionOp::Sawtooth: return 2.0 * (x - std::floor(x + 0.5));
		case ExpressionOp::Square: return (std::sin(2.0 * pi * x) >= 0.0) ? 1.0 : -1.0;
		default: return 0.0;
	}
}

[[nodiscard]] inline double evaluate_ternary_op(ExpressionOp op, double a, double b, double c) noexcept {
	switch (op) {
		case ExpressionOp::Clamp: return std::clamp(a, std::min(b, c), std::max(b, c));
		case ExpressionOp::Mix: return a + (b - a) * c;
		case ExpressionOp::Select: return (a != 0.0) ? b : c;
		case ExpressionOp::SmoothRange: {
			const double span = b - a;
			if (std::abs(span) < 1e-300) return (c >= a) ? 1.0 : 0.0;
			const double t = std::clamp((c - a) / span, 0.0, 1.0);
			return t * t * (3.0 - 2.0 * t);
		}
		default: return 0.0;
	}
}

[[nodiscard]] inline double run_expression_program(const std::vector<ExpressionInstruction>& program, const ExpressionVariables& variables) noexcept {
	std::array<double, kExpressionStackLimit + 8> stack{};
	size_t top = 0;
	for (const ExpressionInstruction& instruction : program) {
		const ExpressionOp op = instruction.op;
		if (op == ExpressionOp::Push) {
			stack[top++] = instruction.value;
		} else if (op == ExpressionOp::Variable) {
			stack[top++] = variables[instruction.index];
		} else if (is_binary_op(op)) {
			const double b = stack[--top];
			stack[top - 1] = evaluate_binary_op(op, stack[top - 1], b);
		} else if (is_unary_op(op)) {
			stack[top - 1] = evaluate_unary_op(op, stack[top - 1]);
		} else if (is_ternary_op(op)) {
			const double c = stack[--top];
			const double b = stack[--top];
			stack[top - 1] = evaluate_ternary_op(op, stack[top - 1], b, c);
		}
	}
	const double result = (top > 0) ? stack[top - 1] : 0.0;
	return std::isfinite(result) ? result : 0.0;
}

struct ExpressionFunction {
	std::string_view name;
	ExpressionOp op;
	int arity;
};

inline constexpr auto kExpressionFunctions = std::to_array<ExpressionFunction>({
	{"sin", ExpressionOp::Sin, 1}, {"cos", ExpressionOp::Cos, 1}, {"tan", ExpressionOp::Tan, 1},
	{"asin", ExpressionOp::Asin, 1}, {"acos", ExpressionOp::Acos, 1}, {"atan", ExpressionOp::Atan, 1},
	{"sinh", ExpressionOp::Sinh, 1}, {"cosh", ExpressionOp::Cosh, 1}, {"tanh", ExpressionOp::Tanh, 1},
	{"exp", ExpressionOp::Exp, 1}, {"log", ExpressionOp::Log, 1}, {"ln", ExpressionOp::Log, 1},
	{"log2", ExpressionOp::Log2, 1}, {"log10", ExpressionOp::Log10, 1}, {"sqrt", ExpressionOp::Sqrt, 1},
	{"cbrt", ExpressionOp::Cbrt, 1}, {"abs", ExpressionOp::Abs, 1}, {"floor", ExpressionOp::Floor, 1},
	{"ceil", ExpressionOp::Ceil, 1}, {"round", ExpressionOp::Round, 1}, {"fract", ExpressionOp::Fract, 1},
	{"sign", ExpressionOp::Sign, 1}, {"rad", ExpressionOp::Radians, 1}, {"deg", ExpressionOp::Degrees, 1},
	{"saturate", ExpressionOp::Saturate, 1}, {"smooth", ExpressionOp::Smooth, 1}, {"noise", ExpressionOp::Noise, 1},
	{"tri", ExpressionOp::Triangle, 1}, {"saw", ExpressionOp::Sawtooth, 1}, {"square", ExpressionOp::Square, 1},
	{"atan2", ExpressionOp::Atan2, 2}, {"min", ExpressionOp::Min, 2}, {"max", ExpressionOp::Max, 2},
	{"hypot", ExpressionOp::Hypot, 2}, {"step", ExpressionOp::Step, 2}, {"pow", ExpressionOp::Pow, 2},
	{"mod", ExpressionOp::Mod, 2}, {"clamp", ExpressionOp::Clamp, 3}, {"mix", ExpressionOp::Mix, 3},
	{"if", ExpressionOp::Select, 3}, {"smoothstep", ExpressionOp::SmoothRange, 3}
});

class ExpressionParser {
public:
	explicit ExpressionParser(std::string_view source) noexcept : source_(source) {}

	[[nodiscard]] bool parse(std::vector<ExpressionInstruction>& program, std::string& error) {
		parse_comparison();
		skip_whitespace();
		if (error_.empty() && position_ < source_.size()) {
			fail(std::string("Unexpected character '") + source_[position_] + "'");
		}
		if (error_.empty() && max_depth_ > static_cast<int>(kExpressionStackLimit)) {
			fail("Expression is nested too deeply");
		}
		if (!error_.empty()) {
			error = error_;
			return false;
		}
		program = std::move(code_);
		return true;
	}

private:
	std::string_view source_;
	size_t position_{0};
	std::vector<ExpressionInstruction> code_{};
	std::string error_{};
	int depth_{0};
	int max_depth_{0};

	void fail(const std::string& message) {
		if (error_.empty()) {
			error_ = message;
		}
		position_ = source_.size();
	}

	void skip_whitespace() noexcept {
		while (position_ < source_.size() && std::isspace(static_cast<unsigned char>(source_[position_])) != 0) {
			++position_;
		}
	}

	[[nodiscard]] bool consume(char c) noexcept {
		skip_whitespace();
		if (position_ < source_.size() && source_[position_] == c) {
			++position_;
			return true;
		}
		return false;
	}

	void emit(ExpressionOp op, double value = 0.0, uint8_t index = 0) {
		code_.push_back(ExpressionInstruction{op, value, index});
		if (op == ExpressionOp::Push || op == ExpressionOp::Variable) {
			++depth_;
		} else if (is_binary_op(op)) {
			--depth_;
		} else if (is_ternary_op(op)) {
			depth_ -= 2;
		}
		max_depth_ = std::max(max_depth_, depth_);
	}

	void parse_comparison() {
		parse_additive();
		for (;;) {
			skip_whitespace();
			if (position_ >= source_.size()) return;
			const char c = source_[position_];
			const char n = (position_ + 1 < source_.size()) ? source_[position_ + 1] : '\0';
			ExpressionOp op = ExpressionOp::Less;
			size_t length = 1;
			if (c == '<' && n == '=') { op = ExpressionOp::LessEqual; length = 2; }
			else if (c == '>' && n == '=') { op = ExpressionOp::GreaterEqual; length = 2; }
			else if (c == '=' && n == '=') { op = ExpressionOp::Equal; length = 2; }
			else if (c == '!' && n == '=') { op = ExpressionOp::NotEqual; length = 2; }
			else if (c == '<') { op = ExpressionOp::Less; }
			else if (c == '>') { op = ExpressionOp::Greater; }
			else return;
			position_ += length;
			parse_additive();
			emit(op);
		}
	}

	void parse_additive() {
		parse_term();
		for (;;) {
			skip_whitespace();
			if (position_ >= source_.size()) return;
			const char c = source_[position_];
			if (c != '+' && c != '-') return;
			++position_;
			parse_term();
			emit(c == '+' ? ExpressionOp::Add : ExpressionOp::Sub);
		}
	}

	void parse_term() {
		parse_unary();
		for (;;) {
			skip_whitespace();
			if (position_ >= source_.size()) return;
			const char c = source_[position_];
			if (c != '*' && c != '/' && c != '%') return;
			++position_;
			parse_unary();
			emit(c == '*' ? ExpressionOp::Mul : (c == '/' ? ExpressionOp::Div : ExpressionOp::Mod));
		}
	}

	void parse_unary() {
		skip_whitespace();
		if (position_ < source_.size() && source_[position_] == '-') {
			++position_;
			parse_unary();
			emit(ExpressionOp::Negate);
			return;
		}
		if (position_ < source_.size() && source_[position_] == '+') {
			++position_;
			parse_unary();
			return;
		}
		parse_power();
	}

	void parse_power() {
		parse_primary();
		skip_whitespace();
		if (position_ < source_.size() && source_[position_] == '^') {
			++position_;
			parse_unary();
			emit(ExpressionOp::Pow);
		}
	}

	void parse_number() {
		const std::string text(source_.substr(position_));
		char* end = nullptr;
		const double value = std::strtod(text.c_str(), &end);
		const size_t consumed = static_cast<size_t>(end - text.c_str());
		if (consumed == 0) {
			fail("Invalid number");
			return;
		}
		position_ += consumed;
		emit(ExpressionOp::Push, value);
	}

	void parse_primary() {
		skip_whitespace();
		if (position_ >= source_.size()) {
			fail("Unexpected end of expression");
			return;
		}
		const char c = source_[position_];
		if (std::isdigit(static_cast<unsigned char>(c)) != 0 || c == '.') {
			parse_number();
			return;
		}
		if (c == '(') {
			++position_;
			parse_comparison();
			if (!consume(')')) fail("Missing closing parenthesis");
			return;
		}
		if (std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_') {
			const size_t start = position_;
			while (position_ < source_.size() && (std::isalnum(static_cast<unsigned char>(source_[position_])) != 0 || source_[position_] == '_')) {
				++position_;
			}
			const std::string_view name = source_.substr(start, position_ - start);
			skip_whitespace();
			if (position_ < source_.size() && source_[position_] == '(') {
				++position_;
				parse_function(name);
				return;
			}
			parse_symbol(name);
			return;
		}
		fail(std::string("Unexpected character '") + c + "'");
	}

	void parse_symbol(std::string_view name) {
		if (name == "pi") { emit(ExpressionOp::Push, std::numbers::pi_v<double>); return; }
		if (name == "tau") { emit(ExpressionOp::Push, 2.0 * std::numbers::pi_v<double>); return; }
		if (name == "e") { emit(ExpressionOp::Push, std::numbers::e_v<double>); return; }
		for (size_t i = 0; i < kExpressionVariableNames.size(); ++i) {
			if (kExpressionVariableNames[i] == name) {
				emit(ExpressionOp::Variable, 0.0, static_cast<uint8_t>(i));
				return;
			}
		}
		fail("Unknown symbol '" + std::string(name) + "'");
	}

	void parse_function(std::string_view name) {
		const ExpressionFunction* entry = nullptr;
		for (const ExpressionFunction& candidate : kExpressionFunctions) {
			if (candidate.name == name) {
				entry = &candidate;
				break;
			}
		}
		if (entry == nullptr) {
			fail("Unknown function '" + std::string(name) + "'");
			return;
		}
		int count = 0;
		skip_whitespace();
		if (position_ < source_.size() && source_[position_] == ')') {
			++position_;
		} else {
			for (;;) {
				parse_comparison();
				++count;
				if (consume(',')) continue;
				if (consume(')')) break;
				fail("Expected ',' or ')'");
				break;
			}
		}
		if (error_.empty() && count != entry->arity) {
			fail("Function '" + std::string(name) + "' expects " + std::to_string(entry->arity) + " argument(s)");
			return;
		}
		if (error_.empty()) emit(entry->op);
	}
};

class Expression {
public:
	Expression() = default;

	explicit Expression(std::string source) {
		assign(std::move(source));
	}

	void assign(std::string source) {
		for (char& c : source) {
			if (c == '\n' || c == '\r' || c == '\t') c = ' ';
		}
		source_ = std::move(source);
		compile();
	}

	[[nodiscard]] const std::string& source() const noexcept { return source_; }
	[[nodiscard]] const std::string& error() const noexcept { return error_; }
	[[nodiscard]] bool valid() const noexcept { return program_ != nullptr; }

	[[nodiscard]] double evaluate(const ExpressionVariables& variables, double fallback = 0.0) const noexcept {
		return (program_ != nullptr) ? run_expression_program(*program_, variables) : fallback;
	}

private:
	std::string source_{};
	std::string error_{};
	std::shared_ptr<const std::vector<ExpressionInstruction>> program_{};

	void compile() {
		program_.reset();
		error_.clear();
		if (source_.find_first_not_of(' ') == std::string::npos) {
			error_ = "Empty expression";
			return;
		}
		std::vector<ExpressionInstruction> program;
		ExpressionParser parser(source_);
		if (parser.parse(program, error_)) {
			program_ = std::make_shared<const std::vector<ExpressionInstruction>>(std::move(program));
		}
	}
};

}
