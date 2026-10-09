#include "relativistic/dark_matter/dark_matter_field.hpp"
#include "relativistic/render/gpu_types.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <unordered_map>

static void test_assert(bool condition) {
	if (!condition) {
		std::abort();
	}
}

static void test_profiles_enclosed_mass_and_derivatives() {
	for (uint32_t i = 0; i < Relativistic::DarkMatter::kDarkMatterProfileCount; ++i) {
		const auto type = static_cast<Relativistic::DarkMatter::DarkMatterProfileType>(i);
		const double shape = Relativistic::DarkMatter::DarkMatterProfileMath::default_shape(type);
		test_assert(Relativistic::DarkMatter::DarkMatterProfileMath::enclosed_fraction(type, 0.0, shape) == 0.0);

		const double f1 = Relativistic::DarkMatter::DarkMatterProfileMath::enclosed_fraction(type, 0.25, shape);
		const double f2 = Relativistic::DarkMatter::DarkMatterProfileMath::enclosed_fraction(type, 1.0, shape);
		const double f3 = Relativistic::DarkMatter::DarkMatterProfileMath::enclosed_fraction(type, 4.0, shape);

		test_assert(f1 >= 0.0);
		test_assert(f2 >= f1);
		test_assert(f3 >= f2);

		const double d1 = Relativistic::DarkMatter::DarkMatterProfileMath::fraction_derivative(type, 1.0, shape);
		test_assert(d1 >= 0.0);
	}
}

static void test_triaxial_ellipsoidal_geometry() {
	Relativistic::Render::GpuDarkMatterHalo halo{};
	halo.position_x = 10.0f;
	halo.position_y = 20.0f;
	halo.position_z = 30.0f;
	halo.axis_ratio_y = 0.5f;
	halo.axis_ratio_z = 0.25f;
	halo.basis = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};

	std::array<double, 3> local{};
	const std::array<double, 3> pos{13.0, 24.0, 32.0};
	const double m = Relativistic::DarkMatter::DarkMatterLensing::ellipsoidal_radius(halo, pos, local);

	test_assert(std::abs(local[0] - 3.0) < 1e-12);
	test_assert(std::abs(local[1] - 4.0) < 1e-12);
	test_assert(std::abs(local[2] - 2.0) < 1e-12);

	const double expected_m = std::sqrt(3.0 * 3.0 + (4.0 / 0.5) * (4.0 / 0.5) + (2.0 / 0.25) * (2.0 / 0.25));
	test_assert(std::abs(m - expected_m) < 1e-12);
}

static void test_gravitational_acceleration_and_scaling() {
	Relativistic::Render::GpuDarkMatterHalo halo{};
	halo.profile = static_cast<uint32_t>(Relativistic::DarkMatter::DarkMatterProfileType::PointMass);
	halo.position_x = 0.0f;
	halo.position_y = 0.0f;
	halo.position_z = 0.0f;
	halo.mass_norm = 10.0f;
	halo.scale_radius = 1.0f;
	halo.softening = 0.0f;
	halo.truncation_radius = 0.0f;
	halo.axis_ratio_y = 1.0f;
	halo.axis_ratio_z = 1.0f;
	halo.basis = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
	halo.lens_scale = 2.5f;

	const auto acc = Relativistic::DarkMatter::DarkMatterLensing::halo_acceleration(halo, {5.0, 0.0, 0.0}, false);
	test_assert(acc[0] < 0.0);
	test_assert(std::abs(acc[1]) < 1e-14);
	test_assert(std::abs(acc[2]) < 1e-14);
	test_assert(std::abs(std::abs(acc[0]) - (10.0 / 25.0)) < 1e-12);

	const auto acc_scaled = Relativistic::DarkMatter::DarkMatterLensing::halo_acceleration(halo, {5.0, 0.0, 0.0}, true);
	test_assert(std::abs(std::abs(acc_scaled[0]) - (10.0 / 25.0 * 2.5)) < 1e-12);
}

