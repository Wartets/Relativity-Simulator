#pragma once

#include <string_view>

#ifndef RELATIVISTIC_VERSION_STRING
#define RELATIVISTIC_VERSION_STRING "0.4.1.017"
#endif

namespace Relativistic::Startup::ApplicationInfo {

inline constexpr std::string_view kProductName{"Relativistic Engine"};
inline constexpr std::string_view kExecutableName{"RelativisticEngine"};
inline constexpr std::string_view kDescription{"Relativistic Physics and Raytracing Engine"};
inline constexpr std::string_view kDeveloper{"Colin Bossu Réaubourg"};
inline constexpr std::string_view kRepositoryUrl{"https://github.com/Wartets/Relativity-Simulator"};
inline constexpr std::string_view kVersion{RELATIVISTIC_VERSION_STRING};

}
