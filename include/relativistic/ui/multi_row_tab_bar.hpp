#pragma once

#include <imgui.h>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <utility>
#include <functional>

#ifndef IMGUI_TAB_ROUND_CORNERS
#define IMGUI_TAB_ROUND_CORNERS ImDrawFlags_RoundCornersTop
#endif

namespace Relativistic::UI {

enum class MultiRowLayoutMode : uint8_t {
	AutoTwoRowsBalanced = 0,
	AutoWrap = 1,
	FixedRows = 2
};

enum class MultiRowLabelFitMode : uint8_t {
	SmartAdaptive = 0,
	FullOnly = 1,
	ShortOnly = 2
};

struct MultiRowTabItem {
	int id{0};
	size_t preferred_row{0};
	std::string full_label{};
	std::string medium_label{};
	std::string short_label{};
	std::string compact_label{};
	std::string tooltip{};
	bool enabled{true};
	std::function<void()> content_callback{};

	MultiRowTabItem& set_medium(std::string m) {
		medium_label = std::move(m);
		return *this;
	}

	MultiRowTabItem& set_short(std::string s) {
		short_label = std::move(s);
		return *this;
	}

	MultiRowTabItem& set_compact(std::string c) {
		compact_label = std::move(c);
		return *this;
	}

	MultiRowTabItem& set_tooltip(std::string t) {
		tooltip = std::move(t);
		return *this;
	}

	MultiRowTabItem& set_row(size_t r) {
		preferred_row = r;
		return *this;
	}

	MultiRowTabItem& set_enabled(bool e) {
		enabled = e;
		return *this;
	}

	MultiRowTabItem& set_content(std::function<void()> cb) {
		content_callback = std::move(cb);
		return *this;
	}

	[[nodiscard]] bool has_content() const noexcept {
		return static_cast<bool>(content_callback);
	}

	void render_content() const {
		if (content_callback) {
			content_callback();
		}
	}

	void ensure_label_tiers() {
		if (medium_label.empty()) {
			medium_label = generate_medium_label(full_label);
		}
		if (short_label.empty()) {
			short_label = generate_short_label(medium_label);
		}
		if (compact_label.empty()) {
			compact_label = generate_compact_label(short_label);
		}
	}

	[[nodiscard]] const std::string& label_for_tier(int tier) const noexcept {
		switch (tier) {
			case 0: return full_label;
			case 1: return medium_label;
			case 2: return short_label;
			case 3: return compact_label;
			default: return compact_label;
		}
	}

private:
	static std::string generate_medium_label(const std::string& full) {
		if (full.size() <= 12) {
			return full;
		}
		std::string result;
		size_t start = 0;
		while (start < full.size()) {
			while (start < full.size() && (full[start] == ' ' || full[start] == '\t')) {
				result += full[start++];
			}
			if (start >= full.size()) break;
			size_t end = start;
			while (end < full.size() && full[end] != ' ' && full[end] != '\t') {
				++end;
			}
			result += shorten_word(full.substr(start, end - start));
			start = end;
		}
		return result;
	}

	static std::string shorten_word(const std::string& word) {
		static const std::pair<const char*, const char*> dictionary[] = {
			{"Configuration", "Config."},
			{"Settings", "Set."},
			{"Parameters", "Params"},
			{"Parameter", "Param"},
			{"Management", "Mgmt"},
			{"Diagnostics", "Diag."},
			{"Diagnostic", "Diag."},
			{"Statistics", "Stats"},
			{"Statistic", "Stat"},
			{"Integrators", "Integ."},
			{"Integrator", "Integ."},
			{"Execution", "Exec."},
			{"Environment", "Env."},
			{"Reference", "Ref."},
			{"Simulation", "Sim."},
			{"Controls", "Ctrl"},
			{"Control", "Ctrl"},
			{"Information", "Info"}
		};
		for (const auto& [long_w, short_w] : dictionary) {
			if (word == long_w) return short_w;
			if (word == std::string(long_w) + ",") return std::string(short_w) + ",";
			if (word == std::string(long_w) + ":") return std::string(short_w) + ":";
		}
		if (word.size() > 9) {
			return word.substr(0, 7) + ".";
		}
		return word;
	}

