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

enum class TimeUnit : uint32_t {
	Seconds = 0,
	Milliseconds = 1,
	Microseconds = 2,
	Nanoseconds = 3,
	Minutes = 4,
	Hours = 5,
	Days = 6,
	Years = 7
};

enum class AccelerationUnit : uint32_t {
	MetersPerSecondSquared = 0,
	CentimetersPerSecondSquared = 1,
	FeetPerSecondSquared = 2,
	StandardGravity = 3,
	KilometersPerSecondSquared = 4
};

enum class AngularVelocityUnit : uint32_t {
	RadiansPerSecond = 0,
	DegreesPerSecond = 1,
	RevolutionsPerMinute = 2,
	Hertz = 3
};

enum class DensityUnit : uint32_t {
	KilogramsPerCubicMeter = 0,
	GramsPerCubicCentimeter = 1,
	PoundsPerCubicFoot = 2,
	SolarMassesPerCubicParsec = 3
};

enum class PressureUnit : uint32_t {
	Pascals = 0,
	Kilopascals = 1,
	Megapascals = 2,
	Gigapascals = 3,
	Bars = 4,
	Atmospheres = 5,
	PSI = 6
};

enum class PowerUnit : uint32_t {
	Watts = 0,
	Kilowatts = 1,
	Megawatts = 2,
	SolarLuminosities = 3,
	Horsepower = 4
};

enum class FrequencyUnit : uint32_t {
	Hertz = 0,
	Kilohertz = 1,
	Megahertz = 2,
	Gigahertz = 3
};

enum class ForceUnit : uint32_t {
	Newtons = 0,
	Kilonewtons = 1,
	Dynes = 2,
	PoundsForce = 3
};

enum class MagneticFieldUnit : uint32_t {
	Teslas = 0,
	Gauss = 1,
	Microteslas = 2
};

enum class VoltageUnit : uint32_t {
	Volts = 0,
	Millivolts = 1,
	Kilovolts = 2,
	Megavolts = 3
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
	TimeUnit time{TimeUnit::Seconds};
	AccelerationUnit acceleration{AccelerationUnit::MetersPerSecondSquared};
	AngularVelocityUnit angular_velocity{AngularVelocityUnit::RadiansPerSecond};
	DensityUnit density{DensityUnit::KilogramsPerCubicMeter};
	PressureUnit pressure{PressureUnit::Pascals};
	PowerUnit power{PowerUnit::Watts};
	FrequencyUnit frequency{FrequencyUnit::Hertz};
	ForceUnit force{ForceUnit::Newtons};
	MagneticFieldUnit magnetic_field{MagneticFieldUnit::Teslas};
	VoltageUnit voltage{VoltageUnit::Volts};
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

