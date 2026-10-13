#pragma once

#include "relativistic/render/gpu_types.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>

namespace Relativistic::Cosmology {

inline constexpr double kCosmologyLogAMin = -6.907755278982137;
inline constexpr double kCosmologyLogAMax = 1.3862943611198906;

enum class QuintessencePotential : uint32_t {
	Exponential = 0,
	InversePowerLaw = 1,
	PseudoNambuGoldstone = 2,
	Supergravity = 3
};

inline constexpr size_t kQuintessencePotentialCount = 4;

inline constexpr std::array<const char*, kQuintessencePotentialCount> kQuintessencePotentialNames{
	"Exponential V0 exp(-lambda phi)",
	"Inverse Power Law (Ratra-Peebles) V0 phi^-alpha",
	"Pseudo-Nambu-Goldstone V0 (1 + cos(phi / f))",
	"Supergravity V0 phi^-alpha exp(phi^2 / 2)"
};

struct DarkEnergyParameters {
	double omega_matter{0.3};
	double omega_radiation{9.0e-5};
	QuintessencePotential potential{QuintessencePotential::InversePowerLaw};
	double potential_slope{2.0};
	double potential_scale{1.0};
	double initial_field{0.1};
	double chaplygin_stiffness{0.8};
	double chaplygin_exponent{1.0};

	[[nodiscard]] bool operator==(const DarkEnergyParameters&) const noexcept = default;

	void sanitize() noexcept {
		const auto finite_or = [](double value, double fallback) noexcept { return std::isfinite(value) ? value : fallback; };
		omega_matter = std::clamp(finite_or(omega_matter, 0.3), 0.01, 0.99);
		omega_radiation = std::clamp(finite_or(omega_radiation, 9.0e-5), 0.0, 0.05);
		potential = static_cast<QuintessencePotential>(std::min<uint32_t>(static_cast<uint32_t>(potential), static_cast<uint32_t>(kQuintessencePotentialCount - 1)));
		potential_slope = std::clamp(finite_or(potential_slope, 2.0), 0.01, 20.0);
		potential_scale = std::clamp(finite_or(potential_scale, 1.0), 0.05, 50.0);
		initial_field = std::clamp(finite_or(initial_field, 0.1), 0.01, 20.0);
		chaplygin_stiffness = std::clamp(finite_or(chaplygin_stiffness, 0.8), 0.0, 1.0);
		chaplygin_exponent = std::clamp(finite_or(chaplygin_exponent, 1.0), 0.0, 10.0);
	}
};

struct ExpansionDiagnostics {
	bool converged{false};
	double equation_of_state_today{-1.0};
	double dark_energy_fraction_today{0.7};
	double deceleration_today{-0.55};
	double age_in_hubble_times{0.95};
};

class TabulatedExpansionHistory {
public:
	[[nodiscard]] static double hubble(const Render::GpuCosmologyProfile& profile, double a) noexcept {
		if (profile.table_size < 2U) {
			return std::max(static_cast<double>(profile.hubble0), 1.0e-6);
		}
		const double log_min = static_cast<double>(profile.table_log_a_min);
		const double span = std::max(static_cast<double>(profile.table_log_a_max) - log_min, 1.0e-9);
		const double x = std::clamp((std::log(std::max(a, 1.0e-6)) - log_min) / span, 0.0, 1.0) * static_cast<double>(profile.table_size - 1U);
		const size_t i0 = std::min<size_t>(static_cast<size_t>(x), static_cast<size_t>(profile.table_size) - 2U);
		const double f = x - static_cast<double>(i0);
		const double log_e = static_cast<double>(profile.log_hubble[i0]) * (1.0 - f) + static_cast<double>(profile.log_hubble[i0 + 1U]) * f;
		return static_cast<double>(profile.hubble0) * std::exp(log_e);
	}
};

class QuintessenceBackground {
public:
	struct State {
		double phi{0.0};
		double dphi{0.0};
	};

	explicit QuintessenceBackground(const DarkEnergyParameters& parameters) noexcept : parameters_(parameters) {}

	[[nodiscard]] bool solve() noexcept {
		double low = std::log(1.0e-8);
		double high = std::log(1.0e8);
		const auto residual = [this](double log_amplitude) noexcept {
			amplitude_ = std::exp(log_amplitude);
			return integrate_to_today();
		};
		if (!(residual(low) < 0.0) || residual(high) < 0.0) {
			return false;
		}
		for (int iteration = 0; iteration < 90; ++iteration) {
			const double middle = 0.5 * (low + high);
			if (residual(middle) < 0.0) {
				low = middle;
			} else {
				high = middle;
			}
		}
		amplitude_ = std::exp(0.5 * (low + high));
		return std::abs(integrate_to_today()) < 1.0e-5;
	}

