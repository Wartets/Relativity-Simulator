#pragma once

#include "relativistic/units/unit_system.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <cmath>
#include <optional>
#include <variant>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace Relativistic::Units {

struct DimensionVector {
	int length{0};
	int mass{0};
	int time{0};
	int current{0};
	int temperature{0};
	int angle{0};

	[[nodiscard]] constexpr bool operator==(const DimensionVector& other) const noexcept {
		return length == other.length && mass == other.mass && time == other.time &&
		       current == other.current && temperature == other.temperature && angle == other.angle;
	}

	[[nodiscard]] constexpr bool is_dimensionless() const noexcept {
		return length == 0 && mass == 0 && time == 0 && current == 0 && temperature == 0 && angle == 0;
	}

	[[nodiscard]] constexpr DimensionVector operator+(const DimensionVector& other) const noexcept {
		return DimensionVector{length + other.length, mass + other.mass, time + other.time,
		                        current + other.current, temperature + other.temperature, angle + other.angle};
	}

	[[nodiscard]] constexpr DimensionVector operator-(const DimensionVector& other) const noexcept {
		return DimensionVector{length - other.length, mass - other.mass, time - other.time,
		                        current - other.current, temperature - other.temperature, angle - other.angle};
	}

	[[nodiscard]] constexpr DimensionVector scaled(int factor) const noexcept {
		return DimensionVector{length * factor, mass * factor, time * factor,
		                        current * factor, temperature * factor, angle * factor};
	}

	[[nodiscard]] constexpr DimensionVector negated() const noexcept {
		return DimensionVector{-length, -mass, -time, -current, -temperature, -angle};
	}
};

struct QuantityValue {
	double value{0.0};
	DimensionVector dimension{};

	[[nodiscard]] static constexpr QuantityValue scalar(double v) noexcept {
		return QuantityValue{v, DimensionVector{}};
	}
};

class ExpressionError final : public std::runtime_error {
public:
	explicit ExpressionError(const std::string& message) : std::runtime_error(message) {}
};

struct UnitToken {
	std::string_view symbol;
	double factor;
	double offset;
	DimensionVector dimension;
};

