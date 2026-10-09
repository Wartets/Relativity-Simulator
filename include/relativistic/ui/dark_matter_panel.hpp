#pragma once

#include <imgui.h>
#include <implot.h>
#include "relativistic/dark_matter/dark_matter_field.hpp"
#include "relativistic/dynamics/pn/pn_body.hpp"
#include "relativistic/dynamics/pn/pn_nbody_system.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/render/geodesic_compute_pipeline.hpp"
#include "relativistic/ui/compatibility_notes.hpp"
#include "relativistic/ui/numeric_slider_utils.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/units/unit_system.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numbers>
#include <string>
#include <string_view>
#include <vector>

namespace Relativistic::UI {

class DarkMatterPanel {
public:
	void render(Orchestrator::SimulationOrchestrator<1024>& orchestrator, const Render::GeodesicComputePipeline* pipeline) {
		auto& field = orchestrator.parameters().dark_matter;
		bool changed = false;

		changed = render_global_controls(orchestrator, field, pipeline) || changed;
		ImGui::Separator();
		changed = render_halo_catalog(orchestrator, field) || changed;

		if (field.count > 0U) {
			selected_ = std::clamp(selected_, 0, static_cast<int>(field.count) - 1);
			DarkMatter::DarkMatterHaloSettings& halo = field.halos[static_cast<size_t>(selected_)];
			ImGui::Separator();
			changed = render_halo_editor(orchestrator, halo) || changed;
			ImGui::Separator();
			render_halo_diagnostics(orchestrator, halo);
			ImGui::Separator();
			render_profile_plots(orchestrator, field, halo);
		}

		ImGui::Separator();
		changed = render_population_tool(orchestrator, field) || changed;
		ImGui::Separator();
		render_tracer_tool(orchestrator, field);

		if (changed) {
			field.sanitize();
			orchestrator.notify_state_changed();
		}
	}

private:
	struct BodyEntry {
		uint32_t id{0};
		std::string label{};
	};

	int selected_{0};
	int new_halo_preset_{0};
	bool mass_log_{true};
	bool scale_log_{true};
	bool softening_log_{true};
	bool truncation_log_{true};

	DarkMatter::DarkMatterPopulationSettings population_{};
	int population_profile_{static_cast<int>(DarkMatter::DarkMatterProfileType::Hernquist)};

	int tracer_count_{32};
	int tracer_distribution_mode_{0}; // 0: Planar Equatorial Disk, 1: Spherical Cloud, 2: Resonant Ring
	double tracer_inner_radius_{15.0};
	double tracer_outer_radius_{120.0};
	double tracer_mass_{1.0e-7};
	double tracer_retrograde_fraction_{0.0};
	double tracer_inclination_spread_deg_{4.0};
	uint64_t tracer_seed_{421337ULL};
	std::string status_{};

	[[nodiscard]] static std::vector<BodyEntry> collect_bodies(Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
		std::vector<BodyEntry> entries;
		auto& system = orchestrator.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(system.bodies_mutex());
		for (const auto& body : system.bodies()) {
			if (!body.enabled) continue;
			const std::string base = body.has_name() ? std::string(body.name_view()) : std::string("Body");
			entries.push_back(BodyEntry{body.id, base + " (#" + std::to_string(body.id) + ")"});
		}
		return entries;
	}

	[[nodiscard]] static bool resolve_halo_center(
		Orchestrator::SimulationOrchestrator<1024>& orchestrator,
		const DarkMatter::DarkMatterHaloSettings& halo,
		std::array<double, 3>& center,
		std::array<double, 3>& velocity
	) {
		auto& system = orchestrator.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(system.bodies_mutex());
		const auto bodies = system.bodies();
		const auto resolver = [&bodies, &velocity](uint32_t id, std::array<double, 3>& out) noexcept -> bool {
			for (const auto& body : bodies) {
				if (body.id == id && body.enabled) {
					out = body.position;
					velocity = body.velocity;
					return true;
				}
			}
			return false;
		};
		velocity = {0.0, 0.0, 0.0};
		const double time = system.time();
		if (!halo.resolve_center(time, resolver, center)) {
			return false;
		}
		for (size_t c = 0; c < 3; ++c) {
			velocity[c] += halo.velocity[c];
		}
		return true;
	}