	[[nodiscard]] bool sample(std::array<double, Render::kCosmologyTableSize>& nodes) const noexcept {
		State state{parameters_.initial_field, 0.0};
		const double step = (kCosmologyLogAMax - kCosmologyLogAMin) / static_cast<double>(nodes.size() - 1);
		for (size_t i = 0; i < nodes.size(); ++i) {
			const double n = kCosmologyLogAMin + step * static_cast<double>(i);
			double h2 = 0.0;
			State unused;
			if (!derivative(n, state, unused, h2)) {
				return false;
			}
			nodes[i] = 0.5 * std::log(h2);
			if (i + 1 < nodes.size() && !advance(state, n, n + step, 8)) {
				return false;
			}
		}
		return true;
	}

	[[nodiscard]] double equation_of_state_today() const noexcept {
		double h2 = 0.0;
		State unused;
		if (!derivative(0.0, today_, unused, h2)) {
			return -1.0;
		}
		const double kinetic = h2 * 0.5 * today_.dphi * today_.dphi;
		const double potential_energy = potential(today_.phi);
		return (kinetic - potential_energy) / std::max(kinetic + potential_energy, 1.0e-300);
	}

private:
	DarkEnergyParameters parameters_;
	double amplitude_{1.0};
	State today_{};

	[[nodiscard]] double potential(double phi) const noexcept {
		const double slope = parameters_.potential_slope;
		const double safe_phi = std::max(phi, 1.0e-6);
		switch (parameters_.potential) {
			case QuintessencePotential::Exponential:
				return amplitude_ * std::exp(-slope * phi);
			case QuintessencePotential::InversePowerLaw:
				return amplitude_ * std::pow(safe_phi, -slope);
			case QuintessencePotential::PseudoNambuGoldstone:
				return amplitude_ * (1.0 + std::cos(phi / parameters_.potential_scale));
			case QuintessencePotential::Supergravity:
			default:
				return amplitude_ * std::pow(safe_phi, -slope) * std::exp(0.5 * phi * phi);
		}
	}

	[[nodiscard]] double potential_derivative(double phi) const noexcept {
		const double slope = parameters_.potential_slope;
		const double safe_phi = std::max(phi, 1.0e-6);
		switch (parameters_.potential) {
			case QuintessencePotential::Exponential:
				return -slope * amplitude_ * std::exp(-slope * phi);
			case QuintessencePotential::InversePowerLaw:
				return -slope * amplitude_ * std::pow(safe_phi, -slope - 1.0);
			case QuintessencePotential::PseudoNambuGoldstone:
				return -amplitude_ * std::sin(phi / parameters_.potential_scale) / parameters_.potential_scale;
			case QuintessencePotential::Supergravity:
			default:
				return amplitude_ * std::pow(safe_phi, -slope) * std::exp(0.5 * phi * phi) * (phi - slope / safe_phi);
		}
	}

	[[nodiscard]] bool derivative(double n, const State& state, State& out, double& h2) const noexcept {
		const double rho_matter = 3.0 * parameters_.omega_matter * std::exp(-3.0 * n);
		const double rho_radiation = 3.0 * parameters_.omega_radiation * std::exp(-4.0 * n);
		const double kinetic = 0.5 * state.dphi * state.dphi;
		const double denominator = 3.0 - kinetic;
		if (denominator <= 1.0e-6) {
			return false;
		}
		h2 = (rho_matter + rho_radiation + potential(state.phi)) / denominator;
		if (!(h2 > 0.0) || !std::isfinite(h2)) {
			return false;
		}
		const double hubble_slope = -0.5 * (rho_matter + 4.0 / 3.0 * rho_radiation) / h2 - kinetic;
		out.phi = state.dphi;
		out.dphi = -(3.0 + hubble_slope) * state.dphi - potential_derivative(state.phi) / h2;
		return std::isfinite(out.dphi);
	}

	[[nodiscard]] bool advance(State& state, double n0, double n1, int steps) const noexcept {
		const double h = (n1 - n0) / static_cast<double>(steps);
		double n = n0;
		for (int i = 0; i < steps; ++i) {
			State k1;
			State k2;
			State k3;
			State k4;
			double h2 = 0.0;
			if (!derivative(n, state, k1, h2)) return false;
			if (!derivative(n + 0.5 * h, State{state.phi + 0.5 * h * k1.phi, state.dphi + 0.5 * h * k1.dphi}, k2, h2)) return false;
			if (!derivative(n + 0.5 * h, State{state.phi + 0.5 * h * k2.phi, state.dphi + 0.5 * h * k2.dphi}, k3, h2)) return false;
			if (!derivative(n + h, State{state.phi + h * k3.phi, state.dphi + h * k3.dphi}, k4, h2)) return false;
			state.phi += h / 6.0 * (k1.phi + 2.0 * k2.phi + 2.0 * k3.phi + k4.phi);
			state.dphi += h / 6.0 * (k1.dphi + 2.0 * k2.dphi + 2.0 * k3.dphi + k4.dphi);
			n += h;
		}
		return std::isfinite(state.phi) && std::isfinite(state.dphi);
	}