	static std::string generate_short_label(const std::string& text) {
		const size_t amp = text.find('&');
		if (amp != std::string::npos) {
			const size_t end = text.find_last_not_of(" \t", amp > 0 ? amp - 1 : 0);
			if (end != std::string::npos) {
				return text.substr(0, end + 1);
			}
		}
		const size_t slash = text.find('/');
		if (slash != std::string::npos) {
			const size_t end = text.find_last_not_of(" \t", slash > 0 ? slash - 1 : 0);
			if (end != std::string::npos) {
				return text.substr(0, end + 1);
			}
		}
		const size_t paren = text.find('(');
		if (paren != std::string::npos) {
			const size_t end = text.find_last_not_of(" \t", paren > 0 ? paren - 1 : 0);
			if (end != std::string::npos) {
				return text.substr(0, end + 1);
			}
		}
		const size_t space = text.find(' ');
		if (space != std::string::npos && space > 2) {
			return text.substr(0, space);
		}
		return text;
	}

	static std::string generate_compact_label(const std::string& text) {
		std::string acronym;
		bool in_word = false;
		size_t word_start = 0;
		for (size_t i = 0; i <= text.size(); ++i) {
			const bool is_alnum = (i < text.size() && std::isalnum(static_cast<unsigned char>(text[i])));
			if (is_alnum && !in_word) {
				in_word = true;
				word_start = i;
			} else if (!is_alnum && in_word) {
				in_word = false;
				const std::string word = text.substr(word_start, i - word_start);
				if (word != "and" && word != "for" && word != "the" && word != "with" && word != "of") {
					acronym += static_cast<char>(std::toupper(static_cast<unsigned char>(word[0])));
				}
			}
		}
		if (acronym.size() >= 2 && acronym.size() <= 5) {
			return acronym;
		}
		std::string first_letters;
		for (char c : text) {
			if (std::isalnum(static_cast<unsigned char>(c)) && first_letters.size() < 4) {
				first_letters += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
			}
		}
		return first_letters.empty() ? text : first_letters;
	}
};

struct MultiRowTabBarConfig {
	MultiRowLayoutMode layout_mode{MultiRowLayoutMode::AutoTwoRowsBalanced};
	MultiRowLabelFitMode fit_mode{MultiRowLabelFitMode::SmartAdaptive};
	size_t target_row_count{2};
	bool auto_compress{true};
	bool equalize_widths{false};
	bool justify_rows{true};
	bool allow_reorder{true};
	bool swap_active_row_to_bottom{false};
	float row_spacing{3.0f};
	float active_elevation{2.0f};
	float tab_height_override{0.0f};
	float min_tab_width{32.0f};
	ImGuiHoveredFlags tooltip_delay_flags{ImGuiHoveredFlags_DelayShort};
};

class MultiRowTabBar {
public:
	MultiRowTabBar() noexcept = default;

	void clear() noexcept {
		tabs_.clear();
		initial_order_.clear();
		tab_order_.clear();
	}

	[[nodiscard]] int allocate_next_id() const noexcept {
		int max_id = -1;
		for (const auto& tab : tabs_) {
			if (tab.id > max_id) max_id = tab.id;
		}
		return max_id + 1;
	}

	MultiRowTabItem& add_tab(std::string full_label, std::string tooltip = "") {
		return add_tab(allocate_next_id(), std::move(full_label), std::move(tooltip));
	}

	MultiRowTabItem& add_tab(std::string full_label, std::function<void()> content_cb, std::string tooltip = "") {
		auto& item = add_tab(allocate_next_id(), std::move(full_label), std::move(tooltip));
		item.set_content(std::move(content_cb));
		return item;
	}

	void render_active_content(int active_tab_id) const {
		const auto* tab = find_tab(active_tab_id);
		if (tab != nullptr && tab->has_content()) {
			tab->render_content();
		}
	}

