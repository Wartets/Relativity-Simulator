#pragma once

#include <cmath>
#include <numbers>
#include <cstdint>
#include <algorithm>

namespace Relativistic::Core {

enum class ConstantsPreset : uint32_t {
	SI = 0,
	Planck = 1,
	Custom = 2
};

struct SIReferenceConstants {
	static constexpr double SPEED_OF_LIGHT = 299792458.0;
	static constexpr double PLANCK_CONSTANT = 6.62607015e-34;
	static constexpr double ELEMENTARY_CHARGE = 1.602176634e-19;
	static constexpr double BOLTZMANN_CONSTANT = 1.380649e-23;
	static constexpr double AVOGADRO_CONSTANT = 6.02214076e23;
	static constexpr double GRAVITATIONAL_CONSTANT = 6.67430e-11;
	static constexpr double FINE_STRUCTURE_CONSTANT = 7.2973525693e-3;
	static constexpr double LUMINOUS_EFFICACY_KCD = 683.0;

	static constexpr double SOLAR_MASS = 1.98847e30;
	static constexpr double ASTRONOMICAL_UNIT = 149597870700.0;
	static constexpr double ELECTRON_MASS = 9.1093837015e-31;
	static constexpr double PROTON_MASS = 1.67262192369e-27;
	static constexpr double NEUTRON_MASS = 1.67492749804e-27;

	[[nodiscard]] static constexpr double reduced_planck() noexcept {
		return PLANCK_CONSTANT / (2.0 * std::numbers::pi);
	}

	[[nodiscard]] static constexpr double coulomb_constant() noexcept {
		return (FINE_STRUCTURE_CONSTANT * reduced_planck() * SPEED_OF_LIGHT) / (ELEMENTARY_CHARGE * ELEMENTARY_CHARGE);
	}

	[[nodiscard]] static constexpr double vacuum_permittivity() noexcept {
		return 1.0 / (4.0 * std::numbers::pi * coulomb_constant());
	}

	[[nodiscard]] static constexpr double vacuum_permeability() noexcept {
		return (4.0 * std::numbers::pi * coulomb_constant()) / (SPEED_OF_LIGHT * SPEED_OF_LIGHT);
	}
};

struct SimBaseConstants {
	double c{SIReferenceConstants::SPEED_OF_LIGHT};
	double g{SIReferenceConstants::GRAVITATIONAL_CONSTANT};
	double h{SIReferenceConstants::PLANCK_CONSTANT};
	double kb{SIReferenceConstants::BOLTZMANN_CONSTANT};
	double na{SIReferenceConstants::AVOGADRO_CONSTANT};
	double ke{SIReferenceConstants::coulomb_constant()};
	double kcd{SIReferenceConstants::LUMINOUS_EFFICACY_KCD};
};

class ConstantsEngine {
private:
	SimBaseConstants sim_{};
	ConstantsPreset active_preset_{ConstantsPreset::SI};

	static double clamp_positive(double v) noexcept {
		return (v > 1e-300) ? v : 1e-300;
	}

	void mark_custom() noexcept {
		active_preset_ = ConstantsPreset::Custom;
	}

public:
	static constexpr double FINE_STRUCTURE_CONSTANT = SIReferenceConstants::FINE_STRUCTURE_CONSTANT;

	ConstantsEngine() noexcept = default;

	[[nodiscard]] constexpr ConstantsPreset active_preset() const noexcept { return active_preset_; }

	[[nodiscard]] constexpr double sim_speed_of_light() const noexcept { return sim_.c; }
	[[nodiscard]] constexpr double sim_gravitational_constant() const noexcept { return sim_.g; }
	[[nodiscard]] constexpr double sim_planck_constant() const noexcept { return sim_.h; }
	[[nodiscard]] constexpr double sim_boltzmann_constant() const noexcept { return sim_.kb; }
	[[nodiscard]] constexpr double sim_avogadro_constant() const noexcept { return sim_.na; }
	[[nodiscard]] constexpr double sim_coulomb_constant() const noexcept { return sim_.ke; }
	[[nodiscard]] constexpr double sim_luminous_efficacy() const noexcept { return sim_.kcd; }

	void set_speed_of_light(double v) noexcept { sim_.c = clamp_positive(v); mark_custom(); }
	void set_gravitational_constant(double v) noexcept { sim_.g = clamp_positive(v); mark_custom(); }
	void set_planck_constant(double v) noexcept { sim_.h = clamp_positive(v); mark_custom(); }
	void set_boltzmann_constant(double v) noexcept { sim_.kb = clamp_positive(v); mark_custom(); }
	void set_avogadro_constant(double v) noexcept { sim_.na = clamp_positive(v); mark_custom(); }
	void set_coulomb_constant(double v) noexcept { sim_.ke = clamp_positive(v); mark_custom(); }
	void set_luminous_efficacy(double v) noexcept { sim_.kcd = clamp_positive(v); mark_custom(); }