	bool render_global_controls(
		Orchestrator::SimulationOrchestrator<1024>& orchestrator,
		DarkMatter::DarkMatterFieldSettings& field,
		const Render::GeodesicComputePipeline* pipeline
	) {
		bool changed = false;
		ImGui::TextColored(ImVec4(0.72f, 0.58f, 1.0f, 1.0f), "Dark Matter Halo Field & Cosmic Structure");
		changed = ImGui::Checkbox("Master Enable Dark Matter Field", &field.enabled) || changed;
		render_setting_tooltip("Master toggle for all dark matter structures. When enabled, halos bend light, radiate emissive density diagnostic glow, and exert gravitational pulls on N-body particles in geometric simulation units (M=G=c=1).");

		const auto& params = orchestrator.parameters();
		const int precision_mode = static_cast<int>(orchestrator.get_custom_param("precision_mode", 0.0));
		const auto note = dark_matter_path_note(orchestrator.active_metric_name(), params.use_gpu_compute, precision_mode);
		if (!note.empty()) {
			ImGui::TextColored(ImVec4(1.0f, 0.72f, 0.3f, 1.0f), "%s", std::string(note).c_str());
		}
		if (pipeline != nullptr) {
			ImGui::TextDisabled("Render Dispatch Path: %s | Precision: %s",
				pipeline->telemetry().used_gpu_path ? "GPU Compute (Vulkan FP64/Scalar)" : "CPU Multi-threaded Host",
				precision_mode == 1 ? "Double-Single Emulation" : "Native FP64");
		}
		ImGui::TextDisabled("Active Metric: %s | Active Halos: %u / %zu | Catalog Mass: %.5g M (Central M=%.4g)",
			orchestrator.active_metric_name().c_str(), field.count, DarkMatter::kMaxHalos, field.total_halo_mass(), orchestrator.parameters().mass);

		ImGui::BeginDisabled(!field.enabled);
		changed = ImGui::Checkbox("Gravitational Light Lensing", &field.lensing_enabled) || changed;
		render_setting_tooltip("Integrates weak-field relativistic photon ray deflection step-by-step along each geodesic. Light bending operates simultaneously with the central metric (Schwarzschild, Kerr, Kerr-Newman, etc.).");
		ImGui::SameLine();
		changed = ImGui::Checkbox("Emissive Density Glow", &field.visualization_enabled) || changed;
		render_setting_tooltip("Renders false-color line-of-sight volumetric column density along null geodesics, making dark matter spatial morphology, ellipsoidal triaxiality, and core radii visually apparent.");
		ImGui::SameLine();
		changed = ImGui::Checkbox("N-Body Gravitational Coupling", &field.affects_bodies) || changed;
		render_setting_tooltip("Injects halo gravitational accelerations directly into the post-Newtonian N-body integrator for all active celestial bodies. Bodies anchored to a specific halo are automatically exempted from self-acceleration.");

		changed = slider_double_with_input("Global Lensing Multiplier", &field.lensing_strength, 0.0, 10.0, "%.3fx") || changed;
		render_setting_tooltip("Scales the gravitational photon kick across all halos. 1.0 represents the physical weak-field Einstein angle deflection. Exaggerating this parameter helps highlight subtle subhalo lensing.");
		changed = slider_double_with_input("Density Visualization Intensity", &field.visual_intensity, 0.0, 15.0, "%.3f") || changed;
		render_setting_tooltip("Global gain factor for the optical emissivity of dark matter halos. Integrates normalized column density along rays without obscuring background stars or accretion disks.");
		changed = slider_double_with_input("Geodesic Step Fraction", &field.step_fraction, 0.02, 1.0, "%.3f") || changed;
		render_setting_tooltip("Controls integration step bounding near halo cores. The ray step size is constrained to a fraction of the ellipsoidal radius m, with a safety floor of 0.5 * scale_radius. Smaller values resolve cusps more accurately at the cost of additional ray steps.");
		ImGui::EndDisabled();
		return changed;
	}

	bool render_halo_catalog(
		Orchestrator::SimulationOrchestrator<1024>& orchestrator,
		DarkMatter::DarkMatterFieldSettings& field
	) {
		bool changed = false;
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.35f, 1.0f), "Halo Catalog Management (%u / %zu Slots)", field.count, DarkMatter::kMaxHalos);

		ImGui::SetNextItemWidth(260.0f);
		ImGui::Combo("Preset Template", &new_halo_preset_, DarkMatter::kDarkMatterPresetNames.data(), static_cast<int>(DarkMatter::kDarkMatterPresetNames.size()));
		render_setting_tooltip("Astrophysical presets scaling automatically with the central black hole mass unit: Galactic NFW, cored dwarf Burkert, triaxial galaxy cluster, compact Hernquist subhalo, or point perturber.");
		ImGui::SameLine();
		if (ImGui::Button("Spawn Halo From Preset")) {
			const double unit = std::max(orchestrator.parameters().mass, 1e-3);
			const int index = field.add_halo(DarkMatter::DarkMatterHaloSettings::from_preset(static_cast<DarkMatter::DarkMatterHaloPreset>(new_halo_preset_), unit));
			if (index >= 0) {
				selected_ = index;
				field.enabled = true;
				changed = true;
				status_ = "Spawned new halo preset in slot #" + std::to_string(index + 1);
			} else {
				status_ = "Error: Halo catalog capacity reached (" + std::to_string(DarkMatter::kMaxHalos) + " maximum).";
			}
		}

