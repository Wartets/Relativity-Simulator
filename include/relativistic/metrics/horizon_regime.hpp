#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <string_view>

namespace Relativistic::Metrics {

enum class HorizonRegime : uint8_t {
	SubExtremal = 0,
	Extremal = 1,
	SuperExtremal = 2
};

enum class HorizonParameter : uint8_t {
	Mass = 0,
	Spin = 1,
	Charge = 2,
	CosmologicalConstant = 3
};

struct HorizonState {
	double mass{1.0};
	double spin{0.0};
	double charge{0.0};
	double cosmological_constant{0.0};
};

struct HorizonConstraintSet {
	bool spin{false};
	bool charge{false};
	bool cosmological{false};

	[[nodiscard]] constexpr bool any() const noexcept {
		return spin || charge || cosmological;
	}

	[[nodiscard]] static HorizonConstraintSet from_metric_name(std::string_view name) noexcept {
		HorizonConstraintSet set;
		if (name.find("Newman") != std::string_view::npos) {
			set.spin = true;
			set.charge = true;
		} else if (name.find("Reissner") != std::string_view::npos) {
			set.charge = true;
		} else if (name.find("Kerr") != std::string_view::npos) {
			set.spin = true;
		} else if (name.find("de Sitter") != std::string_view::npos || name.find("DeSitter") != std::string_view::npos) {
			set.cosmological = true;
		}
		return set;
	}
};

struct HorizonZoneLayout {
	double threshold{0.0};
	bool symmetric{false};
	bool sub_extremal_above{false};
	bool valid{false};
};

class HorizonRegimeAnalyzer {
public:
	static constexpr double kExtremalTolerance = 5.0e-4;
	static constexpr double kGuaranteeFactor = 1.0 - 4.0 * kExtremalTolerance;

	[[nodiscard]] static double extremality_ratio(const HorizonState& state, const HorizonConstraintSet& constraints) noexcept {
		const double mass_squared = std::max(state.mass * state.mass, 1e-30);
		const double spin_squared = constraints.spin ? state.spin * state.spin : 0.0;
		const double charge_squared = constraints.charge ? state.charge * state.charge : 0.0;
		return (spin_squared + charge_squared) / mass_squared;
	}

	[[nodiscard]] static double cosmological_ratio(const HorizonState& state, const HorizonConstraintSet& constraints) noexcept {
		if (!constraints.cosmological) {
			return 0.0;
		}
		return 9.0 * std::max(state.cosmological_constant, 0.0) * state.mass * state.mass;
	}

	[[nodiscard]] static HorizonRegime classify_ratio(double ratio) noexcept {
		if (ratio > 1.0 + kExtremalTolerance) {
			return HorizonRegime::SuperExtremal;
		}
		if (ratio >= 1.0 - kExtremalTolerance) {
			return HorizonRegime::Extremal;
		}
		return HorizonRegime::SubExtremal;
	}

	[[nodiscard]] static HorizonRegime classify(const HorizonState& state, const HorizonConstraintSet& constraints) noexcept {
		const HorizonRegime rotational = (constraints.spin || constraints.charge)
			? classify_ratio(extremality_ratio(state, constraints))
			: HorizonRegime::SubExtremal;
		const HorizonRegime cosmological = constraints.cosmological
			? classify_ratio(cosmological_ratio(state, constraints))
			: HorizonRegime::SubExtremal;
		return (static_cast<uint8_t>(rotational) >= static_cast<uint8_t>(cosmological)) ? rotational : cosmological;
	}

	[[nodiscard]] static const char* regime_name(HorizonRegime regime) noexcept {
		switch (regime) {
			case HorizonRegime::SubExtremal: return "Sub-extremal";
			case HorizonRegime::Extremal: return "Extremal";
			case HorizonRegime::SuperExtremal:
			default: return "Super-extremal";
		}
	}

