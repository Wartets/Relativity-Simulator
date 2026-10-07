#pragma once

#include "relativistic/render/gpu_types.hpp"
#include "relativistic/optics/cie_observer.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>

namespace Relativistic::Render {

struct DiskSurfacePoint {
	double mass{1.0};
	double spin{0.0};
	double inner_radius{6.0};
	double outer_radius{24.0};
	double radius{6.0};
	double azimuth{0.0};
	double redshift{1.0};
	double time{0.0};
	double detail_scale{1.0};
};

struct DiskShadingResult {
	std::array<float, 3> radiance{0.0f, 0.0f, 0.0f};
	double opacity{0.0};
};

struct DiskOrbitalFrame {
	double angular_velocity{0.0};
	double time_dilation{1.0};
};

class AccretionDiskModel {
private:
	static constexpr size_t kSpectralSamples = 32;
	static constexpr double kLambdaStartNm = 380.0;
	static constexpr double kLambdaEndNm = 780.0;
	static constexpr double kPlanckSecondRadiationMicrometerKelvin = 14387.76877;
	static constexpr double kMinimumTemperatureK = 300.0;
	static constexpr double kMaximumTemperatureK = 400000.0;
	static constexpr double kTwoPi = 2.0 * std::numbers::pi_v<double>;

	struct SpectralTable {
		std::array<double, kSpectralSamples + 1> wavelength_micrometers{};
		std::array<std::array<double, 3>, kSpectralSamples + 1> weights{};
	};

	[[nodiscard]] static const SpectralTable& spectral_table() noexcept {
		static const SpectralTable table = [] {
			SpectralTable result;
			const double step = (kLambdaEndNm - kLambdaStartNm) / static_cast<double>(kSpectralSamples);
			for (size_t i = 0; i <= kSpectralSamples; ++i) {
				const double lambda_nm = kLambdaStartNm + static_cast<double>(i) * step;
				const double trapezoid = (i == 0 || i == kSpectralSamples) ? 0.5 : 1.0;
				result.wavelength_micrometers[i] = lambda_nm * 1e-3;
				result.weights[i] = {
					trapezoid * Optics::CIE1931Observer::x_bar(lambda_nm),
					trapezoid * Optics::CIE1931Observer::y_bar(lambda_nm),
					trapezoid * Optics::CIE1931Observer::z_bar(lambda_nm)
				};
			}
			return result;
		}();
		return table;
	}

	[[nodiscard]] static std::array<double, 3> integrate_xyz(double temperature_k) noexcept {
		const auto& table = spectral_table();
		const double temperature = std::clamp(temperature_k, kMinimumTemperatureK, kMaximumTemperatureK);
		std::array<double, 3> xyz{0.0, 0.0, 0.0};
		for (size_t i = 0; i <= kSpectralSamples; ++i) {
			const double lambda_um = table.wavelength_micrometers[i];
			const double exponent = kPlanckSecondRadiationMicrometerKelvin / (lambda_um * temperature);
			if (exponent > 80.0) {
				continue;
			}
			const double lambda_squared = lambda_um * lambda_um;
			const double radiance = 1.0 / (lambda_squared * lambda_squared * lambda_um * std::expm1(exponent));
			for (size_t c = 0; c < 3; ++c) {
				xyz[c] += radiance * table.weights[i][c];
			}
		}
		return xyz;
	}

	[[nodiscard]] static double temperature_shape(double u, double exponent, double zero_torque) noexcept {
		const double boundary = std::max(1.0 - zero_torque * std::sqrt(u), 0.0);
		return std::pow(u, exponent) * std::pow(boundary, 0.25);
	}

	[[nodiscard]] static double temperature_shape_peak(double exponent, double zero_torque) noexcept {
		double low = 1e-4;
		double high = 1.0;
		for (int iteration = 0; iteration < 48; ++iteration) {
			const double a = low + (high - low) / 3.0;
			const double b = high - (high - low) / 3.0;
			if (temperature_shape(a, exponent, zero_torque) < temperature_shape(b, exponent, zero_torque)) {
				low = a;
			} else {
				high = b;
			}
		}
		return std::max(temperature_shape(0.5 * (low + high), exponent, zero_torque), temperature_shape(1.0, exponent, zero_torque));
	}