		ImGui::BeginDisabled(field.count == 0U);
		if (ImGui::Button("Duplicate Selected")) {
			const int index = field.duplicate_halo(static_cast<size_t>(selected_));
			if (index >= 0) {
				selected_ = index;
				changed = true;
				status_ = "Duplicated halo into slot #" + std::to_string(index + 1);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove Selected")) {
			changed = field.remove_halo(static_cast<size_t>(selected_)) || changed;
			selected_ = std::max(selected_ - 1, 0);
			status_ = "Removed halo from catalog.";
		}
		ImGui::SameLine();
		if (ImGui::Button("Move Up") && field.move_halo(static_cast<size_t>(selected_), -1)) {
			--selected_;
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Move Down") && field.move_halo(static_cast<size_t>(selected_), 1)) {
			++selected_;
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Enable All")) {
			for (uint32_t i = 0; i < field.count; ++i) field.halos[i].enabled = true;
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Disable All")) {
			for (uint32_t i = 0; i < field.count; ++i) field.halos[i].enabled = false;
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear Catalog")) {
			field.clear();
			selected_ = 0;
			changed = true;
			status_ = "Cleared all halos from catalog.";
		}
		ImGui::EndDisabled();

		if (!status_.empty()) {
			ImGui::TextColored(ImVec4(0.4f, 0.95f, 0.6f, 1.0f), "%s", status_.c_str());
		}

		ImGui::BeginChild("##DarkMatterHaloListWindow", ImVec2(0.0f, 135.0f), true);
		for (uint32_t i = 0; i < field.count; ++i) {
			DarkMatter::DarkMatterHaloSettings& halo = field.halos[i];
			ImGui::PushID(static_cast<int>(i));
			changed = ImGui::Checkbox("##enabled", &halo.enabled) || changed;
			render_setting_tooltip("Toggle whether this individual halo actively participates in lensing, glow, and N-body gravity.");
			ImGui::SameLine();
			char label[128];
			std::snprintf(label, sizeof(label), "%u. %s | %s | M=%.4g M | a=%.4g M%s",
				i + 1U, halo.name.data(),
				DarkMatter::kDarkMatterProfileNames[static_cast<size_t>(halo.profile)],
				halo.mass, halo.scale_radius,
				halo.anchor_body_id != 0U ? " [Anchored]" : "");
			if (ImGui::Selectable(label, selected_ == static_cast<int>(i))) {
				selected_ = static_cast<int>(i);
			}
			ImGui::PopID();
		}
		if (field.count == 0U) {
			ImGui::TextDisabled("Catalog is empty. Choose a preset template above and press 'Spawn Halo From Preset'.");
		}
		ImGui::EndChild();
		return changed;
	}

