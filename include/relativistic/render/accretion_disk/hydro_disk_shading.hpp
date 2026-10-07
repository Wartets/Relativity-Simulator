#pragma once

#include "relativistic/render/gpu_types.hpp"
#include "relativistic/render/accretion_disk/accretion_disk_model.hpp"
#include "relativistic/hydro/novikov_thorne.hpp"
#include "relativistic/hydro/fishbone_moncrief.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <optional>
#include <utility>

namespace Relativistic::Render {

struct HydroSegmentResult {
	std::array<double, 3> radiance{0.0, 0.0, 0.0};
	double opacity{0.0};
	double redshift{1.0};
};

class HydroDiskContext {
private:
	static constexpr double kHalfPi = std::numbers::pi_v<double> * 0.5;
	static constexpr int kFluxSamples = 192;
	static constexpr int kMaxSegmentSamples = 128;

	GpuHydroDiskProfile profile_{};
	double mass_{1.0};
	double spin_{0.0};
	double inner_radius_{6.0};
	double outer_radius_{24.0};
	std::optional<Hydro::NovikovThorneDisk<double>> thin_disk_{};
	std::optional<Hydro::FishboneMoncriefTorus<double>> torus_{};
	double flux_peak_{1.0};
	double rate_factor_{1.0};
	double max_enthalpy_{1.0};
	double torus_inner_bound_{0.0};
	double torus_outer_bound_{0.0};
	bool torus_valid_{false};
	bool torus_closed_{false};

	void build_thin_disk() noexcept {
		Hydro::NovikovThorneConfig<double> config;
		config.mass = mass_;
		config.spin_parameter = spin_;
		config.accretion_rate = 1.0;
		config.speed_of_light = 1.0;
		config.gravitational_constant = 1.0;
		thin_disk_.emplace(config);

		rate_factor_ = std::pow(std::max(static_cast<double>(profile_.accretion_rate_scale), 1e-6), 0.25);
		const double radius_low = std::max(inner_radius_, thin_disk_->isco_radius() * 1.0001);
		const double radius_high = std::max(outer_radius_, radius_low * 1.01);
		const double ratio = radius_high / radius_low;
		double peak = 0.0;
		for (int i = 0; i < kFluxSamples; ++i) {
			const double fraction = static_cast<double>(i) / static_cast<double>(kFluxSamples - 1);
			peak = std::max(peak, thin_disk_->radiative_flux(radius_low * std::pow(ratio, fraction)));
		}
		flux_peak_ = (peak > 1e-300) ? peak : 1.0;
	}

	void build_torus() noexcept {
		Hydro::FishboneMoncriefConfig<double> config;
		config.mass = mass_;
		config.spin_parameter = spin_;
		config.r_in = static_cast<double>(profile_.torus_inner_radius) * mass_;
		config.r_center = static_cast<double>(profile_.torus_center_radius) * mass_;
		config.gamma = static_cast<double>(profile_.adiabatic_index);
		config.max_density = 1.0;
		config.speed_of_light = 1.0;
		config.gravitational_constant = 1.0;
		torus_.emplace(config);

		max_enthalpy_ = torus_->max_enthalpy();
		torus_valid_ = std::isfinite(max_enthalpy_) && (max_enthalpy_ > 1.0 + 1e-9);
		torus_inner_bound_ = 0.8 * config.r_in;
		if (!torus_valid_) {
			return;
		}

		const double scan_step = 0.25 * config.r_center;
		const double scan_limit = 200.0 * config.r_center;
		double inside_radius = config.r_center;
		double outside_radius = config.r_center;
		bool boundary_found = false;
		while (outside_radius < scan_limit) {
			outside_radius += scan_step;
			if (!torus_->is_inside_torus(outside_radius, kHalfPi)) {
				boundary_found = true;
				break;
			}
			inside_radius = outside_radius;
		}
		if (boundary_found) {
			for (int iteration = 0; iteration < 48; ++iteration) {
				const double middle = 0.5 * (inside_radius + outside_radius);
				if (torus_->is_inside_torus(middle, kHalfPi)) {
					inside_radius = middle;
				} else {
					outside_radius = middle;
				}
			}
			torus_outer_bound_ = 1.05 * outside_radius;
		} else {
			torus_outer_bound_ = scan_limit;
		}
		torus_closed_ = boundary_found;
	}

public:
	HydroDiskContext(const GpuHydroDiskProfile& profile, double mass, double spin, double inner_radius, double outer_radius) noexcept
		: profile_(profile),
		  mass_(std::max(mass, 1e-9)),
		  spin_(std::clamp(spin, -0.999 * std::max(mass, 1e-9), 0.999 * std::max(mass, 1e-9))),
		  inner_radius_(std::max(inner_radius, 1e-9)),
		  outer_radius_(std::max(outer_radius, std::max(inner_radius, 1e-9) * 1.0001)) {
		switch (static_cast<HydroDiskModel>(profile_.model)) {
			case HydroDiskModel::NovikovThorne:
				build_thin_disk();
				break;
			case HydroDiskModel::FishboneMoncrief:
				build_torus();
				break;
			case HydroDiskModel::ThinProcedural:
			default:
				break;
		}
	}