	MultiRowTabItem& add_tab(int id, std::string full_label, std::string tooltip = "") {
		MultiRowTabItem item;
		item.id = id;
		item.full_label = std::move(full_label);
		item.tooltip = std::move(tooltip);
		item.ensure_label_tiers();
		tabs_.push_back(std::move(item));
		initial_order_.push_back(id);
		tab_order_.push_back(id);
		return tabs_.back();
	}

	MultiRowTabItem& add_tab(size_t row_index, int id, std::string full_label, std::string short_label = "", std::string tooltip = "") {
		MultiRowTabItem item;
		item.id = id;
		item.preferred_row = row_index;
		item.full_label = std::move(full_label);
		if (!short_label.empty()) {
			item.short_label = std::move(short_label);
		}
		item.tooltip = std::move(tooltip);
		item.ensure_label_tiers();
		tabs_.push_back(std::move(item));
		initial_order_.push_back(id);
		tab_order_.push_back(id);
		return tabs_.back();
	}

	[[nodiscard]] size_t row_count() const noexcept {
		return last_row_count_;
	}

	[[nodiscard]] MultiRowTabBarConfig& config() noexcept {
		return config_;
	}

	[[nodiscard]] const MultiRowTabBarConfig& config() const noexcept {
		return config_;
	}

	[[nodiscard]] const std::vector<int>& tab_order() const noexcept {
		return tab_order_;
	}

	void set_tab_order(const std::vector<int>& order) noexcept {
		std::vector<int> filtered;
		filtered.reserve(order.size());
		for (int id : order) {
			if (find_tab(id) != nullptr && std::find(filtered.begin(), filtered.end(), id) == filtered.end()) {
				filtered.push_back(id);
			}
		}
		for (const auto& tab : tabs_) {
			if (std::find(filtered.begin(), filtered.end(), tab.id) == filtered.end()) {
				filtered.push_back(tab.id);
			}
		}
		tab_order_ = std::move(filtered);
	}

	void reset_tab_order() noexcept {
		tab_order_ = initial_order_;
	}

	void move_tab(int source_id, int target_id) noexcept {
		auto src_it = std::find(tab_order_.begin(), tab_order_.end(), source_id);
		auto tgt_it = std::find(tab_order_.begin(), tab_order_.end(), target_id);
		if (src_it != tab_order_.end() && tgt_it != tab_order_.end() && src_it != tgt_it) {
			tab_order_.erase(src_it);
			tgt_it = std::find(tab_order_.begin(), tab_order_.end(), target_id);
			tab_order_.insert(tgt_it, source_id);
		}
	}

	void move_earlier(int id) noexcept {
		auto it = std::find(tab_order_.begin(), tab_order_.end(), id);
		if (it != tab_order_.end() && it != tab_order_.begin()) {
			std::iter_swap(it, it - 1);
		}
	}

	void move_later(int id) noexcept {
		auto it = std::find(tab_order_.begin(), tab_order_.end(), id);
		if (it != tab_order_.end() && it + 1 != tab_order_.end()) {
			std::iter_swap(it, it + 1);
		}
	}

	void move_to_start(int id) noexcept {
		auto it = std::find(tab_order_.begin(), tab_order_.end(), id);
		if (it != tab_order_.end() && it != tab_order_.begin()) {
			tab_order_.erase(it);
			tab_order_.insert(tab_order_.begin(), id);
		}
	}

	void move_to_end(int id) noexcept {
		auto it = std::find(tab_order_.begin(), tab_order_.end(), id);
		if (it != tab_order_.end() && it + 1 != tab_order_.end()) {
			tab_order_.erase(it);
			tab_order_.push_back(id);
		}
	}