	inline constexpr double STANDARD_GRAVITY_MPS2 = 9.80665;
	inline constexpr double WATTS_PER_SOLAR_LUMINOSITY = 3.828e26;
	inline constexpr double WATTS_PER_HORSEPOWER = 745.6998715822702;
	inline constexpr double PASCALS_PER_BAR = 1.0e5;
	inline constexpr double PASCALS_PER_ATM = 101325.0;
	inline constexpr double PASCALS_PER_PSI = 6894.757293168361;
	inline constexpr double NEWTONS_PER_POUND_FORCE = 4.4482216152605;
	inline constexpr double TESLAS_PER_GAUSS = 1.0e-4;
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

[[nodiscard]] inline double convert_velocity_to_mps(double value, VelocityUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case VelocityUnit::MetersPerSecond: return value;
		case VelocityUnit::KilometersPerHour: return value / 3.6;
		case VelocityUnit::MilesPerHour: return value * METERS_PER_MILE / 3600.0;
		case VelocityUnit::KilometersPerSecond: return value * METERS_PER_KM;
		case VelocityUnit::FractionOfC: return value * SPEED_OF_LIGHT_MPS;
		case VelocityUnit::ParsecsPerYear: return (value * METERS_PER_PARSEC) / SECONDS_PER_JULIAN_YEAR;
		case VelocityUnit::AstronomicalUnitsPerDay: return (value * METERS_PER_AU) / 86400.0;
		default: return value;
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

[[nodiscard]] inline const char* time_unit_suffix(TimeUnit unit) noexcept {
	switch (unit) {
		case TimeUnit::Seconds: return "s";
		case TimeUnit::Milliseconds: return "ms";
		case TimeUnit::Microseconds: return "us";
		case TimeUnit::Nanoseconds: return "ns";
		case TimeUnit::Minutes: return "min";
		case TimeUnit::Hours: return "h";
		case TimeUnit::Days: return "d";
		case TimeUnit::Years: return "yr";
		default: return "";
	}
}

[[nodiscard]] inline double convert_time_from_seconds(double seconds, TimeUnit unit) noexcept {
	switch (unit) {
		case TimeUnit::Seconds: return seconds;
		case TimeUnit::Milliseconds: return seconds * 1.0e3;
		case TimeUnit::Microseconds: return seconds * 1.0e6;
		case TimeUnit::Nanoseconds: return seconds * 1.0e9;
		case TimeUnit::Minutes: return seconds / 60.0;
		case TimeUnit::Hours: return seconds / 3600.0;
		case TimeUnit::Days: return seconds / 86400.0;
		case TimeUnit::Years: return seconds / Detail::SECONDS_PER_JULIAN_YEAR;
		default: return seconds;
	}
}

[[nodiscard]] inline double convert_time_to_seconds(double value, TimeUnit unit) noexcept {
	switch (unit) {
		case TimeUnit::Seconds: return value;
		case TimeUnit::Milliseconds: return value * 1.0e-3;
		case TimeUnit::Microseconds: return value * 1.0e-6;
		case TimeUnit::Nanoseconds: return value * 1.0e-9;
		case TimeUnit::Minutes: return value * 60.0;
		case TimeUnit::Hours: return value * 3600.0;
		case TimeUnit::Days: return value * 86400.0;
		case TimeUnit::Years: return value * Detail::SECONDS_PER_JULIAN_YEAR;
		default: return value;
	}
}

[[nodiscard]] inline std::string format_time(double seconds, TimeUnit unit, int precision = 3) noexcept {
	std::ostringstream ss;
	ss << std::fixed << std::setprecision(std::clamp(precision, 0, 8));
	ss << convert_time_from_seconds(seconds, unit) << " " << time_unit_suffix(unit);
	return ss.str();
}

[[nodiscard]] inline const char* acceleration_unit_suffix(AccelerationUnit unit) noexcept {
	switch (unit) {
		case AccelerationUnit::MetersPerSecondSquared: return "m/s^2";
		case AccelerationUnit::CentimetersPerSecondSquared: return "cm/s^2";
		case AccelerationUnit::FeetPerSecondSquared: return "ft/s^2";
		case AccelerationUnit::StandardGravity: return "g";
		case AccelerationUnit::KilometersPerSecondSquared: return "km/s^2";
		default: return "";
	}
}

[[nodiscard]] inline double convert_acceleration_from_mps2(double mps2, AccelerationUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case AccelerationUnit::MetersPerSecondSquared: return mps2;
		case AccelerationUnit::CentimetersPerSecondSquared: return mps2 * 100.0;
		case AccelerationUnit::FeetPerSecondSquared: return mps2 / METERS_PER_FOOT;
		case AccelerationUnit::StandardGravity: return mps2 / STANDARD_GRAVITY_MPS2;
		case AccelerationUnit::KilometersPerSecondSquared: return mps2 / 1000.0;
		default: return mps2;
	}
}

[[nodiscard]] inline double convert_acceleration_to_mps2(double value, AccelerationUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case AccelerationUnit::MetersPerSecondSquared: return value;
		case AccelerationUnit::CentimetersPerSecondSquared: return value / 100.0;
		case AccelerationUnit::FeetPerSecondSquared: return value * METERS_PER_FOOT;
		case AccelerationUnit::StandardGravity: return value * STANDARD_GRAVITY_MPS2;
		case AccelerationUnit::KilometersPerSecondSquared: return value * 1000.0;
		default: return value;
	}
}

[[nodiscard]] inline std::string format_acceleration(double mps2, AccelerationUnit unit, int precision = 3) noexcept {
	std::ostringstream ss;
	ss << std::fixed << std::setprecision(std::clamp(precision, 0, 8));
	ss << convert_acceleration_from_mps2(mps2, unit) << " " << acceleration_unit_suffix(unit);
	return ss.str();
}

[[nodiscard]] inline const char* angular_velocity_unit_suffix(AngularVelocityUnit unit) noexcept {
	switch (unit) {
		case AngularVelocityUnit::RadiansPerSecond: return "rad/s";
		case AngularVelocityUnit::DegreesPerSecond: return "deg/s";
		case AngularVelocityUnit::RevolutionsPerMinute: return "rpm";
		case AngularVelocityUnit::Hertz: return "Hz";
		default: return "";
	}
}

