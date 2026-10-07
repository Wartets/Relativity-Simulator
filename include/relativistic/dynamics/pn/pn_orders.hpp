#pragma once

#include <cstdint>
#include "relativistic/core/constants.hpp"

namespace Relativistic::Dynamics {

struct PNOrderConfig {
	bool enable_newtonian{true};
	bool enable_1pn{true};
	bool enable_2pn{true};
	bool enable_2_5pn{true};
	bool enable_3pn{true};
	bool enable_3_5pn{true};
	bool enable_spin_orbit{true};
	bool enable_spin_spin{true};
	bool enable_spin_self{true};
	bool enable_spherical_harmonics{true};
	double speed_of_light{Core::PhysicalConstants<double>::SPEED_OF_LIGHT};
	double gravitational_constant{Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT};
	double r0_scale{1.0};

	[[nodiscard]] static constexpr PNOrderConfig make_all_enabled(
		double c = Core::PhysicalConstants<double>::SPEED_OF_LIGHT,
		double g = Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT
	) noexcept {
		PNOrderConfig cfg{};
		cfg.speed_of_light = c;
		cfg.gravitational_constant = g;
		return cfg;
	}

	[[nodiscard]] static constexpr PNOrderConfig make_conservative_only(
		double c = Core::PhysicalConstants<double>::SPEED_OF_LIGHT,
		double g = Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT
	) noexcept {
		PNOrderConfig cfg{};
		cfg.enable_2_5pn = false;
		cfg.enable_3_5pn = false;
		cfg.speed_of_light = c;
		cfg.gravitational_constant = g;
		return cfg;
	}

	[[nodiscard]] static constexpr PNOrderConfig make_newtonian_only(
		double c = Core::PhysicalConstants<double>::SPEED_OF_LIGHT,
		double g = Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT
	) noexcept {
		PNOrderConfig cfg{};
		cfg.enable_1pn = false;
		cfg.enable_2pn = false;
		cfg.enable_2_5pn = false;
		cfg.enable_3pn = false;
		cfg.enable_3_5pn = false;
		cfg.enable_spin_orbit = false;
		cfg.enable_spin_spin = false;
		cfg.enable_spin_self = false;
		cfg.speed_of_light = c;
		cfg.gravitational_constant = g;
		return cfg;
	}

	[[nodiscard]] static constexpr PNOrderConfig make_1pn_only(
		double c = Core::PhysicalConstants<double>::SPEED_OF_LIGHT,
		double g = Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT
	) noexcept {
		PNOrderConfig cfg{};
		cfg.enable_2pn = false;
		cfg.enable_2_5pn = false;
		cfg.enable_3pn = false;
		cfg.enable_3_5pn = false;
		cfg.enable_spin_orbit = false;
		cfg.enable_spin_spin = false;
		cfg.enable_spin_self = false;
		cfg.speed_of_light = c;
		cfg.gravitational_constant = g;
		return cfg;
	}

	[[nodiscard]] static constexpr PNOrderConfig make_2_5pn_radiation_only(
		double c = Core::PhysicalConstants<double>::SPEED_OF_LIGHT,
		double g = Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT
	) noexcept {
		PNOrderConfig cfg{};
		cfg.enable_1pn = false;
		cfg.enable_2pn = false;
		cfg.enable_3pn = false;
		cfg.enable_3_5pn = false;
		cfg.enable_spin_orbit = false;
		cfg.enable_spin_spin = false;
		cfg.enable_spin_self = false;
		cfg.speed_of_light = c;
		cfg.gravitational_constant = g;
		return cfg;
	}
};

}
