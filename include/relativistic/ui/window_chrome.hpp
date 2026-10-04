#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Relativistic::UI {

enum class WindowThemeId : uint32_t {
	Viewport = 0,
	Controls,
	Scenarios,
	Bodies,
	Performance,
	Telemetry,
	Spectrograph,
	Diagnostics,
	Analysis,
	Keybinds,
	Constants,
	LogConsole,
	CaptureStudio,
	SecondaryViews,
	Count
};

inline constexpr size_t kWindowThemeCount = static_cast<size_t>(WindowThemeId::Count);

struct WindowThemeDescriptor {
	const char* label;
	std::array<float, 3> accent;
};

inline constexpr std::array<WindowThemeDescriptor, kWindowThemeCount> kWindowThemeDescriptors{{
	{"Primary Viewport", {0.12f, 0.30f, 0.58f}},
	{"Master Simulation Controls", {0.08f, 0.42f, 0.40f}},
	{"Scenario Manager", {0.45f, 0.25f, 0.58f}},
	{"Celestial Body Manager", {0.58f, 0.32f, 0.10f}},
	{"Performance Settings", {0.14f, 0.46f, 0.18f}},
	{"Telemetry", {0.55f, 0.16f, 0.30f}},
	{"Spectrograph", {0.46f, 0.42f, 0.08f}},
	{"Curvature Diagnostics", {0.30f, 0.20f, 0.62f}},
	{"Performance Analysis", {0.08f, 0.40f, 0.56f}},
	{"Keybind Settings", {0.48f, 0.20f, 0.20f}},
	{"Physical Constants", {0.58f, 0.18f, 0.46f}},
	{"Engine Log Console", {0.28f, 0.31f, 0.36f}},
	{"Capture Studio", {0.62f, 0.22f, 0.12f}},
	{"Secondary Viewports", {0.20f, 0.46f, 0.30f}}
}};

inline constexpr std::array<std::array<float, 3>, 12> kWindowThemeSwatches{{
	{0.12f, 0.30f, 0.58f}, {0.08f, 0.42f, 0.40f}, {0.45f, 0.25f, 0.58f}, {0.58f, 0.32f, 0.10f},
	{0.14f, 0.46f, 0.18f}, {0.55f, 0.16f, 0.30f}, {0.46f, 0.42f, 0.08f}, {0.30f, 0.20f, 0.62f},
	{0.08f, 0.40f, 0.56f}, {0.62f, 0.22f, 0.12f}, {0.58f, 0.18f, 0.46f}, {0.28f, 0.31f, 0.36f}
}};

struct WindowGeometry {
	float x{0.0f};
	float y{0.0f};
	float width{0.0f};
	float height{0.0f};
};

struct WindowChromeSettings {
	std::array<std::array<float, 3>, kWindowThemeCount> accents{};
	std::array<bool, kWindowThemeCount> customized{};
	std::unordered_map<std::string, WindowGeometry> geometries{};

	WindowChromeSettings() {
		reset_all_accents();
	}

	[[nodiscard]] const std::array<float, 3>& accent(WindowThemeId id) const noexcept {
		return accents[static_cast<size_t>(id)];
	}

	void set_accent(WindowThemeId id, const std::array<float, 3>& rgb) noexcept {
		const size_t index = static_cast<size_t>(id);
		for (size_t c = 0; c < 3; ++c) {
			accents[index][c] = std::clamp(rgb[c], 0.0f, 1.0f);
		}
		customized[index] = true;
	}

	void reset_accent(WindowThemeId id) noexcept {
		const size_t index = static_cast<size_t>(id);
		accents[index] = kWindowThemeDescriptors[index].accent;
		customized[index] = false;
	}

	void reset_all_accents() noexcept {
		for (size_t i = 0; i < kWindowThemeCount; ++i) {
			accents[i] = kWindowThemeDescriptors[i].accent;
			customized[i] = false;
		}
	}