	bool render(int& active_tab_id) {
		if (tabs_.empty()) {
			return false;
		}

		synchronize_tab_order();

		if (find_tab(active_tab_id) == nullptr) {
			active_tab_id = tab_order_.front();
		}

		const ImGuiStyle& style = ImGui::GetStyle();
		const float font_size = ImGui::GetFontSize();
		const float base_tab_h = (config_.tab_height_override > 0.0f)
			? config_.tab_height_override
			: (font_size + style.FramePadding.y * 2.0f);
		const float border_size = std::max(style.TabBorderSize, 1.0f);
		const float rounding = style.TabRounding;
		const float inner_spacing = style.ItemInnerSpacing.x;
		const float avail_w = std::max(ImGui::GetContentRegionAvail().x, 60.0f);
		const bool window_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

		const ImU32 col_bg_active = ImGui::GetColorU32(window_focused ? ImGuiCol_TabActive : ImGuiCol_TabUnfocusedActive);
		const ImU32 col_bg_idle = ImGui::GetColorU32(window_focused ? ImGuiCol_Tab : ImGuiCol_TabUnfocused);
		const ImU32 col_bg_hovered = ImGui::GetColorU32(ImGuiCol_TabHovered);
		const ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
		const ImU32 col_text_active = ImGui::GetColorU32(ImGuiCol_Text);
		const ImU32 col_text_dim = ImGui::ColorConvertFloat4ToU32(ImVec4(
			style.Colors[ImGuiCol_Text].x,
			style.Colors[ImGuiCol_Text].y,
			style.Colors[ImGuiCol_Text].z,
			style.Colors[ImGuiCol_Text].w * 0.76f
		));
		const ImU32 col_accent = ImGui::GetColorU32(ImGuiCol_SliderGrab);

		std::vector<std::vector<int>> logical_rows;
		int active_tier = 0;
		partition_tabs(avail_w, inner_spacing, style.FramePadding.x, logical_rows, active_tier);
		last_row_count_ = logical_rows.size();

		size_t active_row_idx = 0;
		for (size_t r = 0; r < logical_rows.size(); ++r) {
			if (std::find(logical_rows[r].begin(), logical_rows[r].end(), active_tab_id) != logical_rows[r].end()) {
				active_row_idx = r;
				break;
			}
		}

		std::vector<size_t> visual_row_order;
		visual_row_order.reserve(logical_rows.size());
		if (config_.swap_active_row_to_bottom && logical_rows.size() > 1U) {
			for (size_t r = 0; r < logical_rows.size(); ++r) {
				if (r != active_row_idx) visual_row_order.push_back(r);
			}
			visual_row_order.push_back(active_row_idx);
		} else {
			for (size_t r = 0; r < logical_rows.size(); ++r) {
				visual_row_order.push_back(r);
			}
		}

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		bool changed = false;

		struct RenderTabInfo {
			int id{0};
			ImVec2 rect_min{};
			ImVec2 rect_max{};
			std::string display_label{};
			std::string full_label{};
			std::string tooltip{};
			ImVec2 text_size{};
			bool is_active{false};
			bool is_hovered{false};
			bool is_bottom_row{false};
			bool is_drop_target{false};
			bool drop_on_left{true};
		};

		std::vector<RenderTabInfo> tab_render_list;
		std::vector<float> row_baseline_y_list;
		const ImVec2 start_cursor = ImGui::GetCursorScreenPos();
		const float row_start_x = start_cursor.x;

		for (size_t v = 0; v < visual_row_order.size(); ++v) {
			const size_t r = visual_row_order[v];
			const auto& row_ids = logical_rows[r];
			if (row_ids.empty()) continue;

			const bool is_bottom_row = (v == visual_row_order.size() - 1U);
			const size_t count = row_ids.size();
			const float total_gaps = inner_spacing * static_cast<float>(count - 1U);
			const float remaining_w = std::max(avail_w - total_gaps, config_.min_tab_width * static_cast<float>(count));

			std::vector<float> nat_widths(count);
			std::vector<std::string> labels(count);
			std::vector<ImVec2> text_sizes(count);
			float total_nat = 0.0f;

			for (size_t i = 0; i < count; ++i) {
				const auto* tab = find_tab(row_ids[i]);
				labels[i] = tab->label_for_tier(active_tier);
				text_sizes[i] = ImGui::CalcTextSize(labels[i].c_str(), nullptr, true);
				nat_widths[i] = std::max(text_sizes[i].x + style.FramePadding.x * 2.0f, config_.min_tab_width);
				total_nat += nat_widths[i];
			}

			std::vector<float> tab_widths(count);
			if (config_.equalize_widths && remaining_w > 10.0f) {
				const float uniform_w = remaining_w / static_cast<float>(count);
				for (size_t i = 0; i < count; ++i) tab_widths[i] = uniform_w;
			} else if (config_.justify_rows && remaining_w > 10.0f) {
				if (remaining_w >= total_nat) {
					const float extra_per_tab = (remaining_w - total_nat) / static_cast<float>(count);
					for (size_t i = 0; i < count; ++i) tab_widths[i] = nat_widths[i] + extra_per_tab;
				} else {
					for (size_t i = 0; i < count; ++i) {
						tab_widths[i] = std::max(remaining_w * (nat_widths[i] / total_nat), config_.min_tab_width);
					}
				}
			} else {
				for (size_t i = 0; i < count; ++i) {
					tab_widths[i] = std::min(nat_widths[i], remaining_w);
				}
			}

			const ImVec2 row_cursor = ImGui::GetCursorScreenPos();
			row_baseline_y_list.push_back(row_cursor.y + base_tab_h);

			float current_x = row_cursor.x;
			for (size_t i = 0; i < count; ++i) {
				const auto* tab = find_tab(row_ids[i]);
				const float w = tab_widths[i];
				ImGui::SetCursorScreenPos(ImVec2(current_x, row_cursor.y));

				char btn_id[64];
				std::snprintf(btn_id, sizeof(btn_id), "##MRT_Tab_%d", tab->id);
				const bool clicked = ImGui::InvisibleButton(btn_id, ImVec2(w, base_tab_h));
				const bool hovered = ImGui::IsItemHovered();
				const bool is_active = (tab->id == active_tab_id);

				if (clicked && tab->enabled) {
					active_tab_id = tab->id;
					changed = true;
				}

				if (config_.allow_reorder) {
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
						ImGui::SetDragDropPayload("MRT_TAB_REORDER", &tab->id, sizeof(int));
						ImGui::TextUnformatted(tab->full_label.c_str());
						ImGui::EndDragDropSource();
					}
					if (ImGui::BeginDragDropTarget()) {
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MRT_TAB_REORDER")) {
							int dragged_id = *reinterpret_cast<const int*>(payload->Data);
							if (dragged_id != tab->id) {
								move_tab(dragged_id, tab->id);
								changed = true;
							}
						}
						ImGui::EndDragDropTarget();
					}
				}

				if (config_.allow_reorder && ImGui::BeginPopupContextItem()) {
					ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", tab->full_label.c_str());
					ImGui::Separator();
					if (ImGui::MenuItem("Move Left")) {
						move_earlier(tab->id);
						changed = true;
					}
					if (ImGui::MenuItem("Move Right")) {
						move_later(tab->id);
						changed = true;
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Move to First")) {
						move_to_start(tab->id);
						changed = true;
					}
					if (ImGui::MenuItem("Move to Last")) {
						move_to_end(tab->id);
						changed = true;
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Reset Default Tab Order")) {
						reset_tab_order();
						changed = true;
					}
					ImGui::EndPopup();
				}

				const bool tooltip_hovered = ImGui::IsItemHovered(config_.tooltip_delay_flags);
				const ImGuiPayload* active_payload = ImGui::GetDragDropPayload();
				if (tooltip_hovered && !ImGui::IsPopupOpen(btn_id) && active_payload == nullptr) {
					ImGui::BeginTooltip();
					ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", tab->full_label.c_str());
					if (!tab->tooltip.empty()) {
						ImGui::Spacing();
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
						ImGui::TextColored(ImVec4(0.75f, 0.82f, 0.9f, 1.0f), "%s", tab->tooltip.c_str());
						ImGui::PopTextWrapPos();
					}
					if (config_.allow_reorder) {
						ImGui::Spacing();
						ImGui::TextDisabled("Right-click or drag to reorder tabs");
					}
					ImGui::EndTooltip();
				}

				std::string final_label = labels[i];
				if (text_sizes[i].x > w - 4.0f) {
					final_label = truncate_label(labels[i], w - 4.0f);
					text_sizes[i] = ImGui::CalcTextSize(final_label.c_str(), nullptr, true);
				}

				RenderTabInfo info;
				info.id = tab->id;
				info.rect_min = ImVec2(current_x, row_cursor.y);
				info.rect_max = ImVec2(current_x + w, row_cursor.y + base_tab_h);
				info.display_label = std::move(final_label);
				info.full_label = tab->full_label;
				info.tooltip = tab->tooltip;
				info.text_size = text_sizes[i];
				info.is_active = is_active;
				info.is_hovered = hovered;
				info.is_bottom_row = is_bottom_row;

				if (config_.allow_reorder && active_payload != nullptr && active_payload->IsDataType("MRT_TAB_REORDER") && hovered) {
					info.is_drop_target = true;
					info.drop_on_left = (ImGui::GetMousePos().x < (info.rect_min.x + info.rect_max.x) * 0.5f);
				}

				tab_render_list.push_back(std::move(info));
				current_x += w + inner_spacing;
			}