	[[nodiscard]] static double outer_horizon_radius(const HorizonState& state, const HorizonConstraintSet& constraints) noexcept {
		const double spin_squared = constraints.spin ? state.spin * state.spin : 0.0;
		const double charge_squared = constraints.charge ? state.charge * state.charge : 0.0;
		return state.mass + std::sqrt(std::max(state.mass * state.mass - spin_squared - charge_squared, 0.0));
	}

	[[nodiscard]] static double value_of(HorizonParameter parameter, const HorizonState& state) noexcept {
		switch (parameter) {
			case HorizonParameter::Mass: return state.mass;
			case HorizonParameter::Spin: return state.spin;
			case HorizonParameter::Charge: return state.charge;
			case HorizonParameter::CosmologicalConstant:
			default: return state.cosmological_constant;
		}
	}

	[[nodiscard]] static double guaranteed_maximum(HorizonParameter parameter, const HorizonState& state, const HorizonConstraintSet& constraints) noexcept {
		const double mass_squared = state.mass * state.mass;
		switch (parameter) {
			case HorizonParameter::Spin:
				return std::sqrt(std::max(kGuaranteeFactor * mass_squared - (constraints.charge ? state.charge * state.charge : 0.0), 0.0));
			case HorizonParameter::Charge:
				return std::sqrt(std::max(kGuaranteeFactor * mass_squared - (constraints.spin ? state.spin * state.spin : 0.0), 0.0));
			case HorizonParameter::CosmologicalConstant:
				return (constraints.cosmological && state.mass > 0.0) ? kGuaranteeFactor / (9.0 * mass_squared) : std::numeric_limits<double>::infinity();
			case HorizonParameter::Mass:
			default:
				return (constraints.cosmological && state.cosmological_constant > 0.0)
					? std::sqrt(kGuaranteeFactor / (9.0 * state.cosmological_constant))
					: std::numeric_limits<double>::infinity();
		}
	}

	[[nodiscard]] static double guaranteed_minimum(HorizonParameter parameter, const HorizonState& state, const HorizonConstraintSet& constraints) noexcept {
		switch (parameter) {
			case HorizonParameter::Mass: {
				const double spin_squared = constraints.spin ? state.spin * state.spin : 0.0;
				const double charge_squared = constraints.charge ? state.charge * state.charge : 0.0;
				return std::sqrt((spin_squared + charge_squared) / kGuaranteeFactor);
			}
			case HorizonParameter::Spin:
			case HorizonParameter::Charge:
				return -guaranteed_maximum(parameter, state, constraints);
			case HorizonParameter::CosmologicalConstant:
			default:
				return 0.0;
		}
	}

	[[nodiscard]] static double constrain(HorizonParameter parameter, double value, const HorizonState& state, const HorizonConstraintSet& constraints) noexcept {
		switch (parameter) {
			case HorizonParameter::Mass: {
				const double lower = guaranteed_minimum(parameter, state, constraints);
				const double upper = guaranteed_maximum(parameter, state, constraints);
				return (upper >= lower) ? std::clamp(value, lower, upper) : std::max(value, lower);
			}
			case HorizonParameter::Spin: {
				if (!constraints.spin) {
					return value;
				}
				const double limit = guaranteed_maximum(parameter, state, constraints);
				return std::clamp(value, -limit, limit);
			}
			case HorizonParameter::Charge: {
				if (!constraints.charge) {
					return value;
				}
				const double limit = guaranteed_maximum(parameter, state, constraints);
				return std::clamp(value, -limit, limit);
			}
			case HorizonParameter::CosmologicalConstant:
			default: {
				if (!constraints.cosmological) {
					return value;
				}
				return std::clamp(value, 0.0, guaranteed_maximum(parameter, state, constraints));
			}
		}
	}

