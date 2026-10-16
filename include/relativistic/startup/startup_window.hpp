#pragma once

#include "relativistic/startup/application_info.hpp"
#include "relativistic/startup/application_resources.h"
#include "relativistic/startup/platform_bootstrap.hpp"
#include "relativistic/startup/startup_progress.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <future>
#include <string>
#include <thread>

#if defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwmapi.h>
#include <shellapi.h>

namespace Relativistic::Startup {

namespace StartupPalette {

[[nodiscard]] constexpr COLORREF color(unsigned red, unsigned green, unsigned blue) noexcept {
	return static_cast<COLORREF>(red | (green << 8U) | (blue << 16U));
}

inline constexpr COLORREF kBackgroundTop = color(21, 26, 40);
inline constexpr COLORREF kBackgroundBottom = color(10, 12, 19);
inline constexpr COLORREF kTitleBar = color(14, 17, 26);
inline constexpr COLORREF kSeparator = color(36, 42, 58);
inline constexpr COLORREF kBorder = color(46, 53, 72);
inline constexpr COLORREF kTextPrimary = color(232, 236, 244);
inline constexpr COLORREF kTextSecondary = color(141, 150, 170);
inline constexpr COLORREF kTextDisabled = color(88, 96, 114);
inline constexpr COLORREF kTextWarning = color(240, 178, 90);
inline constexpr COLORREF kTextError = color(255, 107, 107);
inline constexpr COLORREF kControlHover = color(42, 48, 66);
inline constexpr COLORREF kControlPressed = color(58, 66, 90);
inline constexpr COLORREF kCloseHover = color(232, 17, 35);
inline constexpr COLORREF kClosePressed = color(164, 12, 24);
inline constexpr COLORREF kBarTrack = color(35, 40, 56);
inline constexpr COLORREF kAccentStart = color(76, 155, 255);
inline constexpr COLORREF kAccentEnd = color(142, 107, 255);
inline constexpr COLORREF kButtonFill = color(22, 27, 40);
inline constexpr COLORREF kButtonHover = color(30, 37, 56);
inline constexpr COLORREF kButtonPressed = color(38, 46, 70);

}

class StartupWindow {
private:
	enum class Control : int {
		None = 0,
		Minimize = 1,
		Close = 2,
		Repository = 3
	};

	struct Surface {
		HDC device{nullptr};
		HBITMAP bitmap{nullptr};
		HGDIOBJ previous{nullptr};

		void create(HDC reference, int width, int height) noexcept {
			device = CreateCompatibleDC(reference);
			bitmap = CreateCompatibleBitmap(reference, width, height);
			previous = (device != nullptr && bitmap != nullptr) ? SelectObject(device, bitmap) : nullptr;
		}

		void destroy() noexcept {
			if (device != nullptr) {
				if (previous != nullptr) {
					SelectObject(device, previous);
				}
				DeleteDC(device);
			}
			if (bitmap != nullptr) {
				DeleteObject(bitmap);
			}
			device = nullptr;
			bitmap = nullptr;
			previous = nullptr;
		}

		[[nodiscard]] bool valid() const noexcept {
			return device != nullptr && bitmap != nullptr && previous != nullptr;
		}
	};

	struct Fonts {
		HFONT brand{nullptr};
		HFONT title{nullptr};
		HFONT subtitle{nullptr};
		HFONT stage{nullptr};
		HFONT detail{nullptr};
		HFONT caption{nullptr};
		HFONT caption_bold{nullptr};
		HFONT button{nullptr};
	};

	static constexpr int kLogicalWidth = 620;
	static constexpr int kLogicalHeight = 372;
	static constexpr UINT kMessageUpdate = WM_APP + 1U;
	static constexpr UINT kMessageClose = WM_APP + 2U;
	static constexpr UINT_PTR kEaseTimerId = 1;
	static constexpr UINT kEaseIntervalMs = 15;
	static constexpr const wchar_t* kWindowClassName = L"RelativisticEngineStartupWindow";

	StartupProgress* progress_{nullptr};
	std::thread thread_{};
	std::promise<bool> ready_{};
	std::atomic<HWND> hwnd_{nullptr};
	std::atomic<bool> update_pending_{false};
	HINSTANCE instance_{nullptr};

