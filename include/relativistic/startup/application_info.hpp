#pragma once

#include <string_view>

#ifndef RELATIVISTIC_VERSION_STRING
#define RELATIVISTIC_VERSION_STRING "0.4.2.002"
#endif

namespace Relativistic::Startup::ApplicationInfo {

inline constexpr std::string_view kProductName{"Relativistic Engine"};
inline constexpr std::string_view kProjectTitle{"Relativity-Simulator"};
inline constexpr std::string_view kExecutableName{"RelativisticEngine"};
inline constexpr std::string_view kDescription{"Deterministic Spacetime Simulation & Polarized Ray-Tracing"};
inline constexpr std::string_view kFullDescription{"General Relativity, Post-Newtonian N-body dynamics, relativistic hydrodynamics, and polarized radiative transfer."};
inline constexpr std::string_view kDeveloper{"Colin Bossu Réaubourg"};
inline constexpr std::string_view kOrganization{"Wartets"};
inline constexpr std::string_view kLicenseName{"MIT License"};
inline constexpr std::string_view kRepositoryUrl{"https://github.com/Wartets/Relativity-Simulator"};
inline constexpr std::string_view kDocumentationUrl{"https://github.com/Wartets/Relativity-Simulator#documentation-map"};
inline constexpr std::string_view kLicenseUrl{"https://github.com/Wartets/Relativity-Simulator/blob/main/LICENSE"};
inline constexpr std::string_view kVersion{RELATIVISTIC_VERSION_STRING};

}