	[[nodiscard]] bool is_volumetric() const noexcept {
		return profile_.model == static_cast<uint32_t>(HydroDiskModel::FishboneMoncrief);
	}

	[[nodiscard]] bool torus_valid() const noexcept { return torus_valid_; }
	[[nodiscard]] bool torus_closed() const noexcept { return torus_closed_; }
	[[nodiscard]] double max_enthalpy() const noexcept { return max_enthalpy_; }
	[[nodiscard]] double torus_outer_radius() const noexcept { return torus_outer_bound_ / 1.05; }
	[[nodiscard]] double torus_outer_bound() const noexcept { return torus_outer_bound_; }
	[[nodiscard]] double torus_inner_bound() const noexcept { return torus_inner_bound_; }
	[[nodiscard]] double torus_specific_angular_momentum() const noexcept { return torus_.has_value() ? torus_->specific_angular_momentum() : 0.0; }
	[[nodiscard]] double torus_potential_inner() const noexcept { return torus_.has_value() ? torus_->w_in() : 0.0; }
	[[nodiscard]] double thin_disk_flux_peak() const noexcept { return flux_peak_; }
	[[nodiscard]] double thin_disk_rate_factor() const noexcept { return rate_factor_; }

	[[nodiscard]] double isco_radius() const noexcept {
		return thin_disk_.has_value() ? thin_disk_->isco_radius() : 0.0;
	}

	[[nodiscard]] double marginally_bound_radius() const noexcept {
		return thin_disk_.has_value() ? thin_disk_->marginally_bound_radius() : 0.0;
	}

	[[nodiscard]] double radiative_efficiency() const noexcept {
		return thin_disk_.has_value() ? thin_disk_->radiative_efficiency() : 0.0;
	}

	[[nodiscard]] double thin_disk_base_temperature(double radius) const noexcept {
		if (!thin_disk_.has_value()) {
			return -1.0;
		}
		const double flux = thin_disk_->radiative_flux(radius);
		return std::pow(std::max(flux / flux_peak_, 0.0), 0.25) * rate_factor_;
	}

	[[nodiscard]] DiskShadingResult shade_thin_disk(const GpuDiskProfile& disk, DiskSurfacePoint point) const noexcept {
		point.base_temperature = thin_disk_base_temperature(point.radius);
		return AccretionDiskModel::shade(disk, point);
	}

