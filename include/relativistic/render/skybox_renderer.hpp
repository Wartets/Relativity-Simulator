#pragma once

#include "relativistic/optics/textures/sky_panorama_image.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <algorithm>
#include <string_view>

namespace Relativistic::Render {

enum class StarProfileModel : uint32_t {
	Gaussian = 0,
	Lorentzian = 1,
	Moffat = 2,
	AiryDiffraction = 3,
	HardDisc = 4,
	DiffractionSpikes = 5,
	PinpointDirac = 6,
	CinematicBloom = 7
};

enum class StarPlacementModel : uint32_t {
	UniformEqualArea = 0,
	GalacticDisk = 1,
	FractalClustered = 2,
	DualPopulation = 3,
	SpiralArmDensity = 4,
	ConstellationAssociations = 5
};

enum class ClusterPlacementModel : uint32_t {
	GlobularHalo = 0,
	OpenClusterPlane = 1,
	UniformIsotropic = 2,
	SuperclusterComplex = 3
};

enum class GalaxyMorphologyModel : uint32_t {
	GrandDesignSpiral = 0,
	FlocculentSpiral = 1,
	BarredSpiral = 2,
	GiantElliptical = 3,
	Lenticular = 4,
	IrregularStarburst = 5,
	RingGalaxy = 6
};

enum class GridCellFillMode : uint32_t {
	None = 0,
	Checkerboard2Color = 1,
	Cyclic3Color = 2,
	Checkerboard4Color = 3,
	RandomHashFill = 4
};

struct ProceduralSkySettings {
	bool enabled{true};
	bool seed_locked{false};
	uint32_t seed{56U};
	double master_intensity{1.0};

	double quality_factor{1.0};
	double star_quality_factor{1.0};
	double nebula_quality_factor{1.0};
	double cluster_quality_factor{1.0};
	double dust_quality_factor{1.0};

	bool adaptive_fov_lod{false};
	bool adaptive_fov_quality{true};
	double adaptive_fov_reference{90.0};
	double adaptive_fov_sensitivity{1.0};
	uint32_t max_star_octaves{8};

	bool dynamic_motion{false};
	bool dynamic_rotation{false};
	double motion_speed{0.05};
	double rotation_rate_deg_s{0.01};
	double time_seconds{0.0};

	bool stars_enabled{true};
	double star_density{91.064};
	double star_brightness{1.81};
	double star_size_scale{1.0};
	double star_size_min{5e-05};
	double star_size_max{0.00263027};
	uint32_t star_size_distribution{1};
	double star_size_param{3.96};
	uint32_t star_luminosity_distribution{1};
	double star_luminosity_param{1.84};
	double star_color_variation{1.0};
	double star_temperature_bias{0.32};
	double star_brightness_variation{0.5};
	double star_scintillation{0.0};
	double star_magnitude_limit{21.0};

	uint32_t star_profile_model{0};
	double star_profile_param{2.5};
	double star_profile_param2{1.0};
	uint32_t star_spike_count{4};
	double star_spike_length{4.0};
	double star_spike_intensity{0.6};
	double star_spike_angle_deg{45.0};
	double star_profile_softness{0.15};
	double star_dirac_concentration{0.95};
	double star_bloom_radius{3.0};
	double star_bloom_intensity{0.5};

	uint32_t star_placement_model{3};
	double star_galactic_concentration{2.5};
	double star_cluster_association_strength{0.6};
	double exotic_quality_factor{1.0};

	bool nebula_enabled{true};
	double nebula_intensity{0.23};
	double nebula_roughness{0.76};
	uint32_t nebula_octaves{5};
	double nebula_frequency{2.82};
	std::array<double, 3> nebula_plane_normal{0.0, 0.4472135955, 0.894427191};
	double nebula_plane_thickness{0.174};
	double nebula_plane_falloff{4.09};
	double nebula_asymmetry{0.43};
	std::array<double, 3> nebula_asymmetry_dir{0.7071067812, 0.0, 0.7071067812};
	std::array<float, 3> nebula_emission_color{0.92f, 0.22f, 0.38f};
	std::array<float, 3> nebula_secondary_color{0.15f, 0.72f, 0.90f};
	std::array<float, 3> nebula_tertiary_color{0.95f, 0.65f, 0.15f};
	double nebula_absorption_density{1.35};
	uint32_t nebula_layer_count{3};
	double nebula_layer_spread{0.69};
	double nebula_core_glow{0.78};
	double nebula_curl_strength{0.46};
	double nebula_color_separation{1.6};
	double nebula_ionization_glow{0.85};
	double nebula_gas_contrast{1.94};

	bool galaxies_enabled{true};
	double galaxy_density{0.12};
	double galaxy_brightness{0.5};
	double galaxy_size_scale{0.32};
	double galaxy_size_min{0.001};
	double galaxy_size_max{0.02};
	uint32_t galaxy_size_distribution{1};
	double galaxy_size_param{1.5};
	double galaxy_spiral_arms{2.0};
	double galaxy_core_ratio{0.4};
	double galaxy_pitch_angle{1.2};
	double galaxy_ellipticity_variance{0.5};
	uint32_t galaxy_type_mix{0};

	bool clusters_enabled{false};
	double cluster_density{0.8};
	double cluster_brightness{1.0};
	double cluster_size_scale{0.6};
	double cluster_size_min{0.002};
	double cluster_size_max{0.03};
	uint32_t cluster_size_distribution{2};
	double cluster_concentration{1.6};
	double cluster_star_density_boost{2.0};
	std::array<float, 3> cluster_core_tint{1.1f, 1.0f, 0.8f};
	uint32_t cluster_placement_model{0};
	double cluster_halo_radius{0.8};
	double cluster_plane_scale_height{0.15};
	bool cluster_stars_enabled{true};
	uint32_t cluster_member_count{48};
	double cluster_member_size{0.0006};
	double cluster_member_brightness{1.2};
	uint32_t cluster_star_profile_model{0};
	double cluster_halo_intensity{0.25};
	bool cluster_inherit_star_profile{true};
	double cluster_spread_factor{1.0};

	bool dust_enabled{true};
	double dust_density{3.85};
	double dust_intensity{1.3};
	double dust_scale{3.82};
	double dust_absorption_ratio{0.8};
	uint32_t dust_fractal_octaves{5};
	double dust_contrast{3.54};
	std::array<float, 3> dust_tint{0.4f, 0.25f, 0.15f};

	uint32_t galaxy_morphology_model{0};
	double galaxy_bar_strength{0.5};
	double galaxy_dust_lane_opacity{0.6};
	double galaxy_clumpiness{0.5};
	double galaxy_disk_thickness{0.2};
	double galaxy_inclination_variance{0.7};
	double galaxy_bulge_sersic_index{4.0};

	bool exotic_objects_enabled{false};
	bool quasar_enabled{true};
	double quasar_density{0.01};
	double quasar_brightness{1.2};
	double quasar_core_size{0.0003};
	double quasar_jet_length{0.08};
	double quasar_jet_intensity{1.5};
	double quasar_jet_opening_angle_deg{5.0};
	double quasar_jet_lorentz_factor{4.0};
	double quasar_accretion_disk_glow{1.0};
	double quasar_precession_angle_deg{10.0};

	bool pulsar_enabled{true};
	double pulsar_density{0.01};
	double pulsar_brightness{1.0};
	double pulsar_pulse_rate{2.5};
	double pulsar_pulse_duty{0.15};
	double pulsar_beam_width_deg{8.0};
	double pulsar_magnetic_tilt_deg{30.0};
	double pulsar_wind_nebula_scale{0.003};
	double pulsar_wind_nebula_intensity{0.4};

	bool snr_enabled{true};
	double snr_density{0.06};
	double snr_brightness{1.2};
	double snr_shell_thickness{0.08};
	double snr_expansion_age{0.5};
	double snr_size{0.015};
	double snr_asymmetry{0.4};
	double snr_filament_scale{3.0};
	double snr_interior_glow{0.3};

	bool grid_enabled{false};
	uint32_t grid_cell_fill_mode{0};
	double grid_opacity{0.5};
	double grid_line_width{0.0001};
	double grid_spacing_deg{30.0};
	uint32_t grid_subdivisions{1};
	double grid_glow{0.0};
	double grid_tick_interval_deg{5.0};
	double grid_tick_length{0.003};
	double grid_tick_width{0.0001};
	double grid_cell_opacity{0.0};
	double grid_equator_width{0.0003};
	double grid_equator_opacity{0.8};
	double grid_polar_width{0.0002};
	double grid_polar_opacity{0.7};
	double grid_meridian_width{0.0003};
	double grid_meridian_opacity{0.8};
	double grid_major_width{0.00015};
	double grid_minor_width{0.00008};

	std::array<float, 3> grid_major_color{0.40f, 0.75f, 1.0f};
	std::array<float, 3> grid_minor_color{0.20f, 0.45f, 0.80f};
	std::array<float, 3> grid_equator_color{0.95f, 0.85f, 0.30f};
	std::array<float, 3> grid_polar_color{0.35f, 0.85f, 0.95f};
	std::array<float, 3> grid_meridian_color{0.95f, 0.45f, 0.35f};
	std::array<float, 3> grid_tick_color{1.0f, 1.0f, 1.0f};
	std::array<float, 3> grid_cell_color1{0.10f, 0.12f, 0.18f};
	std::array<float, 3> grid_cell_color2{0.05f, 0.07f, 0.12f};
	std::array<float, 3> grid_cell_color3{0.08f, 0.15f, 0.15f};
	std::array<float, 3> grid_cell_color4{0.15f, 0.10f, 0.12f};

	bool grid_show_polar_caps{false};
	bool grid_show_equator{true};
	bool grid_show_meridians{false};
	bool grid_show_ticks{false};

