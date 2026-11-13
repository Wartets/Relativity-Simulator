#pragma once

#include <imgui.h>
#include "relativistic/ui/body_manager/body_manager_context.hpp"
#include "relativistic/ui/numeric_slider_utils.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/ui/panel_text_helpers.hpp"
#include "relativistic/orchestrator/command.hpp"
#include "relativistic/dynamics/interaction_compatibility.hpp"
#include "relativistic/units/unit_system.hpp"
#include <mutex>
#include <string>

namespace Relativistic::UI {

class InteractionsTab {
private:
	bool em_permittivity_log_mode_{true};
	bool em_permeability_log_mode_{true};
	bool collision_stiffness_log_mode_{true};
	bool fragmentation_min_mass_log_mode_{true};
	bool fragmentation_energy_integrity_log_mode_{true};
	bool fragmentation_tidal_integrity_log_mode_{true};

public:
	void render(BodyManagerContext& ctx) noexcept {
		auto& orchestrator = ctx.orchestrator();
		auto& cfg = orchestrator.interaction_config();
		std::lock_guard<std::recursive_mutex> interactions_lock(orchestrator.nbody_system().bodies_mutex());
		const auto bodies_span = orchestrator.nbody_system().bodies();

		render_group_title("Physical Coupling Consistency Dashboard", ImVec4(0.4f, 0.85f, 1.0f, 1.0f));
		render_hint_text("Live matrix of all active interaction and coupling models across the N-body system.");

		const bool coulomb_active = cfg.electromagnetic.electricity_enabled;
		const bool dipole_active = cfg.electromagnetic.magnetism_enabled;
		const bool collisions_active = cfg.collisions.enabled;
		const bool thermo_active = cfg.thermodynamics.enabled;
		const bool frag_active = cfg.fragmentation.enabled;
		const bool tidal_active = frag_active && cfg.fragmentation.enable_tidal_stress;
		const bool ann_active = cfg.annihilation.enabled;

		ImGui::Columns(3, "##ConsistencyMatrix", true);
		ImGui::Text("Gravitation:");
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "  Post-Newtonian (1PN)");
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "  Spin-Orbit Coupling");
		ImGui::NextColumn();

