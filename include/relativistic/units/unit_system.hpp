#pragma once

#include <cstdint>
#include <string>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace Relativistic::Units {

enum class DistanceUnit : uint32_t {
	Meters = 0,
	Kilometers = 1,
	Feet = 2,
	Miles = 3,
	NauticalMiles = 4,
	AstronomicalUnits = 5,
	LightYears = 6,
	Parsecs = 7,
	Kiloparsecs = 8,
	SolarRadii = 9
};

enum class MassUnit : uint32_t {
	Kilograms = 0,
	Grams = 1,
	Pounds = 2,
	MetricTonnes = 3,
	SolarMasses = 4,
	EarthMasses = 5,
	JupiterMasses = 6
};

enum class VelocityUnit : uint32_t {
	MetersPerSecond = 0,
	KilometersPerHour = 1,
	MilesPerHour = 2,
	KilometersPerSecond = 3,
	FractionOfC = 4,
	ParsecsPerYear = 5,
	AstronomicalUnitsPerDay = 6
};

struct UnitDisplayPreferences {
	DistanceUnit distance{DistanceUnit::Meters};
	MassUnit mass{MassUnit::Kilograms};
	VelocityUnit velocity{VelocityUnit::MetersPerSecond};
};

namespace Detail {
	inline constexpr double METERS_PER_KM = 1000.0;
	inline constexpr double METERS_PER_FOOT = 0.3048;
	inline constexpr double METERS_PER_MILE = 1609.344;
	inline constexpr double METERS_PER_NAUTICAL_MILE = 1852.0;
	inline constexpr double METERS_PER_AU = 149597870700.0;
	inline constexpr double METERS_PER_LIGHT_YEAR = 9.4607304725808e15;
	inline constexpr double METERS_PER_PARSEC = 3.0856775814913673e16;
	inline constexpr double METERS_PER_SOLAR_RADIUS = 6.9634e8;

	inline constexpr double KG_PER_GRAM = 0.001;
	inline constexpr double KG_PER_POUND = 0.45359237;
	inline constexpr double KG_PER_TONNE = 1000.0;
	inline constexpr double KG_PER_SOLAR_MASS = 1.98847e30;
	inline constexpr double KG_PER_EARTH_MASS = 5.9722e24;
	inline constexpr double KG_PER_JUPITER_MASS = 1.89813e27;

	inline constexpr double SPEED_OF_LIGHT_MPS = 299792458.0;
	inline constexpr double SECONDS_PER_JULIAN_YEAR = 365.25 * 86400.0;
}

[[nodiscard]] inline const char* distance_unit_suffix(DistanceUnit unit) noexcept {
	switch (unit) {
		case DistanceUnit::Meters: return "m";
		case DistanceUnit::Kilometers: return "km";
		case DistanceUnit::Feet: return "ft";
		case DistanceUnit::Miles: return "mi";
		case DistanceUnit::NauticalMiles: return "nmi";
		case DistanceUnit::AstronomicalUnits: return "AU";
		case DistanceUnit::LightYears: return "ly";
		case DistanceUnit::Parsecs: return "pc";
		case DistanceUnit::Kiloparsecs: return "kpc";
		case DistanceUnit::SolarRadii: return "R_sun";
		default: return "";
	}
}

[[nodiscard]] inline double convert_distance_from_meters(double meters, DistanceUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case DistanceUnit::Meters: return meters;
		case DistanceUnit::Kilometers: return meters / METERS_PER_KM;
		case DistanceUnit::Feet: return meters / METERS_PER_FOOT;
		case DistanceUnit::Miles: return meters / METERS_PER_MILE;
		case DistanceUnit::NauticalMiles: return meters / METERS_PER_NAUTICAL_MILE;
		case DistanceUnit::AstronomicalUnits: return meters / METERS_PER_AU;
		case DistanceUnit::LightYears: return meters / METERS_PER_LIGHT_YEAR;
		case DistanceUnit::Parsecs: return meters / METERS_PER_PARSEC;
		case DistanceUnit::Kiloparsecs: return meters / (METERS_PER_PARSEC * 1000.0);
		case DistanceUnit::SolarRadii: return meters / METERS_PER_SOLAR_RADIUS;
		default: return meters;
	}
}

[[nodiscard]] inline double convert_distance_to_meters(double value, DistanceUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case DistanceUnit::Meters: return value;
		case DistanceUnit::Kilometers: return value * METERS_PER_KM;
		case DistanceUnit::Feet: return value * METERS_PER_FOOT;
		case DistanceUnit::Miles: return value * METERS_PER_MILE;
		case DistanceUnit::NauticalMiles: return value * METERS_PER_NAUTICAL_MILE;
		case DistanceUnit::AstronomicalUnits: return value * METERS_PER_AU;
		case DistanceUnit::LightYears: return value * METERS_PER_LIGHT_YEAR;
		case DistanceUnit::Parsecs: return value * METERS_PER_PARSEC;
		case DistanceUnit::Kiloparsecs: return value * METERS_PER_PARSEC * 1000.0;
		case DistanceUnit::SolarRadii: return value * METERS_PER_SOLAR_RADIUS;
		default: return value;
	}
}