	[[nodiscard]] static double smooth_unit(double x) noexcept {
		const double t = std::clamp(x, 0.0, 1.0);
		return t * t * (3.0 - 2.0 * t);
	}

	[[nodiscard]] static double reduce_angle(double angle) noexcept {
		return angle - kTwoPi * std::floor(angle / kTwoPi);
	}

	[[nodiscard]] static constexpr uint32_t hash_u32(uint32_t x) noexcept {
		x ^= x >> 16;
		x *= 0x7feb352dU;
		x ^= x >> 15;
		x *= 0x846ca68bU;
		x ^= x >> 16;
		return x;
	}

	[[nodiscard]] static float lattice_value(int32_t x, int32_t y, int32_t z, uint32_t seed) noexcept {
		const uint32_t h = static_cast<uint32_t>(x) * 374761393U + static_cast<uint32_t>(y) * 668265263U + static_cast<uint32_t>(z) * 2147483647U + seed * 1274126177U;
		return static_cast<float>(hash_u32(h) & 0x00FFFFFFU) * (1.0f / 16777216.0f) * 2.0f - 1.0f;
	}

	[[nodiscard]] static float value_noise(float x, float y, float z, uint32_t seed) noexcept {
		const float floor_x = std::floor(x);
		const float floor_y = std::floor(y);
		const float floor_z = std::floor(z);
		const int32_t xi = static_cast<int32_t>(floor_x);
		const int32_t yi = static_cast<int32_t>(floor_y);
		const int32_t zi = static_cast<int32_t>(floor_z);
		const float fx = x - floor_x;
		const float fy = y - floor_y;
		const float fz = z - floor_z;
		const float ux = fx * fx * (3.0f - 2.0f * fx);
		const float uy = fy * fy * (3.0f - 2.0f * fy);
		const float uz = fz * fz * (3.0f - 2.0f * fz);

		const float c000 = lattice_value(xi, yi, zi, seed);
		const float c100 = lattice_value(xi + 1, yi, zi, seed);
		const float c010 = lattice_value(xi, yi + 1, zi, seed);
		const float c110 = lattice_value(xi + 1, yi + 1, zi, seed);
		const float c001 = lattice_value(xi, yi, zi + 1, seed);
		const float c101 = lattice_value(xi + 1, yi, zi + 1, seed);
		const float c011 = lattice_value(xi, yi + 1, zi + 1, seed);
		const float c111 = lattice_value(xi + 1, yi + 1, zi + 1, seed);

		const float c00 = c000 + (c100 - c000) * ux;
		const float c10 = c010 + (c110 - c010) * ux;
		const float c01 = c001 + (c101 - c001) * ux;
		const float c11 = c011 + (c111 - c011) * ux;
		const float c0 = c00 + (c10 - c00) * uy;
		const float c1 = c01 + (c11 - c01) * uy;
		return c0 + (c1 - c0) * uz;
	}

	[[nodiscard]] static float fractal_noise(float x, float y, float z, int octaves, uint32_t seed) noexcept {
		float total = 0.0f;
		float amplitude = 1.0f;
		float frequency = 1.0f;
		float normalization = 0.0f;
		for (int octave = 0; octave < octaves; ++octave) {
			total += value_noise(x * frequency, y * frequency, z * frequency, seed + static_cast<uint32_t>(octave) * 7919U) * amplitude;
			normalization += amplitude;
			amplitude *= 0.5f;
			frequency *= 2.0f;
		}
		return total / std::max(normalization, 1e-5f);
	}

public:
	static void finalize(GpuDiskProfile& profile) noexcept {
		profile.peak_temperature_k = std::clamp(profile.peak_temperature_k, 1000.0f, 200000.0f);
		profile.floor_temperature_k = std::clamp(profile.floor_temperature_k, 0.0f, profile.peak_temperature_k);
		const double peak = temperature_shape_peak(static_cast<double>(profile.temperature_exponent), static_cast<double>(profile.zero_torque_strength));
		profile.temperature_normalization = static_cast<float>(1.0 / std::max(peak, 1e-6));
		profile.reference_luminance = static_cast<float>(std::max(integrate_xyz(static_cast<double>(profile.peak_temperature_k))[1], 1e-30));
	}

