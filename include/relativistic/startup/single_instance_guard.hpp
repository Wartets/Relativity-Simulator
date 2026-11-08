#pragma once

#include "relativistic/startup/application_info.hpp"
#include "relativistic/startup/platform_bootstrap.hpp"

#include <string>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <system_error>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace Relativistic::Startup {

class SingleInstanceGuard {
private:
#if defined(_WIN32)
	HANDLE handle_{nullptr};

	static constexpr const wchar_t* kMutexName = L"Local\\Wartets.RelativisticEngine.SingleInstance";

	[[nodiscard]] static HWND find_visible_window(const std::wstring& title) noexcept {
		const HWND window = FindWindowW(nullptr, title.c_str());
		return (window != nullptr && IsWindowVisible(window) != FALSE) ? window : nullptr;
	}
#else
	int descriptor_{-1};

	[[nodiscard]] static std::filesystem::path lock_path() {
		std::error_code ec;
		std::filesystem::path directory = std::filesystem::temp_directory_path(ec);
		if (ec) {
			directory = "/tmp";
		}
		return directory / ("wartets-relativistic-engine-" + std::to_string(static_cast<unsigned long>(::getuid())) + ".lock");
	}
#endif

public:
	SingleInstanceGuard() = default;
	SingleInstanceGuard(const SingleInstanceGuard&) = delete;
	SingleInstanceGuard& operator=(const SingleInstanceGuard&) = delete;

	~SingleInstanceGuard() {
		release();
	}

	[[nodiscard]] bool acquire() noexcept {
#if defined(_WIN32)
		if (handle_ != nullptr) {
			return true;
		}
		handle_ = CreateMutexW(nullptr, FALSE, kMutexName);
		if (handle_ == nullptr) {
			return true;
		}
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			CloseHandle(handle_);
			handle_ = nullptr;
			return false;
		}
		return true;
#else
		if (descriptor_ >= 0) {
			return true;
		}
		try {
			descriptor_ = ::open(lock_path().c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
		} catch (...) {
			descriptor_ = -1;
		}
		if (descriptor_ < 0) {
			return true;
		}
		if (::flock(descriptor_, LOCK_EX | LOCK_NB) != 0) {
			const bool already_running = (errno == EWOULDBLOCK);
			::close(descriptor_);
			descriptor_ = -1;
			return !already_running;
		}
		char identifier[32];
		const int length = std::snprintf(identifier, sizeof(identifier), "%ld\n", static_cast<long>(::getpid()));
		if (::ftruncate(descriptor_, 0) != 0) {
		}
		if (length > 0 && ::write(descriptor_, identifier, static_cast<size_t>(length)) < 0) {
		}
		return true;
#endif
	}

	void release() noexcept {
#if defined(_WIN32)
		if (handle_ != nullptr) {
			CloseHandle(handle_);
			handle_ = nullptr;
		}
#else
		if (descriptor_ >= 0) {
			::flock(descriptor_, LOCK_UN);
			::close(descriptor_);
			descriptor_ = -1;
		}
#endif
	}

	static void activate_existing_instance() noexcept {
#if defined(_WIN32)
		try {
			const std::wstring product = widen_utf8(ApplicationInfo::kProductName);
			const std::wstring main_title = product + L" - Primary Simulation Host";
			HWND target = find_visible_window(main_title);
			if (target == nullptr) {
				target = find_visible_window(product);
			}
			if (target == nullptr) {
				return;
			}
			if (IsIconic(target) != FALSE) {
				ShowWindow(target, SW_RESTORE);
			}
			SetForegroundWindow(target);
		} catch (...) {
		}
#endif
	}
};

}