[[nodiscard]] inline double convert_angular_velocity_from_radps(double radps, AngularVelocityUnit unit) noexcept {
	constexpr double pi = 3.14159265358979323846;
	switch (unit) {
		case AngularVelocityUnit::RadiansPerSecond: return radps;
		case AngularVelocityUnit::DegreesPerSecond: return radps * (180.0 / pi);
		case AngularVelocityUnit::RevolutionsPerMinute: return radps * (60.0 / (2.0 * pi));
		case AngularVelocityUnit::Hertz: return radps / (2.0 * pi);
		default: return radps;
	}
}

[[nodiscard]] inline double convert_angular_velocity_to_radps(double value, AngularVelocityUnit unit) noexcept {
	constexpr double pi = 3.14159265358979323846;
	switch (unit) {
		case AngularVelocityUnit::RadiansPerSecond: return value;
		case AngularVelocityUnit::DegreesPerSecond: return value * (pi / 180.0);
		case AngularVelocityUnit::RevolutionsPerMinute: return value * ((2.0 * pi) / 60.0);
		case AngularVelocityUnit::Hertz: return value * (2.0 * pi);
		default: return value;
	}
}

[[nodiscard]] inline std::string format_angular_velocity(double radps, AngularVelocityUnit unit, int precision = 3) noexcept {
	std::ostringstream ss;
	ss << std::fixed << std::setprecision(std::clamp(precision, 0, 8));
	ss << convert_angular_velocity_from_radps(radps, unit) << " " << angular_velocity_unit_suffix(unit);
	return ss.str();
}

[[nodiscard]] inline const char* density_unit_suffix(DensityUnit unit) noexcept {
	switch (unit) {
		case DensityUnit::KilogramsPerCubicMeter: return "kg/m^3";
		case DensityUnit::GramsPerCubicCentimeter: return "g/cm^3";
		case DensityUnit::PoundsPerCubicFoot: return "lb/ft^3";
		case DensityUnit::SolarMassesPerCubicParsec: return "M_sun/pc^3";
		default: return "";
	}
}

[[nodiscard]] inline double convert_density_from_kg_m3(double kg_m3, DensityUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case DensityUnit::KilogramsPerCubicMeter: return kg_m3;
		case DensityUnit::GramsPerCubicCentimeter: return kg_m3 * 0.001;
		case DensityUnit::PoundsPerCubicFoot: return kg_m3 / (KG_PER_POUND / (METERS_PER_FOOT * METERS_PER_FOOT * METERS_PER_FOOT));
		case DensityUnit::SolarMassesPerCubicParsec: return kg_m3 / (KG_PER_SOLAR_MASS / (METERS_PER_PARSEC * METERS_PER_PARSEC * METERS_PER_PARSEC));
		default: return kg_m3;
	}
}

[[nodiscard]] inline double convert_density_to_kg_m3(double value, DensityUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case DensityUnit::KilogramsPerCubicMeter: return value;
		case DensityUnit::GramsPerCubicCentimeter: return value * 1000.0;
		case DensityUnit::PoundsPerCubicFoot: return value * (KG_PER_POUND / (METERS_PER_FOOT * METERS_PER_FOOT * METERS_PER_FOOT));
		case DensityUnit::SolarMassesPerCubicParsec: return value * (KG_PER_SOLAR_MASS / (METERS_PER_PARSEC * METERS_PER_PARSEC * METERS_PER_PARSEC));
		default: return value;
	}
}

[[nodiscard]] inline std::string format_density(double kg_m3, DensityUnit unit, int precision = 3) noexcept {
	std::ostringstream ss;
	ss << std::fixed << std::setprecision(std::clamp(precision, 0, 8));
	ss << convert_density_from_kg_m3(kg_m3, unit) << " " << density_unit_suffix(unit);
	return ss.str();
}

[[nodiscard]] inline const char* pressure_unit_suffix(PressureUnit unit) noexcept {
	switch (unit) {
		case PressureUnit::Pascals: return "Pa";
		case PressureUnit::Kilopascals: return "kPa";
		case PressureUnit::Megapascals: return "MPa";
		case PressureUnit::Gigapascals: return "GPa";
		case PressureUnit::Bars: return "bar";
		case PressureUnit::Atmospheres: return "atm";
		case PressureUnit::PSI: return "psi";
		default: return "";
	}
}

[[nodiscard]] inline double convert_pressure_from_pascals(double pascals, PressureUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case PressureUnit::Pascals: return pascals;
		case PressureUnit::Kilopascals: return pascals / 1.0e3;
		case PressureUnit::Megapascals: return pascals / 1.0e6;
		case PressureUnit::Gigapascals: return pascals / 1.0e9;
		case PressureUnit::Bars: return pascals / PASCALS_PER_BAR;
		case PressureUnit::Atmospheres: return pascals / PASCALS_PER_ATM;
		case PressureUnit::PSI: return pascals / PASCALS_PER_PSI;
		default: return pascals;
	}
}

