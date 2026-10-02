#pragma once

#include "relativistic/capture/expression.hpp"
#include "relativistic/io/capture_settings_io.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string>
#include <vector>

namespace Relativistic::Capture {

enum class EasingKind : uint32_t {
	Linear = 0,
	QuadIn, QuadOut, QuadInOut,
	CubicIn, CubicOut, CubicInOut,
	QuartIn, QuartOut, QuartInOut,
	QuintIn, QuintOut, QuintInOut,
	SineIn, SineOut, SineInOut,
	ExpoIn, ExpoOut, ExpoInOut,
	CircIn, CircOut, CircInOut,
	BackIn, BackOut, BackInOut,
	ElasticIn, ElasticOut, ElasticInOut,
	BounceIn, BounceOut, BounceInOut,
	Smoothstep, Smootherstep, Smoothest,
	Power, PowerInOut, Steps, DelayWindow, CubicBezier, Spring, CustomExpression, CustomCurve
};

inline constexpr size_t kEasingKindCount = static_cast<size_t>(EasingKind::CustomCurve) + 1;

[[nodiscard]] inline const std::vector<const char*>& easing_kind_names() {
	static const std::vector<std::string> storage = [] {
		static constexpr std::array<const char*, 10> families{"Quadratic", "Cubic", "Quartic", "Quintic", "Sinusoidal", "Exponential", "Circular", "Back", "Elastic", "Bounce"};
		static constexpr std::array<const char*, 3> variants{"In", "Out", "In-Out"};
		std::vector<std::string> names{"Linear"};
		for (const char* family : families) {
			for (const char* variant : variants) {
				names.push_back(std::string(family) + " " + variant);
			}
		}
		for (const char* extra : {"Smoothstep", "Smootherstep", "Smoothest (7th Order)", "Power", "Power In-Out", "Steps", "Delay Window", "Cubic Bezier", "Damped Spring", "Custom Expression", "Custom Curve"}) {
			names.emplace_back(extra);
		}
		return names;
	}();
	static const std::vector<const char*> pointers = [] {
		std::vector<const char*> result;
		result.reserve(storage.size());
		for (const std::string& name : storage) result.push_back(name.c_str());
		return result;
	}();
	return pointers;
}

[[nodiscard]] inline std::array<const char*, 4> easing_parameter_labels(EasingKind kind) noexcept {
	switch (kind) {
		case EasingKind::Power:
		case EasingKind::PowerInOut: return {"Exponent", nullptr, nullptr, nullptr};
		case EasingKind::Steps: return {"Step Count", "Jump Mode (0 End, 1 Start, 2 Center)", nullptr, nullptr};
		case EasingKind::DelayWindow: return {"Start Fraction", "End Fraction", nullptr, nullptr};
		case EasingKind::CubicBezier: return {"Control X1", "Control Y1", "Control X2", "Control Y2"};
		case EasingKind::Spring: return {"Damping", "Oscillations", nullptr, nullptr};
		case EasingKind::CustomExpression: return {"Parameter a", "Parameter b", "Parameter c", "Parameter k"};
		default: return {nullptr, nullptr, nullptr, nullptr};
	}
}

struct EasingPoint {
	double position{0.0};
	double value{0.0};
};

[[nodiscard]] inline double ease_family_in(uint32_t family, double u) noexcept {
	constexpr double pi = std::numbers::pi_v<double>;
	switch (family) {
		case 0: return u * u;
		case 1: return u * u * u;
		case 2: return u * u * u * u;
		case 3: return u * u * u * u * u;
		case 4: return 1.0 - std::cos(u * pi * 0.5);
		case 5: return (u <= 0.0) ? 0.0 : std::pow(2.0, 10.0 * u - 10.0);
		case 6: return 1.0 - std::sqrt(std::max(1.0 - u * u, 0.0));
		case 7: {
			constexpr double c1 = 1.70158;
			return (c1 + 1.0) * u * u * u - c1 * u * u;
		}
		case 8: {
			if (u <= 0.0) return 0.0;
			if (u >= 1.0) return 1.0;
			return -std::pow(2.0, 10.0 * u - 10.0) * std::sin((u * 10.0 - 10.75) * (2.0 * pi / 3.0));
		}
		default: {
			double v = 1.0 - u;
			constexpr double n1 = 7.5625;
			constexpr double d1 = 2.75;
			double out = 0.0;
			if (v < 1.0 / d1) out = n1 * v * v;
			else if (v < 2.0 / d1) { v -= 1.5 / d1; out = n1 * v * v + 0.75; }
			else if (v < 2.5 / d1) { v -= 2.25 / d1; out = n1 * v * v + 0.9375; }
			else { v -= 2.625 / d1; out = n1 * v * v + 0.984375; }
			return 1.0 - out;
		}
	}
}

[[nodiscard]] inline double cubic_bezier_ease(double x1, double y1, double x2, double y2, double x) noexcept {
	const auto bx = [&](double t) { const double m = 1.0 - t; return 3.0 * m * m * t * x1 + 3.0 * m * t * t * x2 + t * t * t; };
	const auto by = [&](double t) { const double m = 1.0 - t; return 3.0 * m * m * t * y1 + 3.0 * m * t * t * y2 + t * t * t; };
	const auto dbx = [&](double t) { const double m = 1.0 - t; return 3.0 * m * m * x1 + 6.0 * m * t * (x2 - x1) + 3.0 * t * t * (1.0 - x2); };
	double t = x;
	for (int i = 0; i < 8; ++i) {
		const double error = bx(t) - x;
		if (std::abs(error) < 1e-9) return by(t);
		const double derivative = dbx(t);
		if (std::abs(derivative) < 1e-6) break;
		t = std::clamp(t - error / derivative, 0.0, 1.0);
	}
	double lo = 0.0;
	double hi = 1.0;
	t = x;
	for (int i = 0; i < 40; ++i) {
		const double value = bx(t);
		if (std::abs(value - x) < 1e-9) break;
		if (value < x) lo = t; else hi = t;
		t = (lo + hi) * 0.5;
	}
	return by(t);
}

struct EasingSpec {
	EasingKind kind{EasingKind::Linear};
	std::array<double, 4> parameters{1.0, 0.0, 0.0, 1.0};
	double blend{1.0};
	double repeat{1.0};
	bool ping_pong{false};
	bool reverse{false};
	bool smooth_curve{true};
	Expression expression{"u"};
	std::vector<EasingPoint> points{{0.0, 0.0}, {0.5, 0.5}, {1.0, 1.0}};