class UnitRegistry {
private:
	static constexpr std::array<UnitToken, 164> kUnits{{
		{"siemens", 1.0, 0.0, DimensionVector{-2, -1, 3, 2, 0, 0}},
		{"farad", 1.0, 0.0, DimensionVector{-2, -1, 4, 2, 0, 0}},
		{"mfarad", 1.0e-3, 0.0, DimensionVector{-2, -1, 4, 2, 0, 0}},
		{"ufarad", 1.0e-6, 0.0, DimensionVector{-2, -1, 4, 2, 0, 0}},
		{"nf", 1.0e-9, 0.0, DimensionVector{-2, -1, 4, 2, 0, 0}},
		{"pf", 1.0e-12, 0.0, DimensionVector{-2, -1, 4, 2, 0, 0}},
		{"pa", 1.0, 0.0, DimensionVector{-1, 1, -2, 0, 0, 0}},
		{"kpa", 1000.0, 0.0, DimensionVector{-1, 1, -2, 0, 0, 0}},
		{"mpa", 1.0e6, 0.0, DimensionVector{-1, 1, -2, 0, 0, 0}},
		{"bar", 1.0e5, 0.0, DimensionVector{-1, 1, -2, 0, 0, 0}},
		{"atm", 101325.0, 0.0, DimensionVector{-1, 1, -2, 0, 0, 0}},
		{"hpa", 100.0, 0.0, DimensionVector{-1, 1, -2, 0, 0, 0}},
		{"gpa", 1.0e9, 0.0, DimensionVector{-1, 1, -2, 0, 0, 0}},
		{"torr", 133.32236842105263, 0.0, DimensionVector{-1, 1, -2, 0, 0, 0}},
		{"mmhg", 133.322387415, 0.0, DimensionVector{-1, 1, -2, 0, 0, 0}},
		{"psi", 6894.757293168, 0.0, DimensionVector{-1, 1, -2, 0, 0, 0}},
		{"mbar", 100.0, 0.0, DimensionVector{-1, 1, -2, 0, 0, 0}},
		{"ksi", 6894757.293168, 0.0, DimensionVector{-1, 1, -2, 0, 0, 0}},
		{"hz", 1.0, 0.0, DimensionVector{0, 0, -1, 0, 0, 0}},
		{"khz", 1000.0, 0.0, DimensionVector{0, 0, -1, 0, 0, 0}},
		{"mhz", 1.0e6, 0.0, DimensionVector{0, 0, -1, 0, 0, 0}},
		{"ghz", 1.0e9, 0.0, DimensionVector{0, 0, -1, 0, 0, 0}},
		{"thz", 1.0e12, 0.0, DimensionVector{0, 0, -1, 0, 0, 0}},
		{"phz", 1.0e15, 0.0, DimensionVector{0, 0, -1, 0, 0, 0}},
		{"rpm", 0.10471975511965977, 0.0, DimensionVector{0, 0, -1, 0, 0, 0}},
		{"rps", 6.283185307179586, 0.0, DimensionVector{0, 0, -1, 0, 0, 0}},
		{"rad", 1.0, 0.0, DimensionVector{0, 0, 0, 0, 0, 1}},
		{"deg", 0.017453292519943295, 0.0, DimensionVector{0, 0, 0, 0, 0, 1}},
		{"arcmin", 0.0002908882086657216, 0.0, DimensionVector{0, 0, 0, 0, 0, 1}},
		{"arcsec", 4.84813681109536e-6, 0.0, DimensionVector{0, 0, 0, 0, 0, 1}},
		{"grad", 0.015707963267948967, 0.0, DimensionVector{0, 0, 0, 0, 0, 1}},
		{"rev", 6.283185307179586, 0.0, DimensionVector{0, 0, 0, 0, 0, 1}},
		{"mrad", 0.001, 0.0, DimensionVector{0, 0, 0, 0, 0, 1}},
		{"mas", 4.84813681109536e-9, 0.0, DimensionVector{0, 0, 0, 0, 0, 1}},
		{"uas", 4.84813681109536e-12, 0.0, DimensionVector{0, 0, 0, 0, 0, 1}},
		{"quadrant", 1.5707963267948966, 0.0, DimensionVector{0, 0, 0, 0, 0, 1}},
		{"k", 1.0, 0.0, DimensionVector{0, 0, 0, 0, 1, 0}},
		{"degc", 1.0, 273.15, DimensionVector{0, 0, 0, 0, 1, 0}},
		{"degf", 5.0 / 9.0, 459.67, DimensionVector{0, 0, 0, 0, 1, 0}},
		{"rankine", 5.0 / 9.0, 0.0, DimensionVector{0, 0, 0, 0, 1, 0}},
		{"reaumur", 1.25, 273.15, DimensionVector{0, 0, 0, 0, 1, 0}},
		{"a", 1.0, 0.0, DimensionVector{0, 0, 0, 1, 0, 0}},
		{"ma", 0.001, 0.0, DimensionVector{0, 0, 0, 1, 0, 0}},
		{"ka", 1000.0, 0.0, DimensionVector{0, 0, 0, 1, 0, 0}},
		{"na", 1.0e-9, 0.0, DimensionVector{0, 0, 0, 1, 0, 0}},
		{"s", 1.0, 0.0, DimensionVector{0, 0, 1, 0, 0, 0}},
		{"ms", 0.001, 0.0, DimensionVector{0, 0, 1, 0, 0, 0}},
		{"us", 1.0e-6, 0.0, DimensionVector{0, 0, 1, 0, 0, 0}},
		{"ns", 1.0e-9, 0.0, DimensionVector{0, 0, 1, 0, 0, 0}},
		{"min", 60.0, 0.0, DimensionVector{0, 0, 1, 0, 0, 0}},
		{"h", 3600.0, 0.0, DimensionVector{0, 0, 1, 0, 0, 0}},
		{"day", 86400.0, 0.0, DimensionVector{0, 0, 1, 0, 0, 0}},
		{"yr", 31557600.0, 0.0, DimensionVector{0, 0, 1, 0, 0, 0}},
		{"ps", 1.0e-12, 0.0, DimensionVector{0, 0, 1, 0, 0, 0}},
		{"fs", 1.0e-15, 0.0, DimensionVector{0, 0, 1, 0, 0, 0}},
		{"as", 1.0e-18, 0.0, DimensionVector{0, 0, 1, 0, 0, 0}},
		{"cs", 0.01, 0.0, DimensionVector{0, 0, 1, 0, 0, 0}},
		{"ds", 0.1, 0.0, DimensionVector{0, 0, 1, 0, 0, 0}},
		{"week", 604800.0, 0.0, DimensionVector{0, 0, 1, 0, 0, 0}},
		{"fortnight", 1209600.0, 0.0, DimensionVector{0, 0, 1, 0, 0, 0}},
		{"month_avg", 2629746.0, 0.0, DimensionVector{0, 0, 1, 0, 0, 0}},
		{"decade", 315576000.0, 0.0, DimensionVector{0, 0, 1, 0, 0, 0}},
		{"century", 3155760000.0, 0.0, DimensionVector{0, 0, 1, 0, 0, 0}},
		{"millennium", 31557600000.0, 0.0, DimensionVector{0, 0, 1, 0, 0, 0}},
		{"c", 1.602176634e-19, 0.0, DimensionVector{0, 0, 1, 1, 0, 0}},
		{"coul", 1.0, 0.0, DimensionVector{0, 0, 1, 1, 0, 0}},
		{"tesla", 1.0, 0.0, DimensionVector{0, 1, -2, -1, 0, 0}},
		{"gauss", 1.0e-4, 0.0, DimensionVector{0, 1, -2, -1, 0, 0}},
		{"kg", 1.0, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"g", 0.001, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"mg", 1.0e-6, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"t", 1000.0, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"lb", 0.45359237, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"msun", 1.98847e30, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"m_sun", 1.98847e30, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"solarmass", 1.98847e30, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"mearth", 5.9722e24, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"m_earth", 5.9722e24, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"earthmass", 5.9722e24, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"mjup", 1.89813e27, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"m_jup", 1.89813e27, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"jupitermass", 1.89813e27, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"ug", 1.0e-9, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"ng", 1.0e-12, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"pg", 1.0e-15, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"dag", 0.01, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"hg", 0.1, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"q", 100.0, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"oz", 0.028349523125, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"st", 6.35029318, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"ton_us", 907.18474, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"ton_uk", 1016.0469088, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"amu", 1.66053906660e-27, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"me", 9.1093837139e-31, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"mp", 1.67262192595e-27, 0.0, DimensionVector{0, 1, 0, 0, 0, 0}},
		{"m", 1.0, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"km", 1000.0, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"cm", 0.01, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"mm", 0.001, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"nm", 1.0e-9, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"au", 149597870700.0, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"ly", 9.4607304725808e15, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"pc", 3.0856775814913673e16, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"kpc", 3.0856775814913673e19, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"mpc", 3.0856775814913673e22, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"rsun", 6.9634e8, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"r_sun", 6.9634e8, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"rearth", 6.3710e6, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"r_earth", 6.3710e6, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"rjup", 7.1492e7, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"r_jup", 7.1492e7, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"mi", 1609.344, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"ft", 0.3048, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"in", 0.0254, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"nmi", 1852.0, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"dm", 0.1, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"um", 1.0e-6, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"pm", 1.0e-12, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"fm", 1.0e-15, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"angstrom", 1.0e-10, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"mil", 2.54e-5, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"yd", 0.9144, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"fathom", 1.8288, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"chain", 20.1168, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"furlong", 201.168, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"rod", 5.0292, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"ld", 2.59020683712e13, 0.0, DimensionVector{1, 0, 0, 0, 0, 0}},
		{"n", 1.0, 0.0, DimensionVector{1, 1, -2, 0, 0, 0}},
		{"kn", 1000.0, 0.0, DimensionVector{1, 1, -2, 0, 0, 0}},
		{"mn_force", 1.0e6, 0.0, DimensionVector{1, 1, -2, 0, 0, 0}},
		{"dyn", 1.0e-5, 0.0, DimensionVector{1, 1, -2, 0, 0, 0}},
		{"lbf", 4.4482216152605, 0.0, DimensionVector{1, 1, -2, 0, 0, 0}},
		{"kgf", 9.80665, 0.0, DimensionVector{1, 1, -2, 0, 0, 0}},
		{"pdl", 0.138254954376, 0.0, DimensionVector{1, 1, -2, 0, 0, 0}},
		{"ohm", 1.0, 0.0, DimensionVector{2, 1, -3, -2, 0, 0}},
		{"kohm", 1000.0, 0.0, DimensionVector{2, 1, -3, -2, 0, 0}},
		{"mohm", 1.0e6, 0.0, DimensionVector{2, 1, -3, -2, 0, 0}},
		{"mv", 1.0e6, 0.0, DimensionVector{2, 1, -3, -1, 0, 0}},
		{"kv", 1000.0, 0.0, DimensionVector{2, 1, -3, -1, 0, 0}},
		{"uv", 1.0e-6, 0.0, DimensionVector{2, 1, -3, -1, 0, 0}},
		{"w", 1.0, 0.0, DimensionVector{2, 1, -3, 0, 0, 0}},
		{"kw", 1000.0, 0.0, DimensionVector{2, 1, -3, 0, 0, 0}},
		{"mw", 1.0e-3, 0.0, DimensionVector{2, 1, -3, 0, 0, 0}},
		{"hp", 745.6998715822702, 0.0, DimensionVector{2, 1, -3, 0, 0, 0}},
		{"mw_power", 1.0e6, 0.0, DimensionVector{2, 1, -3, 0, 0, 0}},
		{"gw", 1.0e9, 0.0, DimensionVector{2, 1, -3, 0, 0, 0}},
		{"tw", 1.0e12, 0.0, DimensionVector{2, 1, -3, 0, 0, 0}},
		{"mhp", 745699.8715822702, 0.0, DimensionVector{2, 1, -3, 0, 0, 0}},
		{"henry", 1.0, 0.0, DimensionVector{2, 1, -2, -2, 0, 0}},
		{"weber", 1.0, 0.0, DimensionVector{2, 1, -2, -1, 0, 0}},
		{"j", 1.0, 0.0, DimensionVector{2, 1, -2, 0, 0, 0}},
		{"erg", 1.0e-7, 0.0, DimensionVector{2, 1, -2, 0, 0, 0}},
		{"cal", 4.184, 0.0, DimensionVector{2, 1, -2, 0, 0, 0}},
		{"ev", 1.602176634e-19, 0.0, DimensionVector{2, 1, -2, 0, 0, 0}},
		{"kj", 1000.0, 0.0, DimensionVector{2, 1, -2, 0, 0, 0}},
		{"mj", 1.0e6, 0.0, DimensionVector{2, 1, -2, 0, 0, 0}},
		{"gj", 1.0e9, 0.0, DimensionVector{2, 1, -2, 0, 0, 0}},
		{"wh", 3600.0, 0.0, DimensionVector{2, 1, -2, 0, 0, 0}},
		{"kwh", 3.6e6, 0.0, DimensionVector{2, 1, -2, 0, 0, 0}},
		{"mwh", 3.6e9, 0.0, DimensionVector{2, 1, -2, 0, 0, 0}},
		{"gwh", 3.6e12, 0.0, DimensionVector{2, 1, -2, 0, 0, 0}},
		{"therm", 105505585.257348, 0.0, DimensionVector{2, 1, -2, 0, 0, 0}},
		{"btu", 1055.05585262, 0.0, DimensionVector{2, 1, -2, 0, 0, 0}},
		{"kcal", 4184.0, 0.0, DimensionVector{2, 1, -2, 0, 0, 0}}
	}};

public:
	[[nodiscard]] static std::optional<UnitToken> lookup(std::string_view symbol) noexcept {
		std::string lowered;
		lowered.reserve(symbol.size());
		for (const char ch : symbol) {
			lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
		}
		for (const auto& unit : kUnits) {
			if (unit.symbol == lowered) {
				return unit;
			}
		}
		return std::nullopt;
	}
};