[[nodiscard]] inline double convert_pressure_to_pascals(double value, PressureUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case PressureUnit::Pascals: return value;
		case PressureUnit::Kilopascals: return value * 1.0e3;
		case PressureUnit::Megapascals: return value * 1.0e6;
		case PressureUnit::Gigapascals: return value * 1.0e9;
		case PressureUnit::Bars: return value * PASCALS_PER_BAR;
		case PressureUnit::Atmospheres: return value * PASCALS_PER_ATM;
		case PressureUnit::PSI: return value * PASCALS_PER_PSI;
		default: return value;
	}
}

[[nodiscard]] inline std::string format_pressure(double pascals, PressureUnit unit, int precision = 3) noexcept {
	std::ostringstream ss;
	ss << std::scientific << std::setprecision(std::clamp(precision, 0, 8));
	ss << convert_pressure_from_pascals(pascals, unit) << " " << pressure_unit_suffix(unit);
	return ss.str();
}

[[nodiscard]] inline const char* power_unit_suffix(PowerUnit unit) noexcept {
	switch (unit) {
		case PowerUnit::Watts: return "W";
		case PowerUnit::Kilowatts: return "kW";
		case PowerUnit::Megawatts: return "MW";
		case PowerUnit::SolarLuminosities: return "L_sun";
		case PowerUnit::Horsepower: return "hp";
		default: return "";
	}
}

[[nodiscard]] inline double convert_power_from_watts(double watts, PowerUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case PowerUnit::Watts: return watts;
		case PowerUnit::Kilowatts: return watts / 1.0e3;
		case PowerUnit::Megawatts: return watts / 1.0e6;
		case PowerUnit::SolarLuminosities: return watts / WATTS_PER_SOLAR_LUMINOSITY;
		case PowerUnit::Horsepower: return watts / WATTS_PER_HORSEPOWER;
		default: return watts;
	}
}

[[nodiscard]] inline double convert_power_to_watts(double value, PowerUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case PowerUnit::Watts: return value;
		case PowerUnit::Kilowatts: return value * 1.0e3;
		case PowerUnit::Megawatts: return value * 1.0e6;
		case PowerUnit::SolarLuminosities: return value * WATTS_PER_SOLAR_LUMINOSITY;
		case PowerUnit::Horsepower: return value * WATTS_PER_HORSEPOWER;
		default: return value;
	}
}

[[nodiscard]] inline std::string format_power(double watts, PowerUnit unit, int precision = 3) noexcept {
	std::ostringstream ss;
	ss << std::scientific << std::setprecision(std::clamp(precision, 0, 8));
	ss << convert_power_from_watts(watts, unit) << " " << power_unit_suffix(unit);
	return ss.str();
}

[[nodiscard]] inline const char* frequency_unit_suffix(FrequencyUnit unit) noexcept {
	switch (unit) {
		case FrequencyUnit::Hertz: return "Hz";
		case FrequencyUnit::Kilohertz: return "kHz";
		case FrequencyUnit::Megahertz: return "MHz";
		case FrequencyUnit::Gigahertz: return "GHz";
		default: return "";
	}
}

[[nodiscard]] inline double convert_frequency_from_hertz(double hertz, FrequencyUnit unit) noexcept {
	switch (unit) {
		case FrequencyUnit::Hertz: return hertz;
		case FrequencyUnit::Kilohertz: return hertz / 1.0e3;
		case FrequencyUnit::Megahertz: return hertz / 1.0e6;
		case FrequencyUnit::Gigahertz: return hertz / 1.0e9;
		default: return hertz;
	}
}

[[nodiscard]] inline double convert_frequency_to_hertz(double value, FrequencyUnit unit) noexcept {
	switch (unit) {
		case FrequencyUnit::Hertz: return value;
		case FrequencyUnit::Kilohertz: return value * 1.0e3;
		case FrequencyUnit::Megahertz: return value * 1.0e6;
		case FrequencyUnit::Gigahertz: return value * 1.0e9;
		default: return value;
	}
}

[[nodiscard]] inline std::string format_frequency(double hertz, FrequencyUnit unit, int precision = 3) noexcept {
	std::ostringstream ss;
	ss << std::fixed << std::setprecision(std::clamp(precision, 0, 8));
	ss << convert_frequency_from_hertz(hertz, unit) << " " << frequency_unit_suffix(unit);
	return ss.str();
}