	bool render_halo_editor(
		Orchestrator::SimulationOrchestrator<1024>& orchestrator,
		DarkMatter::DarkMatterHaloSettings& halo
	) {
		bool changed = false;
		ImGui::PushID("DarkMatterHaloEditor");
		ImGui::TextColored(ImVec4(0.45f, 0.85f, 1.0f, 1.0f), "Selected Halo Configuration: %s", halo.name.data());

		if (ImGui::BeginTabBar("DarkMatterHaloEditorTabs")) {
			if (ImGui::BeginTabItem("Density Profile & Mass")) {
				changed = ImGui::InputText("Halo Identifier Name", halo.name.data(), halo.name.size()) || changed;
				render_setting_tooltip("Descriptive name shown in the catalog, scene hierarchies, and session logs.");

				int profile_index = static_cast<int>(halo.profile);
				if (ImGui::Combo("Density Law Profile", &profile_index, DarkMatter::kDarkMatterProfileNames.data(), static_cast<int>(DarkMatter::kDarkMatterProfileNames.size()))) {
					halo.profile = static_cast<DarkMatter::DarkMatterProfileType>(profile_index);
					halo.shape = DarkMatter::DarkMatterProfileMath::default_shape(halo.profile);
					changed = true;
				}
				render_setting_tooltip(
					"Analytic radial density profile model:\n"
					"- Navarro-Frenk-White (NFW): Classic CDM cuspy halo (rho ~ r^-1 inner, r^-3 outer).\n"
					"- Hernquist: Steeper outer fall-off (rho ~ r^-4), analytic potential, ideal for compact spheroids.\n"
					"- Burkert: Constant-density core (rho ~ const inner), models dwarf galaxies and cored halos.\n"
					"- Einasto: Continuous logarithmic curvature slope with parameter alpha.\n"
					"- Plummer: Polytrope n=5 softened core, typical for star clusters and compact clumps.\n"
					"- Cored Isothermal: Flat asymptotic circular velocity curve with finite central core.\n"
					"- Singular Isothermal Sphere (SIS): Constant circular velocity v_c everywhere (rho ~ r^-2).\n"
					"- Dehnen Gamma Model: Generalized family with inner power-law slope gamma in [0, 3).\n"
					"- Point Mass: Pure Keplerian perturber with core softening."
				);

				changed = slider_double_with_input("Enclosed Mass (M)", &halo.mass, 1e-6, 1e12, "%.5g", &mass_log_, 1e-6f, 1e12f) || changed;
				render_setting_tooltip("Mass enclosed inside the mass radius R_mass = min(concentration * scale_radius, truncation_radius). Normalization constant M0 is evaluated analytically from this.");

				changed = slider_double_with_input("Characteristic Scale Radius a (M)", &halo.scale_radius, 1e-3, 1e9, "%.5g", &scale_log_, 1e-3f, 1e9f) || changed;
				render_setting_tooltip("Characteristic scale length 'a' (or r_s) of the profile in geometric units M. Represents the turnover radius where the logarithmic density slope equals -2 in NFW.");

				changed = slider_double_with_input("Halo Concentration (c)", &halo.concentration, 0.5, 200.0, "%.2f") || changed;
				render_setting_tooltip("Concentration parameter c = R_vir / a. Establishes the boundary radius containing the enclosed mass specified above.");

				const char* shape_label = DarkMatter::DarkMatterProfileMath::shape_label(halo.profile);
				if (shape_label != nullptr) {
					const auto bounds = DarkMatter::DarkMatterProfileMath::shape_bounds(halo.profile);
					changed = slider_double_with_input(shape_label, &halo.shape, bounds[0], bounds[1], "%.3f") || changed;
					render_setting_tooltip(halo.profile == DarkMatter::DarkMatterProfileType::Einasto
						? "Einasto shape parameter alpha. Lower values produce a shallower core-like turnover; higher values sharpen the cusp."
						: "Dehnen inner density slope gamma in [0, 3). gamma=1 corresponds to Hernquist; gamma=0 is a cored model; gamma=2 approaches Jaffe.");
				}

				changed = slider_double_with_input("Plummer Core Softening (M)", &halo.softening, 0.0, 1e6, "%.4g", &softening_log_, 1e-4f, 1e6f) || changed;
				render_setting_tooltip("Gravitational softening length epsilon applied in denominator sqrt(r^2 + epsilon^2) to regularize central singularities during ray kicks and body integration.");

				changed = slider_double_with_input("Outer Truncation Radius (M)", &halo.truncation_radius, 0.0, 1e8, "%.4g", &truncation_log_, 1e-2f, 1e8f) || changed;
				render_setting_tooltip("Sharp spatial cutoff radius. For r > r_trunc, density drops to zero and enclosed mass remains constant. A value of 0.0 disables truncation (unbounded halo).");

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Position, Anchor & Velocity")) {
				ImGui::TextColored(ImVec4(0.55f, 0.9f, 0.55f, 1.0f), "Spatial Anchoring & Dynamical Kinematics");
				const auto bodies = collect_bodies(orchestrator);
				int anchor_index = -1;
				for (size_t i = 0; i < bodies.size(); ++i) {
					if (bodies[i].id == halo.anchor_body_id) {
						anchor_index = static_cast<int>(i);
						break;
					}
				}
				const std::string preview = (anchor_index >= 0)
					? bodies[static_cast<size_t>(anchor_index)].label
					: (halo.anchor_body_id != 0U ? std::string("Missing Body #") + std::to_string(halo.anchor_body_id) : std::string("World Fixed Coordinate Center"));

				if (ImGui::BeginCombo("Orbital Body Anchor", preview.c_str())) {
					if (ImGui::Selectable("World Fixed Coordinate Center", halo.anchor_body_id == 0U)) {
						halo.anchor_body_id = 0U;
						changed = true;
					}
					for (const auto& entry : bodies) {
						if (ImGui::Selectable(entry.label.c_str(), entry.id == halo.anchor_body_id)) {
							halo.anchor_body_id = entry.id;
							changed = true;
						}
					}
					ImGui::EndCombo();
				}
				render_setting_tooltip("When anchored to a celestial body, the halo center automatically tracks the body's time-dependent position, with the position below acting as a local Cartesian offset.");

				changed = ImGui::InputScalarN("Position / Relative Offset (M)", ImGuiDataType_Double, halo.position.data(), 3, nullptr, nullptr, "%.4f") || changed;
				render_setting_tooltip("Spatial center coordinates (x, y, z) in simulation units M. If anchored to a body, this vector acts as an offset from the body's instantaneous position.");

				changed = ImGui::InputScalarN("Kinematic Drift Velocity (c)", ImGuiDataType_Double, halo.velocity.data(), 3, nullptr, nullptr, "%.5f") || changed;
				render_setting_tooltip("Constant linear velocity drift vector added to the halo center each second of simulation time: r(t) = r0 + v * t.");

				ImGui::Spacing();
				if (ImGui::Button("Center On Coordinate Origin", ImVec2(220.0f, 24.0f))) {
					halo.position = {0.0, 0.0, 0.0};
					halo.velocity = {0.0, 0.0, 0.0};
					halo.anchor_body_id = 0U;
					changed = true;
				}
				render_setting_tooltip("Resets position and drift velocity to zero, aligning the halo concentric with the central spacetime singularity.");

				ImGui::SameLine();
				if (ImGui::Button("Snap To Active Observer", ImVec2(200.0f, 24.0f))) {
					const auto& cam = orchestrator.camera();
					halo.position = cam.position;
					halo.anchor_body_id = 0U;
					changed = true;
				}
				render_setting_tooltip("Repositions the halo center directly onto the camera's current 3D position.");

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Triaxial Shape & Orientation")) {
				ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.45f, 1.0f), "Ellipsoidal Triaxiality & Euler Basis Orientation");

				changed = slider_double_with_input("Semi-Axis Ratio q_y (b / a)", &halo.axis_ratio_y, 0.05, 4.0, "%.3f") || changed;
				render_setting_tooltip("Intermediate-to-major axis ratio q_y = b / a. Values < 1 flatten the halo along the local Y axis, creating prolate, oblate, or triaxial isodensity surfaces.");

				changed = slider_double_with_input("Semi-Axis Ratio q_z (c / a)", &halo.axis_ratio_z, 0.05, 4.0, "%.3f") || changed;
				render_setting_tooltip("Minor-to-major axis ratio q_z = c / a. Flattens or elongates the halo along the local Z axis.");

				changed = slider_double_with_input("Euler Yaw Angle", &halo.yaw_deg, -180.0, 180.0, "%.1f deg") || changed;
				render_setting_tooltip("Intrinsic rotation angle around world Z axis.");

				changed = slider_double_with_input("Euler Pitch Angle", &halo.pitch_deg, -90.0, 90.0, "%.1f deg") || changed;
				render_setting_tooltip("Intrinsic rotation angle around intermediate Y axis.");

				changed = slider_double_with_input("Euler Roll Angle", &halo.roll_deg, -180.0, 180.0, "%.1f deg") || changed;
				render_setting_tooltip("Intrinsic rotation angle around major X axis.");

				if (ImGui::Button("Reset To Spherical Symmetry (q_y = q_z = 1.0)", ImVec2(280.0f, 24.0f))) {
					halo.axis_ratio_y = 1.0;
					halo.axis_ratio_z = 1.0;
					halo.yaw_deg = 0.0;
					halo.pitch_deg = 0.0;
					halo.roll_deg = 0.0;
					changed = true;
				}
				render_setting_tooltip("Restores isotropic spherical symmetry with identity orientation basis matrix.");

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Optics & Interaction")) {
				ImGui::TextColored(ImVec4(0.8f, 0.65f, 1.0f, 1.0f), "Individual Halo Rendering & Dynamic Switches");

				changed = ImGui::Checkbox("Deflect Light (Lensing)", &halo.lensing_enabled) || changed;
				render_setting_tooltip("Toggles light ray bending for this specific halo during geodesic integration.");
				ImGui::SameLine();
				changed = ImGui::Checkbox("Accelerate Bodies (N-Body)", &halo.affects_bodies) || changed;
				render_setting_tooltip("Toggles gravitational acceleration exerted by this halo onto N-body celestial bodies.");
				ImGui::SameLine();
				changed = ImGui::Checkbox("Visual Density Glow", &halo.visualize) || changed;
				render_setting_tooltip("Toggles volumetric column-density emission rendering for this halo.");

				changed = slider_double_with_input("Halo Deflection Scale", &halo.lens_scale, 0.0, 25.0, "%.3fx") || changed;
				render_setting_tooltip("Per-halo multiplier applied to photon deflection kicks. Allows boosting the lensing signal of dwarf subhalos.");

				changed = slider_double_with_input("Visual Emission Gain", &halo.visual_gain, 0.0, 50.0, "%.3f") || changed;
				render_setting_tooltip("Brightness scaling factor for this halo's false-color volumetric glow.");

				changed = ImGui::ColorEdit3("Emission Tint Color", halo.tint.data()) || changed;
				render_setting_tooltip("False-color chromatic tint assigned to this halo's integrated column density.");

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::PopID();
		return changed;
	}