	UINT dpi_{96};
	int client_width_{kLogicalWidth};
	int client_height_{kLogicalHeight};
	int title_height_{44};
	int icon_size_{32};
	int footer_line_y_{316};
	RECT icon_rect_{};
	RECT brand_rect_{};
	RECT minimize_rect_{};
	RECT close_rect_{};
	RECT hero_title_rect_{};
	RECT hero_subtitle_rect_{};
	RECT hero_version_rect_{};
	RECT stage_rect_{};
	RECT percent_rect_{};
	RECT detail_rect_{};
	RECT bar_rect_{};
	RECT developer_rect_{};
	RECT repository_rect_{};
	RECT ease_area_{};

	Fonts fonts_{};
	HICON icon_{nullptr};
	bool icon_owned_{false};
	Surface backdrop_{};
	Surface frame_{};

	Control hover_{Control::None};
	Control pressed_{Control::None};
	bool tracking_mouse_{false};
	bool ease_timer_active_{false};
	bool cancelling_{false};
	StartupPhase phase_{StartupPhase::Running};
	float displayed_fraction_{0.0f};
	float target_fraction_{0.0f};
	std::wstring stage_text_{};
	std::wstring detail_text_{};

	[[nodiscard]] int px(int logical) const noexcept {
		return MulDiv(logical, static_cast<int>(dpi_), 96);
	}

	[[nodiscard]] static RECT rect_from(int left, int top, int width, int height) noexcept {
		return RECT{left, top, left + width, top + height};
	}

	[[nodiscard]] static COLOR16 channel16(COLORREF value, unsigned shift) noexcept {
		return static_cast<COLOR16>(((value >> shift) & 0xFFU) << 8U);
	}

	[[nodiscard]] static COLORREF blend(COLORREF from, COLORREF to, float t) noexcept {
		const float clamped = std::clamp(t, 0.0f, 1.0f);
		const auto channel = [&](unsigned shift) noexcept {
			const float a = static_cast<float>((from >> shift) & 0xFFU);
			const float b = static_cast<float>((to >> shift) & 0xFFU);
			return static_cast<unsigned>(a + (b - a) * clamped + 0.5f);
		};
		return StartupPalette::color(channel(0), channel(8), channel(16));
	}

	static void fill_solid(HDC device, const RECT& area, COLORREF value) noexcept {
		SetBkColor(device, value);
		ExtTextOutW(device, 0, 0, ETO_OPAQUE, &area, nullptr, 0, nullptr);
	}

	static void fill_gradient(HDC device, const RECT& area, COLORREF from, COLORREF to, bool horizontal) noexcept {
		TRIVERTEX vertices[2]{};
		vertices[0].x = area.left;
		vertices[0].y = area.top;
		vertices[0].Red = channel16(from, 0);
		vertices[0].Green = channel16(from, 8);
		vertices[0].Blue = channel16(from, 16);
		vertices[1].x = area.right;
		vertices[1].y = area.bottom;
		vertices[1].Red = channel16(to, 0);
		vertices[1].Green = channel16(to, 8);
		vertices[1].Blue = channel16(to, 16);
		GRADIENT_RECT mesh{0, 1};
		GradientFill(device, vertices, 2, &mesh, 1, horizontal ? GRADIENT_FILL_RECT_H : GRADIENT_FILL_RECT_V);
	}

	static void draw_text(HDC device, const std::wstring& text, HFONT font, COLORREF value, const RECT& area, UINT flags) noexcept {
		if (text.empty()) {
			return;
		}
		const HGDIOBJ previous = SelectObject(device, font);
		SetBkMode(device, TRANSPARENT);
		SetTextColor(device, value);
		RECT target = area;
		DrawTextW(device, text.c_str(), static_cast<int>(text.size()), &target, flags);
		SelectObject(device, previous);
	}

	void fill_round_rect(HDC device, const RECT& area, int radius, COLORREF fill, COLORREF border) const noexcept {
		const HBRUSH brush = CreateSolidBrush(fill);
		const HPEN pen = CreatePen(PS_SOLID, 1, border);
		const HGDIOBJ previous_brush = SelectObject(device, brush);
		const HGDIOBJ previous_pen = SelectObject(device, pen);
		RoundRect(device, area.left, area.top, area.right, area.bottom, radius, radius);
		SelectObject(device, previous_pen);
		SelectObject(device, previous_brush);
		DeleteObject(pen);
		DeleteObject(brush);
	}

