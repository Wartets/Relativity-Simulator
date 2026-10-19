#include "support/test_harness.hpp"
#include "relativistic/dark_matter/dark_matter_field.hpp"
#include "relativistic/render/gpu_types.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>

using Relativistic::Testing::TestSuite;
namespace DM = Relativistic::DarkMatter;
namespace RN = Relativistic::Render;

namespace {

constexpr std::array<float, 9> identity_basis{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};

void check_profiles(TestSuite& suite) {
	suite.section("Profile mathematics");
	bool zero_at_origin = true;
	bool monotonic = true;
	bool non_negative_derivative = true;
	for (uint32_t i = 0; i < DM::kDarkMatterProfileCount; ++i) {
		const auto type = static_cast<DM::DarkMatterProfileType>(i);
		const double shape = DM::DarkMatterProfileMath::default_shape(type);
		zero_at_origin = zero_at_origin && DM::DarkMatterProfileMath::enclosed_fraction(type, 0.0, shape) == 0.0;
		const double f1 = DM::DarkMatterProfileMath::enclosed_fraction(type, 0.25, shape);
		const double f2 = DM::DarkMatterProfileMath::enclosed_fraction(type, 1.0, shape);
		const double f3 = DM::DarkMatterProfileMath::enclosed_fraction(type, 4.0, shape);
		monotonic = monotonic && f1 >= 0.0 && f2 >= f1 && f3 >= f2;
		non_negative_derivative = non_negative_derivative && DM::DarkMatterProfileMath::fraction_derivative(type, 1.0, shape) >= 0.0;
	}
	suite.check(zero_at_origin, "the enclosed mass fraction vanishes at the centre for every profile");
	suite.check(monotonic, "the enclosed mass fraction is non-negative and non-decreasing for every profile");
	suite.check(non_negative_derivative, "the derivative of the enclosed fraction is non-negative for every profile");
}

void check_triaxial_geometry(TestSuite& suite) {
	suite.section("Triaxial ellipsoidal geometry");
	RN::GpuDarkMatterHalo halo{};
	halo.position_x = 10.0f;
	halo.position_y = 20.0f;
	halo.position_z = 30.0f;
	halo.axis_ratio_y = 0.5f;
	halo.axis_ratio_z = 0.25f;
	halo.basis = identity_basis;
	std::array<double, 3> local{};
	const double m = DM::DarkMatterLensing::ellipsoidal_radius(halo, {13.0, 24.0, 32.0}, local);
	suite.check_near(local[0], 3.0, 1e-12, "the local x coordinate equals the offset from the halo centre");
	suite.check_near(local[1], 4.0, 1e-12, "the local y coordinate equals the offset from the halo centre");
	suite.check_near(local[2], 2.0, 1e-12, "the local z coordinate equals the offset from the halo centre");
	suite.check_near(m, std::sqrt(9.0 + 64.0 + 64.0), 1e-12, "the ellipsoidal radius divides each axis by its axis ratio");
}

void check_acceleration(TestSuite& suite) {
	suite.section("Gravitational acceleration");
	RN::GpuDarkMatterHalo halo{};
	halo.profile = static_cast<uint32_t>(DM::DarkMatterProfileType::PointMass);
	halo.mass_norm = 10.0f;
	halo.scale_radius = 1.0f;
	halo.axis_ratio_y = 1.0f;
	halo.axis_ratio_z = 1.0f;
	halo.basis = identity_basis;
	halo.lens_scale = 2.5f;
	const auto plain = DM::DarkMatterLensing::halo_acceleration(halo, {5.0, 0.0, 0.0}, false);
	suite.check(plain[0] < 0.0, "the acceleration points toward the point mass");
	suite.check_at_most(std::abs(plain[1]) + std::abs(plain[2]), 1e-14, "the acceleration has no transverse component on the axis");
	suite.check_near(std::abs(plain[0]), 10.0 / 25.0, 1e-12, "the magnitude equals M / r^2");
	const auto scaled = DM::DarkMatterLensing::halo_acceleration(halo, {5.0, 0.0, 0.0}, true);
	suite.check_near(std::abs(scaled[0]), 10.0 / 25.0 * 2.5, 1e-12, "the lensing scale multiplies the acceleration when requested");
}

void check_photon_kick(TestSuite& suite) {
	suite.section("Photon deflection");
	RN::GpuDarkMatterHalo halo{};
	halo.profile = static_cast<uint32_t>(DM::DarkMatterProfileType::PointMass);
	halo.mass_norm = 4.0f;
	halo.scale_radius = 1.0f;
	halo.axis_ratio_y = 1.0f;
	halo.axis_ratio_z = 1.0f;
	halo.basis = identity_basis;
	halo.lens_scale = 1.0f;
	halo.flags = RN::DarkMatterHaloFlags::ENABLED | RN::DarkMatterHaloFlags::LENSING;
	RN::GpuDarkMatterField field{};
	field.halo_count = 1U;
	field.flags = RN::DarkMatterFieldFlags::LENSING;
	field.lensing_strength = 1.0f;
	field.step_fraction = 0.35f;
	field.halos[0] = halo;

	const std::array<double, 3> velocity{0.0, 1.0, 0.0};
	const std::array<double, 3> position{10.0, 0.0, 0.0};
	const auto kicked = DM::DarkMatterLensing::kick(field, position, velocity, 0.1);
	const double kicked_speed = std::sqrt(kicked[0] * kicked[0] + kicked[1] * kicked[1] + kicked[2] * kicked[2]);
	suite.check_near(kicked_speed, 1.0, 1e-14, "the kick rotates the photon direction without changing its speed");
	suite.check(kicked[0] < 0.0, "the kick bends the photon toward the halo");
	const double limit = DM::DarkMatterLensing::step_limit(field, position);
	suite.check(limit > 0.0, "the step limit is positive");
	suite.check_at_most(limit, 0.35 * 10.0 + 1e-12, "the step limit does not exceed the step fraction times the distance");
}

void check_emission(TestSuite& suite) {
	suite.section("Volumetric emission");
	RN::GpuDarkMatterHalo halo{};
	halo.profile = static_cast<uint32_t>(DM::DarkMatterProfileType::Hernquist);
	halo.flags = RN::DarkMatterHaloFlags::ENABLED | RN::DarkMatterHaloFlags::VISUAL;
	halo.mass_norm = 5.0f;
	halo.scale_radius = 2.0f;
	halo.truncation_radius = 10.0f;
	halo.axis_ratio_y = 1.0f;
	halo.axis_ratio_z = 1.0f;
	halo.basis = identity_basis;
	halo.visual_gain = 2.0f;
	halo.tint_r = 0.2f;
	halo.tint_g = 0.4f;
	halo.tint_b = 0.8f;
	RN::GpuDarkMatterField field{};
	field.halo_count = 1U;
	field.visual_intensity = 1.5f;
	field.halos[0] = halo;

	field.flags = 0U;
	const auto off = DM::DarkMatterLensing::emission(field, {1.0, 0.0, 0.0}, 0.5);
	suite.check(off[0] == 0.0 && off[1] == 0.0 && off[2] == 0.0, "the emission vanishes while visualization is disabled");
	field.flags = RN::DarkMatterFieldFlags::VISUALIZATION;
	const auto on = DM::DarkMatterLensing::emission(field, {1.0, 0.0, 0.0}, 0.5);
	suite.check(on[0] > 0.0 && on[1] > 0.0 && on[2] > 0.0, "the emission is positive in every channel inside the halo");
	const auto truncated = DM::DarkMatterLensing::emission(field, {15.0, 0.0, 0.0}, 0.5);
	suite.check(truncated[0] == 0.0 && truncated[1] == 0.0 && truncated[2] == 0.0, "the emission vanishes beyond the truncation radius");
}

void check_settings(TestSuite& suite) {
	suite.section("Settings, diagnostics and serialization");
	DM::DarkMatterFieldSettings field_settings{};
	field_settings.enabled = true;
	field_settings.lensing_enabled = true;
	field_settings.visualization_enabled = true;
	field_settings.affects_bodies = true;
	field_settings.lensing_strength = 1.75;
	field_settings.visual_intensity = 0.65;
	field_settings.step_fraction = 0.25;

	DM::DarkMatterHaloSettings halo{};
	halo.set_name("ValidationHalo");
	halo.profile = DM::DarkMatterProfileType::Burkert;
	halo.mass = 4.0;
	halo.scale_radius = 8.0;
	halo.concentration = 5.0;
	halo.softening = 0.1;
	halo.truncation_radius = 50.0;
	halo.position = {12.0, -4.0, 6.0};
	halo.velocity = {0.01, -0.02, 0.005};
	halo.anchor_body_id = 42U;
	halo.axis_ratio_y = 0.85;
	halo.axis_ratio_z = 0.70;
	halo.yaw_deg = 15.0;
	halo.pitch_deg = -10.0;
	halo.roll_deg = 5.0;
	halo.lens_scale = 1.3;
	halo.visual_gain = 3.2;
	halo.tint = {0.7f, 0.3f, 0.9f};

	suite.check(field_settings.add_halo(halo) == 0 && field_settings.count == 1U, "the first halo is stored at index zero");
	suite.check_near(field_settings.total_halo_mass(), 4.0, 1e-15, "the total halo mass equals the stored mass");
	suite.check(halo.normalization() > 0.0, "the profile normalization is positive");
	const double enclosed = halo.enclosed_mass(8.0);
	suite.check(enclosed > 0.0 && enclosed < halo.mass, "the mass enclosed at the scale radius lies strictly between zero and the total mass");
	suite.check(halo.circular_velocity(8.0) > 0.0 && halo.density(8.0) > 0.0, "the circular velocity and the density at the scale radius are positive");

	std::unordered_map<std::string, std::string> serialized{};
	field_settings.store(serialized);
	DM::DarkMatterFieldSettings restored{};
	restored.restore(serialized);
	suite.check(restored.enabled == field_settings.enabled && restored.count == 1U, "the enable flag and the halo count survive serialization");
	suite.check(std::abs(restored.lensing_strength - 1.75) < 1e-12 && std::abs(restored.visual_intensity - 0.65) < 1e-12 && std::abs(restored.step_fraction - 0.25) < 1e-12, "the field scalars survive serialization");

	const auto& back = restored.halos[0];
	suite.check(back.name_view() == halo.name_view() && back.profile == halo.profile && back.anchor_body_id == halo.anchor_body_id, "the name, profile and anchor of the halo survive serialization");
	const double scalar_deviation = std::max({
		std::abs(back.mass - halo.mass), std::abs(back.scale_radius - halo.scale_radius), std::abs(back.concentration - halo.concentration),
		std::abs(back.softening - halo.softening), std::abs(back.truncation_radius - halo.truncation_radius),
		std::abs(back.axis_ratio_y - halo.axis_ratio_y), std::abs(back.axis_ratio_z - halo.axis_ratio_z),
		std::abs(back.yaw_deg - halo.yaw_deg), std::abs(back.pitch_deg - halo.pitch_deg), std::abs(back.roll_deg - halo.roll_deg),
		std::abs(back.lens_scale - halo.lens_scale), std::abs(back.visual_gain - halo.visual_gain)
	});
	suite.check_at_most(scalar_deviation, 1e-12, "every scalar halo parameter survives serialization");
	suite.check_at_most(std::abs(static_cast<double>(back.tint[0]) - static_cast<double>(halo.tint[0])), 1e-6, "the tint survives serialization at single precision");
}

void check_anchor_coupling(TestSuite& suite) {
	suite.section("Anchor resolution and N-body coupling");
	DM::DarkMatterFieldSettings field_settings{};
	field_settings.enabled = true;
	field_settings.affects_bodies = true;
	DM::DarkMatterHaloSettings halo{};
	halo.mass = 5.0;
	halo.scale_radius = 10.0;
	halo.anchor_body_id = 7U;
	halo.position = {1.0, 2.0, 3.0};
	halo.velocity = {0.1, -0.2, 0.3};
	suite.check(field_settings.add_halo(halo) >= 0, "the anchored halo is accepted");

	const auto resolver = [](uint32_t id, std::array<double, 3>& out) noexcept -> bool {
		if (id == 7U) {
			out = {100.0, 200.0, 300.0};
			return true;
		}
		return false;
	};
	const auto resolved = field_settings.resolve(5.0, resolver);
	suite.check(resolved.gpu.halo_count == 1U && resolved.anchor_ids[0] == 7U && resolved.affects_bodies[0] == 1U, "the resolved field keeps the anchor and the body coupling flag");
	const double expected_x = 100.0 + 1.0 + 0.1 * 5.0;
	const double expected_y = 200.0 + 2.0 - 0.2 * 5.0;
	const double expected_z = 300.0 + 3.0 + 0.3 * 5.0;
	suite.check_at_most(std::max({std::abs(static_cast<double>(resolved.gpu.halos[0].position_x) - expected_x), std::abs(static_cast<double>(resolved.gpu.halos[0].position_y) - expected_y), std::abs(static_cast<double>(resolved.gpu.halos[0].position_z) - expected_z)}), 1e-4, "the halo follows its anchor body plus the offset advanced by its velocity");
	const auto self = resolved.body_acceleration({expected_x + 5.0, expected_y, expected_z}, 7U, 1.0);
	suite.check(self[0] == 0.0 && self[1] == 0.0 && self[2] == 0.0, "the anchor body feels no force from its own halo");
	const auto external = resolved.body_acceleration({expected_x + 5.0, expected_y, expected_z}, 9U, 1.0);
	suite.check(external[0] != 0.0, "a different body feels the halo");
}

void check_population(TestSuite& suite) {
	suite.section("Subhalo population");
	DM::DarkMatterFieldSettings field_settings{};
	DM::DarkMatterPopulationSettings population{};
	population.count = 6U;
	population.profile = DM::DarkMatterProfileType::Hernquist;
	population.minimum_mass = 0.05;
	population.maximum_mass = 0.8;
	population.inner_radius = 12.0;
	population.outer_radius = 90.0;
	population.seed = 20241029ULL;
	const uint32_t added = field_settings.spawn_population(population, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, 0U);
	suite.check(added == 6U && field_settings.count == 6U, "the generator spawns the requested number of subhalos");
	bool masses_in_range = true;
	bool radii_in_range = true;
	for (uint32_t i = 0; i < field_settings.count; ++i) {
		const auto& sub = field_settings.halos[i];
		masses_in_range = masses_in_range && sub.mass >= population.minimum_mass && sub.mass <= population.maximum_mass;
		const double r = std::sqrt(sub.position[0] * sub.position[0] + sub.position[1] * sub.position[1] + sub.position[2] * sub.position[2]);
		radii_in_range = radii_in_range && r >= population.inner_radius * 0.999 && r <= population.outer_radius * 1.001;
	}
	suite.check(masses_in_range, "every subhalo mass lies in the configured range");
	suite.check(radii_in_range, "every subhalo lies in the configured shell");
}

void check_presets(TestSuite& suite) {
	suite.section("Presets");
	bool valid = true;
	for (uint32_t i = 0; i < DM::kDarkMatterPresetCount; ++i) {
		const auto halo = DM::DarkMatterHaloSettings::from_preset(static_cast<DM::DarkMatterHaloPreset>(i), 1.0);
		valid = valid && halo.mass > 0.0 && halo.scale_radius > 0.0 && halo.normalization() > 0.0;
	}
	suite.check(valid, "every preset has a positive mass, scale radius and normalization");
}

}

int main() {
	TestSuite suite(
		"dark_matter_field",
		"Verifies the dark matter field of the renderer and the N-body coupling: enclosed mass fractions of every profile, triaxial ellipsoidal geometry, gravitational acceleration and lensing scale, photon deflection and step limit, volumetric emission, settings serialization, anchor resolution, subhalo population generation and presets."
	);
	check_profiles(suite);
	check_triaxial_geometry(suite);
	check_acceleration(suite);
	check_photon_kick(suite);
	check_emission(suite);
	check_settings(suite);
	check_anchor_coupling(suite);
	check_population(suite);
	check_presets(suite);
	return suite.finish();
}
