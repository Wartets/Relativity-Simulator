#pragma once

#if defined(_WIN32)
#include <windows.h>
#endif

namespace Relativistic::Core {

class SystemConsole {
public:
	static void set_visible(bool visible) noexcept {
#if defined(_WIN32)
		HWND console_window = GetConsoleWindow();
		if (console_window != nullptr) {
			ShowWindow(console_window, visible ? SW_SHOW : SW_HIDE);
		}
#else
		static_cast<void>(visible);
#endif
	}

	[[nodiscard]] static bool is_supported() noexcept {
#if defined(_WIN32)
		return true;
#else
		return false;
#endif
	}
};

}