static void test_photon_deflection_kick_and_step_limit() {
	Relativistic::Render::GpuDarkMatterHalo pm_halo{};
	pm_halo.profile = static_cast<uint32_t>(Relativistic::DarkMatter::DarkMatterProfileType::PointMass);
	pm_halo.position_x = 0.0f;
	pm_halo.position_y = 0.0f;
	pm_halo.position_z = 0.0f;
	pm_halo.mass_norm = 4.0f;
	pm_halo.scale_radius = 1.0f;
	pm_halo.softening = 0.0f;
	pm_halo.truncation_radius = 0.0f;
	pm_halo.axis_ratio_y = 1.0f;
	pm_halo.axis_ratio_z = 1.0f;
	pm_halo.basis = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
	pm_halo.lens_scale = 1.0f;
	pm_halo.flags = Relativistic::Render::DarkMatterHaloFlags::ENABLED | Relativistic::Render::DarkMatterHaloFlags::LENSING;

	Relativistic::Render::GpuDarkMatterField field{};
	field.halo_count = 1U;
	field.flags = Relativistic::Render::DarkMatterFieldFlags::LENSING;
	field.lensing_strength = 1.0f;
	field.step_fraction = 0.35f;
	field.halos[0] = pm_halo;

	const std::array<double, 3> velocity{0.0, 1.0, 0.0};
	const std::array<double, 3> position{10.0, 0.0, 0.0};
	const auto kicked = Relativistic::DarkMatter::DarkMatterLensing::kick(field, position, velocity, 0.1);

	const double speed_original = std::sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2]);
	const double speed_kicked = std::sqrt(kicked[0] * kicked[0] + kicked[1] * kicked[1] + kicked[2] * kicked[2]);
	test_assert(std::abs(speed_kicked - speed_original) < 1e-14);
	test_assert(kicked[0] < 0.0);

	const double limit = Relativistic::DarkMatter::DarkMatterLensing::step_limit(field, position);
	test_assert(limit > 0.0);
	test_assert(limit <= 0.35 * 10.0 + 1e-12);
}

static void test_volumetric_emission() {
	Relativistic::Render::GpuDarkMatterHalo halo{};
	halo.profile = static_cast<uint32_t>(Relativistic::DarkMatter::DarkMatterProfileType::Hernquist);
	halo.flags = Relativistic::Render::DarkMatterHaloFlags::ENABLED | Relativistic::Render::DarkMatterHaloFlags::VISUAL;
	halo.position_x = 0.0f;
	halo.position_y = 0.0f;
	halo.position_z = 0.0f;
	halo.mass_norm = 5.0f;
	halo.scale_radius = 2.0f;
	halo.truncation_radius = 10.0f;
	halo.axis_ratio_y = 1.0f;
	halo.axis_ratio_z = 1.0f;
	halo.basis = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
	halo.visual_gain = 2.0f;
	halo.tint_r = 0.2f;
	halo.tint_g = 0.4f;
	halo.tint_b = 0.8f;

	Relativistic::Render::GpuDarkMatterField field{};
	field.halo_count = 1U;
	field.visual_intensity = 1.5f;
	field.halos[0] = halo;

	field.flags = 0U;
	const auto emission_off = Relativistic::DarkMatter::DarkMatterLensing::emission(field, {1.0, 0.0, 0.0}, 0.5);
	test_assert(emission_off[0] == 0.0 && emission_off[1] == 0.0 && emission_off[2] == 0.0);

	field.flags = Relativistic::Render::DarkMatterFieldFlags::VISUALIZATION;
	const auto emission_on = Relativistic::DarkMatter::DarkMatterLensing::emission(field, {1.0, 0.0, 0.0}, 0.5);
	test_assert(emission_on[0] > 0.0);
	test_assert(emission_on[1] > 0.0);
	test_assert(emission_on[2] > 0.0);

	const auto emission_truncated = Relativistic::DarkMatter::DarkMatterLensing::emission(field, {15.0, 0.0, 0.0}, 0.5);
	test_assert(emission_truncated[0] == 0.0 && emission_truncated[1] == 0.0 && emission_truncated[2] == 0.0);
}