	void write(std::ostream& out) const {
		for (size_t i = 0; i < kWindowThemeCount; ++i) {
			out << "wchrome_" << i << "_custom=" << (customized[i] ? 1 : 0) << "\n";
			out << "wchrome_" << i << "_r=" << accents[i][0] << "\n";
			out << "wchrome_" << i << "_g=" << accents[i][1] << "\n";
			out << "wchrome_" << i << "_b=" << accents[i][2] << "\n";
		}
		out << "wgeo_count=" << geometries.size() << "\n";
		size_t index = 0;
		for (const auto& [title, geometry] : geometries) {
			out << "wgeo_" << index << "_name=" << title << "\n";
			out << "wgeo_" << index << "_x=" << geometry.x << "\n";
			out << "wgeo_" << index << "_y=" << geometry.y << "\n";
			out << "wgeo_" << index << "_w=" << geometry.width << "\n";
			out << "wgeo_" << index << "_h=" << geometry.height << "\n";
			++index;
		}
	}

	void read(const std::unordered_map<std::string, std::string>& entries) {
		const auto real = [&entries](const std::string& key, float fallback) noexcept {
			const auto it = entries.find(key);
			return (it != entries.end()) ? static_cast<float>(std::strtod(it->second.c_str(), nullptr)) : fallback;
		};
		for (size_t i = 0; i < kWindowThemeCount; ++i) {
			const std::string prefix = "wchrome_" + std::to_string(i) + "_";
			if (real(prefix + "custom", 0.0f) > 0.5f) {
				set_accent(static_cast<WindowThemeId>(i), {real(prefix + "r", accents[i][0]), real(prefix + "g", accents[i][1]), real(prefix + "b", accents[i][2])});
			}
		}
		geometries.clear();
		const size_t count = std::min<size_t>(static_cast<size_t>(std::max(real("wgeo_count", 0.0f), 0.0f)), 256U);
		for (size_t i = 0; i < count; ++i) {
			const std::string prefix = "wgeo_" + std::to_string(i) + "_";
			const auto name = entries.find(prefix + "name");
			if (name == entries.end() || name->second.empty()) {
				continue;
			}
			geometries[name->second] = WindowGeometry{real(prefix + "x", 0.0f), real(prefix + "y", 0.0f), real(prefix + "w", 0.0f), real(prefix + "h", 0.0f)};
		}
	}
};

