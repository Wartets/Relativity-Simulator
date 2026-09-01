#include "relativistic/ui/ui_manager.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include <cassert>
#include <iostream>

#include <memory>

int main() {
	using namespace Relativistic::Orchestrator;
	using namespace Relativistic::UI;

	auto orchestrator = std::make_unique<SimulationOrchestrator<16384>>();
	UiManager ui_manager(*orchestrator);

	try {
		ui_manager.initialize();
		for (int i = 1; i <= 10; ++i) {
			ui_manager.add_secondary_view("Test View " + std::to_string(i));
		}
		
		for (int i = 0; i < 10; ++i) {
			ui_manager.render_frame();
		}
		
		ui_manager.shutdown();
		std::cout << "UI Multi-Window test passed.\n";
	} catch (const std::exception& e) {
		std::cerr << "UI Test failed: " << e.what() << "\n";
		return 1;
	}

	return 0;
}