	[[nodiscard]] static GpuDiskProfile resolve_primary_profile(const GpuCameraPushConstants& params) noexcept {
		GpuDiskProfile profile = params.primary_disk;
		profile.peak_temperature_k = static_cast<float>(params.disk_temperature_scale_k);
		profile.floor_temperature_k = static_cast<float>(params.disk_temperature_floor_k);
		profile.color_saturation = static_cast<float>(params.disk_color_saturation);
		profile.extra_beaming_exponent = static_cast<float>(params.disk_doppler_beaming_exponent);
		finalize(profile);
		return profile;
	}

	[[nodiscard]] static std::array<double, 3> blackbody_linear_rgb(double temperature_k, double reference_luminance) noexcept {
		const auto xyz = integrate_xyz(temperature_k);
		const double inverse_reference = 1.0 / std::max(reference_luminance, 1e-30);
		const auto rgb = Optics::CIE1931Observer::xyz_to_linear_srgb(Optics::ColorXYZ{xyz[0] * inverse_reference, xyz[1] * inverse_reference, xyz[2] * inverse_reference});
		return {rgb.r, rgb.g, rgb.b};
	}

	[[nodiscard]] static DiskOrbitalFrame orbital_frame(double mass, double spin, double radius) noexcept {
		const double spin_magnitude = std::abs(spin);
		const double sqrt_mass = std::sqrt(std::max(mass, 1e-12));
		const double sqrt_radius = std::sqrt(std::max(radius, 1e-12));
		const double radius_pow_1_5 = radius * sqrt_radius;
		const double angular = sqrt_mass / (radius_pow_1_5 + spin_magnitude * sqrt_mass);
		const double bracket = std::max(radius_pow_1_5 - 3.0 * mass * sqrt_radius + 2.0 * spin_magnitude * sqrt_mass, 1e-9);
		DiskOrbitalFrame frame;
		frame.angular_velocity = (spin < 0.0) ? -angular : angular;
		frame.time_dilation = (radius_pow_1_5 + spin_magnitude * sqrt_mass) / (std::sqrt(radius_pow_1_5) * std::sqrt(bracket));
		return frame;
	}

	[[nodiscard]] static double redshift_factor(const DiskOrbitalFrame& frame, double angular_momentum_ratio) noexcept {
		const double denominator = frame.time_dilation * (1.0 - frame.angular_velocity * angular_momentum_ratio);
		return (std::abs(denominator) > 1e-9) ? std::clamp(1.0 / denominator, 0.05, 4.0) : 1.0;
	}

	[[nodiscard]] static double redshift_at(double mass, double spin, double radius, double angular_momentum_ratio) noexcept {
		return redshift_factor(orbital_frame(mass, spin, radius), angular_momentum_ratio);
	}

