#pragma once

#include "relativistic/core/constants.hpp"
#include "relativistic/dark_matter/dark_matter_profiles.hpp"
#include "relativistic/render/gpu_types.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <numbers>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Relativistic::DarkMatter {

inline constexpr size_t kMaxHalos = Render::kMaxDarkMatterHalos;
inline constexpr size_t kHaloNameCapacity = 32;

enum class DarkMatterProfileType : uint32_t {
	NavarroFrenkWhite = 0,
	Hernquist = 1,
	Burkert = 2,
	Einasto = 3,
	Plummer = 4,
	CoredIsothermal = 5,
	SingularIsothermal = 6,
	Dehnen = 7,
	PointMass = 8,
	Jaffe = 9,
	Moore = 10,
	Soliton = 11,
	BetaModel = 12
};

inline constexpr size_t kDarkMatterProfileCount = 13;

inline constexpr std::array<const char*, kDarkMatterProfileCount> kDarkMatterProfileNames{
	"Navarro-Frenk-White (Cuspy)",
	"Hernquist",
	"Burkert (Cored)",
	"Einasto",
	"Plummer Sphere",
	"Cored Isothermal",
	"Singular Isothermal Sphere",
	"Dehnen Gamma Model",
	"Point Mass Perturber",
	"Jaffe",
	"Moore (Steep Cusp)",
	"Soliton Core (Fuzzy Dark Matter)",
	"King Beta Model (Cluster)"
};

enum class DarkMatterHaloPreset : uint32_t {
	GalacticHalo = 0,
	CoredDwarf = 1,
	ClusterTriaxial = 2,
	CompactSubhalo = 3,
	IsothermalLens = 4,
	PlummerClump = 5,
	PointMassPerturber = 6,
	SolitonCore = 7,
	MooreCuspyHalo = 8,
	JaffeSpheroid = 9,
	BetaModelCluster = 10
};

inline constexpr size_t kDarkMatterPresetCount = 11;

inline constexpr std::array<const char*, kDarkMatterPresetCount> kDarkMatterPresetNames{
	"Galactic Halo (NFW)",
	"Cored Dwarf Halo (Burkert)",
	"Triaxial Cluster Halo (NFW)",
	"Compact Subhalo (Hernquist)",
	"Truncated Isothermal Lens",
	"Plummer Clump",
	"Point Mass Perturber",
	"Soliton Core (Fuzzy Dark Matter)",
	"Moore Cuspy Halo",
	"Jaffe Spheroid",
	"Beta Model Cluster Halo"
};

enum class DarkMatterVisualizationMode : uint32_t {
	Density = 0,
	FieldStrength = 1,
	CircularSpeed = 2
};

inline constexpr size_t kDarkMatterVisualizationModeCount = 3;

inline constexpr std::array<const char*, kDarkMatterVisualizationModeCount> kDarkMatterVisualizationModeNames{
	"Column Density",
	"Gravitational Field Strength",
	"Local Circular Speed Squared"
};

class DarkMatterProfileMath {
public:
	static constexpr double kInverseFourPi = 1.0 / (4.0 * std::numbers::pi_v<double>);
	static constexpr double kInverseTwoPi = 1.0 / (2.0 * std::numbers::pi_v<double>);

	[[nodiscard]] static double default_shape(DarkMatterProfileType type) noexcept {
		switch (type) {
			case DarkMatterProfileType::Einasto: return 0.17;
			case DarkMatterProfileType::Dehnen: return 1.0;
			default: return 0.0;
		}
	}

	[[nodiscard]] static const char* shape_label(DarkMatterProfileType type) noexcept {
		switch (type) {
			case DarkMatterProfileType::Einasto: return "Einasto Shape (alpha)";
			case DarkMatterProfileType::Dehnen: return "Inner Slope (gamma)";
			default: return nullptr;
		}
	}

	[[nodiscard]] static std::array<double, 2> shape_bounds(DarkMatterProfileType type) noexcept {
		switch (type) {
			case DarkMatterProfileType::Einasto: return {0.08, 1.5};
			case DarkMatterProfileType::Dehnen: return {0.0, 2.9};
			default: return {0.0, 0.0};
		}
	}

	[[nodiscard]] static double enclosed_fraction(DarkMatterProfileType type, double x, double shape) noexcept {
		if (!(x > 0.0)) return 0.0;
		switch (type) {
			case DarkMatterProfileType::NavarroFrenkWhite:
				return NFWProfile<double>(kInverseFourPi, 1.0, 1.0).enclosed_mass(x);
			case DarkMatterProfileType::Hernquist:
				return HernquistProfile<double>(1.0, 1.0, 1.0).enclosed_mass(x);
			case DarkMatterProfileType::Burkert:
				return BurkertProfile<double>(kInverseTwoPi, 1.0, 1.0).enclosed_mass(x);
			case DarkMatterProfileType::Einasto:
				return einasto_unit(shape).enclosed_mass(x);
			case DarkMatterProfileType::Plummer: {
				const double q = 1.0 + x * x;
				return x * x * x / (q * std::sqrt(q));
			}
			case DarkMatterProfileType::CoredIsothermal:
				return cored_isothermal_fraction(x);
			case DarkMatterProfileType::SingularIsothermal:
				return x;
			case DarkMatterProfileType::Dehnen:
				return std::pow(x / (1.0 + x), 3.0 - shape);
			case DarkMatterProfileType::Jaffe:
				return x / (1.0 + x);
			case DarkMatterProfileType::Moore:
				return std::log1p(x * std::sqrt(x));
			case DarkMatterProfileType::Soliton:
				return soliton_fraction(x);
			case DarkMatterProfileType::BetaModel:
				return beta_model_fraction(x);
			case DarkMatterProfileType::PointMass:
			default:
				return 1.0;
		}
	}

	[[nodiscard]] static double fraction_derivative(DarkMatterProfileType type, double x, double shape) noexcept {
		if (!(x > 0.0)) return 0.0;
		constexpr double four_pi = 4.0 * std::numbers::pi_v<double>;
		switch (type) {
			case DarkMatterProfileType::NavarroFrenkWhite:
				return four_pi * x * x * NFWProfile<double>(kInverseFourPi, 1.0, 1.0).density(x);
			case DarkMatterProfileType::Hernquist:
				return four_pi * x * x * HernquistProfile<double>(1.0, 1.0, 1.0).density(x);
			case DarkMatterProfileType::Burkert:
				return four_pi * x * x * BurkertProfile<double>(kInverseTwoPi, 1.0, 1.0).density(x);
			case DarkMatterProfileType::Einasto:
				return four_pi * x * x * einasto_unit(shape).density(x);
			case DarkMatterProfileType::Plummer: {
				const double q = 1.0 + x * x;
				return 3.0 * x * x / (q * q * std::sqrt(q));
			}
			case DarkMatterProfileType::CoredIsothermal:
				return x * x / (1.0 + x * x);
			case DarkMatterProfileType::SingularIsothermal:
				return 1.0;
			case DarkMatterProfileType::Dehnen: {
				const double k = 3.0 - shape;
				const double u = x / (1.0 + x);
				const double v = 1.0 + x;
				return k * std::pow(u, k - 1.0) / (v * v);
			}
			case DarkMatterProfileType::Jaffe: {
				const double v = 1.0 + x;
				return 1.0 / (v * v);
			}
			case DarkMatterProfileType::Moore:
				return 1.5 * std::sqrt(x) / (1.0 + x * std::sqrt(x));
			case DarkMatterProfileType::Soliton:
				return soliton_derivative(x);
			case DarkMatterProfileType::BetaModel:
				return x * x / ((1.0 + x * x) * std::sqrt(1.0 + x * x));
			case DarkMatterProfileType::PointMass:
			default:
				return 0.0;
		}
	}

	[[nodiscard]] static double shape_auxiliary(DarkMatterProfileType type, double shape) noexcept {
		return (type == DarkMatterProfileType::Einasto) ? std::lgamma(3.0 / shape) : 0.0;
	}

private:
	[[nodiscard]] static EinastoProfile<double> einasto_unit(double alpha) noexcept {
		const double s = 3.0 / alpha;
		const double d = 2.0 / alpha;
		const double rho_e = std::exp(std::log(alpha) + s * std::log(d) - d - std::lgamma(s)) * kInverseFourPi;
		return EinastoProfile<double>(rho_e, 1.0, alpha, 1.0);
	}

	static constexpr double kSolitonCoreCoefficient = 0.091;

