#pragma once

#include "relativistic/core/constants.hpp"
#include "relativistic/core/pcg64.hpp"
#include "relativistic/core/tensor.hpp"
#include "relativistic/optics/maxwell_juttner.hpp"
#include <cmath>
#include <numbers>
#include <algorithm>
#include <array>
#include <vector>
#include <span>
#include <concepts>

namespace Relativistic::Optics {

struct alignas(32) PhotonPacket {
	double energy_joules{1.0e-19};
	double weight{1.0};
	std::array<double, 3> position{0.0, 0.0, 0.0};
	std::array<double, 3> direction{1.0, 0.0, 0.0};
	uint32_t scatterings_count{0};
};

class InverseComptonEngine {
private:
	static constexpr double SIGMA_THOMSON = 6.6524587321e-29;
	static constexpr double M_ELECTRON = Core::PhysicalConstants<double>::ELECTRON_MASS;
	static constexpr double C_LIGHT = Core::PhysicalConstants<double>::SPEED_OF_LIGHT;
	static constexpr double E_REST_ELECTRON = M_ELECTRON * C_LIGHT * C_LIGHT;

public:
	[[nodiscard]] static constexpr double klein_nishina_total_cross_section(double energy_electron_rest_frame_joules) noexcept {
		const double x = energy_electron_rest_frame_joules / E_REST_ELECTRON;
		if (x < 1e-4) {
			return SIGMA_THOMSON * (1.0 - 2.0 * x + 5.2 * x * x);
		}
		const double term1 = (1.0 + x) / (x * x * x);
		const double term2 = (2.0 * x * (1.0 + x)) / (1.0 + 2.0 * x) - std::log(1.0 + 2.0 * x);
		const double term3 = 0.5 / x * std::log(1.0 + 2.0 * x);
		const double term4 = (1.0 + 3.0 * x) / ((1.0 + 2.0 * x) * (1.0 + 2.0 * x));
		return 0.75 * SIGMA_THOMSON * (term1 * term2 + term3 - term4);
	}

	[[nodiscard]] static constexpr double compton_y_parameter(
		double electron_optical_depth,
		double dimensionless_temperature
	) noexcept {
		const double theta = dimensionless_temperature;
		const double tau = electron_optical_depth;
		const double tau_eff = std::max(tau, tau * tau);
		return 4.0 * theta * (1.0 + 4.0 * theta) * tau_eff;
	}

	[[nodiscard]] static PhotonPacket scatter_single_photon(
		const PhotonPacket& incoming_photon,
		const MaxwellJuttnerDistribution<double>& electron_dist,
		Core::PCG64Engine& rng
	) noexcept {
		PhotonPacket scattered = incoming_photon;
		const auto v_e = electron_dist.sample_velocity_vector(rng);

		const double beta_sq = v_e[0] * v_e[0] + v_e[1] * v_e[1] + v_e[2] * v_e[2];
		const double gamma_e = 1.0 / std::sqrt(std::max(1.0 - beta_sq, 1e-15));

		const double n_dot_beta = incoming_photon.direction[0] * v_e[0] + incoming_photon.direction[1] * v_e[1] + incoming_photon.direction[2] * v_e[2];
		const double e_prime = gamma_e * incoming_photon.energy_joules * (1.0 - n_dot_beta);

		const double eps = e_prime / E_REST_ELECTRON;

		double cos_theta_prime = 1.0;
		for (;;) {
			const double u1 = rng.next_uniform_double();
			const double u2 = rng.next_uniform_double();
			const double cos_cand = 2.0 * u1 - 1.0;
			const double eps_out_ratio = 1.0 / (1.0 + eps * (1.0 - cos_cand));
			const double kn_prob = 0.5 * eps_out_ratio * eps_out_ratio * (eps_out_ratio + 1.0 / eps_out_ratio - (1.0 - cos_cand * cos_cand));
			if (u2 <= kn_prob) {
				cos_theta_prime = cos_cand;
				break;
			}
		}

		const double sin_theta_prime = std::sqrt(std::max(1.0 - cos_theta_prime * cos_theta_prime, 0.0));
		const double phi_prime = 2.0 * std::numbers::pi_v<double> * rng.next_uniform_double();

		const double e_scattered_prime = e_prime / (1.0 + eps * (1.0 - cos_theta_prime));

		const double cos_scatter_lab = cos_theta_prime;
		const double denom = std::max(1.0 - std::sqrt(beta_sq) * cos_scatter_lab, 1e-15);
		const double e_scattered_lab = gamma_e * e_scattered_prime * denom;

		const double beta_mag = std::sqrt(beta_sq);
		std::array<double, 3> n_e = {1.0, 0.0, 0.0};
		if (beta_mag > 1e-12) {
			n_e = {v_e[0] / beta_mag, v_e[1] / beta_mag, v_e[2] / beta_mag};
		}

		const double n_scat_x = sin_theta_prime * std::cos(phi_prime);
		const double n_scat_y = sin_theta_prime * std::sin(phi_prime);
		const double n_scat_z = cos_theta_prime;

		std::array<double, 3> new_dir = {
			n_scat_x * incoming_photon.direction[0] + n_scat_z * n_e[0],
			n_scat_y * incoming_photon.direction[1] + n_scat_z * n_e[1],
			n_scat_z * incoming_photon.direction[2] + n_scat_z * n_e[2]
		};
		const double len = std::sqrt(new_dir[0] * new_dir[0] + new_dir[1] * new_dir[1] + new_dir[2] * new_dir[2]);
		if (len > 0.0) {
			new_dir[0] /= len;
			new_dir[1] /= len;
			new_dir[2] /= len;
		}

		scattered.energy_joules = e_scattered_lab;
		scattered.direction = new_dir;
		scattered.scatterings_count += 1;
		return scattered;
	}

	[[nodiscard]] static std::vector<PhotonPacket> simulate_los_compton_scattering(
		std::span<const PhotonPacket> seed_photons,
		double optical_depth,
		double dimensionless_temp,
		Core::PCG64Engine& rng
	) {
		std::vector<PhotonPacket> output_photons;
		output_photons.reserve(seed_photons.size());

		const MaxwellJuttnerDistribution dist(dimensionless_temp);

		for (const auto& p_in : seed_photons) {
			PhotonPacket current = p_in;
			double tau_rem = optical_depth;

			while (tau_rem > 0.0) {
				const double tau_step = -std::log(std::max(rng.next_uniform_double(), 1e-15));
				if (tau_step > tau_rem) {
					break;
				}
				tau_rem -= tau_step;
				current = scatter_single_photon(current, dist, rng);
			}

			output_photons.push_back(current);
		}

		return output_photons;
	}
};

}