	[[nodiscard]] UINT query_window_dpi(HWND hwnd) const noexcept {
		using DpiFunction = UINT(WINAPI*)(HWND);
		const HMODULE user32 = GetModuleHandleW(L"user32.dll");
		if (user32 != nullptr) {
			const auto function = reinterpret_cast<DpiFunction>(reinterpret_cast<void*>(GetProcAddress(user32, "GetDpiForWindow")));
			if (function != nullptr) {
				const UINT value = function(hwnd);
				if (value > 0) {
					return value;
				}
			}
		}
		const HDC screen = GetDC(nullptr);
		const int fallback = (screen != nullptr) ? GetDeviceCaps(screen, LOGPIXELSX) : 96;
		if (screen != nullptr) {
			ReleaseDC(nullptr, screen);
		}
		return fallback > 0 ? static_cast<UINT>(fallback) : 96U;
	}

	[[nodiscard]] HICON load_shared_icon(int width, int height) const noexcept {
		HICON loaded = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(IDI_APPLICATION_ICON), IMAGE_ICON, width, height, LR_SHARED));
		if (loaded == nullptr) {
			loaded = LoadIconW(nullptr, reinterpret_cast<LPCWSTR>(IDI_APPLICATION));
		}
		return loaded;
	}

	[[nodiscard]] HFONT create_font(int logical_height, int weight) const noexcept {
		return CreateFontW(-px(logical_height), 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
	}

	void destroy_fonts() noexcept {
		for (HFONT* font : {&fonts_.brand, &fonts_.title, &fonts_.subtitle, &fonts_.stage, &fonts_.detail, &fonts_.caption, &fonts_.caption_bold, &fonts_.button}) {
			if (*font != nullptr) {
				DeleteObject(*font);
				*font = nullptr;
			}
		}
	}

	void create_fonts() noexcept {
		fonts_.brand = create_font(14, FW_SEMIBOLD);
		fonts_.title = create_font(34, FW_SEMIBOLD);
		fonts_.subtitle = create_font(15, FW_NORMAL);
		fonts_.stage = create_font(15, FW_SEMIBOLD);
		fonts_.detail = create_font(13, FW_NORMAL);
		fonts_.caption = create_font(12, FW_NORMAL);
		fonts_.caption_bold = create_font(12, FW_BOLD);
		fonts_.button = create_font(13, FW_NORMAL);
	}

	void release_icon() noexcept {
		if (icon_ != nullptr && icon_owned_) {
			DestroyIcon(icon_);
		}
		icon_ = nullptr;
		icon_owned_ = false;
	}

	void release_surfaces() noexcept {
		backdrop_.destroy();
		frame_.destroy();
	}

	void rebuild_layout() noexcept {
		client_width_ = px(kLogicalWidth);
		client_height_ = px(kLogicalHeight);
		title_height_ = px(44);
		icon_size_ = px(32);
		const int margin = px(40);
		const int content_width = client_width_ - 2 * margin;
		const int control_width = px(46);
		const int percent_width = px(64);

		icon_rect_ = rect_from(px(14), (title_height_ - icon_size_) / 2, icon_size_, icon_size_);
		brand_rect_ = rect_from(icon_rect_.right + px(12), 0, px(320), title_height_);
		close_rect_ = rect_from(client_width_ - control_width, 0, control_width, title_height_);
		minimize_rect_ = rect_from(client_width_ - 2 * control_width, 0, control_width, title_height_);

		hero_title_rect_ = rect_from(margin, px(72), content_width, px(44));
		hero_subtitle_rect_ = rect_from(margin, px(120), content_width, px(24));
		hero_version_rect_ = rect_from(margin, px(148), content_width, px(20));

		stage_rect_ = rect_from(margin, px(220), content_width - percent_width, px(24));
		percent_rect_ = rect_from(client_width_ - margin - percent_width, px(220), percent_width, px(24));
		detail_rect_ = rect_from(margin, px(246), content_width, px(22));
		bar_rect_ = rect_from(margin, px(280), content_width, px(6));
		ease_area_ = rect_from(margin, percent_rect_.top, content_width, bar_rect_.bottom + px(2) - percent_rect_.top);

		footer_line_y_ = px(316);
		developer_rect_ = rect_from(margin, footer_line_y_ + px(1), px(280), client_height_ - footer_line_y_ - px(1));
		const int button_width = px(176);
		const int button_height = px(30);
		repository_rect_ = rect_from(client_width_ - margin - button_width, footer_line_y_ + (client_height_ - footer_line_y_ - button_height) / 2, button_width, button_height);
	}

	void apply_dpi_resources() noexcept {
		destroy_fonts();
		create_fonts();
		release_icon();
		rebuild_layout();
		icon_ = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(IDI_APPLICATION_ICON), IMAGE_ICON, icon_size_, icon_size_, LR_DEFAULTCOLOR));
		icon_owned_ = (icon_ != nullptr);
		if (icon_ == nullptr) {
			icon_ = LoadIconW(nullptr, reinterpret_cast<LPCWSTR>(IDI_APPLICATION));
		}
		release_surfaces();
	}

	void place_window(HWND hwnd) noexcept {
		POINT cursor{};
		GetCursorPos(&cursor);
		const HMONITOR monitor = MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO info{};
		info.cbSize = sizeof(info);
		RECT work{0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
		if (GetMonitorInfoW(monitor, &info) != FALSE) {
			work = info.rcWork;
		}
		for (int pass = 0; pass < 2; ++pass) {
			const int x = work.left + ((work.right - work.left) - client_width_) / 2;
			const int y = work.top + ((work.bottom - work.top) - client_height_) / 2;
			SetWindowPos(hwnd, nullptr, x, y, client_width_, client_height_, SWP_NOZORDER | SWP_NOACTIVATE);
			const UINT dpi = query_window_dpi(hwnd);
			if (dpi == dpi_) {
				break;
			}
			dpi_ = dpi;
			apply_dpi_resources();
		}
	}

	void ensure_surfaces(HDC reference) noexcept {
		if (backdrop_.valid() && frame_.valid()) {
			return;
		}
		release_surfaces();
		backdrop_.create(reference, client_width_, client_height_);
		frame_.create(reference, client_width_, client_height_);
		if (!backdrop_.valid() || !frame_.valid()) {
			release_surfaces();
			return;
		}
		render_backdrop();
	}

	void render_backdrop() noexcept {
		const HDC device = backdrop_.device;
		const RECT client{0, 0, client_width_, client_height_};
		fill_gradient(device, client, StartupPalette::kBackgroundTop, StartupPalette::kBackgroundBottom, false);

		const RECT title_bar{0, 0, client_width_, title_height_};
		fill_solid(device, title_bar, StartupPalette::kTitleBar);
		const RECT title_line{0, title_height_, client_width_, title_height_ + 1};
		fill_solid(device, title_line, StartupPalette::kSeparator);

		if (icon_ != nullptr) {
			DrawIconEx(device, icon_rect_.left, icon_rect_.top, icon_, icon_size_, icon_size_, 0, nullptr, DI_NORMAL);
		}
		const UINT single_line = DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS;
		draw_text(device, widen_utf8(ApplicationInfo::kProductName) + L" - Starting", fonts_.brand, StartupPalette::kTextSecondary, brand_rect_, single_line);

		draw_text(device, widen_utf8(ApplicationInfo::kProductName), fonts_.title, StartupPalette::kTextPrimary, hero_title_rect_, single_line);
		draw_text(device, widen_utf8(ApplicationInfo::kDescription), fonts_.subtitle, StartupPalette::kTextSecondary, hero_subtitle_rect_, single_line);
		draw_text(device, L"Version " + widen_utf8(ApplicationInfo::kVersion), fonts_.caption, StartupPalette::kTextDisabled, hero_version_rect_, single_line);

		const RECT footer_line{0, footer_line_y_, client_width_, footer_line_y_ + 1};
		fill_solid(device, footer_line, StartupPalette::kSeparator);

		const std::wstring prefix = L"Developed by ";
		SIZE prefix_size{};
		const HGDIOBJ previous = SelectObject(device, fonts_.caption);
		GetTextExtentPoint32W(device, prefix.c_str(), static_cast<int>(prefix.size()), &prefix_size);
		SelectObject(device, previous);
		draw_text(device, prefix, fonts_.caption, StartupPalette::kTextSecondary, developer_rect_, single_line);
		RECT name_rect = developer_rect_;
		name_rect.left += prefix_size.cx;
		draw_text(device, widen_utf8(ApplicationInfo::kDeveloper), fonts_.caption_bold, StartupPalette::kTextPrimary, name_rect, single_line);

		const HBRUSH border_brush = CreateSolidBrush(StartupPalette::kBorder);
		FrameRect(device, &client, border_brush);
		DeleteObject(border_brush);
	}

	[[nodiscard]] bool is_hot(Control control) const noexcept {
		return hover_ == control;
	}

	[[nodiscard]] bool is_down(Control control) const noexcept {
		return pressed_ == control && hover_ == control;
	}

	void draw_window_controls(HDC device) const noexcept {
		const int half = px(5);
		const int pen_width = std::max(1, px(1));

		if (is_hot(Control::Minimize)) {
			fill_solid(device, minimize_rect_, is_down(Control::Minimize) ? StartupPalette::kControlPressed : StartupPalette::kControlHover);
		}
		const HPEN minimize_pen = CreatePen(PS_SOLID, pen_width, StartupPalette::kTextPrimary);
		HGDIOBJ previous = SelectObject(device, minimize_pen);
		const int minimize_x = (minimize_rect_.left + minimize_rect_.right) / 2;
		const int minimize_y = (minimize_rect_.top + minimize_rect_.bottom) / 2;
		MoveToEx(device, minimize_x - half, minimize_y, nullptr);
		LineTo(device, minimize_x + half + 1, minimize_y);
		SelectObject(device, previous);
		DeleteObject(minimize_pen);

		const bool close_active = !cancelling_ && is_hot(Control::Close);
		if (close_active) {
			fill_solid(device, close_rect_, is_down(Control::Close) ? StartupPalette::kClosePressed : StartupPalette::kCloseHover);
		}
		const COLORREF close_color = cancelling_ ? StartupPalette::kTextDisabled : (close_active ? StartupPalette::color(255, 255, 255) : StartupPalette::kTextPrimary);
		const HPEN close_pen = CreatePen(PS_SOLID, pen_width, close_color);
		previous = SelectObject(device, close_pen);
		const int close_x = (close_rect_.left + close_rect_.right) / 2;
		const int close_y = (close_rect_.top + close_rect_.bottom) / 2;
		MoveToEx(device, close_x - half, close_y - half, nullptr);
		LineTo(device, close_x + half + 1, close_y + half + 1);
		MoveToEx(device, close_x - half, close_y + half, nullptr);
		LineTo(device, close_x + half + 1, close_y - half - 1);
		SelectObject(device, previous);
		DeleteObject(close_pen);
	}

	void draw_status(HDC device) const noexcept {
		const UINT single_line = DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS;
		const std::wstring stage = stage_text_.empty() ? std::wstring(L"Starting") : stage_text_;
		draw_text(device, stage, fonts_.stage, StartupPalette::kTextPrimary, stage_rect_, single_line | DT_LEFT);

		const int percent = static_cast<int>(std::clamp(displayed_fraction_, 0.0f, 1.0f) * 100.0f + 0.5f);
		draw_text(device, std::to_wstring(percent) + L"%", fonts_.stage, StartupPalette::kTextSecondary, percent_rect_, single_line | DT_RIGHT);

		COLORREF detail_color = StartupPalette::kTextSecondary;
		if (phase_ == StartupPhase::Failed) {
			detail_color = StartupPalette::kTextError;
		} else if (cancelling_) {
			detail_color = StartupPalette::kTextWarning;
		}
		draw_text(device, detail_text_, fonts_.detail, detail_color, detail_rect_, single_line | DT_LEFT);
	}

	void draw_progress_bar(HDC device) const noexcept {
		const int height = bar_rect_.bottom - bar_rect_.top;
		const HRGN region = CreateRoundRectRgn(bar_rect_.left, bar_rect_.top, bar_rect_.right + 1, bar_rect_.bottom + 1, height, height);
		SelectClipRgn(device, region);
		fill_solid(device, bar_rect_, StartupPalette::kBarTrack);
		const float fraction = std::clamp(displayed_fraction_, 0.0f, 1.0f);
		const int filled = static_cast<int>(static_cast<float>(bar_rect_.right - bar_rect_.left) * fraction + 0.5f);
		if (filled > 0) {
			RECT fill = bar_rect_;
			fill.right = fill.left + filled;
			const COLORREF end_color = blend(StartupPalette::kAccentStart, StartupPalette::kAccentEnd, fraction);
			fill_gradient(device, fill, StartupPalette::kAccentStart, end_color, true);
		}
		SelectClipRgn(device, nullptr);
		DeleteObject(region);
	}

	void draw_repository_button(HDC device) const noexcept {
		const bool hot = is_hot(Control::Repository);
		const COLORREF fill = is_down(Control::Repository) ? StartupPalette::kButtonPressed : (hot ? StartupPalette::kButtonHover : StartupPalette::kButtonFill);
		fill_round_rect(device, repository_rect_, px(8), fill, hot ? StartupPalette::kAccentStart : StartupPalette::kBorder);
		draw_text(device, L"Open source project", fonts_.button, hot ? StartupPalette::kTextPrimary : StartupPalette::kTextSecondary, repository_rect_, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}

	void paint(HWND hwnd) noexcept {
		PAINTSTRUCT paint_info{};
		const HDC device = BeginPaint(hwnd, &paint_info);
		if (device != nullptr) {
			ensure_surfaces(device);
			if (backdrop_.valid() && frame_.valid()) {
				BitBlt(frame_.device, 0, 0, client_width_, client_height_, backdrop_.device, 0, 0, SRCCOPY);
				draw_window_controls(frame_.device);
				draw_status(frame_.device);
				draw_progress_bar(frame_.device);
				draw_repository_button(frame_.device);
				BitBlt(device, 0, 0, client_width_, client_height_, frame_.device, 0, 0, SRCCOPY);
			}
		}
		EndPaint(hwnd, &paint_info);
	}

	[[nodiscard]] Control control_at(const POINT& point) const noexcept {
		if (PtInRect(&minimize_rect_, point) != FALSE) {
			return Control::Minimize;
		}
		if (PtInRect(&close_rect_, point) != FALSE) {
			return Control::Close;
		}
		if (PtInRect(&repository_rect_, point) != FALSE) {
			return Control::Repository;
		}
		return Control::None;
	}

	[[nodiscard]] static POINT point_from_lparam(LPARAM lparam) noexcept {
		return POINT{static_cast<LONG>(static_cast<short>(LOWORD(lparam))), static_cast<LONG>(static_cast<short>(HIWORD(lparam)))};
	}

	void set_hover(HWND hwnd, Control control) noexcept {
		if (hover_ == control) {
			return;
		}
		hover_ = control;
		InvalidateRect(hwnd, nullptr, FALSE);
	}

	void request_cancel() noexcept {
		if (progress_ == nullptr || cancelling_) {
			return;
		}
		try {
			progress_->request_cancel();
		} catch (...) {
		}
	}

	void activate(HWND hwnd, Control control) noexcept {
		switch (control) {
			case Control::Minimize:
				ShowWindow(hwnd, SW_MINIMIZE);
				break;
			case Control::Close:
				request_cancel();
				break;
			case Control::Repository:
				ShellExecuteW(hwnd, L"open", widen_utf8(ApplicationInfo::kRepositoryUrl).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
				break;
			case Control::None:
			default:
				break;
		}
	}

	void apply_snapshot(HWND hwnd) noexcept {
		update_pending_.store(false, std::memory_order_release);
		if (progress_ == nullptr) {
			return;
		}
		try {
			const StartupSnapshot snapshot = progress_->snapshot();
			target_fraction_ = std::clamp(snapshot.fraction, 0.0f, 1.0f);
			phase_ = snapshot.phase;
			cancelling_ = snapshot.cancel_requested;
			stage_text_ = widen_utf8(snapshot.stage);
			if (cancelling_) {
				detail_text_ = L"Cancelling startup, finishing the current operation...";
			} else if (phase_ == StartupPhase::Failed) {
				detail_text_ = widen_utf8(snapshot.error);
			} else {
				detail_text_ = widen_utf8(snapshot.detail);
			}
		} catch (...) {
			return;
		}
		if (!ease_timer_active_ && std::abs(target_fraction_ - displayed_fraction_) > 0.0005f) {
			ease_timer_active_ = (SetTimer(hwnd, kEaseTimerId, kEaseIntervalMs, nullptr) != 0);
		}
		InvalidateRect(hwnd, nullptr, FALSE);
	}

	void advance_ease(HWND hwnd) noexcept {
		const float delta = target_fraction_ - displayed_fraction_;
		if (std::abs(delta) < 0.0008f) {
			displayed_fraction_ = target_fraction_;
			KillTimer(hwnd, kEaseTimerId);
			ease_timer_active_ = false;
		} else {
			displayed_fraction_ += delta * 0.2f + std::copysign(0.0004f, delta);
		}
		InvalidateRect(hwnd, &ease_area_, FALSE);
	}

	void on_dpi_changed(HWND hwnd, WPARAM wparam, LPARAM lparam) noexcept {
		const UINT dpi = static_cast<UINT>(HIWORD(wparam));
		if (dpi == 0) {
			return;
		}
		dpi_ = dpi;
		apply_dpi_resources();
		const RECT* suggested = reinterpret_cast<const RECT*>(lparam);
		if (suggested != nullptr) {
			SetWindowPos(hwnd, nullptr, suggested->left, suggested->top, client_width_, client_height_, SWP_NOZORDER | SWP_NOACTIVATE);
		}
		InvalidateRect(hwnd, nullptr, FALSE);
	}

	LRESULT handle_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) noexcept {
		switch (message) {
			case WM_ERASEBKGND:
				return 1;
			case WM_PAINT:
				paint(hwnd);
				return 0;
			case WM_NCHITTEST: {
				POINT point = point_from_lparam(lparam);
				ScreenToClient(hwnd, &point);
				if (control_at(point) != Control::None) {
					return HTCLIENT;
				}
				if (point.y >= 0 && point.y < title_height_) {
					return HTCAPTION;
				}
				return HTCLIENT;
			}
			case WM_SETCURSOR:
				if (LOWORD(lparam) == HTCLIENT && hover_ != Control::None && !(cancelling_ && hover_ == Control::Close)) {
					SetCursor(LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(IDC_HAND)));
					return TRUE;
				}
				break;
			case WM_MOUSEMOVE: {
				if (!tracking_mouse_) {
					TRACKMOUSEEVENT track{};
					track.cbSize = sizeof(track);
					track.dwFlags = TME_LEAVE;
					track.hwndTrack = hwnd;
					tracking_mouse_ = (TrackMouseEvent(&track) != FALSE);
				}
				set_hover(hwnd, control_at(point_from_lparam(lparam)));
				return 0;
			}
			case WM_MOUSELEAVE:
				tracking_mouse_ = false;
				set_hover(hwnd, Control::None);
				return 0;
			case WM_NCMOUSEMOVE:
				set_hover(hwnd, Control::None);
				break;
			case WM_LBUTTONDOWN: {
				pressed_ = control_at(point_from_lparam(lparam));
				if (pressed_ != Control::None) {
					SetCapture(hwnd);
					InvalidateRect(hwnd, nullptr, FALSE);
				}
				return 0;
			}
			case WM_LBUTTONUP: {
				const Control released = control_at(point_from_lparam(lparam));
				const Control pressed = pressed_;
				pressed_ = Control::None;
				if (GetCapture() == hwnd) {
					ReleaseCapture();
				}
				InvalidateRect(hwnd, nullptr, FALSE);
				if (pressed != Control::None && pressed == released) {
					activate(hwnd, pressed);
				}
				return 0;
			}
			case WM_KEYDOWN:
				if (wparam == VK_ESCAPE) {
					request_cancel();
					return 0;
				}
				break;
			case WM_CLOSE:
				request_cancel();
				return 0;
			case WM_TIMER:
				if (wparam == kEaseTimerId) {
					advance_ease(hwnd);
				}
				return 0;
			case WM_DPICHANGED:
				on_dpi_changed(hwnd, wparam, lparam);
				return 0;
			case WM_DESTROY:
				if (ease_timer_active_) {
					KillTimer(hwnd, kEaseTimerId);
					ease_timer_active_ = false;
				}
				PostQuitMessage(0);
				return 0;
			default:
				if (message == kMessageUpdate) {
					apply_snapshot(hwnd);
					return 0;
				}
				if (message == kMessageClose) {
					DestroyWindow(hwnd);
					return 0;
				}
				break;
		}
		return DefWindowProcW(hwnd, message, wparam, lparam);
	}

	static LRESULT CALLBACK window_procedure(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
		StartupWindow* self = nullptr;
		if (message == WM_NCCREATE) {
			const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
			self = static_cast<StartupWindow*>(create->lpCreateParams);
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
		} else {
			self = reinterpret_cast<StartupWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
		}
		if (self == nullptr) {
			return DefWindowProcW(hwnd, message, wparam, lparam);
		}
		return self->handle_message(hwnd, message, wparam, lparam);
	}

	void post_update() noexcept {
		const HWND hwnd = hwnd_.load(std::memory_order_acquire);
		if (hwnd == nullptr) {
			return;
		}
		if (!update_pending_.exchange(true, std::memory_order_acq_rel)) {
			PostMessageW(hwnd, kMessageUpdate, 0, 0);
		}
	}

	void run() {
		instance_ = GetModuleHandleW(nullptr);

		WNDCLASSEXW window_class{};
		window_class.cbSize = sizeof(window_class);
		window_class.style = CS_DROPSHADOW;
		window_class.lpfnWndProc = &StartupWindow::window_procedure;
		window_class.hInstance = instance_;
		window_class.hCursor = LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(IDC_ARROW));
		window_class.hIcon = load_shared_icon(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
		window_class.hIconSm = load_shared_icon(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
		window_class.lpszClassName = kWindowClassName;
		if (RegisterClassExW(&window_class) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
			ready_.set_value(false);
			return;
		}

		const std::wstring title = widen_utf8(ApplicationInfo::kProductName);
		const HWND hwnd = CreateWindowExW(WS_EX_APPWINDOW, kWindowClassName, title.c_str(), WS_POPUP | WS_MINIMIZEBOX | WS_SYSMENU, 0, 0, kLogicalWidth, kLogicalHeight, nullptr, nullptr, instance_, this);
		if (hwnd == nullptr) {
			UnregisterClassW(kWindowClassName, instance_);
			ready_.set_value(false);
			return;
		}
		hwnd_.store(hwnd, std::memory_order_release);

		dpi_ = query_window_dpi(hwnd);
		apply_dpi_resources();
		place_window(hwnd);

		const DWORD corner_preference = 2;
		DwmSetWindowAttribute(hwnd, 33, &corner_preference, sizeof(corner_preference));

		ShowWindow(hwnd, SW_SHOWNORMAL);
		UpdateWindow(hwnd);
		SetForegroundWindow(hwnd);

		progress_->set_listener([this]() { post_update(); });
		ready_.set_value(true);
		post_update();

		MSG message{};
		while (GetMessageW(&message, nullptr, 0, 0) > 0) {
			TranslateMessage(&message);
			DispatchMessageW(&message);
		}

		progress_->set_listener({});
		hwnd_.store(nullptr, std::memory_order_release);
		release_surfaces();
		destroy_fonts();
		release_icon();
		UnregisterClassW(kWindowClassName, instance_);
	}

public:
	StartupWindow() = default;
	StartupWindow(const StartupWindow&) = delete;
	StartupWindow& operator=(const StartupWindow&) = delete;

	~StartupWindow() {
		close();
	}

	[[nodiscard]] bool open(StartupProgress& progress) {
		if (thread_.joinable()) {
			return false;
		}
		progress_ = &progress;
		std::future<bool> ready = ready_.get_future();
		thread_ = std::thread([this]() { run(); });
		const bool opened = ready.get();
		if (!opened) {
			thread_.join();
			progress_ = nullptr;
		}
		return opened;
	}

	[[nodiscard]] bool is_open() const noexcept {
		return thread_.joinable();
	}

	void close() noexcept {
		if (!thread_.joinable()) {
			return;
		}
		const HWND hwnd = hwnd_.load(std::memory_order_acquire);
		if (hwnd != nullptr) {
			PostMessageW(hwnd, kMessageClose, 0, 0);
		}
		thread_.join();
		progress_ = nullptr;
	}
};

}

#else

namespace Relativistic::Startup {

class StartupWindow {
public:
	[[nodiscard]] bool open(StartupProgress&) noexcept {
		return false;
	}

	[[nodiscard]] bool is_open() const noexcept {
		return false;
	}

	void close() noexcept {
	}
};

}

#endif