	[[nodiscard]] static double soliton_fraction(double x) noexcept {
		constexpr double b = kSolitonCoreCoefficient;
		if (x < 0.3) {
			const double x2 = x * x;
			return x * x2 * (1.0 / 3.0 - x2 * (8.0 * b / 5.0 - x2 * 36.0 * b * b / 7.0));
		}
		const double theta = std::atan(std::sqrt(b) * x);
		const double s = std::sin(theta);
		const double c = std::cos(theta);
		const double c2 = c * c;
		double c_odd = c;
		double current = theta;
		double i12 = 0.0;
		for (int n = 2; n <= 14; n += 2) {
			current = c_odd * s / static_cast<double>(n) + static_cast<double>(n - 1) / static_cast<double>(n) * current;
			if (n == 12) i12 = current;
			c_odd *= c2;
		}
		return (current - i12) / (b * std::sqrt(b));
	}

	[[nodiscard]] static double soliton_derivative(double x) noexcept {
		const double q = 1.0 + kSolitonCoreCoefficient * x * x;
		const double q2 = q * q;
		const double q4 = q2 * q2;
		return x * x / (q4 * q4);
	}

	[[nodiscard]] static double beta_model_fraction(double x) noexcept {
		if (x < 0.15) {
			const double x2 = x * x;
			return x * x2 * (1.0 / 3.0 - x2 * (0.3 - x2 * (15.0 / 56.0 - x2 * 35.0 / 144.0)));
		}
		return std::asinh(x) - x / std::sqrt(1.0 + x * x);
	}

	[[nodiscard]] static double cored_isothermal_fraction(double x) noexcept {
		if (x < 0.3) {
			const double x2 = x * x;
			double power = x2 * x;
			double sign_value = 1.0;
			double sum = 0.0;
			for (int k = 1; k <= 10; ++k) {
				sum += sign_value * power / static_cast<double>(2 * k + 1);
				power *= x2;
				sign_value = -sign_value;
			}
			return sum;
		}
		return x - std::atan(x);
	}
};

class DarkMatterLensing {
public:
	static constexpr double kMaximumStepDeflection = 0.2;

	[[nodiscard]] static bool is_active(const Render::GpuDarkMatterField& field) noexcept {
		return field.halo_count > 0U && field.flags != 0U;
	}

	[[nodiscard]] static double ellipsoidal_radius(const Render::GpuDarkMatterHalo& halo, const std::array<double, 3>& position, std::array<double, 3>& local) noexcept {
		const double rx = position[0] - static_cast<double>(halo.position_x);
		const double ry = position[1] - static_cast<double>(halo.position_y);
		const double rz = position[2] - static_cast<double>(halo.position_z);
		local[0] = rx * static_cast<double>(halo.basis[0]) + ry * static_cast<double>(halo.basis[1]) + rz * static_cast<double>(halo.basis[2]);
		local[1] = rx * static_cast<double>(halo.basis[3]) + ry * static_cast<double>(halo.basis[4]) + rz * static_cast<double>(halo.basis[5]);
		local[2] = rx * static_cast<double>(halo.basis[6]) + ry * static_cast<double>(halo.basis[7]) + rz * static_cast<double>(halo.basis[8]);
		const double qy = std::max(static_cast<double>(halo.axis_ratio_y), 0.05);
		const double qz = std::max(static_cast<double>(halo.axis_ratio_z), 0.05);
		const double sy = local[1] / qy;
		const double sz = local[2] / qz;
		return std::sqrt(local[0] * local[0] + sy * sy + sz * sz);
	}

	[[nodiscard]] static std::array<double, 3> halo_acceleration(const Render::GpuDarkMatterHalo& halo, const std::array<double, 3>& position, bool apply_lens_scale) noexcept {
		std::array<double, 3> local{};
		const double m = ellipsoidal_radius(halo, position, local);
		if (m < 1e-12) {
			return {0.0, 0.0, 0.0};
		}
		const double a = std::max(static_cast<double>(halo.scale_radius), 1e-9);
		const double truncation = static_cast<double>(halo.truncation_radius);
		const double reach = (truncation > 0.0) ? std::min(m, truncation) : m;
		const double enclosed = static_cast<double>(halo.mass_norm) * DarkMatterProfileMath::enclosed_fraction(static_cast<DarkMatterProfileType>(halo.profile), reach / a, static_cast<double>(halo.shape));
		const double softening = static_cast<double>(halo.softening);
		const double denominator = m * m + softening * softening;
		const double magnitude = enclosed * m / (denominator * std::sqrt(denominator));
		const double qy = std::max(static_cast<double>(halo.axis_ratio_y), 0.05);
		const double qz = std::max(static_cast<double>(halo.axis_ratio_z), 0.05);
		const double lx = -magnitude * local[0] / m;
		const double ly = -magnitude * local[1] / (qy * qy * m);
		const double lz = -magnitude * local[2] / (qz * qz * m);
		const double scale = apply_lens_scale ? static_cast<double>(halo.lens_scale) : 1.0;
		return {
			scale * (lx * static_cast<double>(halo.basis[0]) + ly * static_cast<double>(halo.basis[3]) + lz * static_cast<double>(halo.basis[6])),
			scale * (lx * static_cast<double>(halo.basis[1]) + ly * static_cast<double>(halo.basis[4]) + lz * static_cast<double>(halo.basis[7])),
			scale * (lx * static_cast<double>(halo.basis[2]) + ly * static_cast<double>(halo.basis[5]) + lz * static_cast<double>(halo.basis[8]))
		};
	}

	[[nodiscard]] static std::array<double, 3> total_lensing_acceleration(const Render::GpuDarkMatterField& field, const std::array<double, 3>& position) noexcept {
		std::array<double, 3> total{0.0, 0.0, 0.0};
		const uint32_t count = std::min<uint32_t>(field.halo_count, static_cast<uint32_t>(kMaxHalos));
		for (uint32_t i = 0; i < count; ++i) {
			const auto& halo = field.halos[i];
			if ((halo.flags & Render::DarkMatterHaloFlags::LENSING) == 0U) continue;
			const auto a = halo_acceleration(halo, position, true);
			total[0] += a[0];
			total[1] += a[1];
			total[2] += a[2];
		}
		return total;
	}

	[[nodiscard]] static std::array<double, 3> kick(const Render::GpuDarkMatterField& field, const std::array<double, 3>& position, const std::array<double, 3>& velocity, double dt) noexcept {
		const double speed_sq = velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2];
		if (speed_sq < 1e-24) return velocity;
		const double speed = std::sqrt(speed_sq);
		const std::array<double, 3> direction{velocity[0] / speed, velocity[1] / speed, velocity[2] / speed};
		auto g = total_lensing_acceleration(field, position);
		const double strength = static_cast<double>(field.lensing_strength);
		g[0] *= strength;
		g[1] *= strength;
		g[2] *= strength;
		const double along = g[0] * direction[0] + g[1] * direction[1] + g[2] * direction[2];
		const std::array<double, 3> g_perp{g[0] - along * direction[0], g[1] - along * direction[1], g[2] - along * direction[2]};
		const double g_perp_length = std::sqrt(g_perp[0] * g_perp[0] + g_perp[1] * g_perp[1] + g_perp[2] * g_perp[2]);
		const double angle = 2.0 * g_perp_length * std::abs(dt) * speed;
		if (angle < 1e-18) return velocity;
		const double limiter = (angle > kMaximumStepDeflection) ? (kMaximumStepDeflection / angle) : 1.0;
		const double factor = dt * 2.0 * speed_sq * limiter;
		std::array<double, 3> kicked{velocity[0] + factor * g_perp[0], velocity[1] + factor * g_perp[1], velocity[2] + factor * g_perp[2]};
		const double kicked_length = std::sqrt(kicked[0] * kicked[0] + kicked[1] * kicked[1] + kicked[2] * kicked[2]);
		if (kicked_length < 1e-18) return velocity;
		const double renormalize = speed / kicked_length;
		kicked[0] *= renormalize;
		kicked[1] *= renormalize;
		kicked[2] *= renormalize;
		return kicked;
	}

	[[nodiscard]] static double step_limit(const Render::GpuDarkMatterField& field, const std::array<double, 3>& position) noexcept {
		double limit = 1.0e30;
		const uint32_t count = std::min<uint32_t>(field.halo_count, static_cast<uint32_t>(kMaxHalos));
		for (uint32_t i = 0; i < count; ++i) {
			const auto& halo = field.halos[i];
			if ((halo.flags & (Render::DarkMatterHaloFlags::LENSING | Render::DarkMatterHaloFlags::VISUAL)) == 0U) continue;
			std::array<double, 3> local{};
			const double m = ellipsoidal_radius(halo, position, local);
			const double a = std::max(static_cast<double>(halo.scale_radius), 1e-9);
			limit = std::min(limit, std::max(static_cast<double>(field.step_fraction) * std::max(m, 0.5 * a), 1e-4));
		}
		return limit;
	}

	[[nodiscard]] static std::array<double, 3> emission(const Render::GpuDarkMatterField& field, const std::array<double, 3>& position, double segment_length) noexcept {
		std::array<double, 3> total{0.0, 0.0, 0.0};
		if ((field.flags & Render::DarkMatterFieldFlags::VISUALIZATION) == 0U) return total;
		const uint32_t count = std::min<uint32_t>(field.halo_count, static_cast<uint32_t>(kMaxHalos));
		for (uint32_t i = 0; i < count; ++i) {
			const auto& halo = field.halos[i];
			if ((halo.flags & Render::DarkMatterHaloFlags::VISUAL) == 0U) continue;
			std::array<double, 3> local{};
			const double m = ellipsoidal_radius(halo, position, local);
			const auto mode = static_cast<DarkMatterVisualizationMode>(static_cast<uint32_t>(std::max(field.visual_mode, 0.0f) + 0.5f));
			const auto profile = static_cast<DarkMatterProfileType>(halo.profile);
			const double shape = static_cast<double>(halo.shape);
			const double truncation = static_cast<double>(halo.truncation_radius);
			const double a = std::max(static_cast<double>(halo.scale_radius), 1e-9);
			double column = 0.0;
			if (mode == DarkMatterVisualizationMode::Density) {
				if (truncation > 0.0 && m > truncation) continue;
				const double x = std::max(m / a, 1e-6);
				const double density = DarkMatterProfileMath::fraction_derivative(profile, x, shape) / (x * x);
				column = density * segment_length / a;
			} else {
				const double reach = (truncation > 0.0) ? std::min(m, truncation) : m;
				const double mass_norm = static_cast<double>(halo.mass_norm);
				const double enclosed = mass_norm * DarkMatterProfileMath::enclosed_fraction(profile, reach / a, shape);
				const double softening = static_cast<double>(halo.softening);
				const double denominator = std::max(m * m + softening * softening, 1e-24);
				const double field_strength = enclosed * m / (denominator * std::sqrt(denominator));
				const double norm = std::max(mass_norm, 1e-30);
				const double ratio = (mode == DarkMatterVisualizationMode::FieldStrength) ? (field_strength * a * a / norm) : (field_strength * m * a / norm);
				column = ratio * segment_length / a;
			}
			column = column / (1.0 + column);
			const double weight = column * static_cast<double>(halo.visual_gain);
			total[0] += static_cast<double>(halo.tint_r) * weight;
			total[1] += static_cast<double>(halo.tint_g) * weight;
			total[2] += static_cast<double>(halo.tint_b) * weight;
		}
		const double intensity = static_cast<double>(field.visual_intensity);
		total[0] *= intensity;
		total[1] *= intensity;
		total[2] *= intensity;
		return total;
	}
};