class ExpressionEvaluator {
private:
	std::string_view text_;
	size_t pos_{0};

	void skip_whitespace() noexcept {
		while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) {
			++pos_;
		}
	}

	[[nodiscard]] char peek() const noexcept {
		return pos_ < text_.size() ? text_[pos_] : '\0';
	}

	[[nodiscard]] char advance() noexcept {
		return pos_ < text_.size() ? text_[pos_++] : '\0';
	}

	[[nodiscard]] bool match(char expected) noexcept {
		skip_whitespace();
		if (peek() == expected) {
			++pos_;
			return true;
		}
		return false;
	}

	[[nodiscard]] static bool is_identifier_char(char ch) noexcept {
		return std::isalpha(static_cast<unsigned char>(ch)) != 0 || ch == '_';
	}

	[[nodiscard]] std::string read_identifier() noexcept {
		std::string ident;
		while (pos_ < text_.size() && (is_identifier_char(text_[pos_]) || std::isdigit(static_cast<unsigned char>(text_[pos_])))) {
			ident.push_back(text_[pos_++]);
		}
		return ident;
	}

	[[nodiscard]] double read_number() {
		skip_whitespace();
		const size_t start = pos_;
		bool has_digits = false;
		if (peek() == '+' || peek() == '-') {
			++pos_;
		}
		while (std::isdigit(static_cast<unsigned char>(peek()))) {
			++pos_;
			has_digits = true;
		}
		if (peek() == '.') {
			++pos_;
			while (std::isdigit(static_cast<unsigned char>(peek()))) {
				++pos_;
				has_digits = true;
			}
		}
		if (!has_digits) {
			throw ExpressionError("Expected a number near position " + std::to_string(start));
		}
		if (peek() == 'e' || peek() == 'E') {
			const size_t exp_mark = pos_;
			++pos_;
			if (peek() == '+' || peek() == '-') {
				++pos_;
			}
			if (std::isdigit(static_cast<unsigned char>(peek()))) {
				while (std::isdigit(static_cast<unsigned char>(peek()))) {
					++pos_;
				}
			} else {
				pos_ = exp_mark;
			}
		}
		const std::string token(text_.substr(start, pos_ - start));
		try {
			return std::stod(token);
		} catch (const std::exception&) {
			throw ExpressionError("Malformed numeric literal '" + token + "'");
		}
	}

	[[nodiscard]] QuantityValue parse_unit_suffix(QuantityValue base) {
		skip_whitespace();
		if (!is_identifier_char(peek())) {
			return base;
		}
		QuantityValue result = base;
		bool applied_any = false;
		for (;;) {
			skip_whitespace();
			if (!is_identifier_char(peek())) {
				break;
			}
			const size_t saved_pos = pos_;
			const std::string ident = read_identifier();
			if (ident == "sqrt") {
				pos_ = saved_pos;
				break;
			}
			const auto unit = UnitRegistry::lookup(ident);
			if (!unit.has_value()) {
				pos_ = saved_pos;
				break;
			}
			int exponent = 1;
			skip_whitespace();
			if (peek() == '^') {
				++pos_;
				skip_whitespace();
				const bool negative = (peek() == '-');
				if (negative || peek() == '+') {
					++pos_;
				}
				std::string digits;
				while (std::isdigit(static_cast<unsigned char>(peek()))) {
					digits.push_back(advance());
				}
				if (digits.empty()) {
					throw ExpressionError("Expected integer exponent after '^' for unit '" + ident + "'");
				}
				exponent = std::stoi(digits) * (negative ? -1 : 1);
			}
			double converted = result.value;
			if (unit->offset != 0.0) {
				if (applied_any || exponent != 1) {
					throw ExpressionError("Unit '" + ident + "' with an offset cannot be combined or exponentiated");
				}
				converted = (result.value + unit->offset) * unit->factor;
			} else {
				converted = result.value * std::pow(unit->factor, static_cast<double>(exponent));
			}
			result.value = converted;
			result.dimension = result.dimension + unit->dimension.scaled(exponent);
			applied_any = true;
			skip_whitespace();
			if (peek() == '/') {
				const size_t slash_pos = pos_;
				++pos_;
				skip_whitespace();
				if (!is_identifier_char(peek())) {
					pos_ = slash_pos;
					break;
				}
				const std::string denom_ident = read_identifier();
				const auto denom_unit = UnitRegistry::lookup(denom_ident);
				if (!denom_unit.has_value()) {
					pos_ = slash_pos;
					break;
				}
				int denom_exponent = 1;
				skip_whitespace();
				if (peek() == '^') {
					++pos_;
					skip_whitespace();
					const bool negative = (peek() == '-');
					if (negative || peek() == '+') {
						++pos_;
					}
					std::string digits;
					while (std::isdigit(static_cast<unsigned char>(peek()))) {
						digits.push_back(advance());
					}
					if (digits.empty()) {
						throw ExpressionError("Expected integer exponent after '^' for unit '" + denom_ident + "'");
					}
					denom_exponent = std::stoi(digits) * (negative ? -1 : 1);
				}
				result.value /= std::pow(denom_unit->factor, static_cast<double>(denom_exponent));
				result.dimension = result.dimension - denom_unit->dimension.scaled(denom_exponent);
			}
			if (peek() != '*' && peek() != '.' && !std::isalpha(static_cast<unsigned char>(peek()))) {
				break;
			}
			if (peek() == '*' || peek() == '.') {
				++pos_;
			}
		}
		return result;
	}

	[[nodiscard]] QuantityValue parse_primary() {
		skip_whitespace();
		if (match('(')) {
			QuantityValue inner = parse_additive();
			skip_whitespace();
			if (!match(')')) {
				throw ExpressionError("Missing closing parenthesis");
			}
			return parse_unit_suffix(inner);
		}
		if (peek() == '-') {
			++pos_;
			QuantityValue negated = parse_unary();
			negated.value = -negated.value;
			return negated;
		}
		if (peek() == '+') {
			++pos_;
			return parse_unary();
		}
		skip_whitespace();
		if (is_identifier_char(peek())) {
			const size_t saved_pos = pos_;
			const std::string ident = read_identifier();
			if (ident == "sqrt" && match('(')) {
				QuantityValue arg = parse_additive();
				skip_whitespace();
				if (!match(')')) {
					throw ExpressionError("Missing closing parenthesis for sqrt(...)");
				}
				if (arg.dimension.length % 2 != 0 || arg.dimension.mass % 2 != 0 || arg.dimension.time % 2 != 0 ||
				    arg.dimension.current % 2 != 0 || arg.dimension.temperature % 2 != 0 || arg.dimension.angle % 2 != 0) {
					throw ExpressionError("sqrt(...) requires an argument with an even-power dimension");
				}
				if (arg.value < 0.0) {
					throw ExpressionError("sqrt(...) of a negative quantity is undefined");
				}
				QuantityValue result;
				result.value = std::sqrt(arg.value);
				result.dimension = DimensionVector{
					arg.dimension.length / 2, arg.dimension.mass / 2, arg.dimension.time / 2,
					arg.dimension.current / 2, arg.dimension.temperature / 2, arg.dimension.angle / 2
				};
				return parse_unit_suffix(result);
			}
			pos_ = saved_pos;
		}
		const double literal = read_number();
		return parse_unit_suffix(QuantityValue::scalar(literal));
	}

	[[nodiscard]] QuantityValue parse_power() {
		QuantityValue base = parse_primary();
		skip_whitespace();
		if (match('^')) {
			QuantityValue exponent = parse_unary();
			if (!exponent.dimension.is_dimensionless()) {
				throw ExpressionError("Exponent must be dimensionless");
			}
			if (!base.dimension.is_dimensionless()) {
				const double rounded = std::round(exponent.value);
				if (std::abs(exponent.value - rounded) > 1e-9) {
					throw ExpressionError("Non-integer exponent applied to a dimensioned quantity");
				}
				const int int_exp = static_cast<int>(rounded);
				QuantityValue result;
				result.value = std::pow(base.value, exponent.value);
				result.dimension = base.dimension.scaled(int_exp);
				return result;
			}
			QuantityValue result;
			result.value = std::pow(base.value, exponent.value);
			result.dimension = DimensionVector{};
			return result;
		}
		return base;
	}

	[[nodiscard]] QuantityValue parse_unary() {
		skip_whitespace();
		if (peek() == '-') {
			++pos_;
			QuantityValue value = parse_unary();
			value.value = -value.value;
			return value;
		}
		if (peek() == '+') {
			++pos_;
			return parse_unary();
		}
		return parse_power();
	}

	[[nodiscard]] QuantityValue parse_multiplicative() {
		QuantityValue left = parse_unary();
		for (;;) {
			skip_whitespace();
			if (match('*')) {
				QuantityValue right = parse_unary();
				left.value *= right.value;
				left.dimension = left.dimension + right.dimension;
			} else if (match('/')) {
				QuantityValue right = parse_unary();
				if (right.value == 0.0) {
					throw ExpressionError("Division by zero");
				}
				left.value /= right.value;
				left.dimension = left.dimension - right.dimension;
			} else {
				break;
			}
		}
		return left;
	}

	[[nodiscard]] QuantityValue parse_additive() {
		QuantityValue left = parse_multiplicative();
		for (;;) {
			skip_whitespace();
			if (peek() == '+' && !(pos_ + 1 < text_.size())) {
				break;
			}
			if (match('+')) {
				QuantityValue right = parse_multiplicative();
				if (!(left.dimension == right.dimension)) {
					throw ExpressionError("Cannot add quantities with incompatible units");
				}
				left.value += right.value;
			} else if (match('-')) {
				QuantityValue right = parse_multiplicative();
				if (!(left.dimension == right.dimension)) {
					throw ExpressionError("Cannot subtract quantities with incompatible units");
				}
				left.value -= right.value;
			} else {
				break;
			}
		}
		return left;
	}

