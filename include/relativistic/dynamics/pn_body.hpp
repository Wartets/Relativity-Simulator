#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <cmath>
#include <algorithm>

namespace Relativistic::Dynamics {

struct alignas(64) PostNewtonianBody {
	uint32_t id{0};
	double mass{1.0};
	double radius{1.0};
	std::array<double, 3> position{0.0, 0.0, 0.0};
	std::array<double, 3> velocity{0.0, 0.0, 0.0};
	std::array<double, 3> acceleration{0.0, 0.0, 0.0};
	std::array<double, 3> spin{0.0, 0.0, 0.0};
	double quadrupole_moment{0.0};
	double j2{0.0};
	double j3{0.0};
	double j4{0.0};
	double reference_radius{0.0};

	constexpr PostNewtonianBody() noexcept = default;

	constexpr PostNewtonianBody(
		uint32_t body_id,
		double m,
		double r,
		const std::array<double, 3>& pos,
		const std::array<double, 3>& vel,
		const std::array<double, 3>& s = {0.0, 0.0, 0.0},
		double q = 0.0,
		double j2_val = 0.0,
		double j3_val = 0.0,
		double j4_val = 0.0,
		double r_ref = 0.0
	) noexcept
		: id(body_id),
		  mass(m),
		  radius(r),
		  position(pos),
		  velocity(vel),
		  acceleration{0.0, 0.0, 0.0},
		  spin(s),
		  quadrupole_moment(q),
		  j2(j2_val),
		  j3(j3_val),
		  j4(j4_val),
		  reference_radius((r_ref > 0.0) ? r_ref : r) {}

	[[nodiscard]] double speed_squared() const noexcept {
		return velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2];
	}

	[[nodiscard]] double speed() const noexcept {
		return std::sqrt(speed_squared());
	}

	[[nodiscard]] double spin_magnitude_squared() const noexcept {
		return spin[0] * spin[0] + spin[1] * spin[1] + spin[2] * spin[2];
	}

	[[nodiscard]] double spin_magnitude() const noexcept {
		return std::sqrt(spin_magnitude_squared());
	}

	[[nodiscard]] double kinetic_energy() const noexcept {
		return 0.5 * mass * speed_squared();
	}
};

}
