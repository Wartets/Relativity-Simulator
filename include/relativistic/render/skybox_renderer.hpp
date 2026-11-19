#pragma once

#include "relativistic/optics/textures/sky_panorama_image.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <algorithm>
#include <string_view>

namespace Relativistic::Render {

struct ProceduralSkySettings {
	bool enabled{true};
	bool seed_locked{true};
	uint32_t seed{1337U};
	double master_intensity{1.0};

	double quality_factor{1.0};
	bool adaptive_fov_lod{false};
	double adaptive_fov_reference{60.0};
	uint32_t max_star_octaves{4};

	bool dynamic_motion{false};
	double motion_speed{0.05};
	double time_seconds{0.0};

	bool stars_enabled{true};
	double star_density{1.0};
	double star_brightness{1.0};
	double star_size_min{0.0003};
	double star_size_max{0.0025};
	uint32_t star_size_distribution{1};
	double star_size_param{2.35};
	uint32_t star_luminosity_distribution{1};
	double star_luminosity_param{1.8};
	double star_color_variation{1.0};
	double star_temperature_bias{0.0};
	double star_brightness_variation{0.67};
	double star_scintillation{0.0};
	double star_magnitude_limit{12.0};

	bool nebula_enabled{true};
	double nebula_intensity{1.0};
	double nebula_roughness{0.55};
	uint32_t nebula_octaves{5};
	double nebula_frequency{2.4};
	std::array<double, 3> nebula_plane_normal{0.0, 0.4472135955, 0.894427191};
	double nebula_plane_thickness{0.25};
	double nebula_plane_falloff{2.0};
	double nebula_asymmetry{0.4};
	std::array<double, 3> nebula_asymmetry_dir{0.7071067812, 0.0, 0.7071067812};
	std::array<float, 3> nebula_emission_color{0.88f, 0.38f, 0.68f};
	std::array<float, 3> nebula_secondary_color{0.22f, 0.58f, 0.95f};
	std::array<float, 3> nebula_tertiary_color{0.15f, 0.85f, 0.55f};
	double nebula_absorption_density{0.7};
	uint32_t nebula_layer_count{3};
	double nebula_layer_spread{0.6};
	double nebula_core_glow{0.5};
	double nebula_curl_strength{0.3};

	bool galaxies_enabled{true};
	double galaxy_density{0.12};
	double galaxy_brightness{0.5};
	double galaxy_size_scale{0.25};
	double galaxy_size_min{0.001};
	double galaxy_size_max{0.02};
	uint32_t galaxy_size_distribution{1};
	double galaxy_size_param{1.5};
	double galaxy_spiral_arms{2.0};
	double galaxy_core_ratio{0.4};
	double galaxy_pitch_angle{1.2};
	double galaxy_ellipticity_variance{0.5};
	uint32_t galaxy_type_mix{0};

	bool clusters_enabled{true};
	double cluster_density{0.8};
	double cluster_brightness{1.0};
	double cluster_size_scale{0.6};
	double cluster_size_min{0.002};
	double cluster_size_max{0.03};
	uint32_t cluster_size_distribution{2};
	double cluster_concentration{1.6};
	double cluster_star_density_boost{2.0};
	std::array<float, 3> cluster_core_tint{1.1f, 1.0f, 0.8f};

	bool dust_enabled{true};
	double dust_density{2.5};
	double dust_intensity{1.8};
	double dust_scale{1.6};
	double dust_absorption_ratio{0.75};
	uint32_t dust_fractal_octaves{4};
	double dust_contrast{1.5};
	std::array<float, 3> dust_tint{0.4f, 0.25f, 0.15f};

	bool exotic_objects_enabled{true};
	double quasar_density{0.03};
	double quasar_brightness{3.0};
	double quasar_core_size{0.0005};
	double pulsar_pulse_rate{2.5};
	double pulsar_pulse_duty{0.25};
	bool snr_enabled{true};
	double snr_density{0.02};
	double snr_brightness{1.5};
	double snr_shell_thickness{0.2};

	bool grid_enabled{false};
	double grid_opacity{1.0};
	double grid_line_width{0.002};
	double grid_spacing_deg{15.0};
	std::array<float, 3> grid_major_color{0.25f, 0.50f, 0.95f};
	std::array<float, 3> grid_minor_color{0.10f, 0.20f, 0.40f};
	bool grid_show_polar_caps{true};
	bool grid_show_equator{true};
	bool grid_show_meridians{true};

	std::array<float, 3> background_color{0.0f, 0.0f, 0.0f};
	double hue_shift_deg{-15.7};
	double saturation{0.84};
	double orientation_yaw_deg{56.0};
	double orientation_pitch_deg{0.0};
	double orientation_roll_deg{0.0};

	[[nodiscard]] bool operator==(const ProceduralSkySettings&) const noexcept = default;
};

class SkyboxRenderer {
public:
	struct SphericalPartition {
		static constexpr double kPi = std::numbers::pi_v<double>;
		static constexpr double kTwoPi = 2.0 * std::numbers::pi_v<double>;

		[[nodiscard]] static constexpr uint64_t hash_coords(uint32_t r, uint32_t s, uint64_t seed) noexcept {
			uint64_t h = seed ^ (static_cast<uint64_t>(r) * 0x9e3779b97f4a7c15ULL) ^ (static_cast<uint64_t>(s) * 0xbf58476d1ce4e5b9ULL);
			h ^= h >> 30;
			h *= 0xbf58476d1ce4e5b9ULL;
			h ^= h >> 27;
			h *= 0x94d049bb133111ebULL;
			h ^= h >> 31;
			return h;
		}

		[[nodiscard]] static constexpr double hash_to_unit(uint64_t h) noexcept {
			return static_cast<double>(h & 0x001FFFFFFFFFFFFFULL) * (1.0 / 9007199254740992.0);
		}

		[[nodiscard]] static uint32_t sector_count_for_ring(uint32_t ring, uint32_t total_rings) noexcept {
			const double z_c = -1.0 + (2.0 * static_cast<double>(ring) + 1.0) / static_cast<double>(total_rings);
			const double sin_t = std::sqrt(std::max(1.0 - z_c * z_c, 1.0e-6));
			const uint32_t count = static_cast<uint32_t>(std::max(1.0, std::round(static_cast<double>(total_rings) * kPi * 0.5 * sin_t)));
			return count;
		}
	};

