#pragma once

#include "relativistic/core/constants.hpp"
#include "relativistic/optics/polarized_radiative_transfer.hpp"
#include "relativistic/optics/radiative_processes.hpp"
#include "relativistic/optics/stokes_vector.hpp"
#include "relativistic/render/gpu_types.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>

namespace Relativistic::Magnetosphere {

using Vec3 = std::array<double, 3>;

struct JetShape {
	double value{0.0};
	double derivative{0.0};
};

struct JetMetricFrame {
	double rho{1.0};
	double delta{1.0};
	double phi_radius{1.0};
	double lapse{1.0};
	double frame_dragging{0.0};
};

struct JetFieldSample {
	Vec3 field{0.0, 0.0, 0.0};
	double stream_function{0.0};
	double footpoint_ratio{0.0};
	double footpoint_angle{0.0};
	double field_angular_velocity{0.0};
	double toroidal_covariant{0.0};
};

struct JetPlasmaSample {
	bool valid{false};
	double weight{0.0};
	double electron_density{0.0};
	double field_tesla{0.0};
	double pitch_angle{0.0};
	double doppler{1.0};
	double temperature_k{0.0};
	double lorentz_factor{1.0};
};

struct JetSegmentResult {
	bool active{false};
	std::array<double, 3> radiance{0.0, 0.0, 0.0};
	double transmittance{1.0};
};

class BlandfordZnajekModel {
private:
	Render::GpuJetProfile profile_;
	Render::JetFieldGeometry geometry_;
	double mass_;
	double spin_;
	double horizon_;
	double omega_horizon_;
	double index_{1.0};
	double norm_{1.0};

	[[nodiscard]] static double smooth_unit(double x) noexcept {
		const double t = std::clamp(x, 0.0, 1.0);
		return t * t * (3.0 - 2.0 * t);
	}

	[[nodiscard]] static double dot(const Vec3& a, const Vec3& b) noexcept {
		return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
	}

public:
	explicit BlandfordZnajekModel(const Render::GpuJetProfile& profile) noexcept
		: profile_(profile),
		  geometry_(static_cast<Render::JetFieldGeometry>(static_cast<uint32_t>(profile.geometry + 0.5f))),
		  mass_(std::max(static_cast<double>(profile.mass), 1e-9)),
		  spin_(static_cast<double>(profile.spin)),
		  horizon_(std::max(static_cast<double>(profile.horizon_radius), 1e-9)),
		  omega_horizon_(static_cast<double>(profile.horizon_angular_velocity)) {
		switch (geometry_) {
			case Render::JetFieldGeometry::Paraboloidal:
				index_ = std::max(static_cast<double>(profile.field_line_index), 0.25);
				break;
			case Render::JetFieldGeometry::Vertical:
				index_ = 2.0;
				break;
			case Render::JetFieldGeometry::Monopole:
			case Render::JetFieldGeometry::SplitMonopole:
			default:
				index_ = 1.0;
				break;
		}
		norm_ = std::pow(horizon_, 2.0 - index_);
	}

	[[nodiscard]] double horizon_radius() const noexcept { return horizon_; }
	[[nodiscard]] double horizon_angular_velocity() const noexcept { return omega_horizon_; }
	[[nodiscard]] double field_line_exponent() const noexcept { return index_; }

	[[nodiscard]] JetShape shape(double theta) const noexcept {
		const double s = std::sin(theta);
		const double c = std::cos(theta);
		switch (geometry_) {
			case Render::JetFieldGeometry::SplitMonopole:
			case Render::JetFieldGeometry::Paraboloidal: {
				const double sign = (c > 0.0) ? 1.0 : ((c < 0.0) ? -1.0 : 0.0);
				return JetShape{1.0 - std::abs(c), s * sign};
			}
			case Render::JetFieldGeometry::Vertical:
				return JetShape{0.5 * s * s, s * c};
			case Render::JetFieldGeometry::Monopole:
			default:
				return JetShape{1.0 - c, s};
		}
	}

	[[nodiscard]] double stream_function(double r, double theta) const noexcept {
		return norm_ * std::pow(r, index_) * shape(theta).value;
	}