	[[nodiscard]] HydroSegmentResult integrate_segment(
		const GpuDiskProfile& disk,
		const std::array<double, 3>& from,
		const std::array<double, 3>& to,
		double angular_momentum_ratio
	) const noexcept {
		HydroSegmentResult result;
		if (!torus_valid_ || !torus_.has_value()) {
			return result;
		}

		const double dx = to[0] - from[0];
		const double dy = to[1] - from[1];
		const double dz = to[2] - from[2];
		const double length_sq = dx * dx + dy * dy + dz * dz;
		if (length_sq < 1e-24) {
			return result;
		}
		const double length = std::sqrt(length_sq);

		const double b = (from[0] * dx + from[1] * dy + from[2] * dz) / length_sq;
		const double c = (from[0] * from[0] + from[1] * from[1] + from[2] * from[2] - torus_outer_bound_ * torus_outer_bound_) / length_sq;
		const double discriminant = b * b - c;
		if (discriminant <= 0.0) {
			return result;
		}
		const double root = std::sqrt(discriminant);
		const double t_begin = std::clamp(-b - root, 0.0, 1.0);
		const double t_end = std::clamp(-b + root, 0.0, 1.0);
		if (t_end <= t_begin) {
			return result;
		}

		const double clipped_length = length * (t_end - t_begin);
		const double density = std::max(static_cast<double>(profile_.sampling_density), 0.1);
		const int sample_count = std::clamp(static_cast<int>(std::ceil(clipped_length * density / mass_)), 1, kMaxSegmentSamples);
		const double step_length = clipped_length / static_cast<double>(sample_count);

		const double kappa = static_cast<double>(profile_.optical_depth_scale) * std::max(static_cast<double>(disk.opacity_scale), 0.0);
		const double exponent = 1.0 / std::max(static_cast<double>(profile_.adiabatic_index) - 1.0, 1e-3);
		const double enthalpy_span = std::max(max_enthalpy_ - 1.0, 1e-12);
		const double peak_temperature = static_cast<double>(disk.peak_temperature_k);
		const double floor_temperature = static_cast<double>(disk.floor_temperature_k);
		const double beaming = static_cast<double>(disk.extra_beaming_exponent);
		const double brightness = static_cast<double>(disk.brightness);
		const double reference_luminance = static_cast<double>(disk.reference_luminance);

		double transmittance = 1.0;
		double weight_sum = 0.0;
		double redshift_sum = 0.0;
		for (int i = 0; i < sample_count; ++i) {
			const double t = t_begin + (t_end - t_begin) * ((static_cast<double>(i) + 0.5) / static_cast<double>(sample_count));
			const double px = from[0] + t * dx;
			const double py = from[1] + t * dy;
			const double pz = from[2] + t * dz;
			const double radius = std::sqrt(px * px + py * py + pz * pz);
			if (radius < torus_inner_bound_ || radius > torus_outer_bound_) {
				continue;
			}

			const double theta = std::acos(std::clamp(pz / radius, -1.0, 1.0));
			const double enthalpy = torus_->specific_enthalpy(radius, theta);
			if (!(enthalpy > 1.0 + 1e-9)) {
				continue;
			}
			const double ratio = std::clamp((enthalpy - 1.0) / enthalpy_span, 0.0, 1.0);
			const double optical_depth = kappa * std::pow(ratio, exponent) * step_length / mass_;
			if (optical_depth < 1e-12) {
				continue;
			}

			const auto velocity = torus_->four_velocity(radius, theta);
			if (velocity(0) <= 1e-12) {
				continue;
			}
			DiskOrbitalFrame frame;
			frame.angular_velocity = velocity(3) / velocity(0);
			frame.time_dilation = velocity(0);
			const double redshift = AccretionDiskModel::redshift_factor(frame, angular_momentum_ratio);

			const double temperature = floor_temperature + (peak_temperature - floor_temperature) * ratio;
			const auto linear = AccretionDiskModel::blackbody_linear_rgb(temperature * redshift, reference_luminance);
			const auto graded = AccretionDiskModel::grade_emission(disk, linear, brightness * std::pow(redshift, beaming));

			const double step_transmittance = std::exp(-optical_depth);
			const double weight = transmittance * (1.0 - step_transmittance);
			result.radiance[0] += weight * graded[0];
			result.radiance[1] += weight * graded[1];
			result.radiance[2] += weight * graded[2];
			weight_sum += weight;
			redshift_sum += weight * redshift;
			transmittance *= step_transmittance;
			if (transmittance < 0.002) {
				break;
			}
		}

		result.opacity = 1.0 - transmittance;
		result.redshift = (weight_sum > 1e-30) ? (redshift_sum / weight_sum) : 1.0;
		return result;
	}
};

class HydroDiskShader {
public:
	[[nodiscard]] static bool requires_scalar_pipeline(const GpuHydroDiskProfile& profile) noexcept {
		return profile.model != static_cast<uint32_t>(HydroDiskModel::ThinProcedural);
	}

	[[nodiscard]] static const HydroDiskContext& context(
		const GpuHydroDiskProfile& profile,
		double mass,
		double spin,
		double inner_radius,
		double outer_radius
	) noexcept {
		struct Cache {
			GpuHydroDiskProfile profile{};
			double mass{-1.0};
			double spin{0.0};
			double inner_radius{0.0};
			double outer_radius{0.0};
			std::optional<HydroDiskContext> context{};
		};
		thread_local Cache cache;
		if (!cache.context.has_value() || !(cache.profile == profile) || cache.mass != mass || cache.spin != spin
			|| cache.inner_radius != inner_radius || cache.outer_radius != outer_radius) {
			cache.profile = profile;
			cache.mass = mass;
			cache.spin = spin;
			cache.inner_radius = inner_radius;
			cache.outer_radius = outer_radius;
			cache.context.emplace(profile, mass, spin, inner_radius, outer_radius);
		}
		return *cache.context;
	}