	void apply_preset(ConstantsPreset preset) noexcept {
		active_preset_ = preset;
		switch (preset) {
			case ConstantsPreset::SI:
				sim_.c = SIReferenceConstants::SPEED_OF_LIGHT;
				sim_.g = SIReferenceConstants::GRAVITATIONAL_CONSTANT;
				sim_.h = SIReferenceConstants::PLANCK_CONSTANT;
				sim_.kb = SIReferenceConstants::BOLTZMANN_CONSTANT;
				sim_.na = SIReferenceConstants::AVOGADRO_CONSTANT;
				sim_.ke = SIReferenceConstants::coulomb_constant();
				sim_.kcd = SIReferenceConstants::LUMINOUS_EFFICACY_KCD;
				break;
			case ConstantsPreset::Planck:
				sim_.c = 1.0;
				sim_.g = 1.0;
				sim_.h = 2.0 * std::numbers::pi;
				sim_.kb = 1.0;
				sim_.na = SIReferenceConstants::AVOGADRO_CONSTANT;
				sim_.ke = 1.0;
				sim_.kcd = 1.0;
				break;
			case ConstantsPreset::Custom:
			default:
				break;
		}
	}

	void apply_preset_by_index(uint32_t idx) noexcept {
		apply_preset(static_cast<ConstantsPreset>(std::min(idx, 2u)));
	}

	[[nodiscard]] double sim_reduced_planck() const noexcept {
		return sim_.h / (2.0 * std::numbers::pi);
	}

	[[nodiscard]] double sim_stefan_boltzmann() const noexcept {
		const double h3 = sim_.h * sim_.h * sim_.h;
		const double kb4 = sim_.kb * sim_.kb * sim_.kb * sim_.kb;
		const double pi5 = std::numbers::pi * std::numbers::pi * std::numbers::pi * std::numbers::pi * std::numbers::pi;
		return (2.0 * pi5 * kb4) / (15.0 * h3 * sim_.c * sim_.c);
	}

	[[nodiscard]] double sim_magnetic_coupling() const noexcept {
		return sim_.ke / (sim_.c * sim_.c);
	}

	[[nodiscard]] double sim_vacuum_permittivity() const noexcept {
		return 1.0 / (4.0 * std::numbers::pi * sim_.ke);
	}

	[[nodiscard]] double sim_vacuum_permeability() const noexcept {
		return (4.0 * std::numbers::pi * sim_.ke) / (sim_.c * sim_.c);
	}

	[[nodiscard]] double time_scale() const noexcept {
		const double ratio_g_h = (SIReferenceConstants::GRAVITATIONAL_CONSTANT / sim_.g) * (SIReferenceConstants::PLANCK_CONSTANT / sim_.h);
		const double c_ratio = SIReferenceConstants::SPEED_OF_LIGHT / sim_.c;
		return std::sqrt(ratio_g_h / std::pow(c_ratio, 5.0));
	}

	[[nodiscard]] double length_scale() const noexcept {
		return (SIReferenceConstants::SPEED_OF_LIGHT * time_scale()) / sim_.c;
	}

	[[nodiscard]] double mass_scale() const noexcept {
		const double t0 = time_scale();
		const double l0 = length_scale();
		return (sim_.g / SIReferenceConstants::GRAVITATIONAL_CONSTANT) * (l0 * l0 * l0) / (t0 * t0);
	}

	[[nodiscard]] double charge_scale() const noexcept {
		const double m0 = mass_scale();
		const double l0 = length_scale();
		const double t0 = time_scale();
		const double si_ke = SIReferenceConstants::coulomb_constant();
		const double inner = (sim_.ke / si_ke) * (m0 * l0 * l0 * l0) / (t0 * t0);
		return std::sqrt(std::max(inner, 0.0));
	}

	[[nodiscard]] double temperature_scale() const noexcept {
		const double m0 = mass_scale();
		const double l0 = length_scale();
		const double t0 = time_scale();
		return (sim_.kb / SIReferenceConstants::BOLTZMANN_CONSTANT) * (m0 * l0 * l0) / (t0 * t0);
	}

	[[nodiscard]] double amount_scale() const noexcept {
		return sim_.na / SIReferenceConstants::AVOGADRO_CONSTANT;
	}

	[[nodiscard]] double luminous_intensity_scale() const noexcept {
		return sim_.kcd;
	}

	[[nodiscard]] double current_scale() const noexcept {
		const double t0 = time_scale();
		return (t0 > 0.0) ? (charge_scale() / t0) : 0.0;
	}

	[[nodiscard]] double sim_elementary_charge() const noexcept {
		const double q0 = charge_scale();
		return (q0 > 0.0) ? (SIReferenceConstants::ELEMENTARY_CHARGE / q0) : 0.0;
	}

	[[nodiscard]] double sim_solar_mass() const noexcept {
		const double m0 = mass_scale();
		return (m0 > 0.0) ? (SIReferenceConstants::SOLAR_MASS / m0) : 0.0;
	}

	[[nodiscard]] double sim_astronomical_unit() const noexcept {
		const double l0 = length_scale();
		return (l0 > 0.0) ? (SIReferenceConstants::ASTRONOMICAL_UNIT / l0) : 0.0;
	}

	[[nodiscard]] double sim_electron_mass() const noexcept {
		const double m0 = mass_scale();
		return (m0 > 0.0) ? (SIReferenceConstants::ELECTRON_MASS / m0) : 0.0;
	}

	[[nodiscard]] double sim_proton_mass() const noexcept {
		const double m0 = mass_scale();
		return (m0 > 0.0) ? (SIReferenceConstants::PROTON_MASS / m0) : 0.0;
	}

	[[nodiscard]] double sim_neutron_mass() const noexcept {
		const double m0 = mass_scale();
		return (m0 > 0.0) ? (SIReferenceConstants::NEUTRON_MASS / m0) : 0.0;
	}

	[[nodiscard]] double sim_solar_schwarzschild_radius() const noexcept {
		return (2.0 * sim_.g * sim_solar_mass()) / (sim_.c * sim_.c);
	}
};

}