struct ResolvedDarkMatterField {
	Render::GpuDarkMatterField gpu{};
	std::array<uint32_t, kMaxHalos> anchor_ids{};
	std::array<uint8_t, kMaxHalos> affects_bodies{};

	[[nodiscard]] bool empty() const noexcept {
		return gpu.halo_count == 0U;
	}

	[[nodiscard]] std::array<double, 3> body_acceleration(const std::array<double, 3>& position, uint32_t body_id, double gravitational_constant) const noexcept {
		std::array<double, 3> total{0.0, 0.0, 0.0};
		const uint32_t count = std::min<uint32_t>(gpu.halo_count, static_cast<uint32_t>(kMaxHalos));
		for (uint32_t i = 0; i < count; ++i) {
			if (affects_bodies[i] == 0U) continue;
			if (anchor_ids[i] != 0U && anchor_ids[i] == body_id) continue;
			const auto a = DarkMatterLensing::halo_acceleration(gpu.halos[i], position, false);
			total[0] += a[0] * gravitational_constant;
			total[1] += a[1] * gravitational_constant;
			total[2] += a[2] * gravitational_constant;
		}
		return total;
	}
};

class DarkMatterRandom {
public:
	explicit DarkMatterRandom(uint64_t seed) noexcept : state_(seed ^ 0x9E3779B97F4A7C15ULL) {}

	[[nodiscard]] uint64_t next_u64() noexcept {
		state_ += 0x9E3779B97F4A7C15ULL;
		uint64_t z = state_;
		z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
		z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
		return z ^ (z >> 31);
	}

	[[nodiscard]] double next_uniform() noexcept {
		return static_cast<double>(next_u64() >> 11) * (1.0 / 9007199254740992.0);
	}

private:
	uint64_t state_;
};

struct DarkMatterHaloOrbit {
	bool enabled{false};
	double radius{0.0};
	double eccentricity{0.0};
	double angular_speed{0.0};
	double inclination_deg{0.0};
	double node_deg{0.0};
	double periapsis_deg{0.0};
	double phase_deg{0.0};

	[[nodiscard]] static double mean_motion(double gravitational_constant, double interior_mass, double semi_major_axis) noexcept {
		if (!(semi_major_axis > 0.0)) return 0.0;
		return std::sqrt(std::max(gravitational_constant * interior_mass, 0.0) / (semi_major_axis * semi_major_axis * semi_major_axis));
	}

	void sanitize() noexcept {
		const auto finite_or = [](double value, double fallback) noexcept { return std::isfinite(value) ? value : fallback; };
		radius = std::clamp(finite_or(radius, 0.0), 0.0, 1.0e15);
		eccentricity = std::clamp(finite_or(eccentricity, 0.0), 0.0, 0.95);
		angular_speed = std::clamp(finite_or(angular_speed, 0.0), -1.0e6, 1.0e6);
		inclination_deg = std::clamp(finite_or(inclination_deg, 0.0), -90.0, 90.0);
		node_deg = finite_or(node_deg, 0.0);
		periapsis_deg = finite_or(periapsis_deg, 0.0);
		phase_deg = finite_or(phase_deg, 0.0);
	}

	[[nodiscard]] std::array<double, 3> offset(double time) const noexcept {
		if (!enabled || !(radius > 0.0)) {
			return {0.0, 0.0, 0.0};
		}
		constexpr double degrees = std::numbers::pi_v<double> / 180.0;
		constexpr double two_pi = 2.0 * std::numbers::pi_v<double>;
		const double e = std::clamp(eccentricity, 0.0, 0.95);
		const double mean = std::remainder(angular_speed * time + phase_deg * degrees, two_pi);
		double eccentric = (e > 0.8) ? std::numbers::pi_v<double> : mean;
		for (int iteration = 0; iteration < 24; ++iteration) {
			const double delta = (eccentric - e * std::sin(eccentric) - mean) / (1.0 - e * std::cos(eccentric));
			eccentric -= delta;
			if (std::abs(delta) < 1e-13) break;
		}
		const double in_plane_x = radius * (std::cos(eccentric) - e);
		const double in_plane_y = radius * std::sqrt(1.0 - e * e) * std::sin(eccentric);
		const double cw = std::cos(periapsis_deg * degrees);
		const double sw = std::sin(periapsis_deg * degrees);
		const double px = in_plane_x * cw - in_plane_y * sw;
		const double py = in_plane_x * sw + in_plane_y * cw;
		const double ci = std::cos(inclination_deg * degrees);
		const double si = std::sin(inclination_deg * degrees);
		const double cn = std::cos(node_deg * degrees);
		const double sn = std::sin(node_deg * degrees);
		return {px * cn - py * ci * sn, px * sn + py * ci * cn, py * si};
	}

	[[nodiscard]] std::array<double, 3> velocity(double time) const noexcept {
		if (!enabled || !(radius > 0.0) || angular_speed == 0.0) {
			return {0.0, 0.0, 0.0};
		}
		const double h = 1.0e-4 / std::max(std::abs(angular_speed), 1.0e-9);
		const auto ahead = offset(time + h);
		const auto behind = offset(time - h);
		return {(ahead[0] - behind[0]) / (2.0 * h), (ahead[1] - behind[1]) / (2.0 * h), (ahead[2] - behind[2]) / (2.0 * h)};
	}
};

struct DarkMatterHaloSettings {
	bool enabled{true};
	std::array<char, kHaloNameCapacity> name{};
	DarkMatterProfileType profile{DarkMatterProfileType::NavarroFrenkWhite};
	double mass{8.0};
	double scale_radius{40.0};
	double concentration{10.0};
	double shape{0.0};
	double softening{0.0};
	double truncation_radius{0.0};
	std::array<double, 3> position{0.0, 0.0, 0.0};
	std::array<double, 3> velocity{0.0, 0.0, 0.0};
	uint32_t anchor_body_id{0};
	double axis_ratio_y{1.0};
	double axis_ratio_z{1.0};
	double yaw_deg{0.0};
	double pitch_deg{0.0};
	double roll_deg{0.0};
	double lens_scale{1.0};
	bool lensing_enabled{true};
	bool affects_bodies{true};
	bool visualize{true};
	double visual_gain{1.0};
	std::array<float, 3> tint{0.55f, 0.4f, 1.0f};
	DarkMatterHaloOrbit orbit{};