	[[nodiscard]] double footpoint_angle(double ratio) const noexcept {
		switch (geometry_) {
			case Render::JetFieldGeometry::Vertical:
				return std::asin(std::sqrt(std::clamp(2.0 * ratio, 0.0, 1.0)));
			case Render::JetFieldGeometry::SplitMonopole:
			case Render::JetFieldGeometry::Paraboloidal:
				return std::acos(std::clamp(1.0 - ratio, 0.0, 1.0));
			case Render::JetFieldGeometry::Monopole:
			default:
				return std::acos(std::clamp(1.0 - ratio, -1.0, 1.0));
		}
	}

	[[nodiscard]] double field_angular_velocity(double footpoint) const noexcept {
		if (profile_.angular_velocity_law > 0.5f) {
			return static_cast<double>(profile_.fixed_angular_velocity) / mass_;
		}
		const double s = std::sin(footpoint);
		return omega_horizon_ * static_cast<double>(profile_.angular_velocity_fraction) * (1.0 - static_cast<double>(profile_.angular_velocity_variation) * s * s);
	}

	[[nodiscard]] JetMetricFrame metric_frame(double r, double theta) const noexcept {
		const double sin_t = std::max(std::sin(theta), 1e-6);
		const double cos_t = std::cos(theta);
		const double a2 = spin_ * spin_;
		const double r2 = r * r;
		JetMetricFrame frame;
		const double rho2 = r2 + a2 * cos_t * cos_t;
		frame.rho = std::sqrt(rho2);
		frame.delta = std::max(r2 - 2.0 * mass_ * r + a2, 1e-12);
		const double sigma = (r2 + a2) * (r2 + a2) - a2 * frame.delta * sin_t * sin_t;
		frame.phi_radius = std::sqrt(std::max(sigma * sin_t * sin_t / rho2, 1e-30));
		frame.lapse = std::max(std::sqrt(rho2 * frame.delta / sigma), 1e-9);
		frame.frame_dragging = 2.0 * mass_ * spin_ * r / sigma;
		return frame;
	}

	[[nodiscard]] JetFieldSample field_at(double r, double theta, const JetMetricFrame& frame) const noexcept {
		JetFieldSample out;
		const JetShape local = shape(theta);
		const double r_pow = std::pow(r, index_);
		const double d_theta = norm_ * r_pow * local.derivative;
		const double d_r = index_ * norm_ * r_pow * local.value / r;
		out.stream_function = norm_ * r_pow * local.value;
		out.footpoint_ratio = std::pow(r / horizon_, index_) * local.value;
		out.footpoint_angle = footpoint_angle(out.footpoint_ratio);
		out.field_angular_velocity = field_angular_velocity(out.footpoint_angle);

		const double foot_sin = std::sin(out.footpoint_angle);
		const JetShape foot = shape(out.footpoint_angle);
		const double hemisphere = (geometry_ == Render::JetFieldGeometry::Monopole) ? 1.0 : ((std::cos(theta) >= 0.0) ? 1.0 : -1.0);
		const double a2 = spin_ * spin_;
		const double horizon_sq = horizon_ * horizon_ + a2;
		const double znajek = (out.field_angular_velocity - omega_horizon_) * horizon_sq * foot_sin / (horizon_sq - a2 * foot_sin * foot_sin);
		out.toroidal_covariant = static_cast<double>(profile_.toroidal_field_scale) * znajek * norm_ * std::pow(horizon_, index_) * std::abs(foot.derivative) * hemisphere;

		const double scale = 1.0 / (frame.rho * frame.phi_radius);
		out.field = Vec3{d_theta * scale, -d_r * std::sqrt(frame.delta) * scale, out.toroidal_covariant / frame.phi_radius};
		return out;
	}

	[[nodiscard]] double lorentz_factor(double r) const noexcept {
		const double span = std::max(static_cast<double>(profile_.acceleration_radius), 1e-9);
		const double x = std::pow(std::max(r - static_cast<double>(profile_.inner_radius), 0.0) / span, std::max(static_cast<double>(profile_.acceleration_index), 1e-3));
		const double inner = static_cast<double>(profile_.lorentz_inner);
		return inner + (static_cast<double>(profile_.lorentz_max) - inner) * x / (1.0 + x);
	}

