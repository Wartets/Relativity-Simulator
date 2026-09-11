#pragma once

#include <cstdint>
#include <numbers>

namespace Relativistic::Core {

template <typename T = double>
struct PhysicalConstants {
	static constexpr T SPEED_OF_LIGHT = static_cast<T>(299792458.0);
	static constexpr T GRAVITATIONAL_CONSTANT = static_cast<T>(6.67430e-11);
	static constexpr T PLANCK_CONSTANT = static_cast<T>(6.62607015e-34);
	static constexpr T REDUCED_PLANCK_CONSTANT = static_cast<T>(1.054571817e-34);
	static constexpr T BOLTZMANN_CONSTANT = static_cast<T>(1.380649e-23);
	static constexpr T SOLAR_MASS = static_cast<T>(1.98847e30);
	static constexpr T ASTRONOMICAL_UNIT = static_cast<T>(149597870700.0);
	static constexpr T SOLAR_SCHWARZSCHILD_RADIUS = static_cast<T>(2.0 * GRAVITATIONAL_CONSTANT * SOLAR_MASS / (SPEED_OF_LIGHT * SPEED_OF_LIGHT));
	static constexpr T ELECTRON_MASS = static_cast<T>(9.1093837015e-31);
	static constexpr T PROTON_MASS = static_cast<T>(1.67262192369e-27);
	static constexpr T NEUTRON_MASS = static_cast<T>(1.67492749804e-27);
	static constexpr T ELEMENTARY_CHARGE = static_cast<T>(1.602176634e-19);
	static constexpr T VACUUM_PERMITTIVITY = static_cast<T>(8.8541878128e-12);
	static constexpr T THOMSON_CROSS_SECTION = static_cast<T>(6.6524587321e-29);
	static constexpr T STEFAN_BOLTZMANN_CONSTANT = static_cast<T>(5.670374419e-8);
	static constexpr T WIEN_DISPLACEMENT_CONSTANT = static_cast<T>(2.897771955e-3);
	static constexpr T PARSEC = static_cast<T>(3.0856775814913673e16);
	static constexpr T LIGHT_YEAR = static_cast<T>(9.4607304725808e15);
	static constexpr T EARTH_MASS = static_cast<T>(5.9722e24);
	static constexpr T JUPITER_MASS = static_cast<T>(1.89813e27);
};

template <typename T = double>
struct GeometrizedConstants {
	static constexpr T SPEED_OF_LIGHT = static_cast<T>(1.0);
	static constexpr T GRAVITATIONAL_CONSTANT = static_cast<T>(1.0);
	static constexpr T SOLAR_MASS = static_cast<T>(1477.0);
	static constexpr T PI = std::numbers::pi_v<T>;
	static constexpr T TWO_PI = static_cast<T>(2.0) * std::numbers::pi_v<T>;
	static constexpr T HALF_PI = std::numbers::pi_v<T> / static_cast<T>(2.0);
};

using ConstantsSI = PhysicalConstants<double>;
using ConstantsGeometrized = GeometrizedConstants<double>;

}