	std::array<float, 3> background_color{0.0f, 0.0f, 0.0f};
	double hue_shift_deg{0.0};
	double saturation{0.86};
	double orientation_yaw_deg{0.0};
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

		[[nodiscard]] static constexpr uint64_t hash_3d(int32_t x, int32_t y, int32_t z, uint64_t seed) noexcept {
			uint64_t h = seed ^ (static_cast<uint64_t>(static_cast<uint32_t>(x)) * 0x9e3779b97f4a7c15ULL)
				^ (static_cast<uint64_t>(static_cast<uint32_t>(y)) * 0xbf58476d1ce4e5b9ULL)
				^ (static_cast<uint64_t>(static_cast<uint32_t>(z)) * 0x94d049bb133111ebULL);
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
				const double log_min = std::log(std::max(min_val, 1.0e-12));
				const double log_max = std::log(std::max(max_val, 1.0e-11));
				const double mid = 0.5 * (log_min + log_max);
				const double spread = std::clamp(shape_param * 0.35, 0.05, 2.5) * (log_max - log_min);
				return std::clamp(std::exp(mid + spread * z), min_val, max_val);
			}
			case 4: {
				if (u < 0.85) {
					const double u_sub = u / 0.85;
					return min_val + u_sub * (min_val + 0.12 * (max_val - min_val) - min_val);
				} else {
					const double u_sub = (u - 0.85) / 0.15;
					const double split_val = min_val + 0.60 * (max_val - min_val);
					return split_val + u_sub * (max_val - split_val);
				}
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

	struct GridSampleResult {
		std::array<double, 3> rgb{0.0, 0.0, 0.0};
		double alpha{0.0};
	};

	[[nodiscard]] static GridSampleResult sample_celestial_grid(
		const std::array<double, 3>& dir,
		const ProceduralSkySettings& sky,
		double fov_rad
	) noexcept {
		if (!sky.grid_enabled || (sky.grid_opacity <= 0.0 && sky.grid_cell_opacity <= 0.0)) {
			return GridSampleResult{};
		}
		constexpr double kPi = std::numbers::pi_v<double>;
		constexpr double kHalfPi = 0.5 * kPi;
		constexpr double kTwoPi = 2.0 * kPi;
		constexpr double kDegToRad = kPi / 180.0;

		const double dec = std::asin(std::clamp(dir[2], -1.0, 1.0));
		const double ra = std::atan2(dir[1], dir[0]);

		const double spacing_rad = std::max(sky.grid_spacing_deg, 0.1) * kDegToRad;
		const double pixel_footprint = std::max(fov_rad / 1080.0, 1.0e-7);
		const double cos_dec = std::max(std::cos(dec), 1.0e-6);

		std::array<double, 3> accum_color{0.0, 0.0, 0.0};
		double accum_alpha = 0.0;

		if (sky.grid_cell_fill_mode != static_cast<uint32_t>(GridCellFillMode::None) && sky.grid_cell_opacity > 0.0) {
			const double dec_shifted = dec + kHalfPi;
			const double ra_shifted = (ra < 0.0) ? (ra + kTwoPi) : ra;
			const int32_t cell_dec = static_cast<int32_t>(std::floor(dec_shifted / spacing_rad));
			const int32_t cell_ra = static_cast<int32_t>(std::floor(ra_shifted / spacing_rad));

			std::array<float, 3> picked_cell_color = sky.grid_cell_color1;
			if (sky.grid_cell_fill_mode == static_cast<uint32_t>(GridCellFillMode::Checkerboard2Color)) {
				const int32_t parity = ((cell_dec + cell_ra) % 2 + 2) % 2;
				picked_cell_color = (parity == 0) ? sky.grid_cell_color1 : sky.grid_cell_color2;
			} else if (sky.grid_cell_fill_mode == static_cast<uint32_t>(GridCellFillMode::Cyclic3Color)) {
				const int32_t idx = ((cell_dec + 2 * cell_ra) % 3 + 3) % 3;
				picked_cell_color = (idx == 0) ? sky.grid_cell_color1 : ((idx == 1) ? sky.grid_cell_color2 : sky.grid_cell_color3);
			} else if (sky.grid_cell_fill_mode == static_cast<uint32_t>(GridCellFillMode::Checkerboard4Color)) {
				const int32_t px = (cell_ra % 2 + 2) % 2;
				const int32_t py = (cell_dec % 2 + 2) % 2;
				const int32_t idx = px + 2 * py;
				picked_cell_color = (idx == 0) ? sky.grid_cell_color1 : ((idx == 1) ? sky.grid_cell_color2 : ((idx == 2) ? sky.grid_cell_color3 : sky.grid_cell_color4));
			} else if (sky.grid_cell_fill_mode == static_cast<uint32_t>(GridCellFillMode::RandomHashFill)) {
				const uint64_t ch = SphericalPartition::hash_coords(static_cast<uint32_t>(cell_dec + 10000), static_cast<uint32_t>(cell_ra + 10000), static_cast<uint64_t>(sky.seed) ^ 0x5a17ULL);
				const double hval = SphericalPartition::hash_to_unit(ch);
				const int32_t slot = static_cast<int32_t>(hval * 4.0) % 4;
				picked_cell_color = (slot == 0) ? sky.grid_cell_color1 : ((slot == 1) ? sky.grid_cell_color2 : ((slot == 2) ? sky.grid_cell_color3 : sky.grid_cell_color4));
			}

			const double fill_a = std::clamp(sky.grid_cell_opacity, 0.0, 1.0);
			accum_color = {static_cast<double>(picked_cell_color[0]), static_cast<double>(picked_cell_color[1]), static_cast<double>(picked_cell_color[2])};
			accum_alpha = fill_a;
		}

		if (sky.grid_opacity <= 0.0) {
			return GridSampleResult{accum_color, accum_alpha};
		}

		const double feather = std::max(pixel_footprint * 0.6, 1.0e-8);
		const double hw_maj = std::max(sky.grid_major_width, pixel_footprint) * 0.5;
		const double hw_min = std::max(sky.grid_minor_width, pixel_footprint * 0.7) * 0.5;
		const double hw_eq = std::max(sky.grid_equator_width, pixel_footprint) * 0.5;
		const double hw_pol = std::max(sky.grid_polar_width, pixel_footprint) * 0.5;
		const double hw_mer = std::max(sky.grid_meridian_width, pixel_footprint) * 0.5;
		const double hw_tick = std::max(sky.grid_tick_width, pixel_footprint) * 0.5;

		const double pole_taper = std::clamp((spacing_rad * cos_dec - hw_maj * 0.5) / std::max(hw_maj * 2.5, 1.0e-6), 0.0, 1.0);

		const double dist_dec = std::abs(std::remainder(dec, spacing_rad));
		const double cov_maj_dec = 1.0 - std::clamp((dist_dec - (hw_maj - feather)) / (2.0 * feather), 0.0, 1.0);
		const double dist_ra = std::abs(std::remainder(ra, spacing_rad)) * cos_dec;
		const double cov_maj_ra = (1.0 - std::clamp((dist_ra - (hw_maj - feather)) / (2.0 * feather), 0.0, 1.0)) * pole_taper;
		const double cov_major = std::max(cov_maj_dec, cov_maj_ra);

		double cov_sub = 0.0;
		if (sky.grid_subdivisions > 0U) {
			const double sub_spacing = spacing_rad / static_cast<double>(sky.grid_subdivisions + 1U);
			const double dist_sdec = std::abs(std::remainder(dec, sub_spacing));
			const double cov_sdec = 1.0 - std::clamp((dist_sdec - (hw_min - feather)) / (2.0 * feather), 0.0, 1.0);
			const double dist_sra = std::abs(std::remainder(ra, sub_spacing)) * cos_dec;
			const double sub_taper = std::clamp((sub_spacing * cos_dec - hw_min * 0.5) / std::max(hw_min * 2.5, 1.0e-6), 0.0, 1.0);
			const double cov_sra = (1.0 - std::clamp((dist_sra - (hw_min - feather)) / (2.0 * feather), 0.0, 1.0)) * sub_taper;
			cov_sub = std::max(cov_sdec, cov_sra) * 0.75;
		}

		const double dist_eq = std::abs(dec);
		const double cov_eq = sky.grid_show_equator
			? (1.0 - std::clamp((dist_eq - (hw_eq - feather)) / (2.0 * feather), 0.0, 1.0)) * std::clamp(sky.grid_equator_opacity, 0.0, 1.0)
			: 0.0;

		const double dist_pm = std::abs(std::remainder(ra, kPi)) * cos_dec;
		const double cov_pm = sky.grid_show_meridians
			? (1.0 - std::clamp((dist_pm - (hw_mer - feather)) / (2.0 * feather), 0.0, 1.0)) * pole_taper * std::clamp(sky.grid_meridian_opacity, 0.0, 1.0)
			: 0.0;

		const double dist_cap = std::abs(std::abs(dec) - (kHalfPi - spacing_rad));
		const double cov_cap = sky.grid_show_polar_caps
			? (1.0 - std::clamp((dist_cap - (hw_pol - feather)) / (2.0 * feather), 0.0, 1.0)) * std::clamp(sky.grid_polar_opacity, 0.0, 1.0)
			: 0.0;

		double cov_ticks = 0.0;
		if (sky.grid_show_ticks) {
			const double tick_spacing = std::max(sky.grid_tick_interval_deg, 0.1) * kDegToRad;
			const double tick_extent = std::max(sky.grid_tick_length, pixel_footprint * 2.0);
			if (std::abs(dec) < tick_extent) {
				const double dist_tick_ra = std::abs(std::remainder(ra, tick_spacing)) * cos_dec;
				const double cov_t = (1.0 - std::clamp((dist_tick_ra - (hw_tick - feather)) / (2.0 * feather), 0.0, 1.0))
					* (1.0 - std::clamp((std::abs(dec) - (tick_extent - feather)) / (2.0 * feather), 0.0, 1.0));
				cov_ticks = std::max(cov_ticks, cov_t);
			}
			if (std::abs(std::remainder(ra, kHalfPi) * cos_dec) < tick_extent) {
				const double dist_tick_dec = std::abs(std::remainder(dec, tick_spacing));
				const double cov_t = (1.0 - std::clamp((dist_tick_dec - (hw_tick - feather)) / (2.0 * feather), 0.0, 1.0))
					* (1.0 - std::clamp((std::abs(std::remainder(ra, kHalfPi) * cos_dec) - (tick_extent - feather)) / (2.0 * feather), 0.0, 1.0));
				cov_ticks = std::max(cov_ticks, cov_t * pole_taper);
			}
		}

		const double glow_radius = std::max(hw_maj * 6.0, pixel_footprint * 4.0);
		const double glow_dec = std::exp(-dist_dec * dist_dec / (2.0 * glow_radius * glow_radius));
		const double glow_ra = std::exp(-dist_ra * dist_ra / (2.0 * glow_radius * glow_radius)) * pole_taper;
		const double glow_factor = std::max(glow_dec, glow_ra) * std::clamp(sky.grid_glow, 0.0, 10.0);

		auto composite_layer = [&](double cov, const std::array<float, 3>& col, double boost = 1.0) {
			if (cov <= 0.0) return;
			const double a = std::clamp(cov * sky.grid_opacity, 0.0, 1.0);
			const double r = static_cast<double>(col[0]) * boost;
			const double g = static_cast<double>(col[1]) * boost;
			const double b = static_cast<double>(col[2]) * boost;
			accum_color[0] = accum_color[0] * (1.0 - a) + r * a;
			accum_color[1] = accum_color[1] * (1.0 - a) + g * a;
			accum_color[2] = accum_color[2] * (1.0 - a) + b * a;
			accum_alpha = accum_alpha + a * (1.0 - accum_alpha);
		};

		composite_layer(cov_sub, sky.grid_minor_color, 0.85);
		composite_layer(cov_major, sky.grid_major_color, 1.0 + glow_factor);
		composite_layer(cov_cap, sky.grid_polar_color, 1.1);
		composite_layer(cov_pm, sky.grid_meridian_color, 1.15);
		composite_layer(cov_eq, sky.grid_equator_color, 1.2);
		composite_layer(cov_ticks, sky.grid_tick_color, 1.3);

		return GridSampleResult{accum_color, std::clamp(accum_alpha, 0.0, 1.0)};
	}

	[[nodiscard]] static double bessel_j1_approx(double x) noexcept {
		const double ax = std::abs(x);
		if (ax < 3.75) {
			const double y = (x / 3.75) * (x / 3.75);
			return x * (0.5 + y * (-0.56249985 + y * (0.21093573 + y * (-0.03954289 + y * (0.00443319 - y * 0.00031761)))));
		}
		const double y = 3.75 / ax;
		const double f0 = 0.79788456 + y * (-0.00000077 + y * (-0.00552740 + y * (0.00009512 + y * (0.00137237 - y * 0.00072805))));
		const double theta0 = ax - 2.35619449 + y * (0.04411179 + y * (-0.00008449 + y * (-0.00076126 + y * (0.00009185 + y * 0.00008544))));
		const double result = (f0 / std::sqrt(ax)) * std::cos(theta0);
		return (x < 0.0) ? -result : result;
	}

	[[nodiscard]] static double evaluate_star_psf(
		double theta_sq,
		double sigma,
		double eff_sigma_sq,
		double pixel_footprint_rad,
		const ProceduralSkySettings& sky,
		double local_x,
		double local_y,
		StarProfileModel model
	) noexcept {
		const double theta = std::sqrt(std::max(theta_sq, 0.0));
		switch (model) {
			case StarProfileModel::PinpointDirac: {
				const double tightness = std::clamp(sky.star_dirac_concentration, 0.1, 0.999);
				const double core_w = std::max(pixel_footprint_rad * (1.0 - tightness) * 0.4, 1.0e-9);
				return std::exp(-theta_sq / (2.0 * core_w * core_w));
			}
			case StarProfileModel::CinematicBloom: {
				const double core = std::exp(-theta_sq / (2.0 * eff_sigma_sq));
				const double bloom_w = std::max(sigma * std::clamp(sky.star_bloom_radius, 1.0, 30.0), pixel_footprint_rad * 3.5);
				const double bloom = sky.star_bloom_intensity * 1.8 * std::exp(-theta / bloom_w);
				return core + bloom;
			}
			case StarProfileModel::Lorentzian: {
				const double scale_param = std::max(sky.star_profile_param, 0.05);
				const double gamma_sq = eff_sigma_sq * scale_param;
				return (gamma_sq / (gamma_sq + theta_sq)) + 0.25 * (gamma_sq / (gamma_sq + theta_sq * 0.25));
			}
			case StarProfileModel::Moffat: {
				const double beta = std::clamp(sky.star_profile_param, 1.05, 8.0);
				const double scale = std::max(eff_sigma_sq * sky.star_profile_param2, 1.0e-12);
				return std::pow(1.0 + theta_sq / scale, -beta);
			}
			case StarProfileModel::AiryDiffraction: {
				const double v = std::numbers::pi_v<double> * theta / std::max(sigma, 1.0e-8);
				if (v < 1.0e-4) return 1.0;
				const double j1 = bessel_j1_approx(v);
				const double airy_core = (2.0 * j1 / v) * (2.0 * j1 / v);
				const double ring_boost = std::max(sky.star_profile_param2, 0.1);
				return std::min(airy_core * ring_boost, 2.0);
			}
			case StarProfileModel::HardDisc: {
				const double disc_radius = std::max(sigma, pixel_footprint_rad * 0.5);
				const double softness = std::clamp(sky.star_profile_softness, 0.001, 1.0);
				const double edge_w = std::max(pixel_footprint_rad * 0.4, disc_radius * softness);
				return 1.0 - std::clamp((theta - (disc_radius - edge_w)) / std::max(2.0 * edge_w, 1.0e-9), 0.0, 1.0);
			}
			case StarProfileModel::DiffractionSpikes: {
				const double core = std::exp(-theta_sq / (2.0 * eff_sigma_sq));
				const double angle = std::atan2(local_y, local_x) + sky.star_spike_angle_deg * (std::numbers::pi_v<double> / 180.0);
				const double n_spikes = (sky.star_spike_count == 6U) ? 3.0 : ((sky.star_spike_count == 8U) ? 4.0 : 2.0);
				const double cos_spike = std::abs(std::cos(n_spikes * angle));
				const double spike_beam = std::pow(cos_spike, 32.0);
				const double spike_len = std::max(sigma * std::clamp(sky.star_spike_length, 0.5, 60.0), pixel_footprint_rad * 8.0);
				const double spike_falloff = std::exp(-theta / spike_len);
				const double spikes = sky.star_spike_intensity * 2.5 * spike_beam * spike_falloff;
				return core + spikes;
			}
			case StarProfileModel::Gaussian:
			default:
				return std::exp(-theta_sq / (2.0 * eff_sigma_sq)) + 0.06 * std::exp(-theta / (4.0 * std::max(sigma, 1.0e-8)));
		}
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
		static_cast<void>(zoom_factor);

		constexpr double kPi = SphericalPartition::kPi;
		constexpr double kTwoPi = SphericalPartition::kTwoPi;
		const uint64_t base_seed = static_cast<uint64_t>(sky.seed);

		const double eff_quality = std::max(sky.quality_factor * sky.star_quality_factor, 0.05);
		const double density_eff = std::clamp(sky.star_density * eff_quality, 0.01, 100.0);
		const double base_grid_res = std::clamp(14.0 * std::sqrt(density_eff), 6.0, 120.0);
		const double pixel_footprint_rad = std::max(fov_rad / 1080.0, 1.0e-7);
		const auto profile_model = static_cast<StarProfileModel>(sky.star_profile_model);

		const auto& pn = sky.nebula_plane_normal;
		const double pn_len = std::sqrt(pn[0] * pn[0] + pn[1] * pn[1] + pn[2] * pn[2]);
		const std::array<double, 3> plane_norm = (pn_len > 1.0e-9)
			? std::array<double, 3>{pn[0] / pn_len, pn[1] / pn_len, pn[2] / pn_len}
			: std::array<double, 3>{0.0, 0.4472135955, 0.894427191};

		const double grid_res = base_grid_res;
		const uint64_t population_seed = base_seed;

		const std::array<double, 3> p{dir[0] * grid_res, dir[1] * grid_res, dir[2] * grid_res};
		const int32_t cx = static_cast<int32_t>(std::floor(p[0]));
		const int32_t cy = static_cast<int32_t>(std::floor(p[1]));
		const int32_t cz = static_cast<int32_t>(std::floor(p[2]));

		const double presence_threshold = std::clamp(1.0 - (sky.star_density * eff_quality) * 0.16, 0.01, 0.99);

		for (int32_t dz = -1; dz <= 1; ++dz) {
			for (int32_t dy = -1; dy <= 1; ++dy) {
				for (int32_t dx = -1; dx <= 1; ++dx) {
					const int32_t ix = cx + dx;
					const int32_t iy = cy + dy;
					const int32_t iz = cz + dz;
					const uint64_t cell_hash = SphericalPartition::hash_3d(ix, iy, iz, population_seed);

					if (SphericalPartition::hash_to_unit(cell_hash) <= presence_threshold) continue;

						const double ox = static_cast<double>(ix) + SphericalPartition::hash_to_unit(cell_hash + 11ULL);
						const double oy = static_cast<double>(iy) + SphericalPartition::hash_to_unit(cell_hash + 23ULL);
						const double oz = static_cast<double>(iz) + SphericalPartition::hash_to_unit(cell_hash + 37ULL);
						const double d_len = std::sqrt(ox * ox + oy * oy + oz * oz);
						if (d_len <= 1.0e-9) continue;
						std::array<double, 3> star_dir{ox / d_len, oy / d_len, oz / d_len};

						if (sky.star_placement_model == static_cast<uint32_t>(StarPlacementModel::GalacticDisk)) {
							const double h_plane = star_dir[0] * plane_norm[0] + star_dir[1] * plane_norm[1] + star_dir[2] * plane_norm[2];
							const double p_disk = std::exp(-std::abs(h_plane) * std::clamp(sky.star_galactic_concentration, 0.2, 20.0));
							if (SphericalPartition::hash_to_unit(cell_hash + 101ULL) > (p_disk * 0.95 + 0.05)) continue;
						} else if (sky.star_placement_model == static_cast<uint32_t>(StarPlacementModel::FractalClustered)) {
							const float cl_noise = fbm3d(static_cast<float>(star_dir[0] * 3.8), static_cast<float>(star_dir[1] * 3.8), static_cast<float>(star_dir[2] * 3.8), 3, 0.65f);
							const double cl_p = std::pow(std::clamp((static_cast<double>(cl_noise) + 0.6) * 0.85, 0.0, 1.0), 3.0);
							if (SphericalPartition::hash_to_unit(cell_hash + 102ULL) > cl_p) continue;
						} else if (sky.star_placement_model == static_cast<uint32_t>(StarPlacementModel::DualPopulation)) {
							const double h_plane = star_dir[0] * plane_norm[0] + star_dir[1] * plane_norm[1] + star_dir[2] * plane_norm[2];
							const bool is_disk = SphericalPartition::hash_to_unit(cell_hash + 103ULL) > 0.30;
							if (is_disk) {
								const double p_disk = std::exp(-std::abs(h_plane) * std::clamp(sky.star_galactic_concentration, 0.5, 15.0));
								if (SphericalPartition::hash_to_unit(cell_hash + 107ULL) > p_disk) continue;
							}
						} else if (sky.star_placement_model == static_cast<uint32_t>(StarPlacementModel::SpiralArmDensity)) {
							const double h_plane = star_dir[0] * plane_norm[0] + star_dir[1] * plane_norm[1] + star_dir[2] * plane_norm[2];
							const double phi_plane = std::atan2(star_dir[1], star_dir[0]);
							const double arm_val = std::sin(2.0 * phi_plane - 4.5 * std::log(std::max(std::abs(h_plane) + 0.2, 0.05)));
							const double p_arm = std::clamp(std::exp(-std::abs(h_plane) * 3.0) * (0.15 + 0.85 * (arm_val * 0.5 + 0.5)), 0.02, 1.0);
							if (SphericalPartition::hash_to_unit(cell_hash + 109ULL) > p_arm) continue;
						} else if (sky.star_placement_model == static_cast<uint32_t>(StarPlacementModel::ConstellationAssociations)) {
							const float ast_noise = hash_noise_3d(static_cast<int32_t>(star_dir[0] * 5.0), static_cast<int32_t>(star_dir[1] * 5.0), static_cast<int32_t>(star_dir[2] * 5.0));
							if (SphericalPartition::hash_to_unit(cell_hash + 111ULL) > (static_cast<double>(std::abs(ast_noise)) * 0.9 + 0.1)) continue;
						}

						if (sky.dynamic_motion) {
							const double drift_rate = sky.motion_speed * 0.012;
							const double phase = SphericalPartition::hash_to_unit(cell_hash + 47ULL) * kTwoPi;
							const double d_angle = std::sin(sky.time_seconds * 0.18 + phase) * drift_rate;
							star_dir = rotate_vector(star_dir, d_angle * (180.0 / kPi), 0.0, 0.0);
						}

						const double mu = dir[0] * star_dir[0] + dir[1] * star_dir[1] + dir[2] * star_dir[2];
						if (mu <= 0.994) continue;
						const double theta_sq = 2.0 * std::max(0.0, 1.0 - mu);

						const double radius_rad = sample_distribution(sky.star_size_distribution, sky.star_size_min, sky.star_size_max, sky.star_size_param, cell_hash + 31ULL) * std::max(sky.star_size_scale, 0.05);

					const double subpixel_scale = (profile_model == StarProfileModel::PinpointDirac) ? 0.04 : 0.35;
					const double eff_sigma = std::sqrt(radius_rad * radius_rad * (profile_model == StarProfileModel::HardDisc ? 1.0 : 1.96) + pixel_footprint_rad * pixel_footprint_rad * subpixel_scale);
					const double eff_sigma_sq = eff_sigma * eff_sigma;

					double max_support = eff_sigma * 6.0;
					if (profile_model == StarProfileModel::DiffractionSpikes) max_support = std::max(max_support, eff_sigma * std::clamp(sky.star_spike_length, 1.0, 60.0));
					else if (profile_model == StarProfileModel::CinematicBloom) max_support = std::max(max_support, eff_sigma * std::clamp(sky.star_bloom_radius, 1.0, 30.0));
					else if (profile_model == StarProfileModel::AiryDiffraction) max_support = std::max(max_support, eff_sigma * 12.0);
					else if (profile_model == StarProfileModel::Lorentzian || profile_model == StarProfileModel::Moffat) max_support = std::max(max_support, eff_sigma * 10.0);

					const double max_support_sq = max_support * max_support;
					if (theta_sq > max_support_sq) continue;

					const double window_u = std::clamp(theta_sq / max_support_sq, 0.0, 1.0);
					const double hermite_window = (1.0 - window_u) * (1.0 - window_u);

					double lum = sample_distribution(sky.star_luminosity_distribution, 0.2, 25.0, sky.star_luminosity_param, cell_hash + 53ULL) * sky.star_brightness;

					if (sky.dynamic_motion && sky.star_scintillation > 0.0) {
						const double tw_phase = SphericalPartition::hash_to_unit(cell_hash + 67ULL) * kTwoPi;
						lum *= (1.0 + sky.star_scintillation * std::sin(sky.time_seconds * 3.8 + tw_phase));
					}

					std::array<double, 3> u1{}, u2{};
					if (std::abs(star_dir[2]) < 0.9) {
						u1 = {-star_dir[1], star_dir[0], 0.0};
					} else {
						u1 = {0.0, -star_dir[2], star_dir[1]};
					}
					const double u1_len = std::sqrt(u1[0] * u1[0] + u1[1] * u1[1] + u1[2] * u1[2]);
					u1[0] /= u1_len; u1[1] /= u1_len; u1[2] /= u1_len;
					u2 = {
						star_dir[1] * u1[2] - star_dir[2] * u1[1],
						star_dir[2] * u1[0] - star_dir[0] * u1[2],
						star_dir[0] * u1[1] - star_dir[1] * u1[0]
					};
					const std::array<double, 3> delta{dir[0] - star_dir[0], dir[1] - star_dir[1], dir[2] - star_dir[2]};
					const double loc_x = delta[0] * u1[0] + delta[1] * u1[1] + delta[2] * u1[2];
					const double loc_y = delta[0] * u2[0] + delta[1] * u2[1] + delta[2] * u2[2];

					const double psf = evaluate_star_psf(theta_sq, eff_sigma, eff_sigma_sq, pixel_footprint_rad, sky, loc_x, loc_y, profile_model);
					const double apparent_ratio = std::clamp((radius_rad * radius_rad) / eff_sigma_sq, 0.02, 4.0);
					const double total_flux = lum * apparent_ratio * psf * hermite_window;

					const double temp_norm = std::clamp(SphericalPartition::hash_to_unit(cell_hash + 79ULL) + sky.star_temperature_bias * 0.4, 0.0, 1.0);
					const auto tint = star_blackbody_tint(temp_norm, sky.star_color_variation);
					accum[0] += total_flux * static_cast<double>(tint[0]);
					accum[1] += total_flux * static_cast<double>(tint[1]);
					accum[2] += total_flux * static_cast<double>(tint[2]);
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

		const double eff_qual = std::max(sky.quality_factor * sky.nebula_quality_factor, 0.05);
		int octaves = std::clamp(static_cast<int>(static_cast<double>(sky.nebula_octaves) * std::sqrt(eff_qual)), 1, 8);
		if (sky.adaptive_fov_quality && zoom_factor > 1.2) {
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

		const uint32_t layers = std::clamp(static_cast<uint32_t>(static_cast<double>(sky.nebula_layer_count) * eff_qual + 0.4), 1U, 5U);
		double layer_emission_r = 0.0;
		double layer_emission_g = 0.0;
		double layer_emission_b = 0.0;

		const float sep = static_cast<float>(std::clamp(sky.nebula_color_separation, 0.5, 3.0));
		const float gas_contrast = static_cast<float>(std::clamp(sky.nebula_gas_contrast, 0.5, 3.0));
		const float ion_glow = static_cast<float>(std::clamp(sky.nebula_ionization_glow, 0.0, 3.0));

		for (uint32_t layer = 0; layer < layers; ++layer) {
			const float layer_offset = static_cast<float>(layer) * 17.3f * static_cast<float>(sky.nebula_layer_spread);
			const float layer_scale = 1.0f + static_cast<float>(layer) * 0.75f * static_cast<float>(sky.nebula_layer_spread);

			const float n1 = fbm3d(dx * freq * layer_scale + seed_f + layer_offset, dy * freq * layer_scale - seed_f * 0.7f, dz * freq * layer_scale + 13.5f + layer_offset, octaves, rough);
			const float e1 = std::pow(std::max(0.0f, (n1 + 0.18f) * 1.05f), 1.8f * gas_contrast);

			const float n2 = fbm3d(dx * freq * 1.85f * layer_scale - 27.1f + layer_offset * sep, dy * freq * 1.85f * layer_scale + seed_f * 0.5f, dz * freq * 1.85f * layer_scale - 11.2f, octaves, rough);
			const float e2 = std::pow(std::max(0.0f, (n2 + 0.12f) * 1.10f), 2.2f * gas_contrast) * ion_glow;

			const float n3 = fbm3d(dx * freq * 2.7f * layer_scale + 43.0f, dy * freq * 2.7f * layer_scale + layer_offset * (sep * 1.3f), dz * freq * 2.7f * layer_scale - seed_f, octaves, rough);
			const float e3 = std::pow(std::max(0.0f, (n3 + 0.05f) * 0.95f), 2.4f * gas_contrast);

			const double layer_weight = 1.0 / (1.0 + 0.45 * static_cast<double>(layer));
			layer_emission_r += (static_cast<double>(e1 * sky.nebula_emission_color[0]) + static_cast<double>(e2 * sky.nebula_secondary_color[0] * 0.35f) + static_cast<double>(e3 * sky.nebula_tertiary_color[0])) * layer_weight;
			layer_emission_g += (static_cast<double>(e1 * sky.nebula_emission_color[1] * 0.30f) + static_cast<double>(e2 * sky.nebula_secondary_color[1]) + static_cast<double>(e3 * sky.nebula_tertiary_color[1] * 0.75f)) * layer_weight;
			layer_emission_b += (static_cast<double>(e1 * sky.nebula_emission_color[2] * 0.65f) + static_cast<double>(e2 * sky.nebula_secondary_color[2] * 0.95f) + static_cast<double>(e3 * sky.nebula_tertiary_color[2] * 0.20f)) * layer_weight;
		}

		const float n_dust = ridged_fbm3d(dx * freq * 2.6f + 71.3f, dy * freq * 2.6f - 39.4f, dz * freq * 2.6f + seed_f, octaves, 0.6f);
		const float tau = std::max(0.0f, n_dust - 0.26f) * static_cast<float>(sky.nebula_absorption_density * 2.6);
		const double extinction = std::exp(-static_cast<double>(tau));

		const double eff_intensity = sky.nebula_intensity * sky.nebula_quality_factor;
		const double combined_envelope = (disc_envelope * asymmetry_envelope + core_glow) * extinction * eff_intensity;

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

				const double pos_angle = SphericalPartition::hash_to_unit(cell_hash + 17ULL) * kTwoPi;
				const double cos_pa = std::cos(pos_angle);
				const double sin_pa = std::sin(pos_angle);
				const std::array<double, 3> u1{t1[0] * cos_pa + t2[0] * sin_pa, t1[1] * cos_pa + t2[1] * sin_pa, t1[2] * cos_pa + t2[2] * sin_pa};
				const std::array<double, 3> u2{-t1[0] * sin_pa + t2[0] * cos_pa, -t1[1] * sin_pa + t2[1] * cos_pa, -t1[2] * sin_pa + t2[2] * cos_pa};

				const double inc_raw = SphericalPartition::hash_to_unit(cell_hash + 23ULL);
				const double cos_inc = std::clamp(1.0 - inc_raw * sky.galaxy_inclination_variance, 0.12, 1.0);
				const double sin_inc = std::sqrt(std::max(1.0 - cos_inc * cos_inc, 0.0));

				const std::array<double, 3> delta{dir[0] - gal_dir[0], dir[1] - gal_dir[1], dir[2] - gal_dir[2]};
				const double x_obs = (delta[0] * u1[0] + delta[1] * u1[1] + delta[2] * u1[2]) / std::max(size_rad, 1.0e-6);
				const double y_obs = (delta[0] * u2[0] + delta[1] * u2[1] + delta[2] * u2[2]) / std::max(size_rad, 1.0e-6);

				const double x_disk = x_obs;
				const double y_disk = y_obs / cos_inc;
				const double r_disk = std::sqrt(x_disk * x_disk + y_disk * y_disk);
				const double phi_disk = std::atan2(y_disk, x_disk);

				uint32_t morph = sky.galaxy_morphology_model;
				if (morph == 0U) {
					morph = static_cast<uint32_t>((cell_hash >> 12) % 7U) + 1U;
				}

				double core = 0.0;
				double disk = 0.0;
				std::array<double, 3> tint_core{1.25, 1.05, 0.70};
				std::array<double, 3> tint_disk{0.65, 0.85, 1.35};

				if (morph == 4U) {
					const double sersic_b = 2.0 * sky.galaxy_bulge_sersic_index - 0.333333;
					const double sersic_arg = sersic_b * std::pow(r_disk, 1.0 / sky.galaxy_bulge_sersic_index);
					core = std::exp(-std::clamp(sersic_arg, 0.0, 30.0));
					disk = 0.0;
					tint_core = {1.35, 1.00, 0.65};
				} else if (morph == 5U) {
					core = std::exp(-r_disk * 3.5);
					disk = std::exp(-r_disk * 1.8);
					const double ring_dist = std::abs(r_disk - 0.65);
					const double dust_ring = (1.0 - std::exp(-ring_dist * ring_dist / 0.008)) * sky.galaxy_dust_lane_opacity;
					disk *= (1.0 - dust_ring);
					tint_core = {1.20, 1.05, 0.80};
					tint_disk = {0.95, 0.95, 0.90};
				} else if (morph == 3U) {
					core = std::exp(-r_disk * r_disk * 6.0);
					const double bar_len = 0.55 * std::clamp(sky.galaxy_bar_strength, 0.1, 2.0);
					const double bar_w = 0.15;
					const double bar_profile = std::exp(-(x_disk * x_disk) / (2.0 * bar_len * bar_len) - (y_disk * y_disk) / (2.0 * bar_w * bar_w));
					const double arms = 0.5 + 0.5 * std::cos(2.0 * (phi_disk - r_disk * 3.5 * sky.galaxy_pitch_angle));
					disk = (bar_profile * 0.7 + std::exp(-r_disk * 2.2) * arms * 0.5);
				} else if (morph == 2U) {
					core = std::exp(-r_disk * 4.0);
					const float clump_n = fbm3d(static_cast<float>(x_disk * 6.0), static_cast<float>(y_disk * 6.0), static_cast<float>(cell_hash & 0xFFU), 3, 0.6f);
					const double floc_arms = 0.5 + 0.5 * std::cos(6.0 * phi_disk - r_disk * 7.0 * sky.galaxy_pitch_angle + static_cast<double>(clump_n) * 2.5);
					disk = std::exp(-r_disk * 2.5) * floc_arms * (1.0 + sky.galaxy_clumpiness * static_cast<double>(clump_n));
				} else if (morph == 6U) {
					core = std::exp(-r_disk * 2.5);
					const float irreg_n = fbm3d(static_cast<float>(x_disk * 4.0 + 13.0), static_cast<float>(y_disk * 4.0 - 7.0), static_cast<float>((cell_hash >> 4) & 0xFFU), 4, 0.7f);
					disk = std::pow(std::clamp((static_cast<double>(irreg_n) + 0.5) * 0.8, 0.0, 1.0), 2.2) * 1.5;
					tint_disk = {0.55, 0.85, 1.55};
				} else if (morph == 7U) {
					core = std::exp(-r_disk * r_disk * 8.0) * 1.2;
					const double ring_rad = 0.70;
					const double ring_thick = 0.12;
					const double ring_diff = std::abs(r_disk - ring_rad);
					const double ring_exp = std::exp(-(ring_diff * ring_diff) / (2.0 * ring_thick * ring_thick));
					disk = ring_exp * 1.4;
					tint_disk = {0.45, 0.80, 1.65};
				} else {
					core = std::exp(-r_disk * r_disk * 4.0);
					const double arms = 0.5 + 0.5 * std::cos(sky.galaxy_spiral_arms * phi_disk - r_disk * 4.8 * sky.galaxy_pitch_angle);
					disk = std::exp(-r_disk * 2.2) * arms;
				}

				if (morph != 4U && sky.galaxy_dust_lane_opacity > 0.0) {
					const double near_side = y_obs * sin_inc;
					if (near_side > 0.0) {
						const double dust_lane = std::exp(-std::abs(near_side - 0.18) * 9.0) * sky.galaxy_dust_lane_opacity;
						core *= (1.0 - dust_lane * 0.85);
						disk *= (1.0 - dust_lane * 0.65);
					}
				}

				const double c_ratio = std::clamp(sky.galaxy_core_ratio, 0.0, 1.0);
				const double final_int = (core * c_ratio + disk * (1.0 - c_ratio)) * sky.galaxy_brightness * 1.6;

				const double warm_hash = SphericalPartition::hash_to_unit(cell_hash + 29ULL);
				const double tint_mod = 0.8 + 0.4 * warm_hash;

				accum[0] += final_int * (c_ratio * tint_core[0] + (1.0 - c_ratio) * tint_disk[0]) * tint_mod;
				accum[1] += final_int * (c_ratio * tint_core[1] + (1.0 - c_ratio) * tint_disk[1]);
				accum[2] += final_int * (c_ratio * tint_core[2] + (1.0 - c_ratio) * tint_disk[2]) * (2.0 - tint_mod);
			}
		}
	}

	static void evaluate_seamless_clusters(
		const std::array<double, 3>& dir,
		const ProceduralSkySettings& sky,
		double fov_rad,
		std::array<double, 3>& accum
	) noexcept {
		if (!sky.clusters_enabled || sky.cluster_density <= 0.0) return;

		constexpr double kTwoPi = SphericalPartition::kTwoPi;
		const uint64_t base_seed = static_cast<uint64_t>(sky.seed) ^ 0xbb67ae8584caa73bULL;

		const double eff_density = sky.cluster_density * sky.cluster_quality_factor;
		const uint32_t total_rings = static_cast<uint32_t>(std::clamp(std::round(10.0 * std::sqrt(eff_density)), 3.0, 36.0));
		const double z_clamped = std::clamp(dir[2], -1.0, 1.0);
		const uint32_t ring_center = std::clamp(static_cast<uint32_t>((z_clamped + 1.0) * 0.5 * static_cast<double>(total_rings)), 0U, total_rings - 1U);
		double phi_dir = std::atan2(dir[1], dir[0]);
		if (phi_dir < 0.0) phi_dir += kTwoPi;

		const double presence_threshold = std::clamp(1.0 - eff_density * 0.16, 0.01, 0.99);

		const auto& pn = sky.nebula_plane_normal;
		const double pn_len = std::sqrt(pn[0] * pn[0] + pn[1] * pn[1] + pn[2] * pn[2]);
		const std::array<double, 3> plane_norm = (pn_len > 1.0e-9)
			? std::array<double, 3>{pn[0] / pn_len, pn[1] / pn_len, pn[2] / pn_len}
			: std::array<double, 3>{0.0, 0.4472135955, 0.894427191};

		const auto& an = sky.nebula_asymmetry_dir;
		const double an_len = std::sqrt(an[0] * an[0] + an[1] * an[1] + an[2] * an[2]);
		const std::array<double, 3> asym_dir = (an_len > 1.0e-9)
			? std::array<double, 3>{an[0] / an_len, an[1] / an_len, an[2] / an_len}
			: std::array<double, 3>{0.7071067812, 0.0, 0.7071067812};

		const double pixel_footprint_rad = fov_rad / 1080.0;
		const auto member_profile = sky.cluster_inherit_star_profile
			? static_cast<StarProfileModel>(sky.star_profile_model)
			: static_cast<StarProfileModel>(sky.cluster_star_profile_model);

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

				if (sky.cluster_placement_model == static_cast<uint32_t>(ClusterPlacementModel::GlobularHalo)) {
					const double cos_gc = cluster_center[0] * asym_dir[0] + cluster_center[1] * asym_dir[1] + cluster_center[2] * asym_dir[2];
					const double prob_gc = 1.0 / (1.0 + (1.0 - cos_gc) / std::max(sky.cluster_halo_radius * sky.cluster_halo_radius, 0.04));
					if (SphericalPartition::hash_to_unit(cell_hash + 99ULL) > prob_gc) continue;
				} else if (sky.cluster_placement_model == static_cast<uint32_t>(ClusterPlacementModel::OpenClusterPlane)) {
					const double h_cl = cluster_center[0] * plane_norm[0] + cluster_center[1] * plane_norm[1] + cluster_center[2] * plane_norm[2];
					const double prob_pl = std::exp(-(h_cl * h_cl) / std::max(sky.cluster_plane_scale_height * sky.cluster_plane_scale_height, 0.005));
					if (SphericalPartition::hash_to_unit(cell_hash + 99ULL) > prob_pl) continue;
				}

				const double mu = dir[0] * cluster_center[0] + dir[1] * cluster_center[1] + dir[2] * cluster_center[2];
				if (mu <= 0.9985) continue;
				const double sep_sq = 2.0 * std::max(0.0, 1.0 - mu);

				const double cluster_rad = sample_distribution(sky.cluster_size_distribution, sky.cluster_size_min, sky.cluster_size_max, 1.5, cell_hash + 17ULL) * sky.cluster_size_scale;
				const double rad_sq = cluster_rad * cluster_rad;
				if (sep_sq > 9.0 * rad_sq) continue;

				if (sky.cluster_halo_intensity > 0.0) {
					const double core_denom = rad_sq * 0.40 * std::max(sky.cluster_spread_factor, 0.1);
					const double plummer = 1.0 / std::pow(1.0 + sep_sq / std::max(core_denom, 1.0e-7), sky.cluster_concentration);
					const double flux = plummer * sky.cluster_brightness * std::clamp(sky.cluster_halo_intensity, 0.0, 2.0) * 0.75;

					accum[0] += flux * static_cast<double>(sky.cluster_core_tint[0]);
					accum[1] += flux * static_cast<double>(sky.cluster_core_tint[1]);
					accum[2] += flux * static_cast<double>(sky.cluster_core_tint[2]);
				}

				if (sky.cluster_stars_enabled && sky.cluster_member_count > 0U) {
					std::array<double, 3> u1{}, u2{};
					if (std::abs(cluster_center[2]) < 0.9) {
						u1 = {-cluster_center[1], cluster_center[0], 0.0};
					} else {
						u1 = {0.0, -cluster_center[2], cluster_center[1]};
					}
					const double u1_len = std::sqrt(u1[0] * u1[0] + u1[1] * u1[1] + u1[2] * u1[2]);
					u1[0] /= u1_len; u1[1] /= u1_len; u1[2] /= u1_len;
					u2 = {
						cluster_center[1] * u1[2] - cluster_center[2] * u1[1],
						cluster_center[2] * u1[0] - cluster_center[0] * u1[2],
						cluster_center[0] * u1[1] - cluster_center[1] * u1[0]
					};

					const uint32_t members = std::clamp(static_cast<uint32_t>(static_cast<double>(sky.cluster_member_count) * sky.cluster_quality_factor), 4U, 256U);
					const double r_core = cluster_rad * 0.50 * std::max(sky.cluster_spread_factor, 0.1);

					for (uint32_t m_idx = 0; m_idx < members; ++m_idx) {
						const uint64_t m_hash = SphericalPartition::hash_coords(r_idx * 1000U + s_idx, m_idx, base_seed ^ 0x3c6ef372fe94f82bULL);
						const double u_rad = std::clamp(SphericalPartition::hash_to_unit(m_hash), 0.001, 0.96);
						const double r_m = r_core * std::sqrt(std::pow(u_rad, 2.0 / 3.0) / std::max(1.0 - std::pow(u_rad, 2.0 / 3.0), 1.0e-4));
						const double phi_m = kTwoPi * SphericalPartition::hash_to_unit(m_hash + 13ULL);
						const double dx_m = r_m * std::cos(phi_m);
						const double dy_m = r_m * std::sin(phi_m);

						const double m_len = std::sqrt((cluster_center[0] + u1[0] * dx_m + u2[0] * dy_m) * (cluster_center[0] + u1[0] * dx_m + u2[0] * dy_m)
							+ (cluster_center[1] + u1[1] * dx_m + u2[1] * dy_m) * (cluster_center[1] + u1[1] * dx_m + u2[1] * dy_m)
							+ (cluster_center[2] + u1[2] * dx_m + u2[2] * dy_m) * (cluster_center[2] + u1[2] * dx_m + u2[2] * dy_m));
						const std::array<double, 3> m_star_dir{
							(cluster_center[0] + u1[0] * dx_m + u2[0] * dy_m) / m_len,
							(cluster_center[1] + u1[1] * dx_m + u2[1] * dy_m) / m_len,
							(cluster_center[2] + u1[2] * dx_m + u2[2] * dy_m) / m_len
						};

						const double mu_m = dir[0] * m_star_dir[0] + dir[1] * m_star_dir[1] + dir[2] * m_star_dir[2];
						if (mu_m <= 0.9994) continue;
						const double theta_sq_m = 2.0 * std::max(0.0, 1.0 - mu_m);
						const double m_rad = std::max(sky.cluster_member_size, 1.0e-6);
						const double eff_sigma_m = std::sqrt(m_rad * m_rad + pixel_footprint_rad * pixel_footprint_rad * 0.25);
						const double eff_sigma_sq_m = eff_sigma_m * eff_sigma_m;
						if (theta_sq_m > 16.0 * eff_sigma_sq_m) continue;

						const double psf_m = evaluate_star_psf(theta_sq_m, eff_sigma_m, eff_sigma_sq_m, pixel_footprint_rad, sky, dx_m, dy_m, member_profile);
						const double m_lum = (0.5 + 1.5 * SphericalPartition::hash_to_unit(m_hash + 29ULL)) * sky.cluster_member_brightness;
						const double temp_m = std::clamp(SphericalPartition::hash_to_unit(m_hash + 43ULL) + sky.star_temperature_bias * 0.4, 0.0, 1.0);
						const auto tint_m = star_blackbody_tint(temp_m, sky.star_color_variation);
						const double star_flux = m_lum * psf_m;

						accum[0] += star_flux * static_cast<double>(tint_m[0]) * static_cast<double>(sky.cluster_core_tint[0]);
						accum[1] += star_flux * static_cast<double>(tint_m[1]) * static_cast<double>(sky.cluster_core_tint[1]);
						accum[2] += star_flux * static_cast<double>(tint_m[2]) * static_cast<double>(sky.cluster_core_tint[2]);
					}
				}
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

	static void evaluate_quasars(
		const std::array<double, 3>& dir,
		const ProceduralSkySettings& sky,
		std::array<double, 3>& accum
	) noexcept {
		if (!sky.quasar_enabled || sky.quasar_density <= 0.0) return;
		constexpr double kTwoPi = SphericalPartition::kTwoPi;
		const uint64_t seed_q = static_cast<uint64_t>(sky.seed) ^ 0x9b05688c2b3e6c1fULL;
		const double eff_qual = std::max(sky.quality_factor * sky.exotic_quality_factor, 0.05);
		const double eff_density = std::clamp(sky.quasar_density * eff_qual, 0.01, 10.0);
		const double grid_res = std::clamp(6.0 * std::sqrt(eff_density), 4.0, 36.0);

		const std::array<double, 3> p{dir[0] * grid_res, dir[1] * grid_res, dir[2] * grid_res};
		const int32_t cx = static_cast<int32_t>(std::floor(p[0]));
		const int32_t cy = static_cast<int32_t>(std::floor(p[1]));
		const int32_t cz = static_cast<int32_t>(std::floor(p[2]));
		const double presence = std::clamp(1.0 - eff_density * 0.06, 0.05, 0.99);

		for (int32_t dz = -1; dz <= 1; ++dz) {
			for (int32_t dy = -1; dy <= 1; ++dy) {
				for (int32_t dx = -1; dx <= 1; ++dx) {
					const uint64_t h = SphericalPartition::hash_3d(cx + dx, cy + dy, cz + dz, seed_q);
					if (SphericalPartition::hash_to_unit(h) <= presence) continue;

					const double ox = static_cast<double>(cx + dx) + SphericalPartition::hash_to_unit(h + 11ULL);
					const double oy = static_cast<double>(cy + dy) + SphericalPartition::hash_to_unit(h + 23ULL);
					const double oz = static_cast<double>(cz + dz) + SphericalPartition::hash_to_unit(h + 37ULL);
					const double d_len = std::sqrt(ox * ox + oy * oy + oz * oz);
					if (d_len <= 1.0e-9) continue;
					const std::array<double, 3> q_dir{ox / d_len, oy / d_len, oz / d_len};

					const double mu = dir[0] * q_dir[0] + dir[1] * q_dir[1] + dir[2] * q_dir[2];
					if (mu <= 0.985) continue;
					const double sep_sq = 2.0 * std::max(0.0, 1.0 - mu);

					const double j_theta = SphericalPartition::hash_to_unit(h + 41ULL) * std::numbers::pi_v<double>;
					const double j_phi = SphericalPartition::hash_to_unit(h + 47ULL) * kTwoPi;
					const double st = std::sin(j_theta);
					const std::array<double, 3> jet_axis{st * std::cos(j_phi), st * std::sin(j_phi), std::cos(j_theta)};

					const double cos_view = std::abs(dir[0] * jet_axis[0] + dir[1] * jet_axis[1] + dir[2] * jet_axis[2]);
					const double gamma = std::max(sky.quasar_jet_lorentz_factor, 1.05);
					const double beta = std::sqrt(1.0 - 1.0 / (gamma * gamma));
					const double doppler_app = 1.0 / (gamma * (1.0 - beta * cos_view));
					const double doppler_boost = std::pow(std::clamp(doppler_app, 0.1, 10.0), 3.0);

					const double core_rad = std::max(sky.quasar_core_size, 0.0001);
					const double core_flux = std::exp(-sep_sq / (2.0 * core_rad * core_rad)) * sky.quasar_brightness * 1.5;

					const double d_dot_jet = (dir[0] - q_dir[0]) * jet_axis[0] + (dir[1] - q_dir[1]) * jet_axis[1] + (dir[2] - q_dir[2]) * jet_axis[2];
					const double perp_sq = std::max(sep_sq - d_dot_jet * d_dot_jet, 0.0);
					const double jet_len = std::max(sky.quasar_jet_length, 0.005);
					const double open_rad = std::max(sky.quasar_jet_opening_angle_deg, 1.0) * (std::numbers::pi_v<double> / 180.0);
					const double jet_w = core_rad + std::abs(d_dot_jet) * std::tan(open_rad);
					const double jet_prof = std::exp(-perp_sq / (2.0 * jet_w * jet_w)) * std::exp(-std::abs(d_dot_jet) / jet_len);
					const double knot = 0.8 + 0.2 * std::cos(std::abs(d_dot_jet) * 160.0);
					const double jet_flux = jet_prof * knot * sky.quasar_jet_intensity * sky.quasar_brightness * doppler_boost * 0.4;

					const double max_supp = std::max(jet_len * 2.0, core_rad * 8.0);
					const double window = std::clamp(1.0 - sep_sq / (max_supp * max_supp), 0.0, 1.0);
					const double total = (core_flux + jet_flux) * window * window;

					accum[0] += total * 0.90;
					accum[1] += total * 1.10;
					accum[2] += total * 1.60;
				}
			}
		}
	}

	static void evaluate_pulsars(
		const std::array<double, 3>& dir,
		const ProceduralSkySettings& sky,
		std::array<double, 3>& accum
	) noexcept {
		if (!sky.pulsar_enabled || sky.pulsar_brightness <= 0.0) return;
		constexpr double kTwoPi = SphericalPartition::kTwoPi;
		const uint64_t seed_p = static_cast<uint64_t>(sky.seed) ^ 0x4f1bbcdcu;
		const double eff_qual = std::max(sky.quality_factor * sky.exotic_quality_factor, 0.05);
		const double eff_density = std::clamp(sky.pulsar_density * eff_qual, 0.01, 10.0);
		const double grid_res = std::clamp(6.0 * std::sqrt(eff_density), 4.0, 36.0);

		const std::array<double, 3> p{dir[0] * grid_res, dir[1] * grid_res, dir[2] * grid_res};
		const int32_t cx = static_cast<int32_t>(std::floor(p[0]));
		const int32_t cy = static_cast<int32_t>(std::floor(p[1]));
		const int32_t cz = static_cast<int32_t>(std::floor(p[2]));
		const double presence = std::clamp(1.0 - eff_density * 0.06, 0.05, 0.99);

		for (int32_t dz = -1; dz <= 1; ++dz) {
			for (int32_t dy = -1; dy <= 1; ++dy) {
				for (int32_t dx = -1; dx <= 1; ++dx) {
					const uint64_t h = SphericalPartition::hash_3d(cx + dx, cy + dy, cz + dz, seed_p);
					if (SphericalPartition::hash_to_unit(h) <= presence) continue;

					const double ox = static_cast<double>(cx + dx) + SphericalPartition::hash_to_unit(h + 11ULL);
					const double oy = static_cast<double>(cy + dy) + SphericalPartition::hash_to_unit(h + 23ULL);
					const double oz = static_cast<double>(cz + dz) + SphericalPartition::hash_to_unit(h + 37ULL);
					const double d_len = std::sqrt(ox * ox + oy * oy + oz * oz);
					if (d_len <= 1.0e-9) continue;
					const std::array<double, 3> p_dir{ox / d_len, oy / d_len, oz / d_len};

					const double mu = dir[0] * p_dir[0] + dir[1] * p_dir[1] + dir[2] * p_dir[2];
					if (mu <= 0.996) continue;
					const double sep_sq = 2.0 * std::max(0.0, 1.0 - mu);

					const double spin_t = SphericalPartition::hash_to_unit(h + 49ULL) * std::numbers::pi_v<double>;
					const double spin_p = SphericalPartition::hash_to_unit(h + 53ULL) * kTwoPi;
					const double s_st = std::sin(spin_t);
					const std::array<double, 3> spin_axis{s_st * std::cos(spin_p), s_st * std::sin(spin_p), std::cos(spin_t)};

					const double tilt_rad = sky.pulsar_magnetic_tilt_deg * (std::numbers::pi_v<double> / 180.0);
					const double sweep_phase = std::fmod(sky.time_seconds * sky.pulsar_pulse_rate + SphericalPartition::hash_to_unit(h + 61ULL), 1.0) * kTwoPi;

					std::array<double, 3> u1{}, u2{};
					if (std::abs(spin_axis[2]) < 0.9) {
						u1 = {-spin_axis[1], spin_axis[0], 0.0};
					} else {
						u1 = {0.0, -spin_axis[2], spin_axis[1]};
					}
					const double u1_len = std::sqrt(u1[0] * u1[0] + u1[1] * u1[1] + u1[2] * u1[2]);
					u1[0] /= u1_len; u1[1] /= u1_len; u1[2] /= u1_len;
					u2 = {
						spin_axis[1] * u1[2] - spin_axis[2] * u1[1],
						spin_axis[2] * u1[0] - spin_axis[0] * u1[2],
						spin_axis[0] * u1[1] - spin_axis[1] * u1[0]
					};

					const double sin_tilt = std::sin(tilt_rad);
					const double cos_tilt = std::cos(tilt_rad);
					const std::array<double, 3> beam_vec{
						cos_tilt * spin_axis[0] + sin_tilt * (std::cos(sweep_phase) * u1[0] + std::sin(sweep_phase) * u2[0]),
						cos_tilt * spin_axis[1] + sin_tilt * (std::cos(sweep_phase) * u1[1] + std::sin(sweep_phase) * u2[1]),
						cos_tilt * spin_axis[2] + sin_tilt * (std::cos(sweep_phase) * u1[2] + std::sin(sweep_phase) * u2[2])
					};

					const double mu_beam = std::abs(dir[0] * beam_vec[0] + dir[1] * beam_vec[1] + dir[2] * beam_vec[2]);
					const double beam_rad = std::max(sky.pulsar_beam_width_deg, 0.5) * (std::numbers::pi_v<double> / 180.0);
					const double beam_sep_sq = 2.0 * std::max(0.0, 1.0 - mu_beam);
					const double beam_duty_profile = std::exp(-beam_sep_sq / (2.0 * beam_rad * beam_rad));

					const double core_rad = 0.0004;
					const double core = std::exp(-sep_sq / (2.0 * core_rad * core_rad)) * sky.pulsar_brightness * 1.8;

					const double pwn_scale = std::max(sky.pulsar_wind_nebula_scale, 0.0005);
					const double d_dot_s = (dir[0] - p_dir[0]) * spin_axis[0] + (dir[1] - p_dir[1]) * spin_axis[1] + (dir[2] - p_dir[2]) * spin_axis[2];
					const double pwn_perp_sq = std::max(sep_sq - d_dot_s * d_dot_s, 0.0);
					const double pwn_torus = std::exp(-pwn_perp_sq / (2.0 * pwn_scale * pwn_scale)) * std::exp(-std::abs(d_dot_s) / (pwn_scale * 0.4));
					const double pwn_flux = pwn_torus * sky.pulsar_wind_nebula_intensity * sky.pulsar_brightness * 0.5;

					const double total = (core * beam_duty_profile + pwn_flux);
					accum[0] += total * 0.70;
					accum[1] += total * 1.30;
					accum[2] += total * 1.15;
				}
			}
		}
	}

	static void evaluate_supernova_remnants(
		const std::array<double, 3>& dir,
		const ProceduralSkySettings& sky,
		std::array<double, 3>& accum
	) noexcept {
		if (!sky.snr_enabled || sky.snr_brightness <= 0.0) return;
		const uint64_t seed_s = static_cast<uint64_t>(sky.seed) ^ 0x7a31b9efu;
		const double eff_qual = std::max(sky.quality_factor * sky.exotic_quality_factor, 0.05);
		const double eff_density = std::clamp(sky.snr_density * eff_qual, 0.01, 10.0);
		const double grid_res = std::clamp(6.0 * std::sqrt(eff_density), 4.0, 36.0);

		const std::array<double, 3> p{dir[0] * grid_res, dir[1] * grid_res, dir[2] * grid_res};
		const int32_t cx = static_cast<int32_t>(std::floor(p[0]));
		const int32_t cy = static_cast<int32_t>(std::floor(p[1]));
		const int32_t cz = static_cast<int32_t>(std::floor(p[2]));
		const double presence = std::clamp(1.0 - eff_density * 0.06, 0.05, 0.99);

		for (int32_t dz = -1; dz <= 1; ++dz) {
			for (int32_t dy = -1; dy <= 1; ++dy) {
				for (int32_t dx = -1; dx <= 1; ++dx) {
					const uint64_t h = SphericalPartition::hash_3d(cx + dx, cy + dy, cz + dz, seed_s);
					if (SphericalPartition::hash_to_unit(h) <= presence) continue;

					const double ox = static_cast<double>(cx + dx) + SphericalPartition::hash_to_unit(h + 11ULL);
					const double oy = static_cast<double>(cy + dy) + SphericalPartition::hash_to_unit(h + 23ULL);
					const double oz = static_cast<double>(cz + dz) + SphericalPartition::hash_to_unit(h + 37ULL);
					const double d_len = std::sqrt(ox * ox + oy * oy + oz * oz);
					if (d_len <= 1.0e-9) continue;
					const std::array<double, 3> snr_dir{ox / d_len, oy / d_len, oz / d_len};

					const double mu = dir[0] * snr_dir[0] + dir[1] * snr_dir[1] + dir[2] * snr_dir[2];
					if (mu <= 0.992) continue;
					const double sep_sq = 2.0 * std::max(0.0, 1.0 - mu);

					const double base_radius = std::max(sky.snr_size * (0.8 + 0.4 * sky.snr_expansion_age), 0.003);
					const double dist = std::sqrt(sep_sq);
					if (dist > base_radius * 2.2) continue;

					const float turb = fbm3d(static_cast<float>(dir[0] * sky.snr_filament_scale * 30.0), static_cast<float>(dir[1] * sky.snr_filament_scale * 30.0), static_cast<float>(dir[2] * sky.snr_filament_scale * 30.0), 3, 0.6f);
					const double perturbed_radius = base_radius * (1.0 + sky.snr_asymmetry * 0.35 * static_cast<double>(turb));
					const double shell_thick = std::max(sky.snr_shell_thickness * base_radius, 0.0005);
					const double diff = std::abs(dist - perturbed_radius);
					const double shell_exp = std::exp(-(diff * diff) / (2.0 * shell_thick * shell_thick));
					const double shell_flux = shell_exp * sky.snr_brightness * 1.5;

					const double interior_flux = (dist < perturbed_radius)
						? (std::exp(-dist / perturbed_radius) * sky.snr_interior_glow * sky.snr_brightness * 0.4)
						: 0.0;

					const double max_rad = base_radius * 1.6;
					const double window = std::clamp(1.0 - sep_sq / (max_rad * max_rad), 0.0, 1.0);
					const double hermite = window * window;

					const double total = (shell_flux + interior_flux) * hermite;
					accum[0] += total * 1.40;
					accum[1] += total * 0.75;
					accum[2] += total * 0.95;
				}
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
		if (len_sq < 1.0e-18) return {0.0f, 0.0f, 0.0f};
		const double inv_len = 1.0 / std::sqrt(len_sq);
		const std::array<double, 3> unit_dir{dir_x * inv_len, dir_y * inv_len, dir_z * inv_len};

		double rot_yaw = sky.orientation_yaw_deg;
		if (sky.dynamic_rotation) {
			rot_yaw = std::remainder(rot_yaw + sky.rotation_rate_deg_s * sky.time_seconds, 360.0);
		}
		const std::array<double, 3> oriented_dir = rotate_vector(unit_dir, rot_yaw, sky.orientation_pitch_deg, sky.orientation_roll_deg);

		std::array<double, 3> accum{
			static_cast<double>(sky.background_color[0]),
			static_cast<double>(sky.background_color[1]),
			static_cast<double>(sky.background_color[2])
		};

		const double fov_deg = fov_rad * (180.0 / std::numbers::pi_v<double>);
		const double zoom_factor = std::clamp(sky.adaptive_fov_reference / std::max(fov_deg, 0.05), 1.0, 128.0);

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
			evaluate_seamless_clusters(oriented_dir, sky, fov_rad, accum);
		}

		if (sky.exotic_objects_enabled) {
			if (sky.quasar_enabled) evaluate_quasars(oriented_dir, sky, accum);
			if (sky.pulsar_enabled) evaluate_pulsars(oriented_dir, sky, accum);
			if (sky.snr_enabled) evaluate_supernova_remnants(oriented_dir, sky, accum);
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

		if (sky.grid_enabled) {
			const auto grid_res = sample_celestial_grid(oriented_dir, sky, fov_rad);
			if (grid_res.alpha > 0.0) {
				final_rgb[0] = static_cast<float>(static_cast<double>(final_rgb[0]) * (1.0 - grid_res.alpha) + grid_res.rgb[0] * grid_res.alpha);
				final_rgb[1] = static_cast<float>(static_cast<double>(final_rgb[1]) * (1.0 - grid_res.alpha) + grid_res.rgb[1] * grid_res.alpha);
				final_rgb[2] = static_cast<float>(static_cast<double>(final_rgb[2]) * (1.0 - grid_res.alpha) + grid_res.rgb[2] * grid_res.alpha);
			}
		}

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

		const double intensity_scale = std::max(sky.master_intensity, 0.0);

		if (panorama_source != 0U) {
			const auto sampled = Optics::SkyPanoramaLoader::instance().sample_direction(
				static_cast<Optics::SkyPanoramaId>(panorama_id),
				static_cast<Optics::SkyPanoramaQuality>(panorama_quality),
				dir_x, dir_y, dir_z, sky.orientation_yaw_deg * (std::numbers::pi_v<double> / 180.0)
			);
			if (sampled.has_value()) {
				auto panorama_rgb = apply_hue_saturation(*sampled, sky.hue_shift_deg * (std::numbers::pi_v<double> / 180.0), sky.saturation);
				panorama_rgb[0] = static_cast<float>(std::max(0.0, (static_cast<double>(panorama_rgb[0]) + static_cast<double>(sky.background_color[0])) * intensity_scale));
				panorama_rgb[1] = static_cast<float>(std::max(0.0, (static_cast<double>(panorama_rgb[1]) + static_cast<double>(sky.background_color[1])) * intensity_scale));
				panorama_rgb[2] = static_cast<float>(std::max(0.0, (static_cast<double>(panorama_rgb[2]) + static_cast<double>(sky.background_color[2])) * intensity_scale));

				if (sky.grid_enabled) {
					const double len_sq = dir_x * dir_x + dir_y * dir_y + dir_z * dir_z;
					if (len_sq > 1.0e-18) {
						const double inv_len = 1.0 / std::sqrt(len_sq);
						const std::array<double, 3> unit_dir{dir_x * inv_len, dir_y * inv_len, dir_z * inv_len};
						double rot_yaw = sky.orientation_yaw_deg;
						if (sky.dynamic_rotation) {
							rot_yaw = std::remainder(rot_yaw + sky.rotation_rate_deg_s * time_seconds, 360.0);
						}
						const std::array<double, 3> oriented_dir = rotate_vector(unit_dir, rot_yaw, sky.orientation_pitch_deg, sky.orientation_roll_deg);
						const auto grid_res = sample_celestial_grid(oriented_dir, sky, fov_rad);
						if (grid_res.alpha > 0.0) {
							panorama_rgb[0] = static_cast<float>(static_cast<double>(panorama_rgb[0]) * (1.0 - grid_res.alpha) + grid_res.rgb[0] * grid_res.alpha);
							panorama_rgb[1] = static_cast<float>(static_cast<double>(panorama_rgb[1]) * (1.0 - grid_res.alpha) + grid_res.rgb[1] * grid_res.alpha);
							panorama_rgb[2] = static_cast<float>(static_cast<double>(panorama_rgb[2]) * (1.0 - grid_res.alpha) + grid_res.rgb[2] * grid_res.alpha);
						}
					}
				}
				return panorama_rgb;
			}
		}

		ProceduralSkySettings active_sky = sky;
		active_sky.time_seconds = time_seconds;
		return sample_procedural_sky(dir_x, dir_y, dir_z, active_sky, fov_rad);
	}
};

}