	DarkMatterHaloSettings() noexcept {
		set_name("Dark Matter Halo");
	}

	void set_name(std::string_view value) noexcept {
		const size_t length = std::min(value.size(), name.size() - 1);
		for (size_t i = 0; i < length; ++i) {
			name[i] = value[i];
		}
		name[length] = '\0';
	}

	[[nodiscard]] std::string_view name_view() const noexcept {
		return std::string_view(name.data());
	}

	void sanitize() noexcept {
		const auto finite_or = [](double value, double fallback) noexcept { return std::isfinite(value) ? value : fallback; };
		mass = std::clamp(finite_or(mass, 1.0), 0.0, 1.0e18);
		scale_radius = std::clamp(finite_or(scale_radius, 1.0), 1.0e-6, 1.0e15);
		concentration = std::clamp(finite_or(concentration, 10.0), 0.1, 1.0e4);
		const auto bounds = DarkMatterProfileMath::shape_bounds(profile);
		shape = (bounds[1] > bounds[0]) ? std::clamp(finite_or(shape, DarkMatterProfileMath::default_shape(profile)), bounds[0], bounds[1]) : 0.0;
		softening = std::clamp(finite_or(softening, 0.0), 0.0, 1.0e15);
		truncation_radius = std::clamp(finite_or(truncation_radius, 0.0), 0.0, 1.0e15);
		axis_ratio_y = std::clamp(finite_or(axis_ratio_y, 1.0), 0.1, 10.0);
		axis_ratio_z = std::clamp(finite_or(axis_ratio_z, 1.0), 0.1, 10.0);
		lens_scale = std::clamp(finite_or(lens_scale, 1.0), 0.0, 20.0);
		visual_gain = std::clamp(finite_or(visual_gain, 1.0), 0.0, 50.0);
		yaw_deg = finite_or(yaw_deg, 0.0);
		pitch_deg = finite_or(pitch_deg, 0.0);
		roll_deg = finite_or(roll_deg, 0.0);
		for (double& component : position) component = finite_or(component, 0.0);
		for (double& component : velocity) component = finite_or(component, 0.0);
		for (float& channel : tint) channel = std::clamp(channel, 0.0f, 1.0f);
		orbit.sanitize();
		name[name.size() - 1] = '\0';
	}

	[[nodiscard]] double mass_radius() const noexcept {
		const double outer = concentration * scale_radius;
		return (truncation_radius > 0.0) ? std::min(outer, truncation_radius) : outer;
	}

	[[nodiscard]] double normalization() const noexcept {
		const double fraction = DarkMatterProfileMath::enclosed_fraction(profile, mass_radius() / scale_radius, shape);
		return (fraction > 1e-300) ? (mass / fraction) : 0.0;
	}

	[[nodiscard]] double enclosed_mass(double radius) const noexcept {
		const double reach = (truncation_radius > 0.0) ? std::min(radius, truncation_radius) : radius;
		return normalization() * DarkMatterProfileMath::enclosed_fraction(profile, reach / scale_radius, shape);
	}

	[[nodiscard]] double circular_velocity(double radius) const noexcept {
		return (radius > 0.0) ? std::sqrt(std::max(enclosed_mass(radius), 0.0) / radius) : 0.0;
	}

	[[nodiscard]] double density(double radius) const noexcept {
		if (truncation_radius > 0.0 && radius > truncation_radius) return 0.0;
		const double x = std::max(radius / scale_radius, 1e-9);
		return normalization() * DarkMatterProfileMath::fraction_derivative(profile, x, shape) / (4.0 * std::numbers::pi_v<double> * scale_radius * scale_radius * scale_radius * x * x);
	}

	[[nodiscard]] double half_mass_radius() const noexcept {
		const double boundary = mass_radius();
		const double total = enclosed_mass(boundary);
		if (!(total > 0.0)) return 0.0;
		double low = 0.0;
		double high = boundary;
		for (int iteration = 0; iteration < 80; ++iteration) {
			const double middle = 0.5 * (low + high);
			if (enclosed_mass(middle) < 0.5 * total) {
				low = middle;
			} else {
				high = middle;
			}
		}
		return 0.5 * (low + high);
	}

	[[nodiscard]] double logarithmic_slope(double radius) const noexcept {
		if (!(radius > 0.0)) return 0.0;
		constexpr double h = 1.0e-3;
		const double r_low = radius * (1.0 - h);
		const double r_high = radius * (1.0 + h);
		const double d_low = density(r_low);
		const double d_high = density(r_high);
		if (!(d_low > 0.0) || !(d_high > 0.0)) return 0.0;
		return (std::log(d_high) - std::log(d_low)) / (std::log(r_high) - std::log(r_low));
	}

	[[nodiscard]] double dynamical_time(double radius) const noexcept {
		const double enclosed = enclosed_mass(radius);
		return (enclosed > 0.0 && radius > 0.0) ? std::sqrt(radius * radius * radius / enclosed) : 0.0;
	}

	[[nodiscard]] double mean_density(double radius) const noexcept {
		return (radius > 0.0) ? (3.0 * enclosed_mass(radius) / (4.0 * std::numbers::pi_v<double> * radius * radius * radius)) : 0.0;
	}

	[[nodiscard]] double peak_circular_velocity(double& radius_at_peak) const noexcept {
		constexpr size_t samples = 256;
		const double r_low = 0.02 * scale_radius;
		const double r_high = std::max(4.0 * mass_radius(), 20.0 * scale_radius);
		const double span = std::log(r_high / r_low);
		double best = 0.0;
		radius_at_peak = r_low;
		for (size_t i = 0; i < samples; ++i) {
			const double r = r_low * std::exp(span * static_cast<double>(i) / static_cast<double>(samples - 1));
			const double v = circular_velocity(r);
			if (v > best) {
				best = v;
				radius_at_peak = r;
			}
		}
		return best;
	}

	[[nodiscard]] std::array<double, 9> orientation_axes() const noexcept {
		constexpr double degrees = std::numbers::pi_v<double> / 180.0;
		const double cy = std::cos(yaw_deg * degrees);
		const double sy = std::sin(yaw_deg * degrees);
		const double cp = std::cos(pitch_deg * degrees);
		const double sp = std::sin(pitch_deg * degrees);
		const double cr = std::cos(roll_deg * degrees);
		const double sr = std::sin(roll_deg * degrees);
		return {
			cy * cp, sy * cp, -sp,
			cy * sp * sr - sy * cr, sy * sp * sr + cy * cr, cp * sr,
			cy * sp * cr + sy * sr, sy * sp * cr - cy * sr, cp * cr
		};
	}

	template <typename Resolver>
	[[nodiscard]] bool resolve_center(double time, Resolver&& body_position, std::array<double, 3>& out) const {
		std::array<double, 3> base{0.0, 0.0, 0.0};
		if (anchor_body_id != 0U && !body_position(anchor_body_id, base)) {
			return false;
		}
		const auto orbital = orbit.offset(time);
		for (size_t c = 0; c < 3; ++c) {
			out[c] = base[c] + position[c] + velocity[c] * time + orbital[c];
		}
		return true;
	}

	[[nodiscard]] Render::GpuDarkMatterHalo to_gpu(const std::array<double, 3>& center, bool lensing, bool visual) const noexcept {
		Render::GpuDarkMatterHalo halo{};
		halo.profile = static_cast<uint32_t>(profile);
		halo.flags = Render::DarkMatterHaloFlags::ENABLED
			| ((lensing && lensing_enabled) ? Render::DarkMatterHaloFlags::LENSING : 0U)
			| ((visual && visualize) ? Render::DarkMatterHaloFlags::VISUAL : 0U);
		halo.position_x = static_cast<float>(center[0]);
		halo.position_y = static_cast<float>(center[1]);
		halo.position_z = static_cast<float>(center[2]);
		halo.mass_norm = static_cast<float>(normalization());
		halo.scale_radius = static_cast<float>(scale_radius);
		halo.shape = static_cast<float>(shape);
		halo.softening = static_cast<float>(softening);
		halo.truncation_radius = static_cast<float>(truncation_radius);
		halo.axis_ratio_y = static_cast<float>(axis_ratio_y);
		halo.axis_ratio_z = static_cast<float>(axis_ratio_z);
		const auto axes = orientation_axes();
		for (size_t i = 0; i < 9; ++i) {
			halo.basis[i] = static_cast<float>(axes[i]);
		}
		halo.lens_scale = static_cast<float>(lens_scale);
		halo.visual_gain = static_cast<float>(visual_gain);
		halo.tint_r = tint[0];
		halo.tint_g = tint[1];
		halo.tint_b = tint[2];
		halo.shape_aux = static_cast<float>(DarkMatterProfileMath::shape_auxiliary(profile, shape));
		return halo;
	}

