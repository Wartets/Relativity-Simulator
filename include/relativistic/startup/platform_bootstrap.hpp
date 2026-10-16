#pragma once

#include "relativistic/startup/application_info.hpp"

#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace Relativistic::Startup {

#if defined(_WIN32)
[[nodiscard]] inline std::wstring widen_utf8(std::string_view text) {
	if (text.empty()) {
		return {};
	}
	const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
	if (length <= 0) {
		return {};
	}
	std::wstring result(static_cast<size_t>(length), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length);
	return result;
}
#endif

class PlatformBootstrap {
private:
#if defined(_WIN32)
	static void enable_per_monitor_dpi() noexcept {
		using SetContextFunction = BOOL(WINAPI*)(HANDLE);
		const HMODULE user32 = GetModuleHandleW(L"user32.dll");
		if (user32 == nullptr) {
			return;
		}
		const auto function = reinterpret_cast<SetContextFunction>(reinterpret_cast<void*>(GetProcAddress(user32, "SetProcessDpiAwarenessContext")));
		if (function != nullptr) {
			static_cast<void>(function(reinterpret_cast<HANDLE>(static_cast<intptr_t>(-4))));
		}
	}

	static void register_application_identity() noexcept {
		using SetIdentityFunction = HRESULT(WINAPI*)(PCWSTR);
		const HMODULE shell32 = LoadLibraryW(L"shell32.dll");
		if (shell32 == nullptr) {
			return;
		}
		const auto function = reinterpret_cast<SetIdentityFunction>(reinterpret_cast<void*>(GetProcAddress(shell32, "SetCurrentProcessExplicitAppUserModelID")));
		if (function != nullptr) {
			static_cast<void>(function(L"Wartets.RelativisticEngine"));
		}
	}
#endif

public:
	[[nodiscard]] static std::filesystem::path executable_directory() {
#if defined(_WIN32)
		std::wstring buffer(260, L'\0');
		for (;;) {
			const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
			if (length == 0) {
				return {};
			}
			if (length < buffer.size()) {
				buffer.resize(length);
				break;
			}
			if (buffer.size() >= 32768) {
				return {};
			}
			buffer.resize(buffer.size() * 2);
		}
		return std::filesystem::path(buffer).parent_path();
#elif defined(__linux__)
		std::error_code ec;
		const std::filesystem::path executable = std::filesystem::read_symlink("/proc/self/exe", ec);
		return ec ? std::filesystem::path{} : executable.parent_path();
#else
		return {};
#endif
	}

	static void normalize_working_directory() {
		namespace fs = std::filesystem;
		std::error_code ec;
		const auto has_assets = [&ec](const fs::path& directory) {
			ec.clear();
			return fs::is_directory(directory / "shaders", ec) || fs::is_directory(directory / "scenarios", ec);
		};

		ec.clear();
		const fs::path current = fs::current_path(ec);
		if (!ec && has_assets(current)) {
			return;
		}

		fs::path candidate = executable_directory();
		for (size_t depth = 0; depth <= 4 && !candidate.empty(); ++depth) {
			if (has_assets(candidate)) {
				ec.clear();
				fs::current_path(candidate, ec);
				return;
			}
			const fs::path parent = candidate.parent_path();
			if (parent == candidate) {
				break;
			}
			candidate = parent;
		}
	}

	static void initialize() noexcept {
#if defined(_WIN32)
		enable_per_monitor_dpi();
		register_application_identity();
		static_cast<void>(HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0));
#endif
		try {
			normalize_working_directory();
		} catch (...) {
		}
	}

	static void show_fatal_error(std::string_view title, std::string_view message) noexcept {
		try {
			std::fprintf(stderr, "[ERROR] %.*s\n", static_cast<int>(message.size()), message.data());
#if defined(_WIN32)
			MessageBoxW(nullptr, widen_utf8(message).c_str(), widen_utf8(title).c_str(), MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
#endif
		} catch (...) {
		}
	}
};

}
