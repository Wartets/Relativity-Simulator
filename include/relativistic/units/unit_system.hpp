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
	JupiterMasses = 6,
	Turkeys = 7
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

enum class EnergyUnit : uint32_t {
	Joules = 0,
	Kilojoules = 1,
	Megajoules = 2,
	ElectronVolts = 3,
	KilowattHours = 4,
	Ergs = 5,
	FootPounds = 6,
	Calories = 7
};

enum class AngleUnit : uint32_t {
	Radians = 0,
	Degrees = 1,
	Arcminutes = 2,
	Arcseconds = 3,
	Gradians = 4,
	Revolutions = 5,
	Milliradians = 6
};

enum class TemperatureUnit : uint32_t {
	Kelvin = 0,
	Celsius = 1,
	Fahrenheit = 2,
	Rankine = 3
};

enum class ChargeUnit : uint32_t {
	Coulombs = 0,
	Millicoulombs = 1,
	Microcoulombs = 2,
	ElementaryCharges = 3,
	AmpereHours = 4,
	Statcoulombs = 5
};

enum class CurrentUnit : uint32_t {
	Amperes = 0,
	Milliamperes = 1,
	Microamperes = 2,
	Kiloamperes = 3
};

enum class FrameRateUnit : uint32_t {
	FramesPerSecond = 0,
	Milliseconds = 1,
	Microseconds = 2,
	Hertz = 3
};

struct UnitDisplayPreferences {
	DistanceUnit distance{DistanceUnit::Meters};
	MassUnit mass{MassUnit::Kilograms};
	VelocityUnit velocity{VelocityUnit::MetersPerSecond};
	EnergyUnit energy{EnergyUnit::Joules};
	AngleUnit angle{AngleUnit::Radians};
	TemperatureUnit temperature{TemperatureUnit::Kelvin};
	ChargeUnit charge{ChargeUnit::Coulombs};
	CurrentUnit current{CurrentUnit::Amperes};
	FrameRateUnit frame_rate{FrameRateUnit::FramesPerSecond};
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
	inline constexpr double KG_PER_TURKEY = 11.0;

	inline constexpr double SPEED_OF_LIGHT_MPS = 299792458.0;
	inline constexpr double SECONDS_PER_JULIAN_YEAR = 365.25 * 86400.0;

	inline constexpr double JOULES_PER_KILOJOULE = 1.0e3;
	inline constexpr double JOULES_PER_MEGAJOULE = 1.0e6;
	inline constexpr double JOULES_PER_ELECTRONVOLT = 1.602176634e-19;
	inline constexpr double JOULES_PER_KILOWATT_HOUR = 3.6e6;
	inline constexpr double JOULES_PER_ERG = 1.0e-7;
	inline constexpr double JOULES_PER_FOOT_POUND = 1.3558179483314004;
	inline constexpr double JOULES_PER_CALORIE = 4.184;

	inline constexpr double COULOMBS_PER_ELEMENTARY_CHARGE = 1.602176634e-19;
	inline constexpr double COULOMBS_PER_AMPERE_HOUR = 3600.0;
	inline constexpr double COULOMBS_PER_STATCOULOMB = 3.335641e-10;

	inline constexpr double AMPERES_PER_MILLIAMPERE = 1.0e-3;
	inline constexpr double AMPERES_PER_MICROAMPERE = 1.0e-6;
	inline constexpr double AMPERES_PER_KILOAMPERE = 1.0e3;
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
		case MassUnit::Turkeys: return "turkeys";
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
		case MassUnit::Turkeys: return kilograms / KG_PER_TURKEY;
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
		case MassUnit::Turkeys: return value * KG_PER_TURKEY;
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

[[nodiscard]] inline const char* energy_unit_suffix(EnergyUnit unit) noexcept {
	switch (unit) {
		case EnergyUnit::Joules: return "J";
		case EnergyUnit::Kilojoules: return "kJ";
		case EnergyUnit::Megajoules: return "MJ";
		case EnergyUnit::ElectronVolts: return "eV";
		case EnergyUnit::KilowattHours: return "kWh";
		case EnergyUnit::Ergs: return "erg";
		case EnergyUnit::FootPounds: return "ft*lb";
		case EnergyUnit::Calories: return "cal";
		default: return "";
	}
}

[[nodiscard]] inline double convert_energy_from_joules(double joules, EnergyUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case EnergyUnit::Joules: return joules;
		case EnergyUnit::Kilojoules: return joules / JOULES_PER_KILOJOULE;
		case EnergyUnit::Megajoules: return joules / JOULES_PER_MEGAJOULE;
		case EnergyUnit::ElectronVolts: return joules / JOULES_PER_ELECTRONVOLT;
		case EnergyUnit::KilowattHours: return joules / JOULES_PER_KILOWATT_HOUR;
		case EnergyUnit::Ergs: return joules / JOULES_PER_ERG;
		case EnergyUnit::FootPounds: return joules / JOULES_PER_FOOT_POUND;
		case EnergyUnit::Calories: return joules / JOULES_PER_CALORIE;
		default: return joules;
	}
}

