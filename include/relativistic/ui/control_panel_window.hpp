#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/command.hpp"

namespace Relativistic::UI {

class ControlPanelWindow {
private:
	bool is_open_{true};
	Orchestrator::SimulationOrchestrator<16384>& orchestrator_;

	float mass_{1.0f};
	float spin_{0.0f};
	float charge_{0.0f};
	float lambda_{0.0f};

public:
	explicit ControlPanelWindow(Orchestrator::SimulationOrchestrator<16384>& orchestrator)
		: orchestrator_(orchestrator) {
		mass_ = static_cast<float>(orchestrator.parameters().mass);
		spin_ = static_cast<float>(orchestrator.parameters().spin);
		charge_ = static_cast<float>(orchestrator.parameters().charge);
		lambda_ = static_cast<float>(orchestrator.parameters().cosmological_lambda);
	}

	void render() {
		if (!is_open_) return;

		if (ImGui::Begin("Physical Parameters Control", &is_open_)) {
			if (ImGui::SliderFloat("Mass (M)", &mass_, 0.1f, 100.0f)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Mass, static_cast<double>(mass_))));
			}
			
			if (ImGui::SliderFloat("Spin (a)", &spin_, -0.999f, 0.999f)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Spin, static_cast<double>(spin_))));
			}
			
			if (ImGui::SliderFloat("Charge (Q)", &charge_, -1.0f, 1.0f)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Charge, static_cast<double>(charge_))));
			}
			
			if (ImGui::InputFloat("Cosmological Constant (Lambda)", &lambda_, 1e-5f, 1e-4f, "%.6e")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::CosmologicalLambda, static_cast<double>(lambda_))));
			}

			ImGui::Separator();
			
			if (ImGui::Button("Pause")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_pause()));
			}
			ImGui::SameLine();
			if (ImGui::Button("Resume")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_resume()));
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_reset()));
				mass_ = 1.0f;
				spin_ = 0.0f;
				charge_ = 0.0f;
				lambda_ = 0.0f;
			}
		}
		ImGui::End();
	}
};

}
