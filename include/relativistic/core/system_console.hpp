#pragma once

#include "relativistic/core/engine_log.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <atomic>
#include <cstdio>
#include <iostream>
#endif

namespace Relativistic::Core {

class SystemConsole {
#if defined(_WIN32)
private:
	static inline std::atomic<bool> attached_{false};
	static inline std::atomic<bool> owned_{false};

	[[nodiscard]] static bool stream_is_redirected(DWORD handle_id) noexcept {
		const HANDLE handle = GetStdHandle(handle_id);
		return handle != nullptr && handle != INVALID_HANDLE_VALUE && GetFileType(handle) != FILE_TYPE_UNKNOWN;
	}

	static void rebind(const char* device, const char* mode, FILE* stream) noexcept {
#if defined(_MSC_VER)
		FILE* rebound = nullptr;
		freopen_s(&rebound, device, mode, stream);
#else
		static_cast<void>(std::freopen(device, mode, stream));
#endif
	}
#endif

public:
	static bool attach(bool prefer_parent) noexcept {
#if defined(_WIN32)
		if (attached_.load(std::memory_order_acquire)) {
			return true;
		}
		const bool input_redirected = stream_is_redirected(STD_INPUT_HANDLE);
		const bool output_redirected = stream_is_redirected(STD_OUTPUT_HANDLE);
		const bool error_redirected = stream_is_redirected(STD_ERROR_HANDLE);

		bool acquired = false;
		bool created = false;
		if (GetConsoleWindow() != nullptr) {
			acquired = true;
		} else if (prefer_parent && AttachConsole(ATTACH_PARENT_PROCESS) != FALSE) {
			acquired = true;
		} else if (AllocConsole() != FALSE) {
			acquired = true;
			created = true;
		}
		if (!acquired) {
			return false;
		}

		if (!input_redirected) {
			rebind("CONIN$", "r", stdin);
		}
		if (!output_redirected) {
			rebind("CONOUT$", "w", stdout);
		}
		if (!error_redirected) {
			rebind("CONOUT$", "w", stderr);
		}
		std::cin.clear();
		std::cout.clear();
		std::cerr.clear();
		std::setvbuf(stdout, nullptr, _IONBF, 0);
		std::setvbuf(stderr, nullptr, _IONBF, 0);

		owned_.store(created, std::memory_order_release);
		attached_.store(true, std::memory_order_release);

		if (created) {
			SetConsoleOutputCP(CP_UTF8);
			SetConsoleTitleW(L"Relativistic Engine - Console");
			const HWND window = GetConsoleWindow();
			if (window != nullptr) {
				const HMENU menu = GetSystemMenu(window, FALSE);
				if (menu != nullptr) {
					EnableMenuItem(menu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
				}
			}
			EngineLog::instance().replay_to_console();
		}
		return true;
#else
		static_cast<void>(prefer_parent);
		return true;
#endif
	}

	[[nodiscard]] static bool is_attached() noexcept {
#if defined(_WIN32)
		return attached_.load(std::memory_order_acquire);
#else
		return true;
#endif
	}

	static void set_visible(bool visible) noexcept {
#if defined(_WIN32)
		if (visible && !attach(false)) {
			return;
		}
		if (!owned_.load(std::memory_order_acquire)) {
			return;
		}
		const HWND console_window = GetConsoleWindow();
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
