#pragma once

#include <imgui.h>
#include "relativistic/metrics/horizon_regime.hpp"
#include <algorithm>
#include <cmath>
#include <string>

namespace Relativistic::UI {

[[nodiscard]] inline ImVec4 horizon_regime_color(Metrics::HorizonRegime regime) noexcept {
	switch (regime) {
		case Metrics::HorizonRegime::SubExtremal: return ImVec4(0.18f, 0.66f, 0.34f, 1.0f);
		case Metrics::HorizonRegime::Extremal: return ImVec4(0.96f, 0.76f, 0.16f, 1.0f);
		case Metrics::HorizonRegime::SuperExtremal:
		default: return ImVec4(0.82f, 0.20f, 0.20f, 1.0f);
	}
}

[[nodiscard]] inline ImU32 horizon_regime_color_u32(Metrics::HorizonRegime regime, float alpha) noexcept {
	ImVec4 color = horizon_regime_color(regime);
	color.w = alpha;
	return ImGui::ColorConvertFloat4ToU32(color);
}

inline void render_horizon_zone_legend() noexcept {
	const Metrics::HorizonRegime regimes[] = {Metrics::HorizonRegime::SubExtremal, Metrics::HorizonRegime::Extremal, Metrics::HorizonRegime::SuperExtremal};
	const char* labels[] = {"Sub-extremal (physical black hole)", "Extremal limit", "Super-extremal (naked singularity)"};
	const char* ids[] = {"##HorizonLegendSub", "##HorizonLegendExtremal", "##HorizonLegendSuper"};
	for (int i = 0; i < 3; ++i) {
		ImGui::ColorButton(ids[i], horizon_regime_color(regimes[i]), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoInputs, ImVec2(10.0f, 10.0f));
		ImGui::SameLine();
		ImGui::TextDisabled("%s", labels[i]);
		if (i < 2) {
			ImGui::SameLine();
		}
	}
}

inline void render_horizon_zone_strip(
	const char* id,
	const Metrics::HorizonZoneLayout& layout,
	double range_min,
	double range_max,
	double current_value,
	bool logarithmic,
	Metrics::HorizonRegime regime,
	const std::string& description
) noexcept {
	const float width = std::max(ImGui::CalcItemWidth(), 96.0f);
	constexpr float height = 7.0f;
	ImGui::InvisibleButton(id, ImVec2(width, height));
	const bool hovered = ImGui::IsItemHovered();
	const ImVec2 origin = ImGui::GetItemRectMin();
	const ImVec2 limit = ImGui::GetItemRectMax();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	const bool use_log = logarithmic && range_min > 0.0 && range_max > range_min;
	const double low = use_log ? std::log10(range_min) : range_min;
	const double high = use_log ? std::log10(range_max) : range_max;
	const double span = std::max(high - low, 1e-300);
	const auto position_of = [&](double value) noexcept -> float {
		const double mapped = use_log ? std::log10(std::max(value, range_min)) : value;
		const double fraction = std::clamp((mapped - low) / span, 0.0, 1.0);
		return origin.x + static_cast<float>(fraction) * (limit.x - origin.x);
	};
	const auto fill = [&](float from_x, float to_x, ImU32 color) noexcept {
		if (to_x > from_x) {
			draw_list->AddRectFilled(ImVec2(from_x, origin.y), ImVec2(to_x, limit.y), color);
		}
	};

	const ImU32 sub_color = horizon_regime_color_u32(Metrics::HorizonRegime::SubExtremal, 0.72f);
	const ImU32 super_color = horizon_regime_color_u32(Metrics::HorizonRegime::SuperExtremal, 0.72f);
	const ImU32 extremal_color = horizon_regime_color_u32(Metrics::HorizonRegime::Extremal, 1.0f);

	const float threshold_x = position_of(layout.threshold);
	if (layout.symmetric) {
		const float mirrored_x = position_of(-layout.threshold);
		fill(origin.x, mirrored_x, super_color);
		fill(mirrored_x, threshold_x, sub_color);
		fill(threshold_x, limit.x, super_color);
	} else if (layout.sub_extremal_above) {
		fill(origin.x, threshold_x, super_color);
		fill(threshold_x, limit.x, sub_color);
	} else {
		fill(origin.x, threshold_x, sub_color);
		fill(threshold_x, limit.x, super_color);
	}

	const auto mark_extremal = [&](float x) noexcept {
		if (x > origin.x + 0.5f && x < limit.x - 0.5f) {
			draw_list->AddRectFilled(ImVec2(x - 1.5f, origin.y), ImVec2(x + 1.5f, limit.y), extremal_color);
		}
	};
	mark_extremal(threshold_x);
	if (layout.symmetric) {
		mark_extremal(position_of(-layout.threshold));
	}

	draw_list->AddRect(origin, limit, IM_COL32(0, 0, 0, 150));
	const float value_x = position_of(current_value);
	draw_list->AddRectFilled(ImVec2(value_x - 1.0f, origin.y - 2.0f), ImVec2(value_x + 1.0f, limit.y + 2.0f), IM_COL32(245, 245, 245, 255));
	draw_list->AddTriangleFilled(ImVec2(value_x, origin.y), ImVec2(value_x - 4.0f, origin.y - 5.0f), ImVec2(value_x + 4.0f, origin.y - 5.0f), horizon_regime_color_u32(regime, 1.0f));

	if (hovered) {
		ImGui::SetTooltip("%s", description.c_str());
	}
}

}