[[nodiscard]] inline const char* mass_unit_suffix(MassUnit unit) noexcept {
	switch (unit) {
		case MassUnit::Kilograms: return "kg";
		case MassUnit::Grams: return "g";
		case MassUnit::Pounds: return "lb";
		case MassUnit::MetricTonnes: return "t";
		case MassUnit::SolarMasses: return "M_sun";
		case MassUnit::EarthMasses: return "M_earth";
		case MassUnit::JupiterMasses: return "M_jup";
		default: return "";
	}
}

[[nodiscard]] inline double convert_mass_from_kg(double kilograms, MassUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case MassUnit::Kilograms: return kilograms;
		case MassUnit::Grams: return kilograms / KG_PER_GRAM;
		case MassUnit::Pounds: return kilograms / KG_PER_POUND;
		case MassUnit::MetricTonnes: return kilograms / KG_PER_TONNE;
		case MassUnit::SolarMasses: return kilograms / KG_PER_SOLAR_MASS;
		case MassUnit::EarthMasses: return kilograms / KG_PER_EARTH_MASS;
		case MassUnit::JupiterMasses: return kilograms / KG_PER_JUPITER_MASS;
		default: return kilograms;
	}
}

[[nodiscard]] inline double convert_mass_to_kg(double value, MassUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case MassUnit::Kilograms: return value;
		case MassUnit::Grams: return value * KG_PER_GRAM;
		case MassUnit::Pounds: return value * KG_PER_POUND;
		case MassUnit::MetricTonnes: return value * KG_PER_TONNE;
		case MassUnit::SolarMasses: return value * KG_PER_SOLAR_MASS;
		case MassUnit::EarthMasses: return value * KG_PER_EARTH_MASS;
		case MassUnit::JupiterMasses: return value * KG_PER_JUPITER_MASS;
		default: return value;
	}
}

[[nodiscard]] inline const char* velocity_unit_suffix(VelocityUnit unit) noexcept {
	switch (unit) {
		case VelocityUnit::MetersPerSecond: return "m/s";
		case VelocityUnit::KilometersPerHour: return "km/h";
		case VelocityUnit::MilesPerHour: return "mph";
		case VelocityUnit::KilometersPerSecond: return "km/s";
		case VelocityUnit::FractionOfC: return "c";
		case VelocityUnit::ParsecsPerYear: return "pc/yr";
		case VelocityUnit::AstronomicalUnitsPerDay: return "AU/day";
		default: return "";
	}
}

[[nodiscard]] inline double convert_velocity_from_mps(double meters_per_second, VelocityUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case VelocityUnit::MetersPerSecond: return meters_per_second;
		case VelocityUnit::KilometersPerHour: return meters_per_second * 3.6;
		case VelocityUnit::MilesPerHour: return meters_per_second / METERS_PER_MILE * 3600.0;
		case VelocityUnit::KilometersPerSecond: return meters_per_second / METERS_PER_KM;
		case VelocityUnit::FractionOfC: return meters_per_second / SPEED_OF_LIGHT_MPS;
		case VelocityUnit::ParsecsPerYear: return (meters_per_second * SECONDS_PER_JULIAN_YEAR) / METERS_PER_PARSEC;
		case VelocityUnit::AstronomicalUnitsPerDay: return (meters_per_second * 86400.0) / METERS_PER_AU;
		default: return meters_per_second;
	}
}

[[nodiscard]] inline std::string format_distance(double meters, DistanceUnit unit, int precision = 3) noexcept {
	std::ostringstream ss;
	ss << std::fixed << std::setprecision(std::clamp(precision, 0, 8));
	ss << convert_distance_from_meters(meters, unit) << " " << distance_unit_suffix(unit);
	return ss.str();
}

[[nodiscard]] inline std::string format_mass(double kilograms, MassUnit unit, int precision = 3) noexcept {
	std::ostringstream ss;
	ss << std::fixed << std::setprecision(std::clamp(precision, 0, 8));
	ss << convert_mass_from_kg(kilograms, unit) << " " << mass_unit_suffix(unit);
	return ss.str();
}

[[nodiscard]] inline std::string format_velocity(double meters_per_second, VelocityUnit unit, int precision = 3) noexcept {
	std::ostringstream ss;
	ss << std::fixed << std::setprecision(std::clamp(precision, 0, 8));
	ss << convert_velocity_from_mps(meters_per_second, unit) << " " << velocity_unit_suffix(unit);
	return ss.str();
}

}
