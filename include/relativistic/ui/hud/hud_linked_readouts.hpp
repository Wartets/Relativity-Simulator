#pragma once

#include <cstddef>

namespace Relativistic::UI {

struct HudPolarizationReadout {
	bool valid{false};
	double wavelength_nm{0.0};
	double intensity{0.0};
	double dolp{0.0};
	double docp{0.0};
	double evpa_deg{0.0};
};

struct HudInterferometryReadout {
	bool valid{false};
	size_t visibility_count{0};
	size_t closure_count{0};
	double maximum_baseline_glambda{0.0};
	double resolution_uas{0.0};
	double mean_snr{0.0};
};

struct HudLinkedReadouts {
	HudPolarizationReadout polarization{};
	HudInterferometryReadout interferometry{};
};

}