	[[nodiscard]] static DarkMatterHaloSettings from_preset(DarkMatterHaloPreset preset, double unit) noexcept {
		const double u = std::max(unit, 1e-3);
		DarkMatterHaloSettings halo;
		switch (preset) {
			case DarkMatterHaloPreset::GalacticHalo:
				halo.set_name("Galactic Halo");
				halo.profile = DarkMatterProfileType::NavarroFrenkWhite;
				halo.mass = 8.0 * u;
				halo.scale_radius = 40.0 * u;
				halo.concentration = 10.0;
				halo.tint = {0.5f, 0.4f, 1.0f};
				break;
			case DarkMatterHaloPreset::CoredDwarf:
				halo.set_name("Cored Dwarf Halo");
				halo.profile = DarkMatterProfileType::Burkert;
				halo.mass = 1.2 * u;
				halo.scale_radius = 15.0 * u;
				halo.concentration = 8.0;
				halo.tint = {0.35f, 0.8f, 1.0f};
				break;
			case DarkMatterHaloPreset::ClusterTriaxial:
				halo.set_name("Cluster Halo");
				halo.profile = DarkMatterProfileType::NavarroFrenkWhite;
				halo.mass = 40.0 * u;
				halo.scale_radius = 120.0 * u;
				halo.concentration = 6.0;
				halo.axis_ratio_y = 0.8;
				halo.axis_ratio_z = 0.65;
				halo.yaw_deg = 25.0;
				halo.tint = {1.0f, 0.45f, 0.6f};
				break;
			case DarkMatterHaloPreset::CompactSubhalo:
				halo.set_name("Compact Subhalo");
				halo.profile = DarkMatterProfileType::Hernquist;
				halo.mass = 0.4 * u;
				halo.scale_radius = 4.0 * u;
				halo.concentration = 20.0;
				halo.softening = 0.2 * u;
				halo.tint = {0.4f, 1.0f, 0.6f};
				break;
			case DarkMatterHaloPreset::IsothermalLens:
				halo.set_name("Isothermal Lens");
				halo.profile = DarkMatterProfileType::SingularIsothermal;
				halo.mass = 6.0 * u;
				halo.scale_radius = 10.0 * u;
				halo.concentration = 30.0;
				halo.truncation_radius = 300.0 * u;
				halo.softening = 0.5 * u;
				halo.tint = {1.0f, 0.8f, 0.35f};
				break;
			case DarkMatterHaloPreset::PlummerClump:
				halo.set_name("Plummer Clump");
				halo.profile = DarkMatterProfileType::Plummer;
				halo.mass = 1.0 * u;
				halo.scale_radius = 8.0 * u;
				halo.concentration = 10.0;
				halo.tint = {0.8f, 0.5f, 1.0f};
				break;
			case DarkMatterHaloPreset::SolitonCore:
				halo.set_name("Soliton Core");
				halo.profile = DarkMatterProfileType::Soliton;
				halo.mass = 0.8 * u;
				halo.scale_radius = 6.0 * u;
				halo.concentration = 5.0;
				halo.softening = 0.05 * u;
				halo.tint = {0.9f, 0.5f, 1.0f};
				break;
			case DarkMatterHaloPreset::MooreCuspyHalo:
				halo.set_name("Moore Cuspy Halo");
				halo.profile = DarkMatterProfileType::Moore;
				halo.mass = 12.0 * u;
				halo.scale_radius = 30.0 * u;
				halo.concentration = 12.0;
				halo.softening = 0.1 * u;
				halo.tint = {1.0f, 0.5f, 0.3f};
				break;
			case DarkMatterHaloPreset::JaffeSpheroid:
				halo.set_name("Jaffe Spheroid");
				halo.profile = DarkMatterProfileType::Jaffe;
				halo.mass = 3.0 * u;
				halo.scale_radius = 12.0 * u;
				halo.concentration = 15.0;
				halo.softening = 0.2 * u;
				halo.tint = {0.5f, 1.0f, 0.9f};
				break;
			case DarkMatterHaloPreset::BetaModelCluster:
				halo.set_name("Beta Model Cluster");
				halo.profile = DarkMatterProfileType::BetaModel;
				halo.mass = 30.0 * u;
				halo.scale_radius = 60.0 * u;
				halo.concentration = 10.0;
				halo.axis_ratio_y = 0.9;
				halo.axis_ratio_z = 0.8;
				halo.tint = {0.4f, 0.7f, 1.0f};
				break;
			case DarkMatterHaloPreset::PointMassPerturber:
			default:
				halo.set_name("Point Mass Perturber");
				halo.profile = DarkMatterProfileType::PointMass;
				halo.mass = 0.5 * u;
				halo.scale_radius = 1.0 * u;
				halo.concentration = 1.0;
				halo.softening = 0.5 * u;
				halo.visual_gain = 0.0;
				halo.tint = {1.0f, 1.0f, 1.0f};
				break;
		}
		halo.shape = DarkMatterProfileMath::default_shape(halo.profile);
		halo.sanitize();
		return halo;
	}
};

struct DarkMatterPopulationSettings {
	uint32_t count{12};
	DarkMatterProfileType profile{DarkMatterProfileType::Hernquist};
	double minimum_mass{0.05};
	double maximum_mass{0.6};
	double mass_slope{1.9};
	double inner_radius{15.0};
	double outer_radius{150.0};
	double radial_slope{1.5};
	double reference_mass{0.3};
	double reference_scale_radius{4.0};
	double concentration{12.0};
	double flattening{1.0};
	uint64_t seed{20240607ULL};
	bool lensing{true};
	bool affects_bodies{true};
	bool visualize{true};
};

enum class DarkMatterLayoutPattern : uint32_t {
	Ring = 0,
	BinaryPair = 1,
	LinearChain = 2,
	CubicLattice = 3,
	SphericalShell = 4
};

inline constexpr size_t kDarkMatterLayoutPatternCount = 5;

inline constexpr std::array<const char*, kDarkMatterLayoutPatternCount> kDarkMatterLayoutPatternNames{
	"Orbiting Ring",
	"Binary Pair",
	"Linear Chain",
	"Cubic Lattice",
	"Spherical Shell"
};

struct DarkMatterLayoutSettings {
	DarkMatterLayoutPattern pattern{DarkMatterLayoutPattern::Ring};
	uint32_t count{6};
	double radius{80.0};
	double mass_ratio{0.5};
	double inclination_deg{0.0};
	double node_deg{0.0};
	double eccentricity{0.0};
	double jitter{0.0};
	bool orbiting{true};
	uint64_t seed{777ULL};
};

struct DarkMatterPreferences {
	uint32_t default_preset{0};
	uint32_t unit_mode{0};
	double custom_unit_mass{1.0};
	uint32_t spawn_anchor_mode{0};
	double spawn_camera_distance{60.0};
	bool auto_enable_on_spawn{true};
	bool select_after_spawn{true};
	bool show_diagnostics{true};
	bool show_plots{true};
	bool show_population_tool{true};
	bool show_tracer_tool{true};
	uint32_t plot_samples{180};
	double plot_radius_span{8.0};
	double compactness_warning{0.1};

	void sanitize() noexcept {
		const auto finite_or = [](double value, double fallback) noexcept { return std::isfinite(value) ? value : fallback; };
		default_preset = std::min<uint32_t>(default_preset, static_cast<uint32_t>(kDarkMatterPresetCount - 1));
		unit_mode = std::min<uint32_t>(unit_mode, 1U);
		custom_unit_mass = std::clamp(finite_or(custom_unit_mass, 1.0), 1.0e-6, 1.0e12);
		spawn_anchor_mode = std::min<uint32_t>(spawn_anchor_mode, 2U);
		spawn_camera_distance = std::clamp(finite_or(spawn_camera_distance, 60.0), 1.0e-3, 1.0e9);
		plot_samples = std::clamp<uint32_t>(plot_samples, 32U, 2048U);
		plot_radius_span = std::clamp(finite_or(plot_radius_span, 8.0), 1.0, 1000.0);
		compactness_warning = std::clamp(finite_or(compactness_warning, 0.1), 1.0e-4, 10.0);
	}