	[[nodiscard]] static EasingSpec make(EasingKind kind) {
		EasingSpec spec;
		spec.kind = kind;
		spec.reset_parameters();
		return spec;
	}

	void reset_parameters() {
		parameters = {1.0, 0.0, 0.0, 1.0};
		switch (kind) {
			case EasingKind::Power:
			case EasingKind::PowerInOut: parameters = {2.0, 0.0, 0.0, 1.0}; break;
			case EasingKind::Steps: parameters = {5.0, 0.0, 0.0, 1.0}; break;
			case EasingKind::DelayWindow: parameters = {0.2, 0.8, 0.0, 1.0}; break;
			case EasingKind::CubicBezier: parameters = {0.25, 0.1, 0.25, 1.0}; break;
			case EasingKind::Spring: parameters = {4.0, 3.0, 0.0, 1.0}; break;
			case EasingKind::CustomExpression: expression.assign("u"); break;
			case EasingKind::CustomCurve: points = {{0.0, 0.0}, {0.25, 0.1}, {0.75, 0.9}, {1.0, 1.0}}; break;
			default: break;
		}
	}

	void sort_points() {
		std::stable_sort(points.begin(), points.end(), [](const EasingPoint& a, const EasingPoint& b) noexcept { return a.position < b.position; });
	}

