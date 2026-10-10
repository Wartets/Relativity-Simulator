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

[[nodiscard]] inline std::string_view dark_matter_path_note(std::string_view metric_name, bool use_gpu_compute, int precision_mode) noexcept {
	const bool cpu_only_metric = metric_name.find("BSSN") != std::string_view::npos;
	if (cpu_only_metric) {
		return "This spacetime is traced by the CPU renderer. The dark matter field is applied there with identical halo parameters, but rendering is considerably slower than on the GPU path.";
	}
	if (use_gpu_compute && precision_mode == 1) {
		return "Double-single emulation runs on the CPU fp64 tracer when dark matter is active; GPU acceleration is unavailable in this precision mode.";
	}
	return {};
}

[[nodiscard]] inline bool metric_uses_central_mass(std::string_view metric_name) noexcept {
	return metric_name.find("Schwarzschild") != std::string_view::npos
		|| metric_name.find("Kerr") != std::string_view::npos
		|| metric_name.find("Reissner") != std::string_view::npos
		|| metric_name.find("de Sitter") != std::string_view::npos
		|| metric_name.find("DeSitter") != std::string_view::npos;
}

[[nodiscard]] inline std::string_view dark_matter_metric_regime_note(std::string_view metric_name) noexcept {
	if (metric_name.find("Minkowski") != std::string_view::npos) {
		return "Flat spacetime has no central mass: halos are the only lenses and the only sources of gravity for bodies.";
	}
	if (metric_name.find("Kerr") != std::string_view::npos) {
		return "Rays follow Kerr-Schild geodesics around the rotating source; halo kicks are applied to the Cartesian null momentum on every step.";
	}
	if (metric_name.find("Reissner") != std::string_view::npos) {
		return "Rays follow charged-source geodesics; halo kicks are applied on top of the exact central geometry.";
	}
	if (metric_name.find("de Sitter") != std::string_view::npos || metric_name.find("DeSitter") != std::string_view::npos) {
		return "The cosmological horizon bounds the traced region; halos placed beyond it do not influence the image.";
	}
	if (metric_name.find("Schwarzschild") != std::string_view::npos) {
		return "Halo deflection is superposed on exact Schwarzschild null geodesics; the central mass is included in all rotation curves.";
	}
	if (metric_name.find("FLRW") != std::string_view::npos
		|| metric_name.find("Morris") != std::string_view::npos
		|| metric_name.find("Wormhole") != std::string_view::npos
		|| metric_name.find("Alcubierre") != std::string_view::npos
		|| metric_name.find("Warp") != std::string_view::npos
		|| metric_name.find("BSSN") != std::string_view::npos) {
		return "This spacetime has no central point mass in the tracer, so halo kicks act on the traced rays and the central mass is ignored in all rotation curves.";
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