	void store(std::unordered_map<std::string, std::string>& out) const {
		char buffer[40];
		const auto real = [&buffer](double value) {
			std::snprintf(buffer, sizeof(buffer), "%.17g", value);
			return std::string(buffer);
		};
		const auto flag = [](bool value) { return std::string(value ? "1" : "0"); };
		out["dm_pref_preset"] = std::to_string(default_preset);
		out["dm_pref_unit_mode"] = std::to_string(unit_mode);
		out["dm_pref_unit_mass"] = real(custom_unit_mass);
		out["dm_pref_spawn_anchor"] = std::to_string(spawn_anchor_mode);
		out["dm_pref_spawn_distance"] = real(spawn_camera_distance);
		out["dm_pref_auto_enable"] = flag(auto_enable_on_spawn);
		out["dm_pref_select_spawned"] = flag(select_after_spawn);
		out["dm_pref_diagnostics"] = flag(show_diagnostics);
		out["dm_pref_plots"] = flag(show_plots);
		out["dm_pref_population"] = flag(show_population_tool);
		out["dm_pref_tracers"] = flag(show_tracer_tool);
		out["dm_pref_plot_samples"] = std::to_string(plot_samples);
		out["dm_pref_plot_span"] = real(plot_radius_span);
		out["dm_pref_compactness"] = real(compactness_warning);
	}

	void restore(const std::unordered_map<std::string, std::string>& in) {
		const auto find = [&in](const char* key) -> const std::string* {
			const auto it = in.find(key);
			return (it != in.end()) ? &it->second : nullptr;
		};
		const auto real = [&find](const char* key, double fallback) {
			const std::string* value = find(key);
			if (value == nullptr) return fallback;
			const double parsed = std::strtod(value->c_str(), nullptr);
			return std::isfinite(parsed) ? parsed : fallback;
		};
		const auto flag = [&find](const char* key, bool fallback) {
			const std::string* value = find(key);
			return (value != nullptr) ? (std::strtoul(value->c_str(), nullptr, 10) != 0UL) : fallback;
		};
		const auto integer = [&find](const char* key, uint32_t fallback) {
			const std::string* value = find(key);
			return (value != nullptr) ? static_cast<uint32_t>(std::strtoul(value->c_str(), nullptr, 10)) : fallback;
		};
		default_preset = integer("dm_pref_preset", default_preset);
		unit_mode = integer("dm_pref_unit_mode", unit_mode);
		custom_unit_mass = real("dm_pref_unit_mass", custom_unit_mass);
		spawn_anchor_mode = integer("dm_pref_spawn_anchor", spawn_anchor_mode);
		spawn_camera_distance = real("dm_pref_spawn_distance", spawn_camera_distance);
		auto_enable_on_spawn = flag("dm_pref_auto_enable", auto_enable_on_spawn);
		select_after_spawn = flag("dm_pref_select_spawned", select_after_spawn);
		show_diagnostics = flag("dm_pref_diagnostics", show_diagnostics);
		show_plots = flag("dm_pref_plots", show_plots);
		show_population_tool = flag("dm_pref_population", show_population_tool);
		show_tracer_tool = flag("dm_pref_tracers", show_tracer_tool);
		plot_samples = integer("dm_pref_plot_samples", plot_samples);
		plot_radius_span = real("dm_pref_plot_span", plot_radius_span);
		compactness_warning = real("dm_pref_compactness", compactness_warning);
		sanitize();
	}
};

struct DarkMatterFieldSettings {
	bool enabled{false};
	bool lensing_enabled{true};
	bool visualization_enabled{false};
	bool affects_bodies{true};
	double lensing_strength{1.0};
	double visual_intensity{0.25};
	double step_fraction{0.35};
	uint32_t visualization_mode{0};
	DarkMatterPreferences preferences{};
	uint32_t count{0};
	std::array<DarkMatterHaloSettings, kMaxHalos> halos{};

	void sanitize() noexcept {
		const auto finite_or = [](double value, double fallback) noexcept { return std::isfinite(value) ? value : fallback; };
		lensing_strength = std::clamp(finite_or(lensing_strength, 1.0), 0.0, 8.0);
		visual_intensity = std::clamp(finite_or(visual_intensity, 0.25), 0.0, 20.0);
		step_fraction = std::clamp(finite_or(step_fraction, 0.35), 0.02, 1.0);
		visualization_mode = std::min<uint32_t>(visualization_mode, static_cast<uint32_t>(kDarkMatterVisualizationModeCount - 1));
		preferences.sanitize();
		count = std::min<uint32_t>(count, static_cast<uint32_t>(kMaxHalos));
		for (uint32_t i = 0; i < count; ++i) {
			halos[i].sanitize();
		}
	}

	[[nodiscard]] int add_halo(const DarkMatterHaloSettings& halo) noexcept {
		if (count >= kMaxHalos) {
			return -1;
		}
		halos[count] = halo;
		halos[count].sanitize();
		return static_cast<int>(count++);
	}

	bool remove_halo(size_t index) noexcept {
		if (index >= count) {
			return false;
		}
		for (size_t i = index; i + 1 < count; ++i) {
			halos[i] = halos[i + 1];
		}
		halos[count - 1] = DarkMatterHaloSettings{};
		--count;
		return true;
	}

	[[nodiscard]] int duplicate_halo(size_t index) noexcept {
		if (index >= count || count >= kMaxHalos) {
			return -1;
		}
		DarkMatterHaloSettings copy = halos[index];
		for (double& component : copy.position) {
			component += 0.25 * copy.scale_radius;
		}
		return add_halo(copy);
	}

	bool move_halo(size_t index, int delta) noexcept {
		const int target = static_cast<int>(index) + delta;
		if (index >= count || target < 0 || target >= static_cast<int>(count)) {
			return false;
		}
		std::swap(halos[index], halos[static_cast<size_t>(target)]);
		return true;
	}

	void clear() noexcept {
		for (auto& halo : halos) {
			halo = DarkMatterHaloSettings{};
		}
		count = 0;
	}

	[[nodiscard]] double total_halo_mass() const noexcept {
		double total = 0.0;
		for (uint32_t i = 0; i < count; ++i) {
			if (halos[i].enabled) total += halos[i].mass;
		}
		return total;
	}

	double rescale_total_mass(double target) noexcept {
		const double total = total_halo_mass();
		if (!(total > 0.0) || !(target > 0.0)) {
			return 1.0;
		}
		const double factor = target / total;
		for (uint32_t i = 0; i < count; ++i) {
			if (halos[i].enabled) {
				halos[i].mass *= factor;
				halos[i].sanitize();
			}
		}
		return factor;
	}

	void sort_by_mass(bool descending) noexcept {
		for (uint32_t i = 1; i < count; ++i) {
			DarkMatterHaloSettings key = halos[i];
			uint32_t j = i;
			while (j > 0 && (descending ? (halos[j - 1].mass < key.mass) : (halos[j - 1].mass > key.mass))) {
				halos[j] = halos[j - 1];
				--j;
			}
			halos[j] = key;
		}
	}

	void apply_profile_to_all(DarkMatterProfileType profile) noexcept {
		for (uint32_t i = 0; i < count; ++i) {
			halos[i].profile = profile;
			halos[i].shape = DarkMatterProfileMath::default_shape(profile);
			halos[i].sanitize();
		}
	}

	bool mirror_halo(size_t index) noexcept {
		if (index >= count) {
			return false;
		}
		DarkMatterHaloSettings& halo = halos[index];
		for (double& component : halo.position) component = -component;
		for (double& component : halo.velocity) component = -component;
		halo.orbit.periapsis_deg += 180.0;
		return true;
	}