		ImGui::Text("Electromagnetism:");
		ImGui::TextColored(coulomb_active ? ImVec4(0.3f, 0.9f, 0.5f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "  Coulomb: %s", coulomb_active ? "Active" : "Off");
		ImGui::TextColored(dipole_active ? ImVec4(0.3f, 0.9f, 0.5f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "  Dipoles: %s", dipole_active ? "Active" : "Off");
		ImGui::NextColumn();

		ImGui::Text("Mechanics & Thermal:");
		ImGui::TextColored(collisions_active ? ImVec4(0.3f, 0.9f, 0.5f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "  Contact: %s", collisions_active ? "Hertzian" : "Off");
		ImGui::TextColored(thermo_active ? ImVec4(0.3f, 0.9f, 0.5f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "  Radiative: %s", thermo_active ? "Active" : "Off");
		ImGui::Columns(1);

		ImGui::Columns(2, "##ConsistencyMatrix2", true);
		ImGui::Text("Integrity & Fragmentation:");
		ImGui::TextColored(frag_active ? ImVec4(0.3f, 0.9f, 0.5f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "  Collision Impact: %s", frag_active ? "Active" : "Off");
		ImGui::TextColored(tidal_active ? ImVec4(0.3f, 0.9f, 0.5f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "  Roche Tidal: %s", tidal_active ? "Active" : "Off");
		ImGui::NextColumn();

		ImGui::Text("Matter Annihilation:");
		ImGui::TextColored(ann_active ? ImVec4(0.85f, 0.45f, 1.0f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "  Rest-Mass Conversion: %s", ann_active ? "Active" : "Off");
		ImGui::Columns(1);
		ImGui::Separator();

		ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Electromagnetic Interactions");
		bool electricity = cfg.electromagnetic.electricity_enabled;
		if (ImGui::Checkbox("Enable Electricity (Coulomb Force)", &electricity)) {
			static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionElectricityEnabled, electricity ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Enables attraction and repulsion between charged bodies following Coulomb's law, scaled by the medium permittivity below. Charge is set per body in the Object Catalog tab.");
		bool magnetism = cfg.electromagnetic.magnetism_enabled;
		if (ImGui::Checkbox("Enable Magnetism (Dipole Force)", &magnetism)) {
			static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionMagnetismEnabled, magnetism ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Enables dipole-dipole magnetic forces derived from each body's magnetic moment value. Both interacting bodies must have a non-zero magnetic moment for a force to appear.");
		{
			const auto warn = Dynamics::magnetism_without_moments_warning(bodies_span, cfg.electromagnetic);
			if (!warn.empty()) render_wrapped_colored_text(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), std::string(warn).c_str());
		}
		float permittivity = static_cast<float>(cfg.electromagnetic.vacuum_permittivity);
		if (slider_float_with_input("Medium Permittivity (epsilon)", &permittivity, 1e-14f, 1.0f, "%.4e", &em_permittivity_log_mode_, 1e-15f, 1e2f)) {
			cfg.electromagnetic.vacuum_permittivity = static_cast<double>(permittivity);
			static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionVacuumPermittivity, static_cast<double>(permittivity))));
		}
		render_setting_tooltip("Electrical permittivity of the medium the bodies interact through. Reduces the Coulomb force below its vacuum strength as this value grows. Real vacuum permittivity is roughly 8.85e-12; enable Log for practical control at that scale.");
		float permeability = static_cast<float>(cfg.electromagnetic.vacuum_permeability);
		if (slider_float_with_input("Medium Permeability (mu)", &permeability, 1e-10f, 10.0f, "%.4e", &em_permeability_log_mode_, 1e-11f, 1e3f)) {
			cfg.electromagnetic.vacuum_permeability = static_cast<double>(permeability);
			static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionVacuumPermeability, static_cast<double>(permeability))));
		}
		render_setting_tooltip("Magnetic permeability of the medium the bodies interact through, scaling the dipole-dipole magnetic force.");
		ImGui::Spacing();

		const auto& engine = orchestrator.constants_engine();
		const double true_eps = engine.sim_vacuum_permittivity();
		const double true_mu = engine.sim_vacuum_permeability();
		const bool is_vacuum = (std::abs(cfg.electromagnetic.vacuum_permittivity - true_eps) <= 1e-18 * std::max(true_eps, 1.0))
			&& (std::abs(cfg.electromagnetic.vacuum_permeability - true_mu) <= 1e-12 * std::max(true_mu, 1.0));

		if (is_vacuum) {
			ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "Medium Status: Perfect Vacuum (Synchronized with Constants Engine)");
		} else {
			ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.35f, 1.0f), "Medium Status: Custom Medium (Dielectric / Magnetic Material)");
			render_hint_text("Permittivity or permeability differ from fundamental physical vacuum values, scaling Coulomb or dipole forces accordingly.");
		}

		if (ImGui::Button("Sync To Vacuum Values From Constants Engine", ImVec2(-1.0f, 26.0f))) {
			cfg.electromagnetic.vacuum_permittivity = true_eps;
			cfg.electromagnetic.vacuum_permeability = true_mu;
			static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionVacuumPermittivity, cfg.electromagnetic.vacuum_permittivity)));
			static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionVacuumPermeability, cfg.electromagnetic.vacuum_permeability)));
		}
		render_setting_tooltip("Overwrites the two fields above with the true vacuum permittivity and permeability derived from the fundamental constants c, G, h, kB currently active in the Physical Constants Engine window, keeping both subsystems consistent.");
		if (ImGui::Button("Reset Electromagnetic Defaults", ImVec2(-1.0f, 22.0f))) {
			cfg.electromagnetic = Dynamics::ElectromagneticInteractionConfig{};
			static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionElectricityEnabled, 0.0)));
			static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionMagnetismEnabled, 0.0)));
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "Collisions (Mechanical Contact Dynamics)");
		render_hint_text("Physical Contact Dynamics: Simulates non-destructive mechanical collisions that conserve rest mass, resolving surface contact through Hertzian elasticity, Baumgarte stabilization, restitution, and Coulomb friction.");
		bool collisions = cfg.collisions.enabled;
		if (ImGui::Checkbox("Enable Collisions", &collisions)) {
			static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionsEnabled, collisions ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Detects physical contact between overlapping bodies and resolves it with a Hertzian material-stiffness repulsion plus an impulse-based restitution and friction response. See docs/other/COLLISION_MODEL.md for the underlying derivation.");
		{
			const auto warn = Dynamics::collisions_without_radius_warning(bodies_span, cfg.collisions);
			if (!warn.empty()) render_wrapped_colored_text(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), std::string(warn).c_str());
		}
		if (collisions) {
			int response_idx = static_cast<int>(cfg.collisions.response_model);
			const char* response_names[] = {"Elastic (Restitution-Based)", "Inelastic (Perfectly Damped)"};
			if (ImGui::Combo("Response Model", &response_idx, response_names, IM_ARRAYSIZE(response_names))) {
				static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionResponseModel, static_cast<double>(response_idx))));
			}
			render_setting_tooltip("Elastic uses each body's Restitution property to bounce apart; Inelastic removes all normal-direction relative velocity on contact.");
			bool consider_rotation = cfg.collisions.consider_rotation;
			if (ImGui::Checkbox("Consider Rotation", &consider_rotation)) {
				static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionConsiderRotation, consider_rotation ? 1.0 : 0.0)));
			}
			render_setting_tooltip("Lets tangential friction impulses spin bodies up around their rotation axis, approximated as solid spheres (I = 0.4 * m * r^2).");
			ImGui::SameLine();
			bool consider_friction = cfg.collisions.consider_friction;
			if (ImGui::Checkbox("Consider Friction", &consider_friction)) {
				static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionConsiderFriction, consider_friction ? 1.0 : 0.0)));
			}
			render_setting_tooltip("Applies a Coulomb-clamped tangential impulse opposing contact-point sliding velocity, using the average of both bodies' Friction Coefficient property.");
			float restitution_mult = static_cast<float>(cfg.collisions.restitution_multiplier);
			if (slider_float_with_input("Restitution Multiplier", &restitution_mult, 0.0f, 4.0f, "%.2fx")) {
				static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionRestitutionMultiplier, static_cast<double>(restitution_mult))));
			}
			render_setting_tooltip("Global multiplier applied on top of each body's own Restitution property before it is clamped back into the physical [0, 1] range.");
			float stiffness = static_cast<float>(cfg.collisions.contact_stiffness_scale);
			if (slider_float_with_input("Contact Stiffness Scale", &stiffness, 1e-15f, 1.0f, "%.6e", &collision_stiffness_log_mode_, 1e-15f, 1e3f)) {
				static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionStiffnessScale, static_cast<double>(stiffness))));
			}
			render_setting_tooltip("Scales the Hertzian penetration repulsion force computed from each body's Young's Modulus property and the current overlap depth. Zero disables material-stiffness pushback and relies only on the instantaneous impulse response below.");
			float position_correction = static_cast<float>(cfg.collisions.position_correction_factor);
			if (slider_float_with_input("Position Correction Factor", &position_correction, 0.0f, 1.0f, "%.2f")) {
				static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionPositionCorrectionFactor, static_cast<double>(position_correction))));
			}
			render_setting_tooltip("Baumgarte-style geometric correction that nudges deeply overlapping bodies apart each step, mass-weighted, preventing residual sinking when the material stiffness above is too soft to fully separate stiff bodies in a single step.");
			if (ImGui::Button("Reset Collision Defaults", ImVec2(-1.0f, 22.0f))) {
				cfg.collisions = Dynamics::CollisionInteractionConfig{};
				static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionsEnabled, 0.0)));
			}
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "Thermodynamics");
		bool thermo = cfg.thermodynamics.enabled;
		if (ImGui::Checkbox("Enable Thermodynamics", &thermo)) {
			static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionThermodynamicsEnabled, thermo ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Tracks per-body Temperature and Heat Capacity properties and radiates energy toward the ambient temperature below via the Stefan-Boltzmann law, scaled by each body's Absorption Factor.");
		{
			const auto warn = Dynamics::thermodynamics_disabled_ambient_note(cfg.thermodynamics);
			if (!warn.empty()) render_wrapped_colored_text(ImVec4(0.6f, 0.75f, 1.0f, 1.0f), std::string(warn).c_str());
		}
		if (thermo) {
			float ambient = static_cast<float>(cfg.thermodynamics.ambient_temperature_kelvin);
			if (slider_float_with_input("Ambient Temperature (K, -1 = none)", &ambient, -1.0f, 6000.0f, "%.1f")) {
				static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionAmbientTemperature, static_cast<double>(ambient))));
			}
			render_setting_tooltip("Background radiative temperature bodies cool toward or heat toward via Stefan-Boltzmann emission. Set to -1 to disable ambient radiative coupling entirely while keeping per-body heat capacity bookkeeping active.");
			if (ambient >= 0.0f) {
				ImGui::TextDisabled("%s", Units::format_temperature(static_cast<double>(ambient), orchestrator.unit_preferences().temperature).c_str());
			}
			float coupling = static_cast<float>(cfg.thermodynamics.radiative_coupling_scale);
			if (slider_float_with_input("Radiative Coupling Scale", &coupling, 0.0f, 10.0f, "%.2fx")) {
				static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionRadiativeCouplingScale, static_cast<double>(coupling))));
			}
			render_setting_tooltip("Multiplies the Stefan-Boltzmann radiative exchange rate, letting the ambient coupling be sped up or slowed down without touching the physical Stefan-Boltzmann constant itself.");
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.4f, 1.0f), "Fragmentation");
		bool fragmentation = cfg.fragmentation.enabled;
		if (ImGui::Checkbox("Enable Fragmentation", &fragmentation)) {
			static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionFragmentationEnabled, fragmentation ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Allows bodies to shatter into smaller fragments once their Integrity property is depleted by high-energy collisions and, optionally, sustained tidal stress from the central spacetime source.");
		{
			const auto warn = Dynamics::fragmentation_requires_source_warning(cfg.fragmentation, cfg.collisions);
			if (!warn.empty()) render_wrapped_colored_text(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), std::string(warn).c_str());
		}
		if (fragmentation) {
			float min_fragment_mass = static_cast<float>(cfg.fragmentation.minimum_fragment_mass);
			if (slider_float_with_input("Minimum Fragment Mass", &min_fragment_mass, 1e-9f, 1e3f, "%.4e", &fragmentation_min_mass_log_mode_, 1e-12f, 1e6f)) {
				static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionMinimumFragmentMass, static_cast<double>(min_fragment_mass))));
			}
			render_setting_tooltip("Fragments whose computed mass would fall below this floor are dispersed entirely rather than spawned as new bodies, preventing runaway fragment counts.");
			int max_fragments = static_cast<int>(cfg.fragmentation.max_fragments_per_event);
			if (slider_int_with_input("Max Fragments Per Event", &max_fragments, 1, 8)) {
				static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionFragmentationMaxFragments, static_cast<double>(max_fragments))));
			}
			float energy_to_integrity = static_cast<float>(cfg.fragmentation.collision_energy_to_integrity_loss);
			if (slider_float_with_input("Collision Energy To Integrity Loss", &energy_to_integrity, 1e-12f, 1.0f, "%.4e", &fragmentation_energy_integrity_log_mode_, 1e-12f, 1e2f)) {
				static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionCollisionEnergyToIntegrityLoss, static_cast<double>(energy_to_integrity))));
			}
			render_setting_tooltip("Conversion factor from an impact's kinetic energy along the collision normal into lost Integrity for both colliding bodies.");
			bool tidal_stress = cfg.fragmentation.enable_tidal_stress;
			if (ImGui::Checkbox("Enable Tidal Disruption From Central Source", &tidal_stress)) {
				static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionFragmentationTidalStressEnabled, tidal_stress ? 1.0 : 0.0)));
			}
			render_setting_tooltip("Continuously erodes Integrity for bodies close to the central mass based on the differential gravitational acceleration across their physical radius, approximating tidal stretching near the Roche limit.");
			if (tidal_stress) {
				float tidal_to_integrity = static_cast<float>(cfg.fragmentation.tidal_stress_to_integrity_loss);
				if (slider_float_with_input("Tidal Stress To Integrity Loss", &tidal_to_integrity, 1e-12f, 1.0f, "%.4e", &fragmentation_tidal_integrity_log_mode_, 1e-12f, 1e2f)) {
					static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionFragmentationTidalStressToIntegrityLoss, static_cast<double>(tidal_to_integrity))));
				}
				render_setting_tooltip("Conversion factor from tidal stress energy density into lost Integrity per second.");
			}
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.7f, 0.5f, 1.0f, 1.0f), "Annihilation (Matter-Antimatter Rest-Mass Conversion)");
		render_hint_text("Matter-Antimatter Annihilation: In contrast to mechanical collisions which conserve mass and cause bodies to bounce, annihilation converts overlapping rest mass into radiant energy, destroying or eroding both bodies when opposite charges meet.");
		bool annihilation = cfg.annihilation.enabled;
		if (ImGui::Checkbox("Enable Annihilation", &annihilation)) {
			static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionAnnihilationEnabled, annihilation ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Removes both colliding bodies entirely once they satisfy the contact and charge conditions below, modeling a complete matter-antimatter style annihilation event.");
		{
			const auto warn = Dynamics::annihilation_requires_charge_warning(cfg.annihilation, cfg.electromagnetic);
			if (!warn.empty()) render_wrapped_colored_text(ImVec4(0.6f, 0.75f, 1.0f, 1.0f), std::string(warn).c_str());
		}
		if (annihilation) {
			bool opposite_charge = cfg.annihilation.require_opposite_charge;
			if (ImGui::Checkbox("Require Opposite Charge", &opposite_charge)) {
				static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionAnnihilationRequireOppositeCharge, opposite_charge ? 1.0 : 0.0)));
			}
			float contact_scale = static_cast<float>(cfg.annihilation.contact_distance_scale);
			if (slider_float_with_input("Contact Distance Scale", &contact_scale, 0.01f, 4.0f, "%.2fx")) {
				static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionAnnihilationContactScale, static_cast<double>(contact_scale))));
			}
			render_setting_tooltip("Fraction of the combined radii within which annihilation triggers; values below 1 require deeper overlap than plain collision contact.");
			if (ImGui::Button("Reset Annihilation Defaults", ImVec2(-1.0f, 22.0f))) {
				cfg.annihilation = Dynamics::AnnihilationInteractionConfig{};
				static_cast<void>(orchestrator.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InteractionAnnihilationEnabled, 0.0)));
			}
		}
	}
};

}
