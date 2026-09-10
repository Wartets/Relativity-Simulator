#pragma once

#include <string_view>
#include <cmath>
#include <algorithm>

namespace Relativistic::UI {

[[nodiscard]] inline std::string_view metric_integrator_incompatibility(std::string_view metric_name, std::string_view integrator_name) noexcept {
	const bool is_bssn = metric_name.find("BSSN") != std::string_view::npos;
	const bool is_symplectic = integrator_name.find("Symplectic") != std::string_view::npos || integrator_name.find("Gauss") != std::string_view::npos;
	if (is_bssn && is_symplectic) {
		return "BSSN numerical grids do not expose analytic Christoffel symbols; symplectic Gauss-Legendre integrators rely on iterative implicit stages that converge poorly on the finite-difference metric, causing step failures or energy drift.";
	}
	const bool is_hermite = integrator_name.find("Hermite") != std::string_view::npos;
	if (is_bssn && is_hermite) {
		return "Hermite 4th-order (Aarseth) relies on analytic jerk from repeated finite differencing of a smooth metric; BSSN's interpolated grid metric introduces high-frequency noise that destabilizes the jerk estimate.";
	}
	return {};
}

[[nodiscard]] inline std::string_view precision_gpu_incompatibility(bool use_gpu_compute, int precision_mode) noexcept {
	if (use_gpu_compute && precision_mode == 1) {
		return "GPU compute dispatch only supports native IEEE 754 float64; double-single emulation always falls back to the CPU path, so enabling both provides no GPU acceleration.";
	}
	return {};
}

[[nodiscard]] inline std::string_view metric_spin_incompatibility(std::string_view metric_name, double spin, double mass) noexcept {
	const bool is_kerr_family = metric_name.find("Kerr") != std::string_view::npos;
	if (is_kerr_family && mass > 0.0 && std::abs(spin) > 0.999 * mass) {
		return "Spin magnitude close to or exceeding the extremal limit |a| = M produces a naked singularity with no event horizon; horizon-relative features (ISCO markers, ergosphere overlays, absorption) will behave unpredictably.";
	}
	return {};
}

[[nodiscard]] inline std::string_view wormhole_disk_incompatibility(std::string_view metric_name) noexcept {
	if (metric_name.find("Morris") != std::string_view::npos || metric_name.find("Wormhole") != std::string_view::npos) {
		return "Morris-Thorne wormholes have no photon sphere or ISCO; the accretion-disk color and temperature overlay logic assumes a Schwarzschild-like disk and will not visually apply here.";
	}
	return {};
}

[[nodiscard]] inline std::string_view render_distance_lod_incompatibility(double render_distance_scale, bool lod_enabled, double lod_distance_scale) noexcept {
	if (render_distance_scale > 0.0 && lod_enabled && lod_distance_scale > render_distance_scale) {
		return "LOD threshold distance is farther than the render distance cutoff, so the reduced-detail tier is unreachable; geodesics escape to the sky before LOD ever activates.";
	}
	return {};
}

}