namespace WindowChromeDetail {

#if IMGUI_VERSION_NUM >= 19090
inline constexpr ImGuiCol kTabSelected = ImGuiCol_TabSelected;
inline constexpr ImGuiCol kTabDimmed = ImGuiCol_TabDimmed;
inline constexpr ImGuiCol kTabDimmedSelected = ImGuiCol_TabDimmedSelected;
#else
inline constexpr ImGuiCol kTabSelected = ImGuiCol_TabActive;
inline constexpr ImGuiCol kTabDimmed = ImGuiCol_TabUnfocused;
inline constexpr ImGuiCol kTabDimmedSelected = ImGuiCol_TabUnfocusedActive;
#endif

inline constexpr float kMaximumTitleLuminance = 0.14f;
inline constexpr std::array<float, 3> kWhite{1.0f, 1.0f, 1.0f};

[[nodiscard]] inline float linearize(float c) noexcept {
	return (c <= 0.04045f) ? (c / 12.92f) : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

[[nodiscard]] inline float relative_luminance(const std::array<float, 3>& c) noexcept {
	return 0.2126f * linearize(c[0]) + 0.7152f * linearize(c[1]) + 0.0722f * linearize(c[2]);
}

[[nodiscard]] inline std::array<float, 3> scaled(const std::array<float, 3>& c, float factor) noexcept {
	return {std::clamp(c[0] * factor, 0.0f, 1.0f), std::clamp(c[1] * factor, 0.0f, 1.0f), std::clamp(c[2] * factor, 0.0f, 1.0f)};
}

[[nodiscard]] inline std::array<float, 3> blended(const std::array<float, 3>& a, const std::array<float, 3>& b, float t) noexcept {
	return {a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t};
}

[[nodiscard]] inline ImVec4 to_color(const std::array<float, 3>& c, float alpha) noexcept {
	return ImVec4(c[0], c[1], c[2], alpha);
}

struct WindowThemePalette {
	ImVec4 title_active;
	ImVec4 title_inactive;
	ImVec4 title_collapsed;
	ImVec4 border;
	ImVec4 header;
	ImVec4 header_hovered;
	ImVec4 header_active;
	ImVec4 grab;
	ImVec4 grab_active;
	ImVec4 check;
	ImVec4 tab;
	ImVec4 tab_hovered;
	ImVec4 tab_selected;
	ImVec4 tab_dimmed;
	ImVec4 tab_dimmed_selected;
};

[[nodiscard]] inline WindowThemePalette derive_palette(std::array<float, 3> accent) noexcept {
	for (float& channel : accent) {
		channel = std::clamp(channel, 0.0f, 1.0f);
	}
	std::array<float, 3> base = accent;
	for (int i = 0; i < 32 && relative_luminance(base) > kMaximumTitleLuminance; ++i) {
		base = scaled(base, 0.92f);
	}
	const float peak = std::max({accent[0], accent[1], accent[2], 1e-3f});
	std::array<float, 3> vivid = scaled(accent, 0.92f / peak);
	if (relative_luminance(vivid) < 0.12f) {
		vivid = blended(vivid, kWhite, 0.3f);
	}

	WindowThemePalette p{};
	p.title_active = to_color(base, 1.0f);
	p.title_inactive = to_color(scaled(base, 0.6f), 1.0f);
	p.title_collapsed = to_color(scaled(base, 0.45f), 0.85f);
	p.border = to_color(blended(vivid, base, 0.35f), 0.85f);
	p.header = to_color(blended(base, kWhite, 0.04f), 0.8f);
	p.header_hovered = to_color(blended(base, kWhite, 0.10f), 0.95f);
	p.header_active = to_color(blended(base, kWhite, 0.16f), 1.0f);
	p.grab = to_color(vivid, 1.0f);
	p.grab_active = to_color(blended(vivid, kWhite, 0.3f), 1.0f);
	p.check = to_color(blended(vivid, kWhite, 0.15f), 1.0f);
	p.tab = to_color(scaled(base, 0.75f), 0.95f);
	p.tab_hovered = to_color(blended(base, kWhite, 0.12f), 1.0f);
	p.tab_selected = to_color(base, 1.0f);
	p.tab_dimmed = to_color(scaled(base, 0.5f), 0.95f);
	p.tab_dimmed_selected = to_color(scaled(base, 0.8f), 1.0f);
	return p;
}

[[nodiscard]] inline WindowGeometry fit_to_monitors(WindowGeometry geometry) noexcept {
	const ImGuiPlatformIO& platform = ImGui::GetPlatformIO();
	if (platform.Monitors.Size == 0) {
		return geometry;
	}
	const ImVec2 probe(geometry.x + std::min(geometry.width, 240.0f) * 0.5f, geometry.y + 12.0f);
	const ImGuiPlatformMonitor* host = nullptr;
	for (const ImGuiPlatformMonitor& monitor : platform.Monitors) {
		const ImVec2 min = monitor.WorkPos;
		const ImVec2 max(monitor.WorkPos.x + monitor.WorkSize.x, monitor.WorkPos.y + monitor.WorkSize.y);
		if (probe.x >= min.x && probe.x < max.x && probe.y >= min.y && probe.y < max.y) {
			host = &monitor;
			break;
		}
	}
	const bool relocated = (host == nullptr);
	if (relocated) {
		host = &platform.Monitors[0];
	}
	const float work_x = host->WorkPos.x;
	const float work_y = host->WorkPos.y;
	const float work_w = host->WorkSize.x;
	const float work_h = host->WorkSize.y;
	if (geometry.width > 0.0f) {
		geometry.width = std::min(geometry.width, work_w * 0.95f);
	}
	if (geometry.height > 0.0f) {
		geometry.height = std::min(geometry.height, work_h * 0.95f);
	}
	if (relocated) {
		geometry.x = work_x + 40.0f;
		geometry.y = work_y + 40.0f;
	}
	geometry.x = std::clamp(geometry.x, work_x, std::max(work_x, work_x + work_w - std::max(geometry.width, 120.0f)));
	geometry.y = std::clamp(geometry.y, work_y, std::max(work_y, work_y + work_h - 40.0f));
	return geometry;
}

}

class WindowChromeController {
private:
	struct OwnedWindow {
		ImGuiWindow* window;
		WindowThemeId id;
	};

	static constexpr int kPushedColors = 15;
	static constexpr const char* kPopupId = "##WindowChromeAccentPopup";

	WindowChromeSettings& settings_;
	std::vector<OwnedWindow> owned_{};
	std::vector<std::string> known_titles_{};
	std::unordered_set<std::string> restored_{};
	std::unordered_map<std::string, WindowGeometry> pending_{};
	WindowThemeId popup_target_{WindowThemeId::Viewport};