static void test_settings_diagnostics_and_serialization() {
	Relativistic::DarkMatter::DarkMatterFieldSettings field_settings{};
	field_settings.enabled = true;
	field_settings.lensing_enabled = true;
	field_settings.visualization_enabled = true;
	field_settings.affects_bodies = true;
	field_settings.lensing_strength = 1.75;
	field_settings.visual_intensity = 0.65;
	field_settings.step_fraction = 0.25;

	Relativistic::DarkMatter::DarkMatterHaloSettings halo_settings{};
	halo_settings.set_name("ValidationHalo");
	halo_settings.profile = Relativistic::DarkMatter::DarkMatterProfileType::Burkert;
	halo_settings.mass = 4.0;
	halo_settings.scale_radius = 8.0;
	halo_settings.concentration = 5.0;
	halo_settings.softening = 0.1;
	halo_settings.truncation_radius = 50.0;
	halo_settings.position = {12.0, -4.0, 6.0};
	halo_settings.velocity = {0.01, -0.02, 0.005};
	halo_settings.anchor_body_id = 42U;
	halo_settings.axis_ratio_y = 0.85;
	halo_settings.axis_ratio_z = 0.70;
	halo_settings.yaw_deg = 15.0;
	halo_settings.pitch_deg = -10.0;
	halo_settings.roll_deg = 5.0;
	halo_settings.lens_scale = 1.3;
	halo_settings.visual_gain = 3.2;
	halo_settings.tint = {0.7f, 0.3f, 0.9f};

	const int added_index = field_settings.add_halo(halo_settings);
	test_assert(added_index == 0);
	test_assert(field_settings.count == 1U);
	test_assert(field_settings.total_halo_mass() == 4.0);

	const double norm = halo_settings.normalization();
	test_assert(norm > 0.0);
	const double enclosed_r = halo_settings.enclosed_mass(8.0);
	test_assert(enclosed_r > 0.0 && enclosed_r < halo_settings.mass);
	test_assert(halo_settings.circular_velocity(8.0) > 0.0);
	test_assert(halo_settings.density(8.0) > 0.0);

	std::unordered_map<std::string, std::string> serialized{};
	field_settings.store(serialized);

	Relativistic::DarkMatter::DarkMatterFieldSettings restored{};
	restored.restore(serialized);

	test_assert(restored.enabled == field_settings.enabled);
	test_assert(restored.count == 1U);
	test_assert(std::abs(restored.lensing_strength - field_settings.lensing_strength) < 1e-12);
	test_assert(std::abs(restored.visual_intensity - field_settings.visual_intensity) < 1e-12);
	test_assert(std::abs(restored.step_fraction - field_settings.step_fraction) < 1e-12);

	const auto& halo_restored = restored.halos[0];
	test_assert(halo_restored.name_view() == halo_settings.name_view());
	test_assert(halo_restored.profile == halo_settings.profile);
	test_assert(std::abs(halo_restored.mass - halo_settings.mass) < 1e-12);
	test_assert(std::abs(halo_restored.scale_radius - halo_settings.scale_radius) < 1e-12);
	test_assert(std::abs(halo_restored.concentration - halo_settings.concentration) < 1e-12);
	test_assert(std::abs(halo_restored.softening - halo_settings.softening) < 1e-12);
	test_assert(std::abs(halo_restored.truncation_radius - halo_settings.truncation_radius) < 1e-12);
	test_assert(halo_restored.anchor_body_id == halo_settings.anchor_body_id);
	test_assert(std::abs(halo_restored.axis_ratio_y - halo_settings.axis_ratio_y) < 1e-12);
	test_assert(std::abs(halo_restored.axis_ratio_z - halo_settings.axis_ratio_z) < 1e-12);
	test_assert(std::abs(halo_restored.yaw_deg - halo_settings.yaw_deg) < 1e-12);
	test_assert(std::abs(halo_restored.pitch_deg - halo_settings.pitch_deg) < 1e-12);
	test_assert(std::abs(halo_restored.roll_deg - halo_settings.roll_deg) < 1e-12);
	test_assert(std::abs(halo_restored.lens_scale - halo_settings.lens_scale) < 1e-12);
	test_assert(std::abs(halo_restored.visual_gain - halo_settings.visual_gain) < 1e-12);
	test_assert(std::abs(static_cast<double>(halo_restored.tint[0]) - static_cast<double>(halo_settings.tint[0])) < 1e-6);
}