	[[nodiscard]] double evaluate(double progress) const noexcept {
		double x = std::clamp(progress, 0.0, 1.0);
		if (reverse) x = 1.0 - x;
		const double cycles = std::max(repeat, 1.0);
		if (cycles > 1.0 + 1e-9) {
			const double scaled = x * cycles;
			double cycle = std::floor(scaled);
			double fraction = scaled - cycle;
			if (fraction < 1e-12 && x >= 1.0 && cycle > 0.0) {
				cycle -= 1.0;
				fraction = 1.0;
			}
			if (ping_pong && std::fmod(cycle, 2.0) >= 1.0) fraction = 1.0 - fraction;
			x = fraction;
		}
		const double shaped = evaluate_base(x);
		return x + (shaped - x) * blend;
	}

private:
	[[nodiscard]] double evaluate_base(double x) const noexcept {
		const uint32_t index = static_cast<uint32_t>(kind);
		if (kind == EasingKind::Linear) return x;
		if (index >= 1 && index <= 30) {
			const uint32_t family = (index - 1) / 3;
			const uint32_t variant = (index - 1) % 3;
			if (variant == 0) return ease_family_in(family, x);
			if (variant == 1) return 1.0 - ease_family_in(family, 1.0 - x);
			return (x < 0.5) ? ease_family_in(family, 2.0 * x) * 0.5 : 1.0 - ease_family_in(family, 2.0 - 2.0 * x) * 0.5;
		}
		switch (kind) {
			case EasingKind::Smoothstep: return x * x * (3.0 - 2.0 * x);
			case EasingKind::Smootherstep: return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
			case EasingKind::Smoothest: return x * x * x * x * (35.0 + x * (-84.0 + x * (70.0 - 20.0 * x)));
			case EasingKind::Power: return std::pow(x, std::max(parameters[0], 0.01));
			case EasingKind::PowerInOut: {
				const double p = std::max(parameters[0], 0.01);
				return (x < 0.5) ? 0.5 * std::pow(2.0 * x, p) : 1.0 - 0.5 * std::pow(2.0 - 2.0 * x, p);
			}
			case EasingKind::Steps: {
				const double count = std::max(1.0, std::round(parameters[0]));
				const int mode = static_cast<int>(std::round(parameters[1]));
				const double scaled = x * count;
				const double stepped = (mode == 1) ? std::ceil(scaled) : ((mode == 2) ? std::round(scaled) : std::floor(scaled));
				return std::clamp(stepped / count, 0.0, 1.0);
			}
			case EasingKind::DelayWindow: {
				const double span = parameters[1] - parameters[0];
				if (std::abs(span) < 1e-9) return (x >= parameters[0]) ? 1.0 : 0.0;
				return std::clamp((x - parameters[0]) / span, 0.0, 1.0);
			}
			case EasingKind::CubicBezier:
				return cubic_bezier_ease(std::clamp(parameters[0], 0.0, 1.0), parameters[1], std::clamp(parameters[2], 0.0, 1.0), parameters[3], x);
			case EasingKind::Spring: {
				const double damping = std::max(parameters[0], 0.0);
				const double frequency = std::max(parameters[1], 0.0) * 2.0 * std::numbers::pi_v<double>;
				const auto response = [&](double t) { return 1.0 - std::exp(-damping * t) * std::cos(frequency * t); };
				const double end = response(1.0);
				return (std::abs(end) < 1e-9) ? x : response(x) / end;
			}
			case EasingKind::CustomExpression: {
				ExpressionVariables variables{};
				variables[ExpressionSlot::Progress] = x;
				variables[ExpressionSlot::Duration] = 1.0;
				variables[ExpressionSlot::ParameterA] = parameters[0];
				variables[ExpressionSlot::ParameterB] = parameters[1];
				variables[ExpressionSlot::ParameterC] = parameters[2];
				variables[ExpressionSlot::ParameterK] = parameters[3];
				return expression.evaluate(variables, x);
			}
			case EasingKind::CustomCurve: return evaluate_curve(x);
			default: return x;
		}
	}