	void render_halo_diagnostics(
		Orchestrator::SimulationOrchestrator<1024>& orchestrator,
		const DarkMatter::DarkMatterHaloSettings& halo
	) {
		ImGui::TextColored(ImVec4(0.45f, 0.95f, 0.55f, 1.0f), "Physical Halo Diagnostics & Weak-Field Verification");
		ImGui::TextDisabled("Normalization M0: %.5g | Scale Radius a: %.5g M | Mass Boundary Radius: %.5g M | Central Mass: %.4g M",
			halo.normalization(), halo.scale_radius, halo.mass_radius(), orchestrator.parameters().mass);

		const std::array<double, 6> radii{
			0.1 * halo.scale_radius,
			0.5 * halo.scale_radius,
			halo.scale_radius,
			2.0 * halo.scale_radius,
			halo.mass_radius(),
			halo.truncation_radius > 0.0 ? halo.truncation_radius : 10.0 * halo.scale_radius
		};

		double peak_compactness = 0.0;
		if (ImGui::BeginTable("##DarkMatterHaloDiagnosticsTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Radius r (M)");
			ImGui::TableSetupColumn("Enclosed Mass M(r)");
			ImGui::TableSetupColumn("Circular Speed v_c/c");
			ImGui::TableSetupColumn("Local Density rho");
			ImGui::TableSetupColumn("Compactness M/r");
			ImGui::TableSetupColumn("Einstein Angle 4M/r");
			ImGui::TableHeadersRow();

			for (const double radius : radii) {
				const double enclosed = halo.enclosed_mass(radius);
				const double compactness = enclosed / std::max(radius, 1e-12);
				peak_compactness = std::max(peak_compactness, compactness);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%.4g", radius);
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%.5g", enclosed);
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%.4f", halo.circular_velocity(radius));
				ImGui::TableSetColumnIndex(3);
				ImGui::Text("%.4e", halo.density(radius));
				ImGui::TableSetColumnIndex(4);
				ImGui::Text("%.4e", compactness);
				ImGui::TableSetColumnIndex(5);
				ImGui::Text("%.4f rad", 4.0 * compactness);
			}
			ImGui::EndTable();
		}

		if (peak_compactness > 0.1) {
			ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.35f, 1.0f),
				"Warning: Peak compactness M(r)/r reaches %.2f. The weak-field light deflection approximation assumes M/r << 1; higher compactness will cause non-linear general relativistic corrections to deviate.",
				peak_compactness);
		} else {
			ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), "Weak-field post-Newtonian approximation holds rigorously (Peak M/r = %.2e << 1).", peak_compactness);
		}
	}

	void render_profile_plots(
		Orchestrator::SimulationOrchestrator<1024>& orchestrator,
		const DarkMatter::DarkMatterFieldSettings& field,
		const DarkMatter::DarkMatterHaloSettings& halo
	) {
		constexpr size_t samples = 180;
		const double r_min = std::max(0.01 * halo.scale_radius, 1e-4);
		const double r_max = std::max(8.0 * halo.mass_radius(), 20.0 * r_min);
		std::vector<double> radius(samples);
		std::vector<double> selected_speed(samples);
		std::vector<double> total_speed(samples);
		std::vector<double> central_speed(samples);
		std::vector<double> density(samples);

		const double central_mass = std::max(orchestrator.parameters().mass, 0.0);
		const double log_span = std::log(r_max / r_min);

		for (size_t i = 0; i < samples; ++i) {
			const double r = r_min * std::exp(log_span * static_cast<double>(i) / static_cast<double>(samples - 1));
			radius[i] = r;
			selected_speed[i] = halo.circular_velocity(r);

			double enclosed = central_mass;
			for (uint32_t h = 0; h < field.count; ++h) {
				if (field.halos[h].enabled) {
					enclosed += field.halos[h].enclosed_mass(r);
				}
			}
			total_speed[i] = std::sqrt(std::max(enclosed, 0.0) / r);
			central_speed[i] = std::sqrt(central_mass / r);
			density[i] = std::max(halo.density(r), 1e-300);
		}

		if (ImPlot::BeginPlot("Astrophysical Circular Velocity Curves v_c(r)", ImVec2(-1.0f, 220.0f))) {
			ImPlot::SetupAxes("Galactocentric Radius r (Geometric Units M)", "Circular Velocity v_c / c");
			ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
			ImPlot::PlotLine("Total System (Central BH + All Halos)", radius.data(), total_speed.data(), static_cast<int>(samples));
			ImPlot::PlotLine("Selected Halo Contribution", radius.data(), selected_speed.data(), static_cast<int>(samples));
			ImPlot::PlotLine("Central Keplerian Mass Only", radius.data(), central_speed.data(), static_cast<int>(samples));
			ImPlot::EndPlot();
		}

		if (ImPlot::BeginPlot("Radial Density Profile rho(r)", ImVec2(-1.0f, 190.0f))) {
			ImPlot::SetupAxes("Radius r (M)", "Spatial Density rho(r) [M / M^3]");
			ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
			ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
			ImPlot::PlotLine("rho(r)", radius.data(), density.data(), static_cast<int>(samples));
			ImPlot::EndPlot();
		}
	}

	bool render_population_tool(
		Orchestrator::SimulationOrchestrator<1024>& orchestrator,
		DarkMatter::DarkMatterFieldSettings& field
	) {
		bool changed = false;
		if (!ImGui::CollapsingHeader("Subhalo Population Generator (Hierarchical Lambda-CDM)")) {
			return false;
		}

		ImGui::TextDisabled("Generates a realistic cosmological population of subhalos distributed around the host halo or coordinate origin following a power-law mass function dN/dM ~ M^-alpha and radial spatial profile.");

		int count = static_cast<int>(population_.count);
		if (ImGui::SliderInt("Subhalos To Generate", &count, 1, static_cast<int>(DarkMatter::kMaxHalos))) {
			population_.count = static_cast<uint32_t>(count);
		}

		ImGui::Combo("Subhalo Model Profile", &population_profile_, DarkMatter::kDarkMatterProfileNames.data(), static_cast<int>(DarkMatter::kDarkMatterProfileNames.size()));
		population_.profile = static_cast<DarkMatter::DarkMatterProfileType>(population_profile_);

		slider_double_with_input("Minimum Subhalo Mass M_min", &population_.minimum_mass, 1e-6, 1e6, "%.4g", &mass_log_, 1e-6f, 1e6f);
		slider_double_with_input("Maximum Subhalo Mass M_max", &population_.maximum_mass, 1e-6, 1e6, "%.4g", &mass_log_, 1e-6f, 1e6f);
		slider_double_with_input("Mass Function Slope alpha", &population_.mass_slope, 0.5, 3.5, "%.2f");
		render_setting_tooltip("Differential mass function slope dN/dM ~ M^-alpha. Standard cold dark matter N-body simulations (Aquarius, Via Lactea) find alpha ~ 1.9.");

		slider_double_with_input("Inner Distribution Radius", &population_.inner_radius, 1e-2, 1e7, "%.4g", &scale_log_, 1e-2f, 1e7f);
		slider_double_with_input("Outer Distribution Radius", &population_.outer_radius, 1e-2, 1e7, "%.4g", &scale_log_, 1e-2f, 1e7f);
		slider_double_with_input("Radial Spatial Density Slope", &population_.radial_slope, 0.0, 3.0, "%.2f");
		slider_double_with_input("Vertical Triaxial Flattening q_z", &population_.flattening, 0.05, 1.0, "%.2f");

		slider_double_with_input("Reference Mass M_ref", &population_.reference_mass, 1e-6, 1e6, "%.4g", &mass_log_, 1e-6f, 1e6f);
		slider_double_with_input("Reference Scale Radius a_ref", &population_.reference_scale_radius, 1e-3, 1e6, "%.4g", &scale_log_, 1e-3f, 1e6f);
		render_setting_tooltip("Sets self-similar scaling: a_sub = a_ref * (M_sub / M_ref)^(1/3).");

		slider_double_with_input("Subhalo Concentration", &population_.concentration, 1.0, 100.0, "%.1f");

		ImGui::Checkbox("Subhalos Deflect Light", &population_.lensing);
		ImGui::SameLine();
		ImGui::Checkbox("Subhalos Pull On Bodies", &population_.affects_bodies);
		ImGui::SameLine();
		ImGui::Checkbox("Subhalos Emit Glow", &population_.visualize);

		ImGui::InputScalar("RNG Random Seed", ImGuiDataType_U64, &population_.seed);

		if (ImGui::Button("Generate & Inject Subhalo Population", ImVec2(280.0f, 28.0f))) {
			std::array<double, 3> center{0.0, 0.0, 0.0};
			std::array<double, 3> velocity{0.0, 0.0, 0.0};
			uint32_t anchor = 0U;
			if (field.count > 0U) {
				const auto& host = field.halos[static_cast<size_t>(std::clamp(selected_, 0, static_cast<int>(field.count) - 1))];
				center = host.position;
				velocity = host.velocity;
				anchor = host.anchor_body_id;
			}
			const uint32_t added = field.spawn_population(population_, center, velocity, anchor);
			field.enabled = field.enabled || added > 0U;
			status_ = "Successfully generated and added " + std::to_string(added) + " subhalos to catalog.";
			changed = changed || added > 0U;
		}
		(void)orchestrator;
		return changed;
	}

	void render_tracer_tool(
		Orchestrator::SimulationOrchestrator<1024>& orchestrator,
		const DarkMatter::DarkMatterFieldSettings& field
	) {
		if (!ImGui::CollapsingHeader("N-Body Tracer Test Particles (Rotation Curve Probe)")) {
			return;
		}

		ImGui::TextDisabled("Injects low-mass test bodies into the active N-body integrator on circular orbits balanced against the selected halo and central mass. Useful to witness dark matter rotation curves dynamically in action.");

		ImGui::SliderInt("Number Of Tracers", &tracer_count_, 1, 256);
		const char* dist_modes[] = {"Planar Equatorial Disk", "Isotropic Spherical Shell", "Narrow Resonant Ring"};
		ImGui::Combo("Orbital Distribution Geometry", &tracer_distribution_mode_, dist_modes, IM_ARRAYSIZE(dist_modes));

		slider_double_with_input("Inner Orbit Radius (M)", &tracer_inner_radius_, 0.1, 1e7, "%.4g", &scale_log_, 0.1f, 1e7f);
		slider_double_with_input("Outer Orbit Radius (M)", &tracer_outer_radius_, 0.1, 1e7, "%.4g", &scale_log_, 0.1f, 1e7f);
		slider_double_with_input("Tracer Individual Mass (M)", &tracer_mass_, 1e-12, 1e3, "%.3e", &mass_log_, 1e-12f, 1e3f);
		slider_double_with_input("Retrograde Counter-Rotating Fraction", &tracer_retrograde_fraction_, 0.0, 1.0, "%.2f");
		render_setting_tooltip("Fraction of injected tracer particles initialized with retrograde orbital velocities.");

		if (tracer_distribution_mode_ == 0) {
			slider_double_with_input("Disk Inclination Dispersion", &tracer_inclination_spread_deg_, 0.0, 45.0, "%.1f deg");
		}
		ImGui::InputScalar("Tracer Random Seed", ImGuiDataType_U64, &tracer_seed_);

		ImGui::BeginDisabled(field.count == 0U);
		if (ImGui::Button("Spawn Tracer Test Bodies In Orbit", ImVec2(280.0f, 28.0f))) {
			const auto& halo = field.halos[static_cast<size_t>(std::clamp(selected_, 0, static_cast<int>(std::max<uint32_t>(field.count, 1U)) - 1))];
			std::array<double, 3> center{};
			std::array<double, 3> host_velocity{};

			if (!resolve_halo_center(orchestrator, halo, center, host_velocity)) {
				status_ = "Error: Halo anchor body could not be resolved.";
			} else {
				auto& system = orchestrator.nbody_system();
				std::lock_guard<std::recursive_mutex> lock(system.bodies_mutex());

				const double G = system.config().gravitational_constant;
				const double central_mass = (std::abs(center[0]) + std::abs(center[1]) + std::abs(center[2]) < 1e-6)
					? orchestrator.parameters().mass
					: 0.0;

				const double r_min = std::max(std::min(tracer_inner_radius_, tracer_outer_radius_), 1e-4);
				const double r_max = std::max(std::max(tracer_inner_radius_, tracer_outer_radius_), r_min * 1.0001);

				DarkMatter::DarkMatterRandom rng(tracer_seed_ ^ 0xA5A55A5AULL);
				uint32_t spawned = 0;

				for (int i = 0; i < tracer_count_; ++i) {
					double r = 0.0;
					if (tracer_distribution_mode_ == 2) {
						r = 0.5 * (r_min + r_max) * (1.0 + 0.02 * (rng.next_uniform() - 0.5));
					} else {
						const double u = rng.next_uniform();
						r = r_min * std::exp(u * std::log(r_max / r_min));
					}

					double theta = std::numbers::pi_v<double> * 0.5;
					double phi = 2.0 * std::numbers::pi_v<double> * rng.next_uniform();

					if (tracer_distribution_mode_ == 1) {
						const double cos_t = 2.0 * rng.next_uniform() - 1.0;
						theta = std::acos(std::clamp(cos_t, -1.0, 1.0));
					} else {
						const double spread_rad = (tracer_inclination_spread_deg_ * std::numbers::pi_v<double> / 180.0);
						theta += spread_rad * (rng.next_uniform() - 0.5);
					}

					const double sin_t = std::sin(theta);
					const double cos_t = std::cos(theta);
					const double sin_p = std::sin(phi);
					const double cos_p = std::cos(phi);

					const std::array<double, 3> rel_pos{
						r * sin_t * cos_p,
						r * sin_t * sin_p,
						r * cos_t
					};

					double enclosed = central_mass;
					for (uint32_t h = 0; h < field.count; ++h) {
						if (field.halos[h].enabled) {
							enclosed += field.halos[h].enclosed_mass(r);
						}
					}
					const double v_circ = std::sqrt(std::max(G * enclosed, 0.0) / r);
					const bool retrograde = (rng.next_uniform() < tracer_retrograde_fraction_);
					const double v_sign = retrograde ? -1.0 : 1.0;

					std::array<double, 3> rel_vel{
						-v_sign * v_circ * sin_p,
						 v_sign * v_circ * cos_p,
						 0.0
					};

					if (tracer_distribution_mode_ == 1) {
						std::array<double, 3> radial{sin_t * cos_p, sin_t * sin_p, cos_t};
						std::array<double, 3> random_dir{rng.next_uniform() - 0.5, rng.next_uniform() - 0.5, rng.next_uniform() - 0.5};
						std::array<double, 3> tangent{
							radial[1] * random_dir[2] - radial[2] * random_dir[1],
							radial[2] * random_dir[0] - radial[0] * random_dir[2],
							radial[0] * random_dir[1] - radial[1] * random_dir[0]
						};
						const double tan_len = std::sqrt(tangent[0]*tangent[0] + tangent[1]*tangent[1] + tangent[2]*tangent[2]);
						if (tan_len > 1e-9) {
							rel_vel = {
								tangent[0] / tan_len * v_circ * v_sign,
								tangent[1] / tan_len * v_circ * v_sign,
								tangent[2] / tan_len * v_circ * v_sign
							};
						}
					}

					Dynamics::PostNewtonianBody body{};
					body.id = 0;
					body.mass = tracer_mass_;
					body.radius = std::max(0.02 * r, 0.1);
					body.position = {center[0] + rel_pos[0], center[1] + rel_pos[1], center[2] + rel_pos[2]};
					body.velocity = {host_velocity[0] + rel_vel[0], host_velocity[1] + rel_vel[1], host_velocity[2] + rel_vel[2]};
					body.color = {halo.tint[0], halo.tint[1], halo.tint[2], 1.0f};
					body.preset_3d = Dynamics::Body3DPreset::Asteroid;

					char name_buf[32];
					std::snprintf(name_buf, sizeof(name_buf), "Tracer %u", spawned + 1U);
					body.set_name(name_buf);

					system.add_body(std::move(body));
					++spawned;
				}

				system.update_accelerations();
				orchestrator.notify_state_changed();
				status_ = "Spawned " + std::to_string(spawned) + " N-body tracer test bodies in circular orbit.";
			}
		}
		ImGui::EndDisabled();
	}
};

} // namespace Relativistic::UI