	[[nodiscard]] static DiskShadingResult shade(const GpuDiskProfile& profile, const DiskSurfacePoint& point) noexcept {
		DiskShadingResult result;
		const double mass = std::max(point.mass, 1e-9);
		const double radius = std::max(point.radius, 1e-9);
		const double inner = std::max(point.inner_radius, 1e-9);
		const double outer = std::max(point.outer_radius, inner * 1.0001);

		const double u = std::clamp(inner / radius, 1e-4, 1.0);
		const double base = temperature_shape(u, static_cast<double>(profile.temperature_exponent), static_cast<double>(profile.zero_torque_strength)) * static_cast<double>(profile.temperature_normalization);

		const double softness = std::max(static_cast<double>(profile.edge_softness), 0.05);
		const double envelope = smooth_unit((radius - inner) / (0.8 * softness * mass)) * smooth_unit((outer - radius) / (1.5 * softness * mass));

		const double rotation = static_cast<double>(profile.rotation_speed_scale) * point.time;
		const double shear_phase = reduce_angle(point.azimuth - orbital_frame(mass, point.spin, radius).angular_velocity * rotation);
		const double pattern_phase = reduce_angle(point.azimuth - orbital_frame(mass, point.spin, std::sqrt(inner * outer)).angular_velocity * rotation);

		const float log_ratio = static_cast<float>(std::log(std::max(radius / inner, 1.0)));
		const float log_span = std::max(static_cast<float>(std::log(std::max(outer / inner, 1.0001))), 1e-4f);
		const float radial_coordinate = log_ratio / log_span;
		constexpr float two_pi = 2.0f * std::numbers::pi_v<float>;

		const float ring_wave = 0.5f + 0.5f * std::cos(two_pi * profile.ring_frequency * radial_coordinate);
		const float ring_sharpness = std::max(profile.ring_sharpness, 1.0f);
		const float ring = std::max(1.0f + profile.ring_amplitude * (std::pow(ring_wave, ring_sharpness) * std::sqrt(std::numbers::pi_v<float> * ring_sharpness) - 1.0f), 0.0f);

		const float arms = std::floor(profile.spiral_arm_count + 0.5f);
		const float spiral = (arms > 0.5f)
			? std::max(1.0f + profile.spiral_amplitude * std::cos(arms * (static_cast<float>(pattern_phase) - profile.spiral_pitch * log_ratio)), 0.0f)
			: 1.0f;

		const float shear_cos = std::cos(static_cast<float>(shear_phase));
		const float shear_sin = std::sin(static_cast<float>(shear_phase));
		const int octaves = std::clamp(static_cast<int>(profile.turbulence_octaves + 0.5f), 1, 6);
		const uint32_t seed = static_cast<uint32_t>(std::max(profile.noise_seed, 0.0f));
		const float detail = static_cast<float>(std::clamp(point.detail_scale, 0.0, 1.0));

		const float turbulence_noise = fractal_noise(
			log_ratio * profile.turbulence_scale * profile.radial_stretch,
			shear_cos * profile.turbulence_scale,
			shear_sin * profile.turbulence_scale,
			octaves,
			seed
		);
		const float turbulence = std::exp(1.5f * profile.turbulence_amplitude * detail * turbulence_noise);

		const float grain_noise = value_noise(
			log_ratio * profile.grain_scale * profile.radial_stretch,
			shear_cos * profile.grain_scale,
			shear_sin * profile.grain_scale,
			seed + 104729U
		);
		const float grain = std::max(1.0f + profile.grain_amplitude * detail * grain_noise, 0.0f);

		const float flux_modulation = std::clamp(ring * spiral * turbulence * grain, 0.0f, 8.0f);

		const double temperature_span = std::max(static_cast<double>(profile.peak_temperature_k) - static_cast<double>(profile.floor_temperature_k), 0.0);
		const double local_temperature = (static_cast<double>(profile.floor_temperature_k) + temperature_span * base) * std::pow(std::max(static_cast<double>(flux_modulation), 1e-4), 0.25);
		const double redshift = std::max(point.redshift, 1e-3);
		const double observed_temperature = local_temperature * redshift;

		const auto linear = blackbody_linear_rgb(observed_temperature, static_cast<double>(profile.reference_luminance));
		const double gain = static_cast<double>(profile.brightness) * std::pow(redshift, static_cast<double>(profile.extra_beaming_exponent)) * envelope;
		double red = linear[0] * gain;
		double green = linear[1] * gain;
		double blue = linear[2] * gain;

		const double luma = 0.2126 * red + 0.7152 * green + 0.0722 * blue;
		const double saturation = static_cast<double>(profile.color_saturation);
		red = std::max(0.0, luma + (red - luma) * saturation);
		green = std::max(0.0, luma + (green - luma) * saturation);
		blue = std::max(0.0, luma + (blue - luma) * saturation);

		const double tint_strength = static_cast<double>(profile.tint_strength);
		red *= 1.0 + (static_cast<double>(profile.tint_r) - 1.0) * tint_strength;
		green *= 1.0 + (static_cast<double>(profile.tint_g) - 1.0) * tint_strength;
		blue *= 1.0 + (static_cast<double>(profile.tint_b) - 1.0) * tint_strength;

		result.radiance = {static_cast<float>(red), static_cast<float>(green), static_cast<float>(blue)};
		result.opacity = std::clamp(envelope * 0.95 * static_cast<double>(profile.opacity_scale) * std::clamp(std::sqrt(std::min(static_cast<double>(flux_modulation), 1.0)), 0.15, 1.0), 0.0, 0.98);
		return result;
	}
};

}