	[[nodiscard]] double integrate_to_today() noexcept {
		State state{parameters_.initial_field, 0.0};
		if (!advance(state, kCosmologyLogAMin, 0.0, 1600)) {
			return 1.0e30;
		}
		double h2 = 0.0;
		State unused;
		if (!derivative(0.0, state, unused, h2)) {
			return 1.0e30;
		}
		today_ = state;
		return h2 - 1.0;
	}
};

class ExpansionHistoryBuilder {
public:
	struct Result {
		Render::GpuCosmologyProfile profile{};
		ExpansionDiagnostics diagnostics{};
	};

	[[nodiscard]] static Result build_lambda_cdm(const DarkEnergyParameters& p, double hubble0) noexcept {
		const double omega_lambda = 1.0 - p.omega_matter - p.omega_radiation;
		return assemble(hubble0, [&](double n) noexcept {
			return 0.5 * std::log(p.omega_matter * std::exp(-3.0 * n) + p.omega_radiation * std::exp(-4.0 * n) + omega_lambda);
		}, -1.0, omega_lambda, true);
	}

	[[nodiscard]] static Result build_chaplygin(const DarkEnergyParameters& p, double hubble0) noexcept {
		const double fluid = 1.0 - p.omega_matter - p.omega_radiation;
		const double a_s = p.chaplygin_stiffness;
		const double alpha = p.chaplygin_exponent;
		const auto density = [&](double n) noexcept {
			return std::pow(a_s + (1.0 - a_s) * std::exp(3.0 * (1.0 + alpha) * n * -1.0), 1.0 / (1.0 + alpha));
		};
		const double w_today = -a_s / (a_s + (1.0 - a_s));
		return assemble(hubble0, [&](double n) noexcept {
			return 0.5 * std::log(p.omega_matter * std::exp(-3.0 * n) + p.omega_radiation * std::exp(-4.0 * n) + fluid * density(n));
		}, w_today, fluid, true);
	}

	[[nodiscard]] static Result build_quintessence(const DarkEnergyParameters& p, double hubble0) noexcept {
		QuintessenceBackground background(p);
		std::array<double, Render::kCosmologyTableSize> nodes{};
		if (!background.solve() || !background.sample(nodes)) {
			Result fallback = build_lambda_cdm(p, hubble0);
			fallback.diagnostics.converged = false;
			return fallback;
		}
		return assemble(hubble0, [&](double n) noexcept { return interpolate(nodes, n); }, background.equation_of_state_today(), 1.0 - p.omega_matter - p.omega_radiation, true);
	}

private:
	[[nodiscard]] static double interpolate(const std::array<double, Render::kCosmologyTableSize>& nodes, double n) noexcept {
		const double span = kCosmologyLogAMax - kCosmologyLogAMin;
		const double x = std::clamp((n - kCosmologyLogAMin) / span, 0.0, 1.0) * static_cast<double>(nodes.size() - 1);
		const size_t i0 = std::min<size_t>(static_cast<size_t>(x), nodes.size() - 2);
		const double f = x - static_cast<double>(i0);
		return nodes[i0] * (1.0 - f) + nodes[i0 + 1] * f;
	}

	template <typename LogHubble>
	[[nodiscard]] static Result assemble(double hubble0, LogHubble&& log_hubble, double w_today, double omega_de, bool converged) noexcept {
		Result result;
		auto& profile = result.profile;
		profile.table_size = static_cast<uint32_t>(Render::kCosmologyTableSize);
		profile.table_log_a_min = static_cast<float>(kCosmologyLogAMin);
		profile.table_log_a_max = static_cast<float>(kCosmologyLogAMax);
		profile.hubble0 = static_cast<float>(hubble0);
		const double step = (kCosmologyLogAMax - kCosmologyLogAMin) / static_cast<double>(Render::kCosmologyTableSize - 1);
		for (size_t i = 0; i < Render::kCosmologyTableSize; ++i) {
			profile.log_hubble[i] = static_cast<float>(log_hubble(kCosmologyLogAMin + step * static_cast<double>(i)));
		}
		constexpr double h = 1.0e-3;
		result.diagnostics.converged = converged;
		result.diagnostics.equation_of_state_today = w_today;
		result.diagnostics.dark_energy_fraction_today = omega_de;
		result.diagnostics.deceleration_today = -1.0 - (log_hubble(h) - log_hubble(-h)) / (2.0 * h);
		constexpr int samples = 4000;
		const double dn = -kCosmologyLogAMin / static_cast<double>(samples);
		double age = 0.0;
		for (int i = 0; i < samples; ++i) {
			const double n = kCosmologyLogAMin + dn * (static_cast<double>(i) + 0.5);
			age += dn * std::exp(-log_hubble(n));
		}
		result.diagnostics.age_in_hubble_times = age;
		return result;
	}
};

}