	[[nodiscard]] double evaluate_curve(double x) const noexcept {
		const size_t n = points.size();
		if (n == 0) return x;
		if (n == 1) return points[0].value;
		if (x <= points.front().position) return points.front().value;
		if (x >= points.back().position) return points.back().value;
		size_t i = 0;
		while (i + 2 < n && points[i + 1].position <= x) ++i;
		const EasingPoint& a = points[i];
		const EasingPoint& b = points[i + 1];
		const double span = b.position - a.position;
		if (span <= 1e-12) return b.value;
		const double s = (x - a.position) / span;
		if (!smooth_curve) return a.value + (b.value - a.value) * s;
		const auto slope = [&](size_t k) {
			const size_t lo = (k > 0) ? k - 1 : k;
			const size_t hi = (k + 1 < n) ? k + 1 : k;
			const double dp = points[hi].position - points[lo].position;
			return (dp > 1e-12) ? (points[hi].value - points[lo].value) / dp : 0.0;
		};
		const double m0 = slope(i) * span;
		const double m1 = slope(i + 1) * span;
		const double s2 = s * s;
		const double s3 = s2 * s;
		return (2.0 * s3 - 3.0 * s2 + 1.0) * a.value + (s3 - 2.0 * s2 + s) * m0 + (-2.0 * s3 + 3.0 * s2) * b.value + (s3 - s2) * m1;
	}

public:
	void write(IO::SettingsWriter& writer, const std::string& prefix) const {
		writer.enumeration(prefix + "kind", kind);
		for (size_t i = 0; i < parameters.size(); ++i) {
			writer.real(prefix + "p" + std::to_string(i), parameters[i]);
		}
		writer.real(prefix + "blend", blend);
		writer.real(prefix + "repeat", repeat);
		writer.flag(prefix + "ping_pong", ping_pong);
		writer.flag(prefix + "reverse", reverse);
		writer.flag(prefix + "smooth", smooth_curve);
		writer.text(prefix + "expr", expression.source());
		writer.unsigned_value(prefix + "points", points.size());
		for (size_t i = 0; i < points.size(); ++i) {
			writer.real(prefix + "pt" + std::to_string(i) + ".u", points[i].position);
			writer.real(prefix + "pt" + std::to_string(i) + ".v", points[i].value);
		}
	}

	void read(const IO::SettingsReader& reader, const std::string& prefix) {
		kind = reader.enumeration(prefix + "kind", kind, EasingKind::CustomCurve);
		for (size_t i = 0; i < parameters.size(); ++i) {
			parameters[i] = reader.real(prefix + "p" + std::to_string(i), parameters[i]);
		}
		blend = reader.real(prefix + "blend", blend);
		repeat = std::max(reader.real(prefix + "repeat", repeat), 1.0);
		ping_pong = reader.flag(prefix + "ping_pong", ping_pong);
		reverse = reader.flag(prefix + "reverse", reverse);
		smooth_curve = reader.flag(prefix + "smooth", smooth_curve);
		expression.assign(reader.text(prefix + "expr", expression.source()));
		const size_t count = std::min<size_t>(reader.wide_value(prefix + "points", points.size()), 4096);
		std::vector<EasingPoint> loaded;
		loaded.reserve(count);
		for (size_t i = 0; i < count; ++i) {
			loaded.push_back(EasingPoint{reader.real(prefix + "pt" + std::to_string(i) + ".u", 0.0), reader.real(prefix + "pt" + std::to_string(i) + ".v", 0.0)});
		}
		if (!loaded.empty()) points = std::move(loaded);
		sort_points();
	}
};

}