			if (v + 1U < visual_row_order.size()) {
				ImGui::SetCursorScreenPos(ImVec2(row_start_x, row_cursor.y + base_tab_h + config_.row_spacing));
			}
		}

		for (size_t v = 0; v < visual_row_order.size(); ++v) {
			const float baseline_y = row_baseline_y_list[v];
			const bool is_bottom = (v == visual_row_order.size() - 1U);
			draw_list->AddLine(
				ImVec2(row_start_x, baseline_y),
				ImVec2(row_start_x + avail_w, baseline_y),
				is_bottom ? col_bg_active : col_border,
				border_size
			);
		}

		for (const auto& tab : tab_render_list) {
			if (tab.is_active) continue;
			draw_list->AddRectFilled(tab.rect_min, tab.rect_max, tab.is_hovered ? col_bg_hovered : col_bg_idle, rounding, IMGUI_TAB_ROUND_CORNERS);
			draw_list->AddRect(tab.rect_min, tab.rect_max, col_border, rounding, IMGUI_TAB_ROUND_CORNERS, border_size);
			const ImVec2 text_pos(
				tab.rect_min.x + (tab.rect_max.x - tab.rect_min.x - tab.text_size.x) * 0.5f,
				tab.rect_min.y + (base_tab_h - tab.text_size.y) * 0.5f
			);
			draw_list->AddText(text_pos, tab.is_hovered ? col_text_active : col_text_dim, tab.display_label.c_str());
			if (tab.is_drop_target) {
				const float mx = tab.drop_on_left ? tab.rect_min.x : tab.rect_max.x;
				draw_list->AddLine(ImVec2(mx, tab.rect_min.y - 1.0f), ImVec2(mx, tab.rect_max.y + 1.0f), col_accent, 3.0f);
			}
		}