	[[nodiscard]] double emission_weight(double r, double footpoint) const noexcept {
		const double folded = std::min(footpoint, std::numbers::pi_v<double> - footpoint);
		const double inner = static_cast<double>(profile_.inner_radius);
		const double outer = static_cast<double>(profile_.outer_radius);
		const double soft = std::max(static_cast<double>(profile_.footpoint_softness), 1e-3);
		const double radial_in = smooth_unit((r - inner) / std::max(0.25 * inner, 1e-9));
		const double radial_out = smooth_unit((outer - r) / std::max(0.25 * outer, 1e-9));
		const double angular = 1.0 - smooth_unit((folded - (static_cast<double>(profile_.footpoint_edge) - soft)) / (2.0 * soft));
		return radial_in * radial_out * angular;
	}

	[[nodiscard]] JetPlasmaSample sample_local(double r, double theta, const Vec3& photon) const noexcept {
		JetPlasmaSample out;
		if (r < static_cast<double>(profile_.inner_radius) || r > static_cast<double>(profile_.outer_radius)) {
			return out;
		}
		const double theta_c = std::clamp(theta, 1e-4, std::numbers::pi_v<double> - 1e-4);
		const JetMetricFrame frame = metric_frame(r, theta_c);
		const JetFieldSample field = field_at(r, theta_c, frame);
		const double weight = emission_weight(r, field.footpoint_angle);
		if (weight <= 1e-6) {
			return out;
		}

		const double gamma_target = lorentz_factor(r);
		const double speed = std::sqrt(std::max(1.0 - 1.0 / (gamma_target * gamma_target), 0.0));
		const double b_pol = std::hypot(field.field[0], field.field[1]);
		Vec3 pol_dir{1.0, 0.0, 0.0};
		if (b_pol > 1e-12) {
			pol_dir = Vec3{field.field[0] / b_pol, field.field[1] / b_pol, 0.0};
		}
		if (pol_dir[0] < 0.0) {
			pol_dir = Vec3{-pol_dir[0], -pol_dir[1], 0.0};
		}

		const bool relativistic_flow = profile_.doppler_enabled > 0.5f;
		const double rotation = (field.field_angular_velocity - frame.frame_dragging) * frame.phi_radius / frame.lapse;
		const double rotation_cap = 0.9 * speed;
		const double beta_phi = static_cast<double>(profile_.rotation_coupling) * std::clamp(rotation, -rotation_cap, rotation_cap);
		const double beta_pol = std::sqrt(std::max(speed * speed - beta_phi * beta_phi, 0.0));
		Vec3 beta{beta_pol * pol_dir[0], beta_pol * pol_dir[1], beta_phi};
		if (!relativistic_flow) {
			beta = Vec3{0.0, 0.0, 0.0};
		}
		const double beta_sq = dot(beta, beta);
		const double beta_mag = std::sqrt(beta_sq);
		const double gamma_flow = 1.0 / std::sqrt(std::max(1.0 - beta_sq, 1e-9));

		const double beta_dot_b = dot(beta, field.field);
		const double comoving_scale = gamma_flow / (gamma_flow + 1.0);
		const Vec3 field_comoving{
			field.field[0] / gamma_flow + comoving_scale * beta[0] * beta_dot_b,
			field.field[1] / gamma_flow + comoving_scale * beta[1] * beta_dot_b,
			field.field[2] / gamma_flow + comoving_scale * beta[2] * beta_dot_b
		};

		Vec3 photon_comoving = photon;
		if (beta_mag > 1e-9) {
			const Vec3 beta_hat{beta[0] / beta_mag, beta[1] / beta_mag, beta[2] / beta_mag};
			const double parallel = dot(photon, beta_hat);
			const double denominator = 1.0 - beta_mag * parallel;
			const double parallel_comoving = (parallel - beta_mag) / denominator;
			const double perpendicular_scale = 1.0 / (gamma_flow * denominator);
			for (size_t i = 0; i < 3; ++i) {
				photon_comoving[i] = parallel_comoving * beta_hat[i] + perpendicular_scale * (photon[i] - parallel * beta_hat[i]);
			}
		}

		double doppler = relativistic_flow ? 1.0 / (gamma_flow * (1.0 - dot(beta, photon))) : 1.0;
		if (profile_.redshift_enabled > 0.5f) {
			doppler *= frame.lapse;
		}
		doppler = std::clamp(doppler, 1e-3, 1e3);

		const double field_mag = std::sqrt(dot(field_comoving, field_comoving));
		if (field_mag <= 1e-12) {
			return out;
		}
		const double photon_mag = std::max(std::sqrt(dot(photon_comoving, photon_comoving)), 1e-12);
		const double cos_pitch = std::clamp(dot(photon_comoving, field_comoving) / (photon_mag * field_mag), -1.0, 1.0);

		using Constants = Core::PhysicalConstants<double>;
		const double field_tesla = field_mag * static_cast<double>(profile_.horizon_field_tesla);
		const double sigma = std::max(static_cast<double>(profile_.magnetization) * std::pow(r / horizon_, static_cast<double>(profile_.magnetization_index)), 1e-6);

		out.valid = true;
		out.weight = weight;
		out.field_tesla = field_tesla;
		out.pitch_angle = std::acos(cos_pitch);
		out.doppler = doppler;
		out.lorentz_factor = gamma_flow;
		out.electron_density = Constants::VACUUM_PERMITTIVITY * field_tesla * field_tesla / (sigma * Constants::PROTON_MASS);
		out.temperature_k = static_cast<double>(profile_.electron_temperature_k) * std::pow(static_cast<double>(profile_.inner_radius) / r, static_cast<double>(profile_.temperature_index));
		return out;
	}