	uint32_t spawn_layout(
		const DarkMatterLayoutSettings& layout,
		const DarkMatterHaloSettings& prototype,
		const std::array<double, 3>& center,
		const std::array<double, 3>& velocity,
		uint32_t anchor_body_id,
		double gravitational_constant,
		double central_mass
	) noexcept {
		constexpr double degrees = std::numbers::pi_v<double> / 180.0;
		DarkMatterRandom random(layout.seed);
		const uint32_t requested = (layout.pattern == DarkMatterLayoutPattern::BinaryPair)
			? 2U
			: std::clamp<uint32_t>(layout.count, 1U, static_cast<uint32_t>(kMaxHalos));
		const double extent = std::max(layout.radius, 1e-6);
		const double jitter = std::clamp(layout.jitter, 0.0, 1.0);
		const double g = std::max(gravitational_constant, 1e-30);
		uint32_t added = 0;

		const auto emit = [&](const std::array<double, 3>& offset, const DarkMatterHaloOrbit& orbit, double mass, uint32_t ordinal) noexcept {
			if (count >= kMaxHalos) return;
			DarkMatterHaloSettings halo = prototype;
			char label[kHaloNameCapacity];
			std::snprintf(label, sizeof(label), "%.20s %u", prototype.name.data(), ordinal + 1U);
			halo.set_name(label);
			halo.mass = mass;
			halo.scale_radius = prototype.scale_radius * std::cbrt(mass / std::max(prototype.mass, 1e-30));
			halo.anchor_body_id = anchor_body_id;
			halo.position = {center[0] + offset[0], center[1] + offset[1], center[2] + offset[2]};
			halo.velocity = velocity;
			halo.orbit = orbit;
			if (add_halo(halo) >= 0) ++added;
		};

		const auto place = [&](DarkMatterHaloOrbit orbit, double mass, uint32_t ordinal) noexcept {
			if (layout.orbiting) {
				orbit.enabled = true;
				emit({0.0, 0.0, 0.0}, orbit, mass, ordinal);
				return;
			}
			DarkMatterHaloOrbit probe = orbit;
			probe.enabled = true;
			probe.angular_speed = 0.0;
			const auto offset = probe.offset(0.0);
			orbit.enabled = false;
			emit(offset, orbit, mass, ordinal);
		};

		switch (layout.pattern) {
			case DarkMatterLayoutPattern::Ring: {
				const double interior = central_mass + 0.5 * prototype.mass * static_cast<double>(requested);
				for (uint32_t i = 0; i < requested; ++i) {
					DarkMatterHaloOrbit orbit;
					orbit.radius = std::max(extent * (1.0 + jitter * (2.0 * random.next_uniform() - 1.0)), 1e-3 * extent);
					orbit.eccentricity = layout.eccentricity;
					orbit.inclination_deg = layout.inclination_deg;
					orbit.node_deg = layout.node_deg;
					orbit.phase_deg = 360.0 * static_cast<double>(i) / static_cast<double>(requested);
					orbit.angular_speed = DarkMatterHaloOrbit::mean_motion(g, interior, orbit.radius);
					place(orbit, prototype.mass, i);
				}
				break;
			}
			case DarkMatterLayoutPattern::BinaryPair: {
				const double primary = prototype.mass;
				const double secondary = primary * std::max(layout.mass_ratio, 1e-3);
				const double total = primary + secondary;
				const double rate = DarkMatterHaloOrbit::mean_motion(g, total, extent);
				DarkMatterHaloOrbit first;
				first.radius = extent * secondary / total;
				first.eccentricity = layout.eccentricity;
				first.inclination_deg = layout.inclination_deg;
				first.node_deg = layout.node_deg;
				first.angular_speed = rate;
				DarkMatterHaloOrbit second = first;
				second.radius = extent * primary / total;
				second.periapsis_deg = first.periapsis_deg + 180.0;
				place(first, primary, 0U);
				place(second, secondary, 1U);
				break;
			}
			case DarkMatterLayoutPattern::LinearChain: {
				const double axis_x = std::cos(layout.node_deg * degrees) * std::cos(layout.inclination_deg * degrees);
				const double axis_y = std::sin(layout.node_deg * degrees) * std::cos(layout.inclination_deg * degrees);
				const double axis_z = std::sin(layout.inclination_deg * degrees);
				for (uint32_t i = 0; i < requested; ++i) {
					const double t = (requested > 1U) ? (-1.0 + 2.0 * static_cast<double>(i) / static_cast<double>(requested - 1U)) : 0.0;
					const double wobble = 0.1 * jitter * extent;
					emit({
						extent * t * axis_x + wobble * (random.next_uniform() - 0.5),
						extent * t * axis_y + wobble * (random.next_uniform() - 0.5),
						extent * t * axis_z + wobble * (random.next_uniform() - 0.5)
					}, DarkMatterHaloOrbit{}, prototype.mass, i);
				}
				break;
			}
			case DarkMatterLayoutPattern::CubicLattice: {
				const uint32_t side = std::max<uint32_t>(static_cast<uint32_t>(std::ceil(std::cbrt(static_cast<double>(requested)))), 1U);
				const double spacing = 2.0 * extent / static_cast<double>(side);
				const double half = 0.5 * static_cast<double>(side - 1U);
				for (uint32_t i = 0; i < requested; ++i) {
					const double gx = static_cast<double>(i % side) - half;
					const double gy = static_cast<double>((i / side) % side) - half;
					const double gz = static_cast<double>(i / (side * side)) - half;
					emit({
						spacing * (gx + jitter * (random.next_uniform() - 0.5)),
						spacing * (gy + jitter * (random.next_uniform() - 0.5)),
						spacing * (gz + jitter * (random.next_uniform() - 0.5))
					}, DarkMatterHaloOrbit{}, prototype.mass, i);
				}
				break;
			}
			case DarkMatterLayoutPattern::SphericalShell:
			default: {
				constexpr double golden_angle = 2.399963229728653;
				for (uint32_t i = 0; i < requested; ++i) {
					const double z = 1.0 - 2.0 * (static_cast<double>(i) + 0.5) / static_cast<double>(requested);
					const double rho = std::sqrt(std::max(1.0 - z * z, 0.0));
					const double phi = golden_angle * static_cast<double>(i);
					const double radius = std::max(extent * (1.0 + jitter * (2.0 * random.next_uniform() - 1.0)), 1e-3 * extent);
					emit({radius * rho * std::cos(phi), radius * rho * std::sin(phi), radius * z}, DarkMatterHaloOrbit{}, prototype.mass, i);
				}
				break;
			}
		}
		return added;
	}

	uint32_t spawn_population(const DarkMatterPopulationSettings& settings, const std::array<double, 3>& center, const std::array<double, 3>& velocity, uint32_t anchor_body_id) noexcept {
		DarkMatterRandom random(settings.seed);
		const double mass_low = std::max(settings.minimum_mass, 1e-9);
		const double mass_high = std::max(settings.maximum_mass, mass_low * 1.0001);
		const double slope = settings.mass_slope;
		const double radius_low = std::max(settings.inner_radius, 1e-6);
		const double radius_high = std::max(settings.outer_radius, radius_low * 1.0001);
		const double radial_exponent = 3.0 - settings.radial_slope;
		uint32_t added = 0;
		for (uint32_t n = 0; n < settings.count; ++n) {
			if (count >= kMaxHalos) break;
			const double u_mass = random.next_uniform();
			double mass = 0.0;
			if (std::abs(slope - 1.0) < 1e-6) {
				mass = mass_low * std::pow(mass_high / mass_low, u_mass);
			} else {
				const double e = 1.0 - slope;
				mass = std::pow(std::pow(mass_low, e) + u_mass * (std::pow(mass_high, e) - std::pow(mass_low, e)), 1.0 / e);
			}
			const double u_radius = random.next_uniform();
			double radius = 0.0;
			if (std::abs(radial_exponent) < 1e-6) {
				radius = radius_low * std::pow(radius_high / radius_low, u_radius);
			} else {
				radius = std::pow(std::pow(radius_low, radial_exponent) + u_radius * (std::pow(radius_high, radial_exponent) - std::pow(radius_low, radial_exponent)), 1.0 / radial_exponent);
			}
			const double cos_theta = 2.0 * random.next_uniform() - 1.0;
			const double sin_theta = std::sqrt(std::max(1.0 - cos_theta * cos_theta, 0.0));
			const double phi = 2.0 * std::numbers::pi_v<double> * random.next_uniform();
			DarkMatterHaloSettings halo;
			char label[kHaloNameCapacity];
			std::snprintf(label, sizeof(label), "Subhalo %u", n + 1U);
			halo.set_name(label);
			halo.profile = settings.profile;
			halo.mass = mass;
			halo.scale_radius = settings.reference_scale_radius * std::cbrt(mass / std::max(settings.reference_mass, 1e-9));
			halo.concentration = settings.concentration;
			halo.shape = DarkMatterProfileMath::default_shape(settings.profile);
			halo.softening = 0.1 * halo.scale_radius;
			halo.anchor_body_id = anchor_body_id;
			halo.position = {
				center[0] + radius * sin_theta * std::cos(phi),
				center[1] + radius * sin_theta * std::sin(phi),
				center[2] + radius * cos_theta * settings.flattening
			};
			halo.velocity = velocity;
			halo.lensing_enabled = settings.lensing;
			halo.affects_bodies = settings.affects_bodies;
			halo.visualize = settings.visualize;
			halo.tint = {
				static_cast<float>(0.4 + 0.6 * random.next_uniform()),
				static_cast<float>(0.4 + 0.6 * random.next_uniform()),
				static_cast<float>(0.7 + 0.3 * random.next_uniform())
			};
			if (add_halo(halo) >= 0) ++added;
		}
		return added;
	}