public:
	[[nodiscard]] static QuantityValue evaluate(std::string_view expression) {
		ExpressionEvaluator evaluator;
		evaluator.text_ = expression;
		evaluator.pos_ = 0;
		if (expression.empty()) {
			throw ExpressionError("Empty expression");
		}
		QuantityValue result = evaluator.parse_additive();
		evaluator.skip_whitespace();
		if (evaluator.pos_ != evaluator.text_.size()) {
			throw ExpressionError(
				"Unexpected trailing characters '" + std::string(evaluator.text_.substr(evaluator.pos_)) +
				"' near position " + std::to_string(evaluator.pos_)
			);
		}
		if (!std::isfinite(result.value)) {
			throw ExpressionError("Expression evaluated to a non-finite value");
		}
		return result;
	}

	[[nodiscard]] static std::optional<double> evaluate_expect_dimension(
		std::string_view expression,
		const DimensionVector& expected_dimension,
		std::string* error_message = nullptr
	) noexcept {
		try {
			const QuantityValue result = evaluate(expression);
			if (!(result.dimension == expected_dimension)) {
				if (error_message != nullptr) {
					*error_message = "Result has incompatible physical dimension";
				}
				return std::nullopt;
			}
			return result.value;
		} catch (const ExpressionError& err) {
			if (error_message != nullptr) {
				*error_message = err.what();
			}
			return std::nullopt;
		} catch (const std::exception& err) {
			if (error_message != nullptr) {
				*error_message = err.what();
			}
			return std::nullopt;
		}
	}

	[[nodiscard]] static std::optional<double> evaluate_dimensionless(std::string_view expression, std::string* error_message = nullptr) noexcept {
		return evaluate_expect_dimension(expression, DimensionVector{}, error_message);
	}
};