	[[nodiscard]] double magnetic_flux_weber() const noexcept {
		const double horizon_meters = horizon_ * static_cast<double>(profile_.meters_per_unit);
		return 2.0 * std::numbers::pi_v<double> * static_cast<double>(profile_.horizon_field_tesla) * horizon_meters * horizon_meters * shape(std::numbers::pi_v<double> * 0.5).value;
	}

	[[nodiscard]] double blandford_znajek_power_watts() const noexcept {
		using Constants = Core::PhysicalConstants<double>;
		const double omega_si = omega_horizon_ * Constants::SPEED_OF_LIGHT / std::max(static_cast<double>(profile_.meters_per_unit), 1e-30);
		const double x = omega_horizon_ * mass_;
		const double correction = std::max(1.0 + 1.38 * x * x - 9.2 * x * x * x * x, 0.0);
		const double kappa = (geometry_ == Render::JetFieldGeometry::Monopole || geometry_ == Render::JetFieldGeometry::SplitMonopole) ? 0.053 : 0.044;
		const double flux = magnetic_flux_weber();
		return kappa * flux * flux * omega_si * omega_si * correction * Constants::VACUUM_PERMITTIVITY * Constants::SPEED_OF_LIGHT;
	}
};

class JetEmissionIntegrator {
private:
	struct Transfer {
		Optics::StokesEmissivity<double> emissivity;
		Optics::StokesTransferMatrix<double> absorption;
	};