[[nodiscard]] inline double convert_energy_to_joules(double value, EnergyUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case EnergyUnit::Joules: return value;
		case EnergyUnit::Kilojoules: return value * JOULES_PER_KILOJOULE;
		case EnergyUnit::Megajoules: return value * JOULES_PER_MEGAJOULE;
		case EnergyUnit::ElectronVolts: return value * JOULES_PER_ELECTRONVOLT;
		case EnergyUnit::KilowattHours: return value * JOULES_PER_KILOWATT_HOUR;
		case EnergyUnit::Ergs: return value * JOULES_PER_ERG;
		case EnergyUnit::FootPounds: return value * JOULES_PER_FOOT_POUND;
		case EnergyUnit::Calories: return value * JOULES_PER_CALORIE;
		default: return value;
	}
}

[[nodiscard]] inline const char* angle_unit_suffix(AngleUnit unit) noexcept {
	switch (unit) {
		case AngleUnit::Radians: return "rad";
		case AngleUnit::Degrees: return "deg";
		case AngleUnit::Arcminutes: return "arcmin";
		case AngleUnit::Arcseconds: return "arcsec";
		case AngleUnit::Gradians: return "gon";
		case AngleUnit::Revolutions: return "rev";
		case AngleUnit::Milliradians: return "mrad";
		default: return "";
	}
}

[[nodiscard]] inline double convert_angle_from_radians(double radians, AngleUnit unit) noexcept {
	constexpr double pi = 3.14159265358979323846;
	const double degrees = radians * (180.0 / pi);
	switch (unit) {
		case AngleUnit::Radians: return radians;
		case AngleUnit::Degrees: return degrees;
		case AngleUnit::Arcminutes: return degrees * 60.0;
		case AngleUnit::Arcseconds: return degrees * 3600.0;
		case AngleUnit::Gradians: return radians * (200.0 / pi);
		case AngleUnit::Revolutions: return radians / (2.0 * pi);
		case AngleUnit::Milliradians: return radians * 1000.0;
		default: return radians;
	}
}

[[nodiscard]] inline double convert_angle_to_radians(double value, AngleUnit unit) noexcept {
	constexpr double pi = 3.14159265358979323846;
	switch (unit) {
		case AngleUnit::Radians: return value;
		case AngleUnit::Degrees: return value * (pi / 180.0);
		case AngleUnit::Arcminutes: return value * (pi / (180.0 * 60.0));
		case AngleUnit::Arcseconds: return value * (pi / (180.0 * 3600.0));
		case AngleUnit::Gradians: return value * (pi / 200.0);
		case AngleUnit::Revolutions: return value * (2.0 * pi);
		case AngleUnit::Milliradians: return value / 1000.0;
		default: return value;
	}
}

[[nodiscard]] inline const char* temperature_unit_suffix(TemperatureUnit unit) noexcept {
	switch (unit) {
		case TemperatureUnit::Kelvin: return "K";
		case TemperatureUnit::Celsius: return "C";
		case TemperatureUnit::Fahrenheit: return "F";
		case TemperatureUnit::Rankine: return "R";
		default: return "";
	}
}

[[nodiscard]] inline double convert_temperature_from_kelvin(double kelvin, TemperatureUnit unit) noexcept {
	switch (unit) {
		case TemperatureUnit::Kelvin: return kelvin;
		case TemperatureUnit::Celsius: return kelvin - 273.15;
		case TemperatureUnit::Fahrenheit: return (kelvin - 273.15) * 1.8 + 32.0;
		case TemperatureUnit::Rankine: return kelvin * 1.8;
		default: return kelvin;
	}
}

[[nodiscard]] inline double convert_temperature_to_kelvin(double value, TemperatureUnit unit) noexcept {
	switch (unit) {
		case TemperatureUnit::Kelvin: return value;
		case TemperatureUnit::Celsius: return value + 273.15;
		case TemperatureUnit::Fahrenheit: return (value - 32.0) / 1.8 + 273.15;
		case TemperatureUnit::Rankine: return value / 1.8;
		default: return value;
	}
}

[[nodiscard]] inline const char* charge_unit_suffix(ChargeUnit unit) noexcept {
	switch (unit) {
		case ChargeUnit::Coulombs: return "C";
		case ChargeUnit::Millicoulombs: return "mC";
		case ChargeUnit::Microcoulombs: return "uC";
		case ChargeUnit::ElementaryCharges: return "e";
		case ChargeUnit::AmpereHours: return "Ah";
		case ChargeUnit::Statcoulombs: return "statC";
		default: return "";
	}
}

[[nodiscard]] inline double convert_charge_from_coulombs(double coulombs, ChargeUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case ChargeUnit::Coulombs: return coulombs;
		case ChargeUnit::Millicoulombs: return coulombs * 1.0e3;
		case ChargeUnit::Microcoulombs: return coulombs * 1.0e6;
		case ChargeUnit::ElementaryCharges: return coulombs / COULOMBS_PER_ELEMENTARY_CHARGE;
		case ChargeUnit::AmpereHours: return coulombs / COULOMBS_PER_AMPERE_HOUR;
		case ChargeUnit::Statcoulombs: return coulombs / COULOMBS_PER_STATCOULOMB;
		default: return coulombs;
	}
}