	[[nodiscard]] static std::pair<double, double> primary_disk_bounds(double mass, double spin, float inner_radius_scale, float outer_radius_mass_units) noexcept {
		const double safe_mass = std::max(mass, 1e-4);
		const double safe_spin = std::clamp(spin, -0.999 * safe_mass, 0.999 * safe_mass);
		const double horizon = (std::abs(safe_spin) > 1e-12)
			? (safe_mass + std::sqrt(std::max(safe_mass * safe_mass - safe_spin * safe_spin, 0.0)))
			: (2.0 * safe_mass);
		const double isco = (std::abs(safe_spin) > 1e-12) ? std::max(horizon * 1.05, 6.0 * safe_mass - 4.0 * safe_spin) : (6.0 * safe_mass);
		const double inner = isco * static_cast<double>(std::max(inner_radius_scale, 1.0f));
		const double outer = std::max(static_cast<double>(outer_radius_mass_units) * safe_mass, inner * 1.05);
		return {inner, outer};
	}

	static void resolve_gpu_profile(GpuCameraPushConstants& push) noexcept {
		GpuHydroDiskProfile base{};
		base.model = push.hydro_disk.model;
		base.accretion_rate_scale = push.hydro_disk.accretion_rate_scale;
		base.torus_inner_radius = push.hydro_disk.torus_inner_radius;
		base.torus_center_radius = push.hydro_disk.torus_center_radius;
		base.adiabatic_index = push.hydro_disk.adiabatic_index;
		base.optical_depth_scale = push.hydro_disk.optical_depth_scale;
		base.sampling_density = push.hydro_disk.sampling_density;

		GpuHydroDiskProfile resolved = base;
		if (base.model != static_cast<uint32_t>(HydroDiskModel::ThinProcedural)) {
			const double mass = std::max(push.metric_mass, 1e-4);
			const bool spin_active = (push.metric_type == 2U || push.metric_type == 5U);
			const double spin = spin_active ? std::clamp(push.metric_spin, -0.999 * mass, 0.999 * mass) : 0.0;
			const auto bounds = primary_disk_bounds(mass, spin, push.primary_disk.inner_radius_scale, push.primary_disk.outer_radius_mass_units);
			const HydroDiskContext& hydro = context(base, mass, spin, bounds.first, bounds.second);
			if (base.model == static_cast<uint32_t>(HydroDiskModel::NovikovThorne)) {
				resolved.flux_peak = static_cast<float>(hydro.thin_disk_flux_peak());
				resolved.rate_factor = static_cast<float>(hydro.thin_disk_rate_factor());
			} else if (hydro.torus_valid()) {
				resolved.flags |= HydroDiskFlags::TORUS_VALID;
				resolved.specific_angular_momentum = static_cast<float>(hydro.torus_specific_angular_momentum());
				resolved.potential_inner = static_cast<float>(hydro.torus_potential_inner());
				resolved.max_enthalpy = static_cast<float>(hydro.max_enthalpy());
				resolved.inner_bound = static_cast<float>(hydro.torus_inner_bound());
				resolved.outer_bound = static_cast<float>(hydro.torus_outer_bound());
				push.primary_disk.outer_radius_mass_units = std::max(push.primary_disk.outer_radius_mass_units, static_cast<float>(hydro.torus_outer_bound() / mass));
			}
		}
		push.hydro_disk = resolved;
	}

	[[nodiscard]] static std::pair<double, double> equilibrium_torus_radii(double mass, double spin) noexcept {
		const double safe_mass = std::max(mass, 1e-9);
		Hydro::NovikovThorneConfig<double> config;
		config.mass = safe_mass;
		config.spin_parameter = std::clamp(spin, -0.999 * safe_mass, 0.999 * safe_mass);
		config.speed_of_light = 1.0;
		config.gravitational_constant = 1.0;
		const Hydro::NovikovThorneDisk<double> disk(config);
		const double isco = disk.isco_radius() / safe_mass;
		return {isco, 2.0 * isco};
	}
};

}