namespace Dimensions {
	inline constexpr DimensionVector Length{1, 0, 0, 0, 0, 0};
	inline constexpr DimensionVector Mass{0, 1, 0, 0, 0, 0};
	inline constexpr DimensionVector Time{0, 0, 1, 0, 0, 0};
	inline constexpr DimensionVector Current{0, 0, 0, 1, 0, 0};
	inline constexpr DimensionVector Temperature{0, 0, 0, 0, 1, 0};
	inline constexpr DimensionVector Angle{0, 0, 0, 0, 0, 1};
	inline constexpr DimensionVector Dimensionless{0, 0, 0, 0, 0, 0};
	inline constexpr DimensionVector Velocity{1, 0, -1, 0, 0, 0};
	inline constexpr DimensionVector Charge{0, 0, 1, 1, 0, 0};
	inline constexpr DimensionVector Energy{2, 1, -2, 0, 0, 0};
	inline constexpr DimensionVector Force{1, 1, -2, 0, 0, 0};
	inline constexpr DimensionVector Power{2, 1, -3, 0, 0, 0};
	inline constexpr DimensionVector Frequency{0, 0, -1, 0, 0, 0};
	inline constexpr DimensionVector Pressure{-1, 1, -2, 0, 0, 0};
	inline constexpr DimensionVector Acceleration{1, 0, -2, 0, 0, 0};
	inline constexpr DimensionVector GravitationalConstant{3, -1, -2, 0, 0, 0};
	inline constexpr DimensionVector PlanckAction{2, 1, -1, 0, 0, 0};
	inline constexpr DimensionVector BoltzmannConstant{2, 1, -2, 0, -1, 0};
	inline constexpr DimensionVector CoulombConstant{3, 1, -4, -2, 0, 0};
	inline constexpr DimensionVector Area{2, 0, 0, 0, 0, 0};
	inline constexpr DimensionVector Volume{3, 0, 0, 0, 0, 0};
	inline constexpr DimensionVector Density{-3, 1, 0, 0, 0, 0};
	inline constexpr DimensionVector AngularVelocity{0, 0, -1, 0, 0, 1};
	inline constexpr DimensionVector MomentOfInertia{2, 1, 0, 0, 0, 0};
	inline constexpr DimensionVector Torque{2, 1, -2, 0, 0, 0};
	inline constexpr DimensionVector Luminosity{2, 1, -3, 0, 0, 0};
	inline constexpr DimensionVector MagneticField{0, 1, -2, -1, 0, 0};
	inline constexpr DimensionVector MassFlowRate{0, 1, -1, 0, 0, 0};
	inline constexpr DimensionVector Momentum{1, 1, -1, 0, 0, 0};
}

}