	[[nodiscard]] static bool is_top_level(const ImGuiWindow* window) noexcept {
		constexpr ImGuiWindowFlags excluded = ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_ChildMenu;
		return (window->Flags & excluded) == 0;
	}

	void apply_initial_geometry(ImGuiWindow* window) {
		if (window->DockIsActive || (window->Flags & ImGuiWindowFlags_NoMove) != 0) {
			return;
		}
		const std::string title = window->Name;
		const WindowGeometry* source = nullptr;
		const auto pending = pending_.find(title);
		if (pending != pending_.end()) {
			source = &pending->second;
		} else {
			const auto stored = settings_.geometries.find(title);
			if (stored != settings_.geometries.end()) {
				source = &stored->second;
			}
		}
		if (source == nullptr) {
			return;
		}
		const WindowGeometry fitted = WindowChromeDetail::fit_to_monitors(*source);
		ImGui::SetWindowPos(window, ImVec2(fitted.x, fitted.y), ImGuiCond_Always);
		if (fitted.width >= 64.0f && fitted.height >= 32.0f) {
			ImGui::SetWindowSize(window, ImVec2(fitted.width, fitted.height), ImGuiCond_Always);
		}
		if (pending != pending_.end()) {
			pending_.erase(pending);
		}
	}

	void adopt_new_windows(const std::vector<const ImGuiWindow*>& active_before, WindowThemeId id) {
		ImGuiContext& context = *ImGui::GetCurrentContext();
		for (ImGuiWindow* window : context.Windows) {
			if (window->LastFrameActive != context.FrameCount || !is_top_level(window)) {
				continue;
			}
			if (std::find(active_before.begin(), active_before.end(), window) != active_before.end()) {
				continue;
			}
			owned_.push_back(OwnedWindow{window, id});
			const std::string title = window->Name;
			if (std::find(known_titles_.begin(), known_titles_.end(), title) == known_titles_.end()) {
				known_titles_.push_back(title);
			}
			if (restored_.insert(title).second) {
				apply_initial_geometry(window);
			}
		}
	}

public:
	class Scope {
	public:
		Scope(WindowChromeController& controller, WindowThemeId id)
			: controller_(controller), id_(id) {
			ImGuiContext& context = *ImGui::GetCurrentContext();
			active_before_.reserve(static_cast<size_t>(context.Windows.Size));
			for (const ImGuiWindow* window : context.Windows) {
				if (window->LastFrameActive == context.FrameCount) {
					active_before_.push_back(window);
				}
			}
			const WindowChromeDetail::WindowThemePalette p = WindowChromeDetail::derive_palette(controller.settings_.accent(id));
			ImGui::PushStyleColor(ImGuiCol_TitleBg, p.title_inactive);
			ImGui::PushStyleColor(ImGuiCol_TitleBgActive, p.title_active);
			ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, p.title_collapsed);
			ImGui::PushStyleColor(ImGuiCol_Border, p.border);
			ImGui::PushStyleColor(ImGuiCol_Header, p.header);
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, p.header_hovered);
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, p.header_active);
			ImGui::PushStyleColor(ImGuiCol_SliderGrab, p.grab);
			ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, p.grab_active);
			ImGui::PushStyleColor(ImGuiCol_CheckMark, p.check);
			ImGui::PushStyleColor(ImGuiCol_Tab, p.tab);
			ImGui::PushStyleColor(ImGuiCol_TabHovered, p.tab_hovered);
			ImGui::PushStyleColor(WindowChromeDetail::kTabSelected, p.tab_selected);
			ImGui::PushStyleColor(WindowChromeDetail::kTabDimmed, p.tab_dimmed);
			ImGui::PushStyleColor(WindowChromeDetail::kTabDimmedSelected, p.tab_dimmed_selected);
		}

		~Scope() {
			ImGui::PopStyleColor(kPushedColors);
			controller_.adopt_new_windows(active_before_, id_);
		}

		Scope(const Scope&) = delete;
		Scope& operator=(const Scope&) = delete;
		Scope(Scope&&) = delete;
		Scope& operator=(Scope&&) = delete;

	private:
		WindowChromeController& controller_;
		WindowThemeId id_;
		std::vector<const ImGuiWindow*> active_before_{};
	};

	explicit WindowChromeController(WindowChromeSettings& settings) noexcept
		: settings_(settings) {}

	[[nodiscard]] Scope scope(WindowThemeId id) {
		return Scope(*this, id);
	}

	void begin_frame() noexcept {
		owned_.clear();
	}

	void place(std::string_view title, const WindowGeometry& geometry) {
		const std::string key(title);
		pending_[key] = geometry;
		if (ImGuiWindow* window = ImGui::FindWindowByName(key.c_str())) {
			restored_.insert(key);
			apply_initial_geometry(window);
		}
	}

	void process_context_requests() {
		if (!ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			return;
		}
		ImGuiContext& context = *ImGui::GetCurrentContext();
		const ImVec2 mouse = ImGui::GetIO().MousePos;
		for (const OwnedWindow& owned : owned_) {
			const ImGuiWindow* window = owned.window;
			ImRect rect;
			bool hovered = false;
			bool found_rect = false;
			if (window->DockIsActive && window->DockNode != nullptr) {
				if (window->DockNode->TabBar != nullptr) {
					const ImGuiTabBar* tab_bar = window->DockNode->TabBar;
					for (int n = 0; n < tab_bar->Tabs.Size; ++n) {
						const ImGuiTabItem& tab = tab_bar->Tabs[n];
						if (tab.Window == window && tab.Width > 0.0f) {
							const float min_x = tab_bar->BarRect.Min.x + tab.Offset - tab_bar->ScrollingAnim;
							rect = ImRect(min_x, tab_bar->BarRect.Min.y, min_x + tab.Width, tab_bar->BarRect.Max.y);
							hovered = (window->DockNode->HostWindow == context.HoveredWindow || context.HoveredWindow == window);
							found_rect = true;
							break;
						}
					}
				}
				if (!found_rect && (window->Flags & ImGuiWindowFlags_NoTitleBar) == 0) {
					rect = window->TitleBarRect();
					hovered = (context.HoveredWindow == window || window->DockNode->HostWindow == context.HoveredWindow);
					found_rect = true;
				}
			} else if ((window->Flags & ImGuiWindowFlags_NoTitleBar) == 0) {
				rect = window->TitleBarRect();
				hovered = (context.HoveredWindow == window);
				found_rect = true;
			}
			if (found_rect && hovered && rect.Contains(mouse)) {
				popup_target_ = owned.id;
				ImGui::OpenPopup(kPopupId);
				return;
			}
		}
	}

	void render_popup() {
		if (!ImGui::BeginPopup(kPopupId)) {
			return;
		}
		const size_t index = static_cast<size_t>(popup_target_);
		ImGui::TextUnformatted(kWindowThemeDescriptors[index].label);
		ImGui::Separator();
		std::array<float, 3> rgb = settings_.accents[index];
		if (ImGui::ColorEdit3("Accent Color", rgb.data(), ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoAlpha)) {
			settings_.set_accent(popup_target_, rgb);
		}
		ImGui::TextDisabled("Suggested Colors");
		for (size_t i = 0; i < kWindowThemeSwatches.size(); ++i) {
			ImGui::PushID(static_cast<int>(i));
			const auto& swatch = kWindowThemeSwatches[i];
			if (ImGui::ColorButton("##swatch", ImVec4(swatch[0], swatch[1], swatch[2], 1.0f), ImGuiColorEditFlags_NoTooltip, ImVec2(24.0f, 24.0f))) {
				settings_.set_accent(popup_target_, swatch);
			}
			ImGui::PopID();
			if ((i % 6U) != 5U) {
				ImGui::SameLine();
			}
		}
		ImGui::Separator();
		if (ImGui::Button("Reset This Window Color")) {
			settings_.reset_accent(popup_target_);
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset All Window Colors")) {
			settings_.reset_all_accents();
		}
		ImGui::EndPopup();
	}

	void capture_geometries() {
		for (const std::string& title : known_titles_) {
			const ImGuiWindow* window = ImGui::FindWindowByName(title.c_str());
			if (window == nullptr || !window->WasActive || window->DockIsActive || (window->Flags & ImGuiWindowFlags_NoMove) != 0) {
				continue;
			}
			if (window->SizeFull.x < 8.0f || window->SizeFull.y < 8.0f) {
				continue;
			}
			settings_.geometries[title] = WindowGeometry{window->Pos.x, window->Pos.y, window->SizeFull.x, window->SizeFull.y};
		}
	}
};

}