[[nodiscard]] inline const char* force_unit_suffix(ForceUnit unit) noexcept {
	switch (unit) {
		case ForceUnit::Newtons: return "N";
		case ForceUnit::Kilonewtons: return "kN";
		case ForceUnit::Dynes: return "dyn";
		case ForceUnit::PoundsForce: return "lbf";
		default: return "";
	}
}

[[nodiscard]] inline double convert_force_from_newtons(double newtons, ForceUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case ForceUnit::Newtons: return newtons;
		case ForceUnit::Kilonewtons: return newtons / 1.0e3;
		case ForceUnit::Dynes: return newtons * 1.0e5;
		case ForceUnit::PoundsForce: return newtons / NEWTONS_PER_POUND_FORCE;
		default: return newtons;
	}
}

[[nodiscard]] inline double convert_force_to_newtons(double value, ForceUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case ForceUnit::Newtons: return value;
		case ForceUnit::Kilonewtons: return value * 1.0e3;
		case ForceUnit::Dynes: return value * 1.0e-5;
		case ForceUnit::PoundsForce: return value * NEWTONS_PER_POUND_FORCE;
		default: return value;
	}
}

[[nodiscard]] inline std::string format_force(double newtons, ForceUnit unit, int precision = 3) noexcept {
	std::ostringstream ss;
	ss << std::scientific << std::setprecision(std::clamp(precision, 0, 8));
	ss << convert_force_from_newtons(newtons, unit) << " " << force_unit_suffix(unit);
	return ss.str();
}

[[nodiscard]] inline const char* magnetic_field_unit_suffix(MagneticFieldUnit unit) noexcept {
	switch (unit) {
		case MagneticFieldUnit::Teslas: return "T";
		case MagneticFieldUnit::Gauss: return "G";
		case MagneticFieldUnit::Microteslas: return "uT";
		default: return "";
	}
}

[[nodiscard]] inline double convert_magnetic_field_from_teslas(double teslas, MagneticFieldUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case MagneticFieldUnit::Teslas: return teslas;
		case MagneticFieldUnit::Gauss: return teslas / TESLAS_PER_GAUSS;
		case MagneticFieldUnit::Microteslas: return teslas * 1.0e6;
		default: return teslas;
	}
}

[[nodiscard]] inline double convert_magnetic_field_to_teslas(double value, MagneticFieldUnit unit) noexcept {
	using namespace Detail;
	switch (unit) {
		case MagneticFieldUnit::Teslas: return value;
		case MagneticFieldUnit::Gauss: return value * TESLAS_PER_GAUSS;
		case MagneticFieldUnit::Microteslas: return value * 1.0e-6;
		default: return value;
	}
}

[[nodiscard]] inline std::string format_magnetic_field(double teslas, MagneticFieldUnit unit, int precision = 4) noexcept {
	std::ostringstream ss;
	ss << std::scientific << std::setprecision(std::clamp(precision, 0, 8));
	ss << convert_magnetic_field_from_teslas(teslas, unit) << " " << magnetic_field_unit_suffix(unit);
	return ss.str();
}

[[nodiscard]] inline const char* voltage_unit_suffix(VoltageUnit unit) noexcept {
	switch (unit) {
		case VoltageUnit::Volts: return "V";
		case VoltageUnit::Millivolts: return "mV";
		case VoltageUnit::Kilovolts: return "kV";
		case VoltageUnit::Megavolts: return "MV";
		default: return "";
	}
}

[[nodiscard]] inline double convert_voltage_from_volts(double volts, VoltageUnit unit) noexcept {
	switch (unit) {
		case VoltageUnit::Volts: return volts;
		case VoltageUnit::Millivolts: return volts * 1.0e3;
		case VoltageUnit::Kilovolts: return volts / 1.0e3;
		case VoltageUnit::Megavolts: return volts / 1.0e6;
		default: return volts;
	}
}

[[nodiscard]] inline double convert_voltage_to_volts(double value, VoltageUnit unit) noexcept {
	switch (unit) {
		case VoltageUnit::Volts: return value;
		case VoltageUnit::Millivolts: return value * 1.0e-3;
		case VoltageUnit::Kilovolts: return value * 1.0e3;
		case VoltageUnit::Megavolts: return value * 1.0e6;
		default: return value;
	}
}

[[nodiscard]] inline std::string format_voltage(double volts, VoltageUnit unit, int precision = 3) noexcept {
	std::ostringstream ss;
	ss << std::scientific << std::setprecision(std::clamp(precision, 0, 8));
	ss << convert_voltage_from_volts(volts, unit) << " " << voltage_unit_suffix(unit);
	return ss.str();
}

}