	template <typename Resolver>
	[[nodiscard]] ResolvedDarkMatterField resolve(double time, Resolver&& body_position) const {
		ResolvedDarkMatterField result;
		if (!enabled) {
			return result;
		}
		result.gpu.lensing_strength = static_cast<float>(lensing_strength);
		result.gpu.visual_intensity = static_cast<float>(visual_intensity);
		result.gpu.step_fraction = static_cast<float>(step_fraction);
		result.gpu.visual_mode = static_cast<float>(std::min<uint32_t>(visualization_mode, static_cast<uint32_t>(kDarkMatterVisualizationModeCount - 1)));
		uint32_t used = 0;
		uint32_t flags = 0;
		for (uint32_t i = 0; i < count && used < kMaxHalos; ++i) {
			const DarkMatterHaloSettings& halo = halos[i];
			if (!halo.enabled || !(halo.mass > 0.0)) continue;
			std::array<double, 3> center{};
			if (!halo.resolve_center(time, body_position, center)) continue;
			const bool lensing = lensing_enabled && halo.lensing_enabled;
			const bool visual = visualization_enabled && halo.visualize;
			result.gpu.halos[used] = halo.to_gpu(center, lensing, visual);
			result.anchor_ids[used] = halo.anchor_body_id;
			result.affects_bodies[used] = (affects_bodies && halo.affects_bodies) ? 1U : 0U;
			if (lensing) flags |= Render::DarkMatterFieldFlags::LENSING;
			if (visual && halo.visual_gain > 0.0) flags |= Render::DarkMatterFieldFlags::VISUALIZATION;
			++used;
		}
		result.gpu.halo_count = used;
		result.gpu.flags = flags;
		return result;
	}

	void store(std::unordered_map<std::string, std::string>& out) const {
		const auto real = [](double value) {
			char buffer[40];
			std::snprintf(buffer, sizeof(buffer), "%.17g", value);
			return std::string(buffer);
		};
		const auto flag = [](bool value) { return std::string(value ? "1" : "0"); };
		out["dm_enabled"] = flag(enabled);
		out["dm_lensing"] = flag(lensing_enabled);
		out["dm_visualization"] = flag(visualization_enabled);
		out["dm_affects_bodies"] = flag(affects_bodies);
		out["dm_lensing_strength"] = real(lensing_strength);
		out["dm_visual_intensity"] = real(visual_intensity);
		out["dm_step_fraction"] = real(step_fraction);
		out["dm_count"] = std::to_string(count);
		out["dm_vis_mode"] = std::to_string(visualization_mode);
		preferences.store(out);
		for (uint32_t i = 0; i < count; ++i) {
			const DarkMatterHaloSettings& h = halos[i];
			const std::string p = "dm_h" + std::to_string(i) + "_";
			out[p + "enabled"] = flag(h.enabled);
			out[p + "name"] = std::string(h.name_view());
			out[p + "profile"] = std::to_string(static_cast<uint32_t>(h.profile));
			out[p + "mass"] = real(h.mass);
			out[p + "scale"] = real(h.scale_radius);
			out[p + "concentration"] = real(h.concentration);
			out[p + "shape"] = real(h.shape);
			out[p + "softening"] = real(h.softening);
			out[p + "truncation"] = real(h.truncation_radius);
			out[p + "px"] = real(h.position[0]);
			out[p + "py"] = real(h.position[1]);
			out[p + "pz"] = real(h.position[2]);
			out[p + "vx"] = real(h.velocity[0]);
			out[p + "vy"] = real(h.velocity[1]);
			out[p + "vz"] = real(h.velocity[2]);
			out[p + "anchor"] = std::to_string(h.anchor_body_id);
			out[p + "qy"] = real(h.axis_ratio_y);
			out[p + "qz"] = real(h.axis_ratio_z);
			out[p + "yaw"] = real(h.yaw_deg);
			out[p + "pitch"] = real(h.pitch_deg);
			out[p + "roll"] = real(h.roll_deg);
			out[p + "lens_scale"] = real(h.lens_scale);
			out[p + "lensing"] = flag(h.lensing_enabled);
			out[p + "bodies"] = flag(h.affects_bodies);
			out[p + "visualize"] = flag(h.visualize);
			out[p + "gain"] = real(h.visual_gain);
			out[p + "tr"] = real(h.tint[0]);
			out[p + "tg"] = real(h.tint[1]);
			out[p + "tb"] = real(h.tint[2]);
			out[p + "orbit_on"] = flag(h.orbit.enabled);
			out[p + "orbit_a"] = real(h.orbit.radius);
			out[p + "orbit_e"] = real(h.orbit.eccentricity);
			out[p + "orbit_w"] = real(h.orbit.angular_speed);
			out[p + "orbit_i"] = real(h.orbit.inclination_deg);
			out[p + "orbit_n"] = real(h.orbit.node_deg);
			out[p + "orbit_p"] = real(h.orbit.periapsis_deg);
			out[p + "orbit_m"] = real(h.orbit.phase_deg);
		}
	}

	void restore(const std::unordered_map<std::string, std::string>& in) {
		const auto find = [&in](const std::string& key) -> const std::string* {
			const auto it = in.find(key);
			return (it != in.end()) ? &it->second : nullptr;
		};
		const auto real = [&find](const std::string& key, double fallback) {
			const std::string* value = find(key);
			if (value == nullptr) return fallback;
			const double parsed = std::strtod(value->c_str(), nullptr);
			return std::isfinite(parsed) ? parsed : fallback;
		};
		const auto flag = [&find](const std::string& key, bool fallback) {
			const std::string* value = find(key);
			return (value != nullptr) ? (std::strtoul(value->c_str(), nullptr, 10) != 0UL) : fallback;
		};
		const auto integer = [&find](const std::string& key, uint32_t fallback) {
			const std::string* value = find(key);
			return (value != nullptr) ? static_cast<uint32_t>(std::strtoul(value->c_str(), nullptr, 10)) : fallback;
		};
		if (find("dm_count") == nullptr) {
			return;
		}
		enabled = flag("dm_enabled", enabled);
		lensing_enabled = flag("dm_lensing", lensing_enabled);
		visualization_enabled = flag("dm_visualization", visualization_enabled);
		affects_bodies = flag("dm_affects_bodies", affects_bodies);
		lensing_strength = real("dm_lensing_strength", lensing_strength);
		visual_intensity = real("dm_visual_intensity", visual_intensity);
		step_fraction = real("dm_step_fraction", step_fraction);
		visualization_mode = std::min<uint32_t>(integer("dm_vis_mode", visualization_mode), static_cast<uint32_t>(kDarkMatterVisualizationModeCount - 1));
		preferences.restore(in);
		clear();
		count = std::min<uint32_t>(integer("dm_count", 0U), static_cast<uint32_t>(kMaxHalos));
		for (uint32_t i = 0; i < count; ++i) {
			DarkMatterHaloSettings& h = halos[i];
			const std::string p = "dm_h" + std::to_string(i) + "_";
			h.enabled = flag(p + "enabled", true);
			if (const std::string* label = find(p + "name")) h.set_name(*label);
			h.profile = static_cast<DarkMatterProfileType>(std::min<uint32_t>(integer(p + "profile", 0U), static_cast<uint32_t>(kDarkMatterProfileCount - 1)));
			h.mass = real(p + "mass", h.mass);
			h.scale_radius = real(p + "scale", h.scale_radius);
			h.concentration = real(p + "concentration", h.concentration);
			h.shape = real(p + "shape", h.shape);
			h.softening = real(p + "softening", 0.0);
			h.truncation_radius = real(p + "truncation", 0.0);
			h.position = {real(p + "px", 0.0), real(p + "py", 0.0), real(p + "pz", 0.0)};
			h.velocity = {real(p + "vx", 0.0), real(p + "vy", 0.0), real(p + "vz", 0.0)};
			h.anchor_body_id = integer(p + "anchor", 0U);
			h.axis_ratio_y = real(p + "qy", 1.0);
			h.axis_ratio_z = real(p + "qz", 1.0);
			h.yaw_deg = real(p + "yaw", 0.0);
			h.pitch_deg = real(p + "pitch", 0.0);
			h.roll_deg = real(p + "roll", 0.0);
			h.lens_scale = real(p + "lens_scale", 1.0);
			h.lensing_enabled = flag(p + "lensing", true);
			h.affects_bodies = flag(p + "bodies", true);
			h.visualize = flag(p + "visualize", true);
			h.visual_gain = real(p + "gain", 1.0);
			h.tint = {static_cast<float>(real(p + "tr", 0.55)), static_cast<float>(real(p + "tg", 0.4)), static_cast<float>(real(p + "tb", 1.0))};
			h.orbit.enabled = flag(p + "orbit_on", false);
			h.orbit.radius = real(p + "orbit_a", 0.0);
			h.orbit.eccentricity = real(p + "orbit_e", 0.0);
			h.orbit.angular_speed = real(p + "orbit_w", 0.0);
			h.orbit.inclination_deg = real(p + "orbit_i", 0.0);
			h.orbit.node_deg = real(p + "orbit_n", 0.0);
			h.orbit.periapsis_deg = real(p + "orbit_p", 0.0);
			h.orbit.phase_deg = real(p + "orbit_m", 0.0);
		}
		sanitize();
	}
};

}
