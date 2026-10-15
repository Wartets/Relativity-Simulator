#pragma once

#include <cstddef>

namespace Relativistic::UI {

inline constexpr const char* kMetricCatalog[] = {
	"Flat Minkowski",
	"Schwarzschild Black Hole",
	"Kerr Rotating Black Hole",
	"Reissner-Nordstrom Charged",
	"Kerr-Newman Charged Rotating",
	"Schwarzschild-de Sitter (Lambda)",
	"FLRW Cosmological Expansion",
	"Morris-Thorne Traversable Wormhole",
	"Alcubierre Warp Drive Bubble",
	"BSSN 3+1 Numerical Grid",
	"Lemaitre-Tolman-Bondi Void",
	"Quintessence Dark Energy Cosmology",
	"Chaplygin Gas Cosmology",
	"Evaporating Vaidya Black Hole",
	"Eddington-Finkelstein Black Hole",
	"Painleve-Gullstrand Black Hole",
	"TeVeS Tensor-Vector-Scalar Gravity"
};

inline constexpr size_t kMetricCatalogSize = sizeof(kMetricCatalog) / sizeof(kMetricCatalog[0]);

}