	[[nodiscard]] static Transfer transfer_coefficients(const Render::GpuJetProfile& profile, const JetPlasmaSample& sample) noexcept {
		using Engine = Optics::RadiativeProcessEngine<double>;
		const double nu_emit = static_cast<double>(profile.observing_frequency_hz) / sample.doppler;
		const auto model = static_cast<Render::JetEmissionModel>(static_cast<uint32_t>(profile.emission_model + 0.5f));

		Optics::PolarizedPlasmaState<double> plasma;
		plasma.electron_density = sample.electron_density * sample.weight;
		plasma.ion_density = plasma.electron_density;
		plasma.electron_temperature_k = sample.temperature_k;
		plasma.magnetic_field_tesla = sample.field_tesla;
		plasma.pitch_angle_rad = sample.pitch_angle;
		plasma.power_law_index = static_cast<double>(profile.power_law_index);
		plasma.non_thermal_fraction = static_cast<double>(profile.non_thermal_fraction);
		plasma.gamma_min = static_cast<double>(profile.gamma_min);

		double ji = 0.0, jq = 0.0, ju = 0.0, jv = 0.0;
		double ai = 0.0, aq = 0.0, au = 0.0, av = 0.0, rq = 0.0, ru = 0.0, rv = 0.0;

		if (model != Render::JetEmissionModel::Thermal) {
			const auto j = Engine::non_thermal_synchrotron_emissivity(nu_emit, plasma);
			const auto k = Engine::non_thermal_synchrotron_absorptivity(nu_emit, plasma);
			ji += j.j_i; jq += j.j_q; ju += j.j_u; jv += j.j_v;
			ai += k.alpha_i; aq += k.alpha_q; au += k.alpha_u; av += k.alpha_v;
			rq += k.rho_q; ru += k.rho_u; rv += k.rho_v;
		}
		if (model != Render::JetEmissionModel::NonThermal) {
			auto thermal = plasma;
			if (model == Render::JetEmissionModel::Hybrid) {
				thermal.electron_density *= (1.0 - plasma.non_thermal_fraction);
				thermal.ion_density = thermal.electron_density;
			}
			const auto j = Engine::thermal_synchrotron_emissivity(nu_emit, thermal);
			const auto k = Engine::thermal_synchrotron_absorptivity(nu_emit, thermal);
			ji += j.j_i; jq += j.j_q; ju += j.j_u; jv += j.j_v;
			ai += k.alpha_i; aq += k.alpha_q; au += k.alpha_u; av += k.alpha_v;
			rq += k.rho_q; ru += k.rho_u; rv += k.rho_v;
		}

		const double g = sample.doppler;
		const double g2 = g * g;
		const double absorption_scale = static_cast<double>(profile.absorption_scale) / g;
		const double faraday_scale = static_cast<double>(profile.faraday_scale) / g;
		return Transfer{
			Optics::StokesEmissivity<double>(ji * g2, jq * g2, ju * g2, jv * g2),
			Optics::StokesTransferMatrix<double>(ai * absorption_scale, aq * absorption_scale, au * absorption_scale, av * absorption_scale, rq * faraday_scale, ru * faraday_scale, rv * faraday_scale)
		};
	}

	[[nodiscard]] static std::array<double, 3> polarization_color(double x) noexcept {
		return {std::clamp(2.0 * x, 0.0, 1.0), std::clamp(1.0 - std::abs(2.0 * x - 1.0), 0.0, 1.0), std::clamp(1.0 - 2.0 * x, 0.0, 1.0)};
	}

public:
	[[nodiscard]] static double canonical_intensity(const Render::GpuJetProfile& profile) noexcept {
		const BlandfordZnajekModel model(profile);
		const double radius = std::max(2.0 * model.horizon_radius(), 1.5 * static_cast<double>(profile.inner_radius));
		const JetPlasmaSample sample = model.sample_local(radius, 0.5, Vec3{0.0, 1.0, 0.0});
		if (!sample.valid) {
			return 0.0;
		}
		const Transfer transfer = transfer_coefficients(profile, sample);
		const double path = radius * static_cast<double>(profile.meters_per_unit);
		const auto stokes = Optics::PolarizedRadiativeTransfer<double>::step_delano_analytical(Optics::StokesVector<double>{}, transfer.emissivity, transfer.absorption, path);
		return std::isfinite(stokes.i) ? stokes.i : 0.0;
	}

	[[nodiscard]] static JetSegmentResult integrate(const Render::GpuJetProfile& profile, const Vec3& from, const Vec3& to) noexcept {
		JetSegmentResult result;
		const Vec3 delta{to[0] - from[0], to[1] - from[1], to[2] - from[2]};
		const double length = std::sqrt(delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2]);
		if (length < 1e-12) {
			return result;
		}
		const double inner = static_cast<double>(profile.inner_radius);
		const double outer = static_cast<double>(profile.outer_radius);
		const double r_from = std::sqrt(from[0] * from[0] + from[1] * from[1] + from[2] * from[2]);
		const double r_to = std::sqrt(to[0] * to[0] + to[1] * to[1] + to[2] * to[2]);
		if (std::max(r_from, r_to) < inner) {
			return result;
		}
		const double t_closest = std::clamp(-(from[0] * delta[0] + from[1] * delta[1] + from[2] * delta[2]) / (length * length), 0.0, 1.0);
		const Vec3 closest{from[0] + t_closest * delta[0], from[1] + t_closest * delta[1], from[2] + t_closest * delta[2]};
		if (std::sqrt(closest[0] * closest[0] + closest[1] * closest[1] + closest[2] * closest[2]) > outer) {
			return result;
		}