	[[nodiscard]] static double sample_distribution(uint32_t dist_type, double min_val, double max_val, double shape_param, uint64_t hash_val) noexcept {
		const double u = SphericalPartition::hash_to_unit(hash_val);
		if (min_val >= max_val) return min_val;
		switch (dist_type) {
			case 1: {
				const double alpha = std::max(shape_param, 0.05);
				if (std::abs(alpha - 1.0) < 1.0e-4) {
					return min_val * std::pow(max_val / min_val, u);
				}
				const double one_minus_a = 1.0 - alpha;
				const double term = (1.0 - u) * std::pow(min_val, one_minus_a) + u * std::pow(max_val, one_minus_a);
				return std::clamp(std::pow(std::max(term, 1.0e-30), 1.0 / one_minus_a), min_val, max_val);
			}
			case 2: {
				const double lambda = std::max(shape_param, 0.1);
				const double norm = 1.0 - std::exp(-lambda * (max_val - min_val));
				return min_val - std::log(std::max(1.0 - u * norm, 1.0e-15)) / lambda;
			}
			case 3: {
				const double u2 = std::max(SphericalPartition::hash_to_unit(hash_val ^ 0x94d049bb133111ebULL), 1.0e-15);
				const double z = std::sqrt(-2.0 * std::log(std::max(u, 1.0e-15))) * std::cos(2.0 * std::numbers::pi_v<double> * u2);
				const double mid = 0.5 * (std::log(min_val) + std::log(max_val));
				const double spread = std::max(shape_param, 0.1) * 0.25 * (std::log(max_val) - std::log(min_val));
				return std::clamp(std::exp(mid + spread * z), min_val, max_val);
			}
			case 4: {
				const double u2 = std::max(SphericalPartition::hash_to_unit(hash_val ^ 0x94d049bb133111ebULL), 1.0e-15);
				const double z = std::sqrt(-2.0 * std::log(std::max(u, 1.0e-15))) * std::cos(2.0 * std::numbers::pi_v<double> * u2);
				const double mid = 0.5 * (min_val + max_val);
				const double spread = std::max(shape_param, 0.1) * 0.25 * (max_val - min_val);
				return std::clamp(mid + spread * z, min_val, max_val);
			}
			case 0:
			default:
				return min_val + u * (max_val - min_val);
		}
	}

private:
	[[nodiscard]] static float hash_noise_3d(int32_t x, int32_t y, int32_t z) noexcept {
		int32_t n = x + y * 57 + z * 113;
		n = (n << 13) ^ n;
		return (1.0f - static_cast<float>((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) * (1.0f / 1073741824.0f));
	}

	[[nodiscard]] static float noise3d(float x, float y, float z) noexcept {
		const int32_t xi = static_cast<int32_t>(std::floor(x));
		const int32_t yi = static_cast<int32_t>(std::floor(y));
		const int32_t zi = static_cast<int32_t>(std::floor(z));

		const float fx = x - static_cast<float>(xi);
		const float fy = y - static_cast<float>(yi);
		const float fz = z - static_cast<float>(zi);

		const float u = fx * fx * fx * (fx * (fx * 6.0f - 15.0f) + 10.0f);
		const float v = fy * fy * fy * (fy * (fy * 6.0f - 15.0f) + 10.0f);
		const float w = fz * fz * fz * (fz * (fz * 6.0f - 15.0f) + 10.0f);

		const float c000 = hash_noise_3d(xi, yi, zi);
		const float c100 = hash_noise_3d(xi + 1, yi, zi);
		const float c010 = hash_noise_3d(xi, yi + 1, zi);
		const float c110 = hash_noise_3d(xi + 1, yi + 1, zi);
		const float c001 = hash_noise_3d(xi, yi, zi + 1);
		const float c101 = hash_noise_3d(xi + 1, yi, zi + 1);
		const float c011 = hash_noise_3d(xi, yi + 1, zi + 1);
		const float c111 = hash_noise_3d(xi + 1, yi + 1, zi + 1);

		const float c00 = c000 * (1.0f - u) + c100 * u;
		const float c10 = c010 * (1.0f - u) + c110 * u;
		const float c01 = c001 * (1.0f - u) + c101 * u;
		const float c11 = c011 * (1.0f - u) + c111 * u;

		const float c0 = c00 * (1.0f - v) + c10 * v;
		const float c1 = c01 * (1.0f - v) + c11 * v;

		return c0 * (1.0f - w) + c1 * w;
	}

	[[nodiscard]] static float fbm3d(float x, float y, float z, int octaves, float roughness) noexcept {
		float total = 0.0f;
		float amplitude = 1.0f;
		float frequency = 1.0f;
		float max_val = 0.0f;
		for (int i = 0; i < octaves; ++i) {
			total += noise3d(x * frequency, y * frequency, z * frequency) * amplitude;
			max_val += amplitude;
			amplitude *= roughness;
			frequency *= 2.02f;
		}
		return total / std::max(max_val, 1e-5f);
	}

	[[nodiscard]] static float ridged_fbm3d(float x, float y, float z, int octaves, float roughness) noexcept {
		float total = 0.0f;
		float amplitude = 1.0f;
		float frequency = 1.0f;
		float max_val = 0.0f;
		for (int i = 0; i < octaves; ++i) {
			float val = noise3d(x * frequency, y * frequency, z * frequency);
			val = 1.0f - std::abs(val);
			val *= val;
			total += val * amplitude;
			max_val += amplitude;
			amplitude *= roughness;
			frequency *= 2.05f;
		}
		return total / std::max(max_val, 1e-5f);
	}

	[[nodiscard]] static std::array<double, 3> rotate_vector(const std::array<double, 3>& v, double yaw_deg, double pitch_deg, double roll_deg) noexcept {
		constexpr double deg_to_rad = std::numbers::pi_v<double> / 180.0;
		const double cy = std::cos(yaw_deg * deg_to_rad);
		const double sy = std::sin(yaw_deg * deg_to_rad);
		const double cp = std::cos(pitch_deg * deg_to_rad);
		const double sp = std::sin(pitch_deg * deg_to_rad);
		const double cr = std::cos(roll_deg * deg_to_rad);
		const double sr = std::sin(roll_deg * deg_to_rad);

		const double x1 = v[0] * cy - v[1] * sy;
		const double y1 = v[0] * sy + v[1] * cy;
		const double z1 = v[2];

		const double x2 = x1 * cp + z1 * sp;
		const double y2 = y1;
		const double z2 = -x1 * sp + z1 * cp;

		return {
			x2,
			y2 * cr - z2 * sr,
			y2 * sr + z2 * cr
		};
	}

	[[nodiscard]] static std::array<float, 3> apply_hue_saturation(const std::array<float, 3>& rgb, double hue_shift_rad, double saturation) noexcept {
		if (hue_shift_rad == 0.0 && saturation == 1.0) {
			return rgb;
		}
		const float r = rgb[0], g = rgb[1], b = rgb[2];
		const float max_c = std::max({r, g, b});
		const float min_c = std::min({r, g, b});
		const float delta = max_c - min_c;
		float h = 0.0f;
		if (delta > 1e-8f) {
			if (max_c == r) {
				h = std::fmod((g - b) / delta, 6.0f);
			} else if (max_c == g) {
				h = (b - r) / delta + 2.0f;
			} else {
				h = (r - g) / delta + 4.0f;
			}
			h *= 60.0f;
			if (h < 0.0f) h += 360.0f;
		}
		const float s_val = (max_c > 1e-8f) ? (delta / max_c) : 0.0f;
		const float v_val = max_c;

		h += static_cast<float>(hue_shift_rad * (180.0 / std::numbers::pi_v<double>));
		h = std::fmod(h, 360.0f);
		if (h < 0.0f) h += 360.0f;
		const float s_new = std::clamp(s_val * static_cast<float>(saturation), 0.0f, 1.0f);

		const float c_val = v_val * s_new;
		const float x_val = c_val * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
		const float m_val = v_val - c_val;

		float r2 = 0.0f, g2 = 0.0f, b2 = 0.0f;
		if (h < 60.0f) { r2 = c_val; g2 = x_val; b2 = 0.0f; }
		else if (h < 120.0f) { r2 = x_val; g2 = c_val; b2 = 0.0f; }
		else if (h < 180.0f) { r2 = 0.0f; g2 = c_val; b2 = x_val; }
		else if (h < 240.0f) { r2 = 0.0f; g2 = x_val; b2 = c_val; }
		else if (h < 300.0f) { r2 = x_val; g2 = 0.0f; b2 = c_val; }
		else { r2 = c_val; g2 = 0.0f; b2 = x_val; }

		return {r2 + m_val, g2 + m_val, b2 + m_val};
	}

	[[nodiscard]] static std::array<double, 3> sample_celestial_grid(
		const std::array<double, 3>& dir,
		const ProceduralSkySettings& sky
	) noexcept {
		if (!sky.grid_enabled || sky.grid_opacity <= 0.0) {
			return {0.0, 0.0, 0.0};
		}
		constexpr double kPi = std::numbers::pi_v<double>;
		constexpr double kHalfPi = 0.5 * kPi;
		constexpr double kDegToRad = kPi / 180.0;

		const double dec = std::asin(std::clamp(dir[2], -1.0, 1.0));
		const double ra = std::atan2(dir[1], dir[0]);

		const double spacing_rad = std::max(sky.grid_spacing_deg, 1.0) * kDegToRad;
		const double half_w = std::max(sky.grid_line_width * 0.5, 1.0e-5);
		const double cos_dec = std::max(std::cos(dec), 1.0e-6);

		const double dist_dec = std::abs(std::remainder(dec, spacing_rad));
		const bool on_dec = dist_dec < half_w;

		const double dist_ra_angle = std::abs(std::remainder(ra, spacing_rad));
		const double dist_ra_sphere = dist_ra_angle * cos_dec;
		const double meridian_spacing_sphere = spacing_rad * cos_dec;
		const double pole_fade = std::clamp((meridian_spacing_sphere - 2.5 * half_w) / (3.0 * half_w), 0.0, 1.0);
		const bool on_ra = (dist_ra_sphere < half_w) && (pole_fade > 0.0);

		const bool on_equator = sky.grid_show_equator && (std::abs(dec) < half_w * 1.6);
		const bool on_meridian = sky.grid_show_meridians && (std::abs(std::remainder(ra, kHalfPi)) * cos_dec < half_w * 1.6) && (pole_fade > 0.0);
		const bool on_polar_cap = sky.grid_show_polar_caps && (std::abs(std::abs(dec) - (kHalfPi - spacing_rad)) < half_w);

		if (!on_dec && !on_ra && !on_equator && !on_meridian && !on_polar_cap) {
			return {0.0, 0.0, 0.0};
		}

		const double op = std::clamp(sky.grid_opacity, 0.0, 1.0);
		if (on_equator || on_meridian) {
			return {
				static_cast<double>(sky.grid_major_color[0]) * op * 1.3,
				static_cast<double>(sky.grid_major_color[1]) * op * 1.3,
				static_cast<double>(sky.grid_major_color[2]) * op * 1.3
			};
		}
		const double ra_weight = on_ra ? (op * pole_fade) : op;
		return {
			static_cast<double>(sky.grid_minor_color[0]) * ra_weight,
			static_cast<double>(sky.grid_minor_color[1]) * ra_weight,
			static_cast<double>(sky.grid_minor_color[2]) * ra_weight
		};
	}

	[[nodiscard]] static std::array<float, 3> star_blackbody_tint(double temp_norm, double color_variation) noexcept {
		float sr = 1.0f, sg = 1.0f, sb = 1.0f;
		if (temp_norm < 0.20) {
			sr = 1.55f; sg = 0.65f; sb = 0.35f;
		} else if (temp_norm < 0.45) {
			sr = 1.35f; sg = 1.05f; sb = 0.70f;
		} else if (temp_norm < 0.70) {
			sr = 1.05f; sg = 1.05f; sb = 1.05f;
		} else if (temp_norm < 0.88) {
			sr = 0.80f; sg = 0.95f; sb = 1.35f;
		} else {
			sr = 0.65f; sg = 0.80f; sb = 1.60f;
		}
		const float mix = static_cast<float>(std::clamp(color_variation, 0.0, 3.0));
		return {
			std::max(0.0f, 1.0f + (sr - 1.0f) * mix),
			std::max(0.0f, 1.0f + (sg - 1.0f) * mix),
			std::max(0.0f, 1.0f + (sb - 1.0f) * mix)
		};
	}

	static void evaluate_seamless_spherical_stars(
		const std::array<double, 3>& dir,
		const ProceduralSkySettings& sky,
		double fov_rad,
		double zoom_factor,
		std::array<double, 3>& accum
	) noexcept {
		if (!sky.stars_enabled || sky.star_density <= 0.0) return;

		constexpr double kPi = SphericalPartition::kPi;
		constexpr double kTwoPi = SphericalPartition::kTwoPi;
		const uint64_t base_seed = static_cast<uint64_t>(sky.seed);

		const double density_eff = std::clamp(sky.star_density * sky.quality_factor, 0.02, 10.0);
		const uint32_t total_rings = static_cast<uint32_t>(std::clamp(std::round(24.0 * std::sqrt(density_eff)), 8.0, 128.0));

		const double z_clamped = std::clamp(dir[2], -1.0, 1.0);
		const uint32_t ring_center = std::clamp(static_cast<uint32_t>((z_clamped + 1.0) * 0.5 * static_cast<double>(total_rings)), 0U, total_rings - 1U);
		double phi_dir = std::atan2(dir[1], dir[0]);
		if (phi_dir < 0.0) phi_dir += kTwoPi;

		const double pixel_footprint_rad = fov_rad / 1080.0;
		const double presence_threshold = std::clamp(1.0 - sky.star_density * 0.16, 0.04, 0.99);

		for (int dr = -1; dr <= 1; ++dr) {
			const int r_cand = static_cast<int>(ring_center) + dr;
			if (r_cand < 0 || r_cand >= static_cast<int>(total_rings)) continue;
			const uint32_t r_idx = static_cast<uint32_t>(r_cand);
			const uint32_t s_count = SphericalPartition::sector_count_for_ring(r_idx, total_rings);
			const uint32_t s_center = std::clamp(static_cast<uint32_t>(phi_dir / kTwoPi * static_cast<double>(s_count)), 0U, s_count - 1U);

			for (int ds = -1; ds <= 1; ++ds) {
				const uint32_t s_idx = (s_center + s_count + ds) % s_count;
				const uint64_t cell_hash = SphericalPartition::hash_coords(r_idx, s_idx, base_seed);

				if (SphericalPartition::hash_to_unit(cell_hash) <= presence_threshold) continue;

				const double z_c = -1.0 + (2.0 * static_cast<double>(r_idx) + 1.0) / static_cast<double>(total_rings);
				const double phi_c = kTwoPi * (static_cast<double>(s_idx) + 0.5) / static_cast<double>(s_count);

				const double z_star = std::clamp(z_c + (SphericalPartition::hash_to_unit(cell_hash + 11ULL) - 0.5) * (1.8 / static_cast<double>(total_rings)), -1.0, 1.0);
				const double phi_star = std::remainder(phi_c + (SphericalPartition::hash_to_unit(cell_hash + 23ULL) - 0.5) * (1.8 * kTwoPi / static_cast<double>(s_count)), kTwoPi);
				const double r_xy = std::sqrt(std::max(1.0 - z_star * z_star, 0.0));
				std::array<double, 3> star_dir{r_xy * std::cos(phi_star), r_xy * std::sin(phi_star), z_star};

				if (sky.dynamic_motion) {
					const double drift_rate = sky.motion_speed * 0.015;
					const double phase = SphericalPartition::hash_to_unit(cell_hash + 47ULL) * kTwoPi;
					const double d_angle = std::sin(sky.time_seconds * 0.2 + phase) * drift_rate;
					star_dir = rotate_vector(star_dir, d_angle * (180.0 / kPi), 0.0, 0.0);
				}

				const double mu = dir[0] * star_dir[0] + dir[1] * star_dir[1] + dir[2] * star_dir[2];
				if (mu <= 0.9990) continue;
				const double theta_sq = 2.0 * std::max(0.0, 1.0 - mu);

				const double radius_rad = sample_distribution(sky.star_size_distribution, sky.star_size_min, sky.star_size_max, sky.star_size_param, cell_hash + 31ULL);
				const double eff_sigma = std::sqrt(radius_rad * radius_rad + pixel_footprint_rad * pixel_footprint_rad * 0.25);
				const double eff_sigma_sq = eff_sigma * eff_sigma;
				if (theta_sq > 16.0 * eff_sigma_sq) continue;

				double lum = sample_distribution(sky.star_luminosity_distribution, 0.1, 10.0, sky.star_luminosity_param, cell_hash + 53ULL) * sky.star_brightness;
				if (sky.dynamic_motion && sky.star_scintillation > 0.0) {
					const double tw_phase = SphericalPartition::hash_to_unit(cell_hash + 67ULL) * kTwoPi;
					lum *= (1.0 + sky.star_scintillation * std::sin(sky.time_seconds * 4.2 + tw_phase));
				}
				const double flux_norm = (radius_rad * radius_rad) / eff_sigma_sq;
				const double psf = std::exp(-theta_sq / (2.0 * eff_sigma_sq)) + 0.06 * std::exp(-std::sqrt(theta_sq) / (3.0 * eff_sigma));
				const double total_flux = lum * flux_norm * psf;

				const double temp_norm = std::clamp(SphericalPartition::hash_to_unit(cell_hash + 79ULL) + sky.star_temperature_bias * 0.4, 0.0, 1.0);
				const auto tint = star_blackbody_tint(temp_norm, sky.star_color_variation);
				accum[0] += total_flux * static_cast<double>(tint[0]);
				accum[1] += total_flux * static_cast<double>(tint[1]);
				accum[2] += total_flux * static_cast<double>(tint[2]);
			}
		}

		if (sky.adaptive_fov_lod && zoom_factor > 1.2) {
			const double fine_rings_d = static_cast<double>(total_rings) * 1.8;
			const uint32_t fine_rings = static_cast<uint32_t>(std::clamp(fine_rings_d, 16.0, 256.0));
			const uint32_t r_center_fine = std::clamp(static_cast<uint32_t>((z_clamped + 1.0) * 0.5 * static_cast<double>(fine_rings)), 0U, fine_rings - 1U);
			const double fade_in = std::clamp((zoom_factor - 1.2) / 2.0, 0.0, 1.0);
			const uint64_t fine_seed = base_seed ^ 0x517cc1b727220a95ULL;

			for (int dr = -1; dr <= 1; ++dr) {
				const int r_cand = static_cast<int>(r_center_fine) + dr;
				if (r_cand < 0 || r_cand >= static_cast<int>(fine_rings)) continue;
				const uint32_t r_idx = static_cast<uint32_t>(r_cand);
				const uint32_t s_count = SphericalPartition::sector_count_for_ring(r_idx, fine_rings);
				const uint32_t s_center = std::clamp(static_cast<uint32_t>(phi_dir / kTwoPi * static_cast<double>(s_count)), 0U, s_count - 1U);

				for (int ds = -1; ds <= 1; ++ds) {
					const uint32_t s_idx = (s_center + s_count + ds) % s_count;
					const uint64_t cell_hash = SphericalPartition::hash_coords(r_idx, s_idx, fine_seed);

					if (SphericalPartition::hash_to_unit(cell_hash) <= 0.86) continue;

					const double z_c = -1.0 + (2.0 * static_cast<double>(r_idx) + 1.0) / static_cast<double>(fine_rings);
					const double phi_c = kTwoPi * (static_cast<double>(s_idx) + 0.5) / static_cast<double>(s_count);

					const double z_star = std::clamp(z_c + (SphericalPartition::hash_to_unit(cell_hash + 11ULL) - 0.5) * (1.8 / static_cast<double>(fine_rings)), -1.0, 1.0);
					const double phi_star = std::remainder(phi_c + (SphericalPartition::hash_to_unit(cell_hash + 23ULL) - 0.5) * (1.8 * kTwoPi / static_cast<double>(s_count)), kTwoPi);
					const double r_xy = std::sqrt(std::max(1.0 - z_star * z_star, 0.0));
					const std::array<double, 3> star_dir{r_xy * std::cos(phi_star), r_xy * std::sin(phi_star), z_star};

					const double mu = dir[0] * star_dir[0] + dir[1] * star_dir[1] + dir[2] * star_dir[2];
					if (mu <= 0.9996) continue;
					const double theta_sq = 2.0 * std::max(0.0, 1.0 - mu);
					const double radius_rad = sky.star_size_min * 0.65;
					const double eff_sigma = std::sqrt(radius_rad * radius_rad + pixel_footprint_rad * pixel_footprint_rad * 0.25);
					const double eff_sigma_sq = eff_sigma * eff_sigma;
					if (theta_sq > 12.0 * eff_sigma_sq) continue;

					const double lum = sample_distribution(sky.star_luminosity_distribution, 0.05, 3.0, sky.star_luminosity_param, cell_hash + 33ULL) * sky.star_brightness * fade_in;
					const double psf = std::exp(-theta_sq / (2.0 * eff_sigma_sq));
					const auto tint = star_blackbody_tint(SphericalPartition::hash_to_unit(cell_hash + 44ULL), sky.star_color_variation);

					accum[0] += lum * psf * static_cast<double>(tint[0]);
					accum[1] += lum * psf * static_cast<double>(tint[1]);
					accum[2] += lum * psf * static_cast<double>(tint[2]);
				}
			}
		}
	}

	static void evaluate_volumetric_nebulae(
		const std::array<double, 3>& dir,
		const ProceduralSkySettings& sky,
		double zoom_factor,
		std::array<double, 3>& accum
	) noexcept {
		if (!sky.nebula_enabled || sky.nebula_intensity <= 0.0) return;

		const auto& pn = sky.nebula_plane_normal;
		const double n_len = std::sqrt(pn[0] * pn[0] + pn[1] * pn[1] + pn[2] * pn[2]);
		const std::array<double, 3> plane_norm = (n_len > 1e-9)
			? std::array<double, 3>{pn[0] / n_len, pn[1] / n_len, pn[2] / n_len}
			: std::array<double, 3>{0.0, 0.4472135955, 0.894427191};

		const double h = dir[0] * plane_norm[0] + dir[1] * plane_norm[1] + dir[2] * plane_norm[2];
		const double thickness = std::max(sky.nebula_plane_thickness, 0.02);
		const double falloff_exp = std::clamp(sky.nebula_plane_falloff, 1.0, 6.0);
		const double disc_envelope = std::exp(-std::pow(std::abs(h) / thickness, falloff_exp));

		const auto& an = sky.nebula_asymmetry_dir;
		const double an_len = std::sqrt(an[0] * an[0] + an[1] * an[1] + an[2] * an[2]);
		const std::array<double, 3> asym_dir = (an_len > 1e-9)
			? std::array<double, 3>{an[0] / an_len, an[1] / an_len, an[2] / an_len}
			: std::array<double, 3>{0.7071067812, 0.0, 0.7071067812};

		const double cos_psi = dir[0] * asym_dir[0] + dir[1] * asym_dir[1] + dir[2] * asym_dir[2];
		const double asymmetry_envelope = std::pow(std::clamp((1.0 + cos_psi) * 0.5, 0.0, 1.0), std::clamp(sky.nebula_asymmetry * 2.5 + 0.1, 0.1, 8.0));
		const double core_glow = sky.nebula_core_glow * std::exp(-2.5 * std::max(0.0, 1.0 - cos_psi));

		int octaves = static_cast<int>(std::clamp(sky.nebula_octaves, 1U, 8U));
		if (sky.adaptive_fov_lod && zoom_factor > 1.5) {
			octaves = std::min(8, octaves + static_cast<int>(std::floor(std::log2(zoom_factor))));
		}

		const float freq = static_cast<float>(sky.nebula_frequency);
		const float rough = static_cast<float>(std::clamp(sky.nebula_roughness, 0.1, 0.95));
		const float seed_f = static_cast<float>(sky.seed % 1024U) * 1.37f;

		float dx = static_cast<float>(dir[0]);
		float dy = static_cast<float>(dir[1]);
		float dz = static_cast<float>(dir[2]);

		if (sky.nebula_curl_strength > 0.01) {
			const float curl_scale = static_cast<float>(sky.nebula_curl_strength * 0.25);
			const float cx = noise3d(dy * 2.0f, dz * 2.0f, seed_f);
			const float cy = noise3d(dz * 2.0f, dx * 2.0f, seed_f + 7.1f);
			const float cz = noise3d(dx * 2.0f, dy * 2.0f, seed_f + 13.3f);
			dx += cx * curl_scale;
			dy += cy * curl_scale;
			dz += cz * curl_scale;
			const float r_inv = 1.0f / std::max(std::sqrt(dx * dx + dy * dy + dz * dz), 1.0e-6f);
			dx *= r_inv; dy *= r_inv; dz *= r_inv;
		}

		const uint32_t layers = std::clamp(sky.nebula_layer_count, 1U, 5U);
		double layer_emission_r = 0.0;
		double layer_emission_g = 0.0;
		double layer_emission_b = 0.0;

		for (uint32_t layer = 0; layer < layers; ++layer) {
			const float layer_offset = static_cast<float>(layer) * 17.3f * static_cast<float>(sky.nebula_layer_spread);
			const float layer_scale = 1.0f + static_cast<float>(layer) * 0.75f * static_cast<float>(sky.nebula_layer_spread);

			const float n1 = fbm3d(dx * freq * layer_scale + seed_f + layer_offset, dy * freq * layer_scale - seed_f * 0.7f, dz * freq * layer_scale + 13.5f + layer_offset, octaves, rough);
			const float e1 = std::pow(std::max(0.0f, (n1 + 0.15f) * 0.9f), 1.8f);

			const float n2 = fbm3d(dx * freq * 1.9f * layer_scale - 27.1f + layer_offset, dy * freq * 1.9f * layer_scale + seed_f * 0.5f, dz * freq * 1.9f * layer_scale - 11.2f, octaves, rough);
			const float e2 = std::pow(std::max(0.0f, (n2 + 0.05f) * 0.85f), 2.2f);

			const float n3 = fbm3d(dx * freq * 2.8f * layer_scale + 43.0f, dy * freq * 2.8f * layer_scale + layer_offset, dz * freq * 2.8f * layer_scale - seed_f, octaves, rough);
			const float e3 = std::pow(std::max(0.0f, n3 * 0.75f), 2.5f);

			const double layer_weight = 1.0 / (1.0 + 0.45 * static_cast<double>(layer));
			layer_emission_r += (static_cast<double>(e1 * sky.nebula_emission_color[0]) + static_cast<double>(e2 * sky.nebula_secondary_color[0]) + static_cast<double>(e3 * sky.nebula_tertiary_color[0])) * layer_weight;
			layer_emission_g += (static_cast<double>(e1 * sky.nebula_emission_color[1]) + static_cast<double>(e2 * sky.nebula_secondary_color[1]) + static_cast<double>(e3 * sky.nebula_tertiary_color[1])) * layer_weight;
			layer_emission_b += (static_cast<double>(e1 * sky.nebula_emission_color[2]) + static_cast<double>(e2 * sky.nebula_secondary_color[2]) + static_cast<double>(e3 * sky.nebula_tertiary_color[2])) * layer_weight;
		}

		const float n_dust = ridged_fbm3d(dx * freq * 2.6f + 71.3f, dy * freq * 2.6f - 39.4f, dz * freq * 2.6f + seed_f, octaves, 0.6f);
		const float tau = std::max(0.0f, n_dust - 0.22f) * static_cast<float>(sky.nebula_absorption_density * 4.2);
		const double extinction = std::exp(-static_cast<double>(tau));

		const double combined_envelope = (disc_envelope * asymmetry_envelope + core_glow) * extinction * sky.nebula_intensity;

		accum[0] += layer_emission_r * combined_envelope;
		accum[1] += layer_emission_g * combined_envelope;
		accum[2] += layer_emission_b * combined_envelope;
	}

	static void evaluate_seamless_galaxies(
		const std::array<double, 3>& dir,
		const ProceduralSkySettings& sky,
		std::array<double, 3>& accum
	) noexcept {
		if (!sky.galaxies_enabled || sky.galaxy_density <= 0.0) return;

		constexpr double kTwoPi = SphericalPartition::kTwoPi;
		const uint64_t base_seed = static_cast<uint64_t>(sky.seed) ^ 0x6a09e667f3bcc908ULL;

		const uint32_t total_rings = static_cast<uint32_t>(std::clamp(std::round(14.0 * std::sqrt(sky.galaxy_density)), 4.0, 36.0));
		const double z_clamped = std::clamp(dir[2], -1.0, 1.0);
		const uint32_t ring_center = std::clamp(static_cast<uint32_t>((z_clamped + 1.0) * 0.5 * static_cast<double>(total_rings)), 0U, total_rings - 1U);
		double phi_dir = std::atan2(dir[1], dir[0]);
		if (phi_dir < 0.0) phi_dir += kTwoPi;

		const double presence_threshold = std::clamp(1.0 - sky.galaxy_density * 0.25, 0.02, 0.99);

		for (int dr = -1; dr <= 1; ++dr) {
			const int r_cand = static_cast<int>(ring_center) + dr;
			if (r_cand < 0 || r_cand >= static_cast<int>(total_rings)) continue;
			const uint32_t r_idx = static_cast<uint32_t>(r_cand);
			const uint32_t s_count = SphericalPartition::sector_count_for_ring(r_idx, total_rings);
			const uint32_t s_center = std::clamp(static_cast<uint32_t>(phi_dir / kTwoPi * static_cast<double>(s_count)), 0U, s_count - 1U);

			for (int ds = -1; ds <= 1; ++ds) {
				const uint32_t s_idx = (s_center + s_count + ds) % s_count;
				const uint64_t cell_hash = SphericalPartition::hash_coords(r_idx, s_idx, base_seed);

				if (SphericalPartition::hash_to_unit(cell_hash) > presence_threshold) continue;

				const double z_c = -1.0 + (2.0 * static_cast<double>(r_idx) + 1.0) / static_cast<double>(total_rings);
				const double phi_c = kTwoPi * (static_cast<double>(s_idx) + 0.5) / static_cast<double>(s_count);

				const double z_gal = std::clamp(z_c + (SphericalPartition::hash_to_unit(cell_hash + 3ULL) - 0.5) * (1.6 / static_cast<double>(total_rings)), -1.0, 1.0);
				const double phi_gal = std::remainder(phi_c + (SphericalPartition::hash_to_unit(cell_hash + 7ULL) - 0.5) * (1.6 * kTwoPi / static_cast<double>(s_count)), kTwoPi);
				const double r_xy = std::sqrt(std::max(1.0 - z_gal * z_gal, 0.0));
				const std::array<double, 3> gal_dir{r_xy * std::cos(phi_gal), r_xy * std::sin(phi_gal), z_gal};

				const double mu = dir[0] * gal_dir[0] + dir[1] * gal_dir[1] + dir[2] * gal_dir[2];
				if (mu <= 0.9985) continue;
				const double sep_sq = 2.0 * std::max(0.0, 1.0 - mu);

				const double size_rad = sample_distribution(sky.galaxy_size_distribution, sky.galaxy_size_min, sky.galaxy_size_max, sky.galaxy_size_param, cell_hash + 13ULL) * sky.galaxy_size_scale;
				const double size_sq = size_rad * size_rad;
				if (sep_sq > 9.0 * size_sq) continue;

				std::array<double, 3> t1{}, t2{};
				if (std::abs(gal_dir[2]) < 0.9) {
					t1 = {-gal_dir[1], gal_dir[0], 0.0};
				} else {
					t1 = {0.0, -gal_dir[2], gal_dir[1]};
				}
				const double t1_len = std::sqrt(t1[0] * t1[0] + t1[1] * t1[1] + t1[2] * t1[2]);
				t1[0] /= t1_len; t1[1] /= t1_len; t1[2] /= t1_len;
				t2 = {
					gal_dir[1] * t1[2] - gal_dir[2] * t1[1],
					gal_dir[2] * t1[0] - gal_dir[0] * t1[2],
					gal_dir[0] * t1[1] - gal_dir[1] * t1[0]
				};

				const double roll_angle = SphericalPartition::hash_to_unit(cell_hash + 17ULL) * kTwoPi;
				const double cos_r = std::cos(roll_angle);
				const double sin_r = std::sin(roll_angle);
				const std::array<double, 3> u1{t1[0] * cos_r + t2[0] * sin_r, t1[1] * cos_r + t2[1] * sin_r, t1[2] * cos_r + t2[2] * sin_r};
				const std::array<double, 3> u2{-t1[0] * sin_r + t2[0] * cos_r, -t1[1] * sin_r + t2[1] * cos_r, -t1[2] * sin_r + t2[2] * cos_r};

				const std::array<double, 3> delta{dir[0] - gal_dir[0], dir[1] - gal_dir[1], dir[2] - gal_dir[2]};
				const double dx_local = delta[0] * u1[0] + delta[1] * u1[1] + delta[2] * u1[2];
				const double dy_local = delta[0] * u2[0] + delta[1] * u2[1] + delta[2] * u2[2];

				const double ellip = 1.0 + SphericalPartition::hash_to_unit(cell_hash + 21ULL) * sky.galaxy_ellipticity_variance;
				const double r_norm = std::sqrt(dx_local * dx_local + dy_local * dy_local * ellip * ellip) / std::max(size_rad, 1e-6);
				const double phi_local = std::atan2(dy_local, dx_local);

				const double core = std::exp(-r_norm * r_norm * 4.0);
				const double arms = 0.5 + 0.5 * std::cos(sky.galaxy_spiral_arms * phi_local - r_norm * 4.8 * sky.galaxy_pitch_angle);
				const double disk = std::exp(-r_norm * 2.2) * arms;

				const double intensity = (core * sky.galaxy_core_ratio + disk * (1.0 - sky.galaxy_core_ratio)) * sky.galaxy_brightness * 2.0;

				const bool warm = SphericalPartition::hash_to_unit(cell_hash + 19ULL) > 0.45;
				accum[0] += intensity * (warm ? 1.15 : 0.75);
				accum[1] += intensity * (warm ? 0.95 : 0.90);
				accum[2] += intensity * (warm ? 0.70 : 1.25);
			}
		}
	}

	static void evaluate_seamless_clusters(
		const std::array<double, 3>& dir,
		const ProceduralSkySettings& sky,
		std::array<double, 3>& accum
	) noexcept {
		if (!sky.clusters_enabled || sky.cluster_density <= 0.0) return;

		constexpr double kTwoPi = SphericalPartition::kTwoPi;
		const uint64_t base_seed = static_cast<uint64_t>(sky.seed) ^ 0xbb67ae8584caa73bULL;

		const uint32_t total_rings = static_cast<uint32_t>(std::clamp(std::round(10.0 * std::sqrt(sky.cluster_density)), 3.0, 28.0));
		const double z_clamped = std::clamp(dir[2], -1.0, 1.0);
		const uint32_t ring_center = std::clamp(static_cast<uint32_t>((z_clamped + 1.0) * 0.5 * static_cast<double>(total_rings)), 0U, total_rings - 1U);
		double phi_dir = std::atan2(dir[1], dir[0]);
		if (phi_dir < 0.0) phi_dir += kTwoPi;

		const double presence_threshold = std::clamp(1.0 - sky.cluster_density * 0.16, 0.01, 0.99);

		for (int dr = -1; dr <= 1; ++dr) {
			const int r_cand = static_cast<int>(ring_center) + dr;
			if (r_cand < 0 || r_cand >= static_cast<int>(total_rings)) continue;
			const uint32_t r_idx = static_cast<uint32_t>(r_cand);
			const uint32_t s_count = SphericalPartition::sector_count_for_ring(r_idx, total_rings);
			const uint32_t s_center = std::clamp(static_cast<uint32_t>(phi_dir / kTwoPi * static_cast<double>(s_count)), 0U, s_count - 1U);

			for (int ds = -1; ds <= 1; ++ds) {
				const uint32_t s_idx = (s_center + s_count + ds) % s_count;
				const uint64_t cell_hash = SphericalPartition::hash_coords(r_idx, s_idx, base_seed);

				if (SphericalPartition::hash_to_unit(cell_hash) > presence_threshold) continue;

				const double z_c = -1.0 + (2.0 * static_cast<double>(r_idx) + 1.0) / static_cast<double>(total_rings);
				const double phi_c = kTwoPi * (static_cast<double>(s_idx) + 0.5) / static_cast<double>(s_count);

				const double z_cl = std::clamp(z_c + (SphericalPartition::hash_to_unit(cell_hash + 5ULL) - 0.5) * (1.6 / static_cast<double>(total_rings)), -1.0, 1.0);
				const double phi_cl = std::remainder(phi_c + (SphericalPartition::hash_to_unit(cell_hash + 9ULL) - 0.5) * (1.6 * kTwoPi / static_cast<double>(s_count)), kTwoPi);
				const double r_xy = std::sqrt(std::max(1.0 - z_cl * z_cl, 0.0));
				const std::array<double, 3> cluster_center{r_xy * std::cos(phi_cl), r_xy * std::sin(phi_cl), z_cl};

				const double mu = dir[0] * cluster_center[0] + dir[1] * cluster_center[1] + dir[2] * cluster_center[2];
				if (mu <= 0.9990) continue;
				const double sep_sq = 2.0 * std::max(0.0, 1.0 - mu);

				const double cluster_rad = sample_distribution(sky.cluster_size_distribution, sky.cluster_size_min, sky.cluster_size_max, 1.5, cell_hash + 17ULL) * sky.cluster_size_scale;
				const double rad_sq = cluster_rad * cluster_rad;
				if (sep_sq > 6.0 * rad_sq) continue;

				const double plummer = 1.0 / std::pow(1.0 + sep_sq / std::max(rad_sq * 0.25, 1e-8), sky.cluster_concentration);
				const double flux = plummer * sky.cluster_brightness * 2.2;

				accum[0] += flux * static_cast<double>(sky.cluster_core_tint[0]);
				accum[1] += flux * static_cast<double>(sky.cluster_core_tint[1]);
				accum[2] += flux * static_cast<double>(sky.cluster_core_tint[2]);
			}
		}
	}

	static void evaluate_interstellar_dust(
		const std::array<double, 3>& dir,
		const ProceduralSkySettings& sky,
		std::array<double, 3>& accum
	) noexcept {
		if (!sky.dust_enabled || sky.dust_intensity <= 0.0) return;

		const float scale = static_cast<float>(std::clamp(sky.dust_scale, 0.2, 8.0));
		const int octaves = static_cast<int>(std::clamp(sky.dust_fractal_octaves, 1U, 6U));
		const float seed_f = static_cast<float>(sky.seed % 4096U) * 0.77f;

		const float dx = static_cast<float>(dir[0]);
		const float dy = static_cast<float>(dir[1]);
		const float dz = static_cast<float>(dir[2]);

		const float d_val = fbm3d(dx * scale + seed_f, dy * scale - 53.0f, dz * scale + seed_f * 0.5f, octaves, 0.55f);
		const float threshold = static_cast<float>(std::clamp(1.0 - 0.2 * sky.dust_density, 0.0, 0.95));

		if (d_val > threshold) {
			const float normalized = (d_val - threshold) / (1.0f - threshold);
			const double lum = static_cast<double>(std::pow(normalized, static_cast<float>(sky.dust_contrast))) * sky.dust_intensity * 0.35;
			accum[0] += lum * static_cast<double>(sky.dust_tint[0]);
			accum[1] += lum * static_cast<double>(sky.dust_tint[1]);
			accum[2] += lum * static_cast<double>(sky.dust_tint[2]);
		}
	}

	static void evaluate_seamless_exotic_sources(
		const std::array<double, 3>& dir,
		const ProceduralSkySettings& sky,
		std::array<double, 3>& accum
	) noexcept {
		if (!sky.exotic_objects_enabled || sky.quasar_density <= 0.0) return;

		constexpr double kTwoPi = SphericalPartition::kTwoPi;
		const uint64_t base_seed = static_cast<uint64_t>(sky.seed) ^ 0x9b05688c2b3e6c1fULL;

		const uint32_t total_rings = static_cast<uint32_t>(std::clamp(std::round(8.0 * std::sqrt(sky.quasar_density)), 3.0, 24.0));
		const double z_clamped = std::clamp(dir[2], -1.0, 1.0);
		const uint32_t ring_center = std::clamp(static_cast<uint32_t>((z_clamped + 1.0) * 0.5 * static_cast<double>(total_rings)), 0U, total_rings - 1U);
		double phi_dir = std::atan2(dir[1], dir[0]);
		if (phi_dir < 0.0) phi_dir += kTwoPi;

		const double presence_threshold = std::clamp(1.0 - sky.quasar_density * 0.10, 0.005, 0.99);

		for (int dr = -1; dr <= 1; ++dr) {
			const int r_cand = static_cast<int>(ring_center) + dr;
			if (r_cand < 0 || r_cand >= static_cast<int>(total_rings)) continue;
			const uint32_t r_idx = static_cast<uint32_t>(r_cand);
			const uint32_t s_count = SphericalPartition::sector_count_for_ring(r_idx, total_rings);
			const uint32_t s_center = std::clamp(static_cast<uint32_t>(phi_dir / kTwoPi * static_cast<double>(s_count)), 0U, s_count - 1U);

			for (int ds = -1; ds <= 1; ++ds) {
				const uint32_t s_idx = (s_center + s_count + ds) % s_count;
				const uint64_t cell_hash = SphericalPartition::hash_coords(r_idx, s_idx, base_seed);

				if (SphericalPartition::hash_to_unit(cell_hash) > presence_threshold) continue;

				const double z_c = -1.0 + (2.0 * static_cast<double>(r_idx) + 1.0) / static_cast<double>(total_rings);
				const double phi_c = kTwoPi * (static_cast<double>(s_idx) + 0.5) / static_cast<double>(s_count);

				const double z_q = std::clamp(z_c + (SphericalPartition::hash_to_unit(cell_hash + 3ULL) - 0.5) * (1.6 / static_cast<double>(total_rings)), -1.0, 1.0);
				const double phi_q = std::remainder(phi_c + (SphericalPartition::hash_to_unit(cell_hash + 7ULL) - 0.5) * (1.6 * kTwoPi / static_cast<double>(s_count)), kTwoPi);
				const double r_xy = std::sqrt(std::max(1.0 - z_q * z_q, 0.0));
				const std::array<double, 3> q_dir{r_xy * std::cos(phi_q), r_xy * std::sin(phi_q), z_q};

				const double mu = dir[0] * q_dir[0] + dir[1] * q_dir[1] + dir[2] * q_dir[2];
				if (mu <= 0.99992) continue;
				const double sep_sq = 2.0 * std::max(0.0, 1.0 - mu);

				const double q_rad = std::max(sky.quasar_core_size, 0.0001);
				const double q_rad_sq = q_rad * q_rad;
				if (sep_sq > 8.0 * q_rad_sq) continue;

				double flux = std::exp(-sep_sq / (2.0 * q_rad_sq)) * sky.quasar_brightness * 3.5;
				if (sky.dynamic_motion) {
					const double pulse_phase = std::fmod(sky.time_seconds * sky.pulsar_pulse_rate + SphericalPartition::hash_to_unit(cell_hash + 11ULL), 1.0);
					const double duty = std::clamp(sky.pulsar_pulse_duty, 0.05, 0.95);
					const double pulse = (pulse_phase < duty) ? (1.0 / duty) : 0.0;
					flux *= (0.35 + 0.65 * pulse);
				}

				accum[0] += flux * 0.85;
				accum[1] += flux * 1.10;
				accum[2] += flux * 1.55;
			}
		}
	}

public:
	[[nodiscard]] static std::array<float, 3> sample_procedural_sky(
		double dir_x, double dir_y, double dir_z,
		const ProceduralSkySettings& sky,
		double fov_rad
	) noexcept {
		if (!sky.enabled) {
			return {0.0f, 0.0f, 0.0f};
		}

		const double len_sq = dir_x * dir_x + dir_y * dir_y + dir_z * dir_z;
		if (len_sq < 1e-18) return {0.0f, 0.0f, 0.0f};
		const double inv_len = 1.0 / std::sqrt(len_sq);
		const std::array<double, 3> unit_dir{dir_x * inv_len, dir_y * inv_len, dir_z * inv_len};

		const std::array<double, 3> oriented_dir = rotate_vector(unit_dir, sky.orientation_yaw_deg, sky.orientation_pitch_deg, sky.orientation_roll_deg);

		std::array<double, 3> accum{
			static_cast<double>(sky.background_color[0]),
			static_cast<double>(sky.background_color[1]),
			static_cast<double>(sky.background_color[2])
		};

		if (sky.grid_enabled) {
			const auto grid_rgb = sample_celestial_grid(oriented_dir, sky);
			accum[0] += grid_rgb[0];
			accum[1] += grid_rgb[1];
			accum[2] += grid_rgb[2];
		}

		double zoom_factor = 1.0;
		if (sky.adaptive_fov_lod) {
			const double fov_deg = fov_rad * (180.0 / std::numbers::pi_v<double>);
			zoom_factor = std::clamp(sky.adaptive_fov_reference / std::max(fov_deg, 0.05), 1.0, 16.0);
		}

		if (sky.nebula_enabled) {
			evaluate_volumetric_nebulae(oriented_dir, sky, zoom_factor, accum);
		}

		if (sky.dust_enabled) {
			evaluate_interstellar_dust(oriented_dir, sky, accum);
		}

		if (sky.galaxies_enabled) {
			evaluate_seamless_galaxies(oriented_dir, sky, accum);
		}

		if (sky.clusters_enabled) {
			evaluate_seamless_clusters(oriented_dir, sky, accum);
		}

		if (sky.exotic_objects_enabled) {
			evaluate_seamless_exotic_sources(oriented_dir, sky, accum);
		}

		if (sky.stars_enabled) {
			evaluate_seamless_spherical_stars(oriented_dir, sky, fov_rad, zoom_factor, accum);
		}

		const double intensity_scale = std::max(sky.master_intensity, 0.0);
		accum[0] *= intensity_scale;
		accum[1] *= intensity_scale;
		accum[2] *= intensity_scale;

		std::array<float, 3> final_rgb{
			static_cast<float>(std::max(0.0, accum[0])),
			static_cast<float>(std::max(0.0, accum[1])),
			static_cast<float>(std::max(0.0, accum[2]))
		};

		final_rgb = apply_hue_saturation(final_rgb, sky.hue_shift_deg * (std::numbers::pi_v<double> / 180.0), sky.saturation);
		return final_rgb;
	}

	[[nodiscard]] static std::array<float, 3> sample_sky(
		double dir_x, double dir_y, double dir_z,
		const ProceduralSkySettings& sky,
		double fov_rad,
		double time_seconds,
		uint32_t panorama_source = 0,
		uint32_t panorama_id = 0,
		uint32_t panorama_quality = 1
	) noexcept {
		if (!sky.enabled) {
			return {0.0f, 0.0f, 0.0f};
		}

		if (panorama_source != 0U) {
			const auto sampled = Optics::SkyPanoramaLoader::instance().sample_direction(
				static_cast<Optics::SkyPanoramaId>(panorama_id),
				static_cast<Optics::SkyPanoramaQuality>(panorama_quality),
				dir_x, dir_y, dir_z, sky.orientation_yaw_deg * (std::numbers::pi_v<double> / 180.0)
			);
			if (sampled.has_value()) {
				auto panorama_rgb = apply_hue_saturation(*sampled, sky.hue_shift_deg * (std::numbers::pi_v<double> / 180.0), sky.saturation);
				panorama_rgb[0] = std::max(0.0f, panorama_rgb[0] + sky.background_color[0]);
				panorama_rgb[1] = std::max(0.0f, panorama_rgb[1] + sky.background_color[1]);
				panorama_rgb[2] = std::max(0.0f, panorama_rgb[2] + sky.background_color[2]);
				return panorama_rgb;
			}
		}

		ProceduralSkySettings active_sky = sky;
		active_sky.time_seconds = time_seconds;
		return sample_procedural_sky(dir_x, dir_y, dir_z, active_sky, fov_rad);
	}
};

}
