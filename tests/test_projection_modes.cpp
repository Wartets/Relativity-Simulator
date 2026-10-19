#include "support/test_harness.hpp"
#include "relativistic/observer/camera_projections.hpp"
#include "relativistic/observer/direction_projection.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <string>

using Relativistic::Observer::CameraProjector;
using Relativistic::Observer::ProjectionMode;
using Relativistic::Observer::direction_to_screen_uv;
using Relativistic::Testing::TestSuite;

namespace {

constexpr std::array<const char*, 8> kModeNames{
	"Pinhole",
	"Auto-Zoom",
	"Fisheye Stereographic",
	"Equirectangular 360",
	"Fisheye Equidistant",
	"Fisheye Orthographic",
	"Panini Cylindrical",
	"Hammer-Aitoff"
};

constexpr std::array<double, 4> kFieldsOfViewDeg{30.0, 60.0, 90.0, 120.0};
constexpr double kDegreesToRadians = std::numbers::pi_v<double> / 180.0;

[[nodiscard]] constexpr bool is_all_sky(size_t mode_index) noexcept {
	return mode_index == 3 || mode_index == 7;
}

[[nodiscard]] std::array<double, 3> ray_direction(size_t mode_index, double u, double v, double fov_rad) {
	const auto local = CameraProjector<double>::compute_ray_direction(static_cast<ProjectionMode>(mode_index), u, v, fov_rad);
	return {local[0], local[1], local[2]};
}

[[nodiscard]] double norm(const std::array<double, 3>& vector) noexcept {
	return std::sqrt(vector[0] * vector[0] + vector[1] * vector[1] + vector[2] * vector[2]);
}

void check_unit_rays(TestSuite& suite, size_t mode_index) {
	size_t samples = 0;
	size_t invalid = 0;
	double worst = 0.0;
	for (const double fov_deg : kFieldsOfViewDeg) {
		for (int iu = -4; iu <= 4; ++iu) {
			for (int iv = -4; iv <= 4; ++iv) {
				const auto direction = ray_direction(mode_index, 0.25 * iu, 0.25 * iv, fov_deg * kDegreesToRadians);
				const double length = norm(direction);
				++samples;
				if (!std::isfinite(length)) {
					++invalid;
				} else {
					worst = std::max(worst, std::abs(length - 1.0));
				}
			}
		}
	}
	suite.check(invalid == 0 && worst <= 1e-9, std::string(kModeNames[mode_index]) + ": sampled rays are finite and unit length", "samples=" + std::to_string(samples) + " invalid=" + std::to_string(invalid) + " worst_deviation=" + Relativistic::Testing::format_number(worst, 4));
}

void check_pixel_grid(TestSuite& suite, size_t mode_index) {
	constexpr size_t width = 96;
	constexpr size_t height = 54;
	constexpr double aspect = static_cast<double>(width) / static_cast<double>(height);
	size_t invalid = 0;
	for (size_t y = 0; y < height; ++y) {
		const double v = 1.0 - (static_cast<double>(y) + 0.5) / static_cast<double>(height) * 2.0;
		for (size_t x = 0; x < width; ++x) {
			const double u_raw = (static_cast<double>(x) + 0.5) / static_cast<double>(width) * 2.0 - 1.0;
			const double u = is_all_sky(mode_index) ? u_raw : u_raw * aspect;
			const auto direction = ray_direction(mode_index, u, v, 60.0 * kDegreesToRadians);
			if (!std::isfinite(norm(direction)) || std::abs(norm(direction) - 1.0) > 1e-9) {
				++invalid;
			}
		}
	}
	suite.check(invalid == 0, std::string(kModeNames[mode_index]) + ": every pixel of a 96x54 renderer grid yields a valid ray", "invalid=" + std::to_string(invalid));
}

void check_center_and_symmetry(TestSuite& suite, size_t mode_index) {
	const double fov = 60.0 * kDegreesToRadians;
	const auto center = ray_direction(mode_index, 0.0, 0.0, fov);
	suite.check(std::abs(center[0] - 1.0) < 1e-9 && std::abs(center[1]) < 1e-9 && std::abs(center[2]) < 1e-9, std::string(kModeNames[mode_index]) + ": centre pixel looks straight along the forward axis");

	double horizontal = 0.0;
	double vertical = 0.0;
	for (int iu = 1; iu <= 4; ++iu) {
		for (int iv = 0; iv <= 4; ++iv) {
			const double u = 0.2 * iu;
			const double v = 0.2 * iv;
			const auto base = ray_direction(mode_index, u, v, fov);
			const auto mirrored_u = ray_direction(mode_index, -u, v, fov);
			const auto mirrored_v = ray_direction(mode_index, u, -v, fov);
			horizontal = std::max({horizontal, std::abs(base[0] - mirrored_u[0]), std::abs(base[1] - mirrored_u[1]), std::abs(base[2] + mirrored_u[2])});
			vertical = std::max({vertical, std::abs(base[0] - mirrored_v[0]), std::abs(base[1] + mirrored_v[1]), std::abs(base[2] - mirrored_v[2])});
		}
	}
	suite.check_at_most(horizontal, 1e-9, std::string(kModeNames[mode_index]) + ": left-right mirror symmetry");
	suite.check_at_most(vertical, 1e-9, std::string(kModeNames[mode_index]) + ": up-down mirror symmetry");
}

void check_monotonic_angle(TestSuite& suite, size_t mode_index) {
	const double fov = 60.0 * kDegreesToRadians;
	double previous = -1.0;
	bool monotonic = true;
	for (int step = 0; step <= 18; ++step) {
		const auto direction = ray_direction(mode_index, 0.05 * step, 0.0, fov);
		const double angle = std::acos(std::clamp(direction[0], -1.0, 1.0));
		if (angle + 1e-12 < previous) {
			monotonic = false;
		}
		previous = angle;
	}
	suite.check(monotonic, std::string(kModeNames[mode_index]) + ": off-axis angle grows monotonically along the horizontal axis");
}

void check_round_trip(TestSuite& suite, size_t mode_index) {
	const double fov = 60.0 * kDegreesToRadians;
	size_t missing = 0;
	double worst = 0.0;
	for (int iu = -3; iu <= 3; ++iu) {
		for (int iv = -3; iv <= 3; ++iv) {
			const double u = 0.2 * iu;
			const double v = 0.2 * iv;
			const auto direction = ray_direction(mode_index, u, v, fov);
			const auto screen = direction_to_screen_uv(static_cast<ProjectionMode>(mode_index), direction[0], direction[2], direction[1], fov);
			if (!screen.has_value()) {
				++missing;
				continue;
			}
			worst = std::max({worst, std::abs(screen->first - u), std::abs(screen->second - v)});
		}
	}
	suite.check(missing == 0 && worst <= 1e-6, std::string(kModeNames[mode_index]) + ": direction-to-screen projection inverts the ray generator", "missing=" + std::to_string(missing) + " worst_error=" + Relativistic::Testing::format_number(worst, 4));
}

void check_pinhole_geometry(TestSuite& suite) {
	for (const double fov_deg : kFieldsOfViewDeg) {
		const double fov = fov_deg * kDegreesToRadians;
		const auto horizontal_edge = ray_direction(0, 1.0, 0.0, fov);
		const auto vertical_edge = ray_direction(0, 0.0, 1.0, fov);
		const std::string suffix = " at fov " + Relativistic::Testing::format_number(fov_deg, 4) + " deg";
		suite.check_near(std::acos(std::clamp(horizontal_edge[0], -1.0, 1.0)), 0.5 * fov, 1e-12, "Pinhole: horizontal edge ray subtends half the field of view" + suffix);
		suite.check_near(std::acos(std::clamp(vertical_edge[0], -1.0, 1.0)), 0.5 * fov, 1e-12, "Pinhole: vertical edge ray subtends half the field of view" + suffix);
	}
	const auto behind = direction_to_screen_uv(ProjectionMode::Pinhole, -1.0, 0.0, 0.0, 60.0 * kDegreesToRadians);
	suite.check(!behind.has_value(), "Pinhole: a direction behind the camera has no screen position");
}

}