	[[nodiscard]] static bool project(HorizonState& state, const HorizonConstraintSet& constraints) noexcept {
		bool changed = false;
		if (constraints.cosmological) {
			const double limit = guaranteed_maximum(HorizonParameter::CosmologicalConstant, state, constraints);
			if (state.cosmological_constant > limit) {
				state.cosmological_constant = limit;
				changed = true;
			}
		}
		if (constraints.spin || constraints.charge) {
			const double spin_squared = constraints.spin ? state.spin * state.spin : 0.0;
			const double charge_squared = constraints.charge ? state.charge * state.charge : 0.0;
			const double allowed = kGuaranteeFactor * state.mass * state.mass;
			if (spin_squared + charge_squared > allowed) {
				const double scale = std::sqrt(allowed / (spin_squared + charge_squared));
				if (constraints.spin) {
					state.spin *= scale;
				}
				if (constraints.charge) {
					state.charge *= scale;
				}
				changed = true;
			}
		}
		return changed;
	}

	[[nodiscard]] static HorizonZoneLayout zone_layout(HorizonParameter parameter, const HorizonState& state, const HorizonConstraintSet& constraints) noexcept {
		HorizonZoneLayout layout;
		const double mass_squared = state.mass * state.mass;
		switch (parameter) {
			case HorizonParameter::Mass:
				if (constraints.cosmological) {
					if (state.cosmological_constant > 0.0) {
						layout.threshold = 1.0 / (3.0 * std::sqrt(state.cosmological_constant));
						layout.sub_extremal_above = false;
						layout.valid = true;
					}
				} else if (constraints.spin || constraints.charge) {
					const double spin_squared = constraints.spin ? state.spin * state.spin : 0.0;
					const double charge_squared = constraints.charge ? state.charge * state.charge : 0.0;
					layout.threshold = std::sqrt(spin_squared + charge_squared);
					layout.sub_extremal_above = true;
					layout.valid = layout.threshold > 0.0;
				}
				break;
			case HorizonParameter::Spin:
				if (constraints.spin) {
					layout.threshold = std::sqrt(std::max(mass_squared - (constraints.charge ? state.charge * state.charge : 0.0), 0.0));
					layout.symmetric = true;
					layout.valid = true;
				}
				break;
			case HorizonParameter::Charge:
				if (constraints.charge) {
					layout.threshold = std::sqrt(std::max(mass_squared - (constraints.spin ? state.spin * state.spin : 0.0), 0.0));
					layout.symmetric = true;
					layout.valid = true;
				}
				break;
			case HorizonParameter::CosmologicalConstant:
			default:
				if (constraints.cosmological && state.mass > 0.0) {
					layout.threshold = 1.0 / (9.0 * mass_squared);
					layout.valid = true;
				}
				break;
		}
		return layout;
	}

	[[nodiscard]] static std::string describe(const HorizonState& state, const HorizonConstraintSet& constraints) {
		char buffer[256];
		const HorizonRegime regime = classify(state, constraints);
		if (constraints.cosmological) {
			std::snprintf(buffer, sizeof(buffer), "%s: 9 Lambda M^2 = %.4f (Nariai limit at 1). Black hole and cosmological horizons %s.",
				regime_name(regime), cosmological_ratio(state, constraints), regime == HorizonRegime::SuperExtremal ? "do not exist" : "coexist");
		} else if (constraints.spin || constraints.charge) {
			const double ratio = extremality_ratio(state, constraints);
			if (regime == HorizonRegime::SuperExtremal) {
				std::snprintf(buffer, sizeof(buffer), "%s: (a^2 + Q^2) / M^2 = %.4f > 1. No event horizon, naked singularity.", regime_name(regime), ratio);
			} else {
				std::snprintf(buffer, sizeof(buffer), "%s: (a^2 + Q^2) / M^2 = %.4f. Outer horizon at r = %.4f M.",
					regime_name(regime), ratio, outer_horizon_radius(state, constraints) / std::max(state.mass, 1e-30));
			}
		} else {
			std::snprintf(buffer, sizeof(buffer), "%s: event horizon at r = 2 M.", regime_name(regime));
		}
		return std::string(buffer);
	}
};

}
