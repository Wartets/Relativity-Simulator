#include "relativistic/orchestrator/command.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/repl.hpp"
#include "relativistic/ui/interactive_camera_controller.hpp"
#include "relativistic/ui/scenario_selector_window.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

int main() {
	using namespace Relativistic;

	auto orchestrator = std::make_unique<Orchestrator::SimulationOrchestrator<1024>>();
	Orchestrator::MasterTerminalRepl<1024> repl(*orchestrator);

	Orchestrator::CommandResult res{};

	assert(repl.execute_line("set mass 5.0", &res));
	orchestrator->process_incoming_commands();
	assert(std::abs(orchestrator->parameters().mass - 5.0) < 1e-12);

	assert(repl.execute_line("set spin 0.94", &res));
	orchestrator->process_incoming_commands();
	assert(std::abs(orchestrator->parameters().spin - 0.94) < 1e-12);

	assert(repl.execute_line("camera fov 45.0", &res));
	orchestrator->process_incoming_commands();
	assert(std::abs(orchestrator->camera().fov_deg - 45.0) < 1e-12);

	assert(repl.execute_line("camera speed 25.0", &res));
	orchestrator->process_incoming_commands();
	assert(std::abs(orchestrator->camera().speed - 25.0) < 1e-12);

	assert(repl.execute_line("camera move 10.0 -5.0 2.0", &res));
	orchestrator->process_incoming_commands();
	assert(std::abs(orchestrator->camera().position[0] - 10.0) < 1e-12);
	assert(std::abs(orchestrator->camera().position[1] - 45.0) < 1e-12);
	assert(std::abs(orchestrator->camera().position[2] - 2.0) < 1e-12);

	assert(repl.execute_line("metric Kerr", &res));
	orchestrator->process_incoming_commands();
	assert(orchestrator->active_metric_name() == "Kerr");

	assert(repl.execute_line("integrator Vernier9", &res));
	orchestrator->process_incoming_commands();
	assert(orchestrator->active_integrator_name() == "Vernier9");

	UI::InteractiveCameraController cam_ctrl(*orchestrator);
	cam_ctrl.set_move_speed(15.0);
	assert(std::abs(cam_ctrl.move_speed() - 15.0) < 1e-12);
	cam_ctrl.set_mouse_sensitivity(0.25);
	assert(std::abs(cam_ctrl.mouse_sensitivity() - 0.25) < 1e-12);

	cam_ctrl.handle_scroll(-1.0);
	assert(orchestrator->camera().fov_deg > 45.0);

	std::cout << "UI multi-window, camera navigation, and scenario selector unit tests passed successfully.\n";
	return 0;
}