		for (const auto& tab : tab_render_list) {
			if (!tab.is_active) continue;

			const ImVec2 elevated_min(tab.rect_min.x, tab.rect_min.y - config_.active_elevation);
			const ImVec2 elevated_max(tab.rect_max.x, tab.rect_max.y + (tab.is_bottom_row ? 1.5f : 1.0f));

			draw_list->AddRectFilled(elevated_min, elevated_max, col_bg_active, rounding, IMGUI_TAB_ROUND_CORNERS);
			draw_list->AddRectFilled(elevated_min, ImVec2(elevated_max.x, elevated_min.y + 2.5f), col_accent, rounding, IMGUI_TAB_ROUND_CORNERS);
			draw_list->AddRect(elevated_min, elevated_max, col_border, rounding, IMGUI_TAB_ROUND_CORNERS, border_size);

			const ImVec2 text_pos(
				elevated_min.x + (elevated_max.x - elevated_min.x - tab.text_size.x) * 0.5f,
				elevated_min.y + (base_tab_h - tab.text_size.y) * 0.5f
			);
			draw_list->AddText(text_pos, col_text_active, tab.display_label.c_str());
			if (tab.is_drop_target) {
				const float mx = tab.drop_on_left ? elevated_min.x : elevated_max.x;
				draw_list->AddLine(ImVec2(mx, elevated_min.y - 1.0f), ImVec2(mx, elevated_max.y + 1.0f), col_accent, 3.0f);
			}
		}