static void test_anchor_resolution_and_nbody_coupling() {
	Relativistic::DarkMatter::DarkMatterFieldSettings field_settings{};
	field_settings.enabled = true;
	field_settings.affects_bodies = true;

	Relativistic::DarkMatter::DarkMatterHaloSettings halo_settings{};
	halo_settings.mass = 5.0;
	halo_settings.scale_radius = 10.0;
	halo_settings.anchor_body_id = 7U;
	halo_settings.position = {1.0, 2.0, 3.0};
	halo_settings.velocity = {0.1, -0.2, 0.3};
	test_assert(field_settings.add_halo(halo_settings) >= 0);

	const auto resolver = [](uint32_t id, std::array<double, 3>& out) noexcept -> bool {
		if (id == 7U) {
			out = {100.0, 200.0, 300.0};
			return true;
		}
		return false;
	};

	const auto resolved = field_settings.resolve(5.0, resolver);
	test_assert(resolved.gpu.halo_count == 1U);
	test_assert(resolved.anchor_ids[0] == 7U);
	test_assert(resolved.affects_bodies[0] == 1U);

	const double expected_x = 100.0 + 1.0 + 0.1 * 5.0;
	const double expected_y = 200.0 + 2.0 - 0.2 * 5.0;
	const double expected_z = 300.0 + 3.0 + 0.3 * 5.0;

	test_assert(std::abs(static_cast<double>(resolved.gpu.halos[0].position_x) - expected_x) < 1e-4);
	test_assert(std::abs(static_cast<double>(resolved.gpu.halos[0].position_y) - expected_y) < 1e-4);
	test_assert(std::abs(static_cast<double>(resolved.gpu.halos[0].position_z) - expected_z) < 1e-4);

	const auto self_acceleration = resolved.body_acceleration({expected_x + 5.0, expected_y, expected_z}, 7U, 1.0);
	test_assert(self_acceleration[0] == 0.0 && self_acceleration[1] == 0.0 && self_acceleration[2] == 0.0);

	const auto external_acceleration = resolved.body_acceleration({expected_x + 5.0, expected_y, expected_z}, 9U, 1.0);
	test_assert(external_acceleration[0] != 0.0);
}

static void test_subhalo_population_generator() {
	Relativistic::DarkMatter::DarkMatterFieldSettings field_settings{};
	Relativistic::DarkMatter::DarkMatterPopulationSettings population_settings{};
	population_settings.count = 6U;
	population_settings.profile = Relativistic::DarkMatter::DarkMatterProfileType::Hernquist;
	population_settings.minimum_mass = 0.05;
	population_settings.maximum_mass = 0.8;
	population_settings.inner_radius = 12.0;
	population_settings.outer_radius = 90.0;
	population_settings.seed = 20241029ULL;

	const uint32_t added = field_settings.spawn_population(population_settings, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, 0U);
	test_assert(added == 6U);
	test_assert(field_settings.count == 6U);

	for (uint32_t i = 0; i < field_settings.count; ++i) {
		const auto& subhalo = field_settings.halos[i];
		test_assert(subhalo.mass >= population_settings.minimum_mass);
		test_assert(subhalo.mass <= population_settings.maximum_mass);
		const double r = std::sqrt(subhalo.position[0] * subhalo.position[0] + subhalo.position[1] * subhalo.position[1] + subhalo.position[2] * subhalo.position[2]);
		test_assert(r >= population_settings.inner_radius * 0.999);
		test_assert(r <= population_settings.outer_radius * 1.001);
	}
}

static void test_preset_definitions() {
	for (uint32_t i = 0; i < Relativistic::DarkMatter::kDarkMatterPresetCount; ++i) {
		const auto preset = static_cast<Relativistic::DarkMatter::DarkMatterHaloPreset>(i);
		const auto halo = Relativistic::DarkMatter::DarkMatterHaloSettings::from_preset(preset, 1.0);
		test_assert(halo.mass > 0.0);
		test_assert(halo.scale_radius > 0.0);
		test_assert(halo.normalization() > 0.0);
	}
}

int main() {
	test_profiles_enclosed_mass_and_derivatives();
	test_triaxial_ellipsoidal_geometry();
	test_gravitational_acceleration_and_scaling();
	test_photon_deflection_kick_and_step_limit();
	test_volumetric_emission();
	test_settings_diagnostics_and_serialization();
	test_anchor_resolution_and_nbody_coupling();
	test_subhalo_population_generator();
	test_preset_definitions();
	return 0;
}