		const BlandfordZnajekModel model(profile);
		const double mass = std::max(static_cast<double>(profile.mass), 1e-9);
		const size_t max_samples = static_cast<size_t>(std::max(static_cast<int>(profile.max_samples + 0.5f), 1));
		const size_t count = std::clamp<size_t>(static_cast<size_t>(std::ceil(length / mass * static_cast<double>(profile.sampling_density))), size_t{1}, max_samples);
		const double step_meters = length / static_cast<double>(count) * static_cast<double>(profile.meters_per_unit);
		const Vec3 photon{-delta[0] / length, -delta[1] / length, -delta[2] / length};
		const double normalization = std::exp(-static_cast<double>(profile.log_reference_intensity)) * static_cast<double>(profile.brightness);
		const double coherence = static_cast<double>(profile.polarization_coherence);
		const auto mode = static_cast<Render::JetDisplayMode>(static_cast<uint32_t>(profile.display_mode + 0.5f));
		const std::array<double, 3> tint{static_cast<double>(profile.tint_r), static_cast<double>(profile.tint_g), static_cast<double>(profile.tint_b)};

		double transmittance = 1.0;
		for (size_t i = 0; i < count; ++i) {
			const double t = (static_cast<double>(i) + 0.5) / static_cast<double>(count);
			const Vec3 pos{from[0] + t * delta[0], from[1] + t * delta[1], from[2] + t * delta[2]};
			const double r = std::sqrt(pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2]);
			if (r < inner || r > outer) {
				continue;
			}
			const double theta = std::acos(std::clamp(pos[2] / r, -1.0, 1.0));
			const double rho = std::hypot(pos[0], pos[1]);
			const Vec3 e_r{pos[0] / r, pos[1] / r, pos[2] / r};
			const Vec3 e_phi = (rho > 1e-12) ? Vec3{-pos[1] / rho, pos[0] / rho, 0.0} : Vec3{0.0, 1.0, 0.0};
			const Vec3 e_theta{
				e_phi[1] * e_r[2] - e_phi[2] * e_r[1],
				e_phi[2] * e_r[0] - e_phi[0] * e_r[2],
				e_phi[0] * e_r[1] - e_phi[1] * e_r[0]
			};
			const Vec3 photon_local{
				photon[0] * e_r[0] + photon[1] * e_r[1] + photon[2] * e_r[2],
				photon[0] * e_theta[0] + photon[1] * e_theta[1] + photon[2] * e_theta[2],
				photon[0] * e_phi[0] + photon[1] * e_phi[1] + photon[2] * e_phi[2]
			};

			const JetPlasmaSample sample = model.sample_local(r, theta, photon_local);
			if (!sample.valid) {
				continue;
			}
			const Transfer transfer = transfer_coefficients(profile, sample);
			const auto stokes = Optics::PolarizedRadiativeTransfer<double>::step_delano_analytical(Optics::StokesVector<double>{}, transfer.emissivity, transfer.absorption, step_meters);
			const double tau = transfer.absorption.alpha_i * step_meters;
			if (!std::isfinite(stokes.i) || !std::isfinite(tau)) {
				continue;
			}

			const double intensity = std::max(stokes.i, 0.0) * normalization;
			const double polarized = std::hypot(stokes.q, stokes.u) * coherence * normalization;
			std::array<double, 3> color{};
			if (mode == Render::JetDisplayMode::PolarizedIntensity) {
				for (size_t c = 0; c < 3; ++c) {
					color[c] = tint[c] * polarized;
				}
			} else if (mode == Render::JetDisplayMode::PolarizationFraction) {
				const double fraction = (stokes.i > 0.0) ? std::clamp(coherence * std::hypot(stokes.q, stokes.u) / stokes.i, 0.0, 1.0) : 0.0;
				const auto mapped = polarization_color(fraction);
				for (size_t c = 0; c < 3; ++c) {
					color[c] = mapped[c] * intensity;
				}
			} else {
				for (size_t c = 0; c < 3; ++c) {
					color[c] = tint[c] * intensity;
				}
			}
			for (size_t c = 0; c < 3; ++c) {
				result.radiance[c] += transmittance * color[c];
			}
			transmittance *= std::exp(-std::clamp(tau, 0.0, 600.0));
			result.active = true;
		}
		result.transmittance = transmittance;
		return result;
	}
};

}