		const float final_y = row_baseline_y_list.back() + style.ItemSpacing.y;
		ImGui::SetCursorScreenPos(ImVec2(row_start_x, final_y));
		ImGui::Dummy(ImVec2(avail_w, 0.0f));

		return changed;
	}

private:
	std::vector<MultiRowTabItem> tabs_{};
	std::vector<int> initial_order_{};
	std::vector<int> tab_order_{};
	MultiRowTabBarConfig config_{};
	size_t last_row_count_{2};

	[[nodiscard]] const MultiRowTabItem* find_tab(int id) const noexcept {
		for (const auto& tab : tabs_) {
			if (tab.id == id) return &tab;
		}
		return nullptr;
	}

	[[nodiscard]] MultiRowTabItem* find_tab(int id) noexcept {
		for (auto& tab : tabs_) {
			if (tab.id == id) return &tab;
		}
		return nullptr;
	}

	void synchronize_tab_order() noexcept {
		if (tab_order_.size() != tabs_.size()) {
			std::vector<int> valid;
			valid.reserve(tabs_.size());
			for (int id : tab_order_) {
				if (find_tab(id) != nullptr && std::find(valid.begin(), valid.end(), id) == valid.end()) {
					valid.push_back(id);
				}
			}
			for (const auto& tab : tabs_) {
				if (std::find(valid.begin(), valid.end(), tab.id) == valid.end()) {
					valid.push_back(tab.id);
				}
			}
			tab_order_ = std::move(valid);
		}
	}

	void partition_tabs(float avail_w, float inner_spacing, float pad_x, std::vector<std::vector<int>>& out_rows, int& out_tier) const {
		out_rows.clear();
		const size_t n = tab_order_.size();
		if (n == 0) return;

		if (config_.layout_mode == MultiRowLayoutMode::FixedRows) {
			size_t max_row = 1;
			for (int id : tab_order_) {
				const auto* t = find_tab(id);
				if (t != nullptr && t->preferred_row > max_row) max_row = t->preferred_row;
			}
			out_rows.resize(max_row + 1U);
			for (int id : tab_order_) {
				const auto* t = find_tab(id);
				if (t != nullptr) {
					out_rows[t->preferred_row].push_back(id);
				}
			}
			out_tier = select_best_tier_for_rows(out_rows, avail_w, inner_spacing, pad_x);
			return;
		}

		if (config_.layout_mode == MultiRowLayoutMode::AutoWrap) {
			out_tier = 0;
			float current_row_w = 0.0f;
			std::vector<int> current_row;
			for (size_t i = 0; i < n; ++i) {
				const auto* t = find_tab(tab_order_[i]);
				const float tw = ImGui::CalcTextSize(t->label_for_tier(out_tier).c_str(), nullptr, true).x + pad_x * 2.0f;
				const float needed = current_row.empty() ? tw : (current_row_w + inner_spacing + tw);
				if (!current_row.empty() && needed > avail_w) {
					out_rows.push_back(std::move(current_row));
					current_row.clear();
					current_row.push_back(tab_order_[i]);
					current_row_w = tw;
				} else {
					current_row.push_back(tab_order_[i]);
					current_row_w = needed;
				}
			}
			if (!current_row.empty()) {
				out_rows.push_back(std::move(current_row));
			}
			return;
		}

		if (n == 1) {
			out_rows.push_back(tab_order_);
			out_tier = 0;
			return;
		}

		const int max_tier = (config_.fit_mode == MultiRowLabelFitMode::FullOnly) ? 0
			: (config_.fit_mode == MultiRowLabelFitMode::ShortOnly) ? 2 : 3;

		int best_tier = max_tier;
		size_t best_split = n / 2;
		float best_diff = 1.0e10f;
		bool found_fit = false;

		for (int tier = 0; tier <= max_tier; ++tier) {
			std::vector<float> widths(n);
			for (size_t i = 0; i < n; ++i) {
				const auto* t = find_tab(tab_order_[i]);
				widths[i] = std::max(ImGui::CalcTextSize(t->label_for_tier(tier).c_str(), nullptr, true).x + pad_x * 2.0f, config_.min_tab_width);
			}

			for (size_t k = 1; k < n; ++k) {
				float w0 = 0.0f;
				for (size_t i = 0; i < k; ++i) w0 += widths[i];
				w0 += inner_spacing * static_cast<float>(k - 1);

				float w1 = 0.0f;
				for (size_t i = k; i < n; ++i) w1 += widths[i];
				w1 += inner_spacing * static_cast<float>(n - k - 1);

				if (w0 <= avail_w && w1 <= avail_w) {
					const float diff = std::abs(w0 - w1);
					if (!found_fit || diff < best_diff) {
						found_fit = true;
						best_tier = tier;
						best_split = k;
						best_diff = diff;
					}
				}
			}

			if (found_fit) {
				break;
			}
		}

		if (!found_fit) {
			best_tier = max_tier;
			std::vector<float> widths(n);
			for (size_t i = 0; i < n; ++i) {
				const auto* t = find_tab(tab_order_[i]);
				widths[i] = std::max(ImGui::CalcTextSize(t->label_for_tier(best_tier).c_str(), nullptr, true).x + pad_x * 2.0f, config_.min_tab_width);
			}

			float min_max_w = 1.0e10f;
			for (size_t k = 1; k < n; ++k) {
				float w0 = 0.0f;
				for (size_t i = 0; i < k; ++i) w0 += widths[i];
				w0 += inner_spacing * static_cast<float>(k - 1);

				float w1 = 0.0f;
				for (size_t i = k; i < n; ++i) w1 += widths[i];
				w1 += inner_spacing * static_cast<float>(n - k - 1);

				const float max_w = std::max(w0, w1);
				if (max_w < min_max_w) {
					min_max_w = max_w;
					best_split = k;
				}
			}
		}

		out_tier = best_tier;
		out_rows.resize(2);
		out_rows[0].assign(tab_order_.begin(), tab_order_.begin() + best_split);
		out_rows[1].assign(tab_order_.begin() + best_split, tab_order_.end());
	}

	[[nodiscard]] int select_best_tier_for_rows(const std::vector<std::vector<int>>& rows, float avail_w, float inner_spacing, float pad_x) const {
		const int max_tier = (config_.fit_mode == MultiRowLabelFitMode::FullOnly) ? 0
			: (config_.fit_mode == MultiRowLabelFitMode::ShortOnly) ? 2 : 3;

		for (int tier = 0; tier <= max_tier; ++tier) {
			bool all_fit = true;
			for (const auto& row_ids : rows) {
				if (row_ids.empty()) continue;
				float total_w = 0.0f;
				for (int id : row_ids) {
					const auto* t = find_tab(id);
					total_w += std::max(ImGui::CalcTextSize(t->label_for_tier(tier).c_str(), nullptr, true).x + pad_x * 2.0f, config_.min_tab_width);
				}
				total_w += inner_spacing * static_cast<float>(row_ids.size() - 1);
				if (total_w > avail_w) {
					all_fit = false;
					break;
				}
			}
			if (all_fit) {
				return tier;
			}
		}
		return max_tier;
	}

	static std::string truncate_label(const std::string& text, float max_w) {
		if (max_w <= 12.0f) {
			return ".";
		}
		const float full_w = ImGui::CalcTextSize(text.c_str(), nullptr, true).x;
		if (full_w <= max_w) {
			return text;
		}
		const float ellipsis_w = ImGui::CalcTextSize("..", nullptr, true).x;
		if (ellipsis_w >= max_w) {
			return "..";
		}
		const float target_w = max_w - ellipsis_w;
		std::string result;
		for (char c : text) {
			std::string next = result + c;
			if (ImGui::CalcTextSize(next.c_str(), nullptr, true).x > target_w) {
				break;
			}
			result = next;
		}
		return result.empty() ? text.substr(0, 1) + ".." : result + "..";
	}
};

}