int main() {
	TestSuite suite(
		"projection_modes",
		"Verifies every camera projection mode of the observer: unit-length finite rays over several fields of view, renderer pixel grids, forward centre ray, mirror symmetry, monotonic angular mapping, pinhole edge geometry and the round trip between ray generation and screen projection."
	);

	suite.section("Mode catalogue");
	suite.check(static_cast<size_t>(ProjectionMode::Pinhole) == 0, "Pinhole is mode 0");
	suite.check(static_cast<size_t>(ProjectionMode::Equirectangular360) == 3, "Equirectangular 360 is mode 3");
	suite.check(static_cast<size_t>(ProjectionMode::HammerAitoff) == 7, "Hammer-Aitoff is mode 7");

	for (size_t mode_index = 0; mode_index < kModeNames.size(); ++mode_index) {
		suite.section(std::string("Projection mode ") + std::to_string(mode_index) + ": " + kModeNames[mode_index]);
		check_unit_rays(suite, mode_index);
		check_pixel_grid(suite, mode_index);
		check_center_and_symmetry(suite, mode_index);
		check_monotonic_angle(suite, mode_index);
		check_round_trip(suite, mode_index);
	}

	suite.section("Pinhole analytic geometry");
	check_pinhole_geometry(suite);
	return suite.finish();
}