[[nodiscard]] inline double convert_charge_to_coulombs(double value, ChargeUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case ChargeUnit::Coulombs: return value;
		case ChargeUnit::Millicoulombs: return value * 1.0e-3;
		case ChargeUnit::Microcoulombs: return value * 1.0e-6;
		case ChargeUnit::ElementaryCharges: return value * COULOMBS_PER_ELEMENTARY_CHARGE;
		case ChargeUnit::AmpereHours: return value * COULOMBS_PER_AMPERE_HOUR;
		case ChargeUnit::Statcoulombs: return value * COULOMBS_PER_STATCOULOMB;
		default: return value;
	}
}

[[nodiscard]] inline const char* current_unit_suffix(CurrentUnit unit) noexcept {
	switch (unit) {
		case CurrentUnit::Amperes: return "A";
		case CurrentUnit::Milliamperes: return "mA";
		case CurrentUnit::Microamperes: return "uA";
		case CurrentUnit::Kiloamperes: return "kA";
		default: return "";
	}
}

[[nodiscard]] inline double convert_current_from_amperes(double amperes, CurrentUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case CurrentUnit::Amperes: return amperes;
		case CurrentUnit::Milliamperes: return amperes / AMPERES_PER_MILLIAMPERE;
		case CurrentUnit::Microamperes: return amperes / AMPERES_PER_MICROAMPERE;
		case CurrentUnit::Kiloamperes: return amperes / AMPERES_PER_KILOAMPERE;
		default: return amperes;
	}
}

[[nodiscard]] inline double convert_current_to_amperes(double value, CurrentUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case CurrentUnit::Amperes: return value;
		case CurrentUnit::Milliamperes: return value * AMPERES_PER_MILLIAMPERE;
		case CurrentUnit::Microamperes: return value * AMPERES_PER_MICROAMPERE;
		case CurrentUnit::Kiloamperes: return value * AMPERES_PER_KILOAMPERE;
		default: return value;
	}
}

[[nodiscard]] inline const char* frame_rate_unit_suffix(FrameRateUnit unit) noexcept {
	switch (unit) {
		case FrameRateUnit::Milliseconds: return "ms";
		case FrameRateUnit::Microseconds: return "us";
		case FrameRateUnit::Hertz: return "Hz";
		case FrameRateUnit::FramesPerSecond:
		default: return "FPS";
	}
}

[[nodiscard]] inline double convert_frame_time_ms_to_display(double frame_time_ms, FrameRateUnit unit) noexcept {
	switch (unit) {
		case FrameRateUnit::Milliseconds: return frame_time_ms;
		case FrameRateUnit::Microseconds: return frame_time_ms * 1000.0;
		case FrameRateUnit::Hertz:
		case FrameRateUnit::FramesPerSecond:
		default: return (frame_time_ms > 1e-9) ? (1000.0 / frame_time_ms) : 0.0;
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

[[nodiscard]] inline std::string format_energy(double joules, EnergyUnit unit, int precision = 4) noexcept {
	std::ostringstream ss;
	ss << std::scientific << std::setprecision(std::clamp(precision, 0, 10));
	ss << convert_energy_from_joules(joules, unit) << " " << energy_unit_suffix(unit);
	return ss.str();
}

[[nodiscard]] inline std::string format_angle(double radians, AngleUnit unit, int precision = 3) noexcept {
	std::ostringstream ss;
	ss << std::fixed << std::setprecision(std::clamp(precision, 0, 8));
	ss << convert_angle_from_radians(radians, unit) << " " << angle_unit_suffix(unit);
	return ss.str();
}

[[nodiscard]] inline std::string format_temperature(double kelvin, TemperatureUnit unit, int precision = 2) noexcept {
	std::ostringstream ss;
	ss << std::fixed << std::setprecision(std::clamp(precision, 0, 8));
	ss << convert_temperature_from_kelvin(kelvin, unit) << " " << temperature_unit_suffix(unit);
	return ss.str();
}

[[nodiscard]] inline std::string format_charge(double coulombs, ChargeUnit unit, int precision = 4) noexcept {
	std::ostringstream ss;
	ss << std::scientific << std::setprecision(std::clamp(precision, 0, 10));
	ss << convert_charge_from_coulombs(coulombs, unit) << " " << charge_unit_suffix(unit);
	return ss.str();
}

[[nodiscard]] inline std::string format_current(double amperes, CurrentUnit unit, int precision = 4) noexcept {
	std::ostringstream ss;
	ss << std::scientific << std::setprecision(std::clamp(precision, 0, 10));
	ss << convert_current_from_amperes(amperes, unit) << " " << current_unit_suffix(unit);
	return ss.str();
}

[[nodiscard]] inline std::string format_frame_time(double frame_time_ms, FrameRateUnit unit, int precision = 2) noexcept {
	std::ostringstream ss;
	ss << std::fixed << std::setprecision(std::clamp(precision, 0, 6));
	ss << convert_frame_time_ms_to_display(frame_time_ms, unit) << " " << frame_rate_unit_suffix(unit);
	return ss.str();
}

}
