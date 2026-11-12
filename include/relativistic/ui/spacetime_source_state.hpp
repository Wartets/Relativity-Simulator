#pragma once

#include <string>

namespace Relativistic::UI {

struct SpacetimeSourceValues {
	double mass{1.0};
	double spin{0.0};
	double charge{0.0};
	double cosmological_constant{0.0};
};

struct SpacetimeSourceEditState {
	bool mass_log_mode{false};
	bool lambda_log_mode{true};
	double mass_quantity_kg{0.0};
	std::string mass_expression_error{};
};

}
