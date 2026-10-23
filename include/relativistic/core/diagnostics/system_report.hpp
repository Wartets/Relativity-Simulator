#pragma once

#include "relativistic/core/diagnostics/diagnostics_registry.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

#if defined(_WIN32)
#include <psapi.h>
#include <tlhelp32.h>
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#endif

namespace Relativistic::Core::Diagnostics {

#if defined(_WIN32)
[[nodiscard]] inline std::string narrow_string(const wchar_t* source) {
	if (source == nullptr || source[0] == L'\0') {
		return {};
	}
	const int required = WideCharToMultiByte(CP_UTF8, 0, source, -1, nullptr, 0, nullptr, nullptr);
	if (required <= 1) {
		return {};
	}
	std::string result(static_cast<size_t>(required), '\0');
	WideCharToMultiByte(CP_UTF8, 0, source, -1, result.data(), required, nullptr, nullptr);
	result.resize(static_cast<size_t>(required - 1));
	return result;
}
#endif

class SystemReport {
private:
	static void copy_matching_lines(std::FILE* out, const char* path, const char* const* prefixes, size_t prefix_count, size_t line_limit) noexcept {
		std::FILE* file = std::fopen(path, "r");
		if (file == nullptr) {
			return;
		}
		char line[512];
		size_t printed = 0;
		while (printed < line_limit && std::fgets(line, sizeof(line), file) != nullptr) {
			for (size_t index = 0; index < prefix_count; ++index) {
				if (std::strncmp(line, prefixes[index], std::strlen(prefixes[index])) == 0) {
					std::fprintf(out, "  %s", line);
					++printed;
					break;
				}
			}
		}
		std::fclose(file);
	}

public:
	static void query_cpu_brand(char* destination, size_t capacity) noexcept {
		if (capacity == 0) {
			return;
		}
		copy_text(destination, capacity, "unavailable");
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
		unsigned int registers[12]{};
#if defined(_MSC_VER)
		int info[4]{};
		__cpuid(info, static_cast<int>(0x80000000U));
		if (static_cast<unsigned int>(info[0]) < 0x80000004U) {
			return;
		}
		for (unsigned int block = 0; block < 3; ++block) {
			__cpuid(info, static_cast<int>(0x80000002U + block));
			for (unsigned int lane = 0; lane < 4; ++lane) {
				registers[block * 4 + lane] = static_cast<unsigned int>(info[lane]);
			}
		}
#else
		if (__get_cpuid_max(0x80000000U, nullptr) < 0x80000004U) {
			return;
		}
		for (unsigned int block = 0; block < 3; ++block) {
			__get_cpuid(0x80000002U + block, &registers[block * 4], &registers[block * 4 + 1], &registers[block * 4 + 2], &registers[block * 4 + 3]);
		}
#endif
		char brand[49]{};
		std::memcpy(brand, registers, 48);
		const char* start = brand;
		while (*start == ' ') {
			++start;
		}
		copy_text(destination, capacity, start);
#endif
	}

	static void write_operating_system(std::FILE* out) noexcept {
#if defined(_WIN32)
		using RtlGetVersionFunction = LONG(WINAPI*)(RTL_OSVERSIONINFOW*);
		RTL_OSVERSIONINFOW version{};
		version.dwOSVersionInfoSize = sizeof(version);
		RtlGetVersionFunction function = nullptr;
		const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
		if (ntdll != nullptr) {
			function = reinterpret_cast<RtlGetVersionFunction>(reinterpret_cast<void*>(GetProcAddress(ntdll, "RtlGetVersion")));
		}
		if (function != nullptr && function(&version) == 0) {
			std::fprintf(out, "  Operating system     Windows %lu.%lu build %lu\n", static_cast<unsigned long>(version.dwMajorVersion), static_cast<unsigned long>(version.dwMinorVersion), static_cast<unsigned long>(version.dwBuildNumber));
		} else {
			std::fprintf(out, "  Operating system     Windows (version unavailable)\n");
		}
		SYSTEM_INFO info{};
		GetNativeSystemInfo(&info);
		std::fprintf(out, "  Architecture id      %u\n", static_cast<unsigned>(info.wProcessorArchitecture));
		std::fprintf(out, "  Page size            %lu bytes\n", static_cast<unsigned long>(info.dwPageSize));
#else
		std::FILE* file = std::fopen("/etc/os-release", "r");
		if (file != nullptr) {
			char line[256];
			while (std::fgets(line, sizeof(line), file) != nullptr) {
				if (std::strncmp(line, "PRETTY_NAME=", 12) == 0) {
					std::fprintf(out, "  Operating system     %s", line + 12);
				}
			}
			std::fclose(file);
		}
		std::FILE* kernel = std::fopen("/proc/version", "r");
		if (kernel != nullptr) {
			char line[256];
			if (std::fgets(line, sizeof(line), kernel) != nullptr) {
				std::fprintf(out, "  Kernel               %s", line);
			}
			std::fclose(kernel);
		}
#endif
	}

	static void write_processor(std::FILE* out) noexcept {
		char brand[96];
		query_cpu_brand(brand, sizeof(brand));
		std::fprintf(out, "  Processor            %s\n", brand);
		std::fprintf(out, "  Logical processors   %u\n", std::thread::hardware_concurrency());
	}

	static void write_memory(std::FILE* out) noexcept {
#if defined(_WIN32)
		MEMORYSTATUSEX status{};
		status.dwLength = sizeof(status);
		if (GlobalMemoryStatusEx(&status) != FALSE) {
			std::fprintf(
				out, "  Physical memory      %.1f MiB total, %.1f MiB available (%lu%% in use)\n",
				static_cast<double>(status.ullTotalPhys) / 1048576.0, static_cast<double>(status.ullAvailPhys) / 1048576.0, static_cast<unsigned long>(status.dwMemoryLoad)
			);
			std::fprintf(
				out, "  Commit limit         %.1f MiB total, %.1f MiB available\n",
				static_cast<double>(status.ullTotalPageFile) / 1048576.0, static_cast<double>(status.ullAvailPageFile) / 1048576.0
			);
			std::fprintf(out, "  Virtual address      %.1f MiB total, %.1f MiB available\n", static_cast<double>(status.ullTotalVirtual) / 1048576.0, static_cast<double>(status.ullAvailVirtual) / 1048576.0);
		}
#else
		static constexpr const char* prefixes[] = {"MemTotal", "MemAvailable", "SwapTotal", "SwapFree"};
		copy_matching_lines(out, "/proc/meminfo", prefixes, 4, 8);
#endif
	}

	static void write_process(std::FILE* out) noexcept {
		std::fprintf(out, "  Process id           %llu\n", static_cast<unsigned long long>(current_process_id()));
#if defined(_WIN32)
		using GetProcessMemoryInfoFunction = BOOL(WINAPI*)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
		const HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
		GetProcessMemoryInfoFunction function = nullptr;
		if (kernel != nullptr) {
			function = reinterpret_cast<GetProcessMemoryInfoFunction>(reinterpret_cast<void*>(GetProcAddress(kernel, "K32GetProcessMemoryInfo")));
		}
		PROCESS_MEMORY_COUNTERS_EX counters{};
		counters.cb = sizeof(counters);
		if (function != nullptr && function(GetCurrentProcess(), reinterpret_cast<PPROCESS_MEMORY_COUNTERS>(&counters), sizeof(counters)) != FALSE) {
			std::fprintf(out, "  Working set          %.1f MiB (peak %.1f MiB)\n", static_cast<double>(counters.WorkingSetSize) / 1048576.0, static_cast<double>(counters.PeakWorkingSetSize) / 1048576.0);
			std::fprintf(out, "  Private bytes        %.1f MiB (peak pagefile %.1f MiB)\n", static_cast<double>(counters.PrivateUsage) / 1048576.0, static_cast<double>(counters.PeakPagefileUsage) / 1048576.0);
			std::fprintf(out, "  Page faults          %lu\n", static_cast<unsigned long>(counters.PageFaultCount));
		}
		DWORD handles = 0;
		if (GetProcessHandleCount(GetCurrentProcess(), &handles) != FALSE) {
			std::fprintf(out, "  Open handles         %lu\n", static_cast<unsigned long>(handles));
		}
		std::fprintf(out, "  GDI objects          %lu\n", static_cast<unsigned long>(GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS)));
		std::fprintf(out, "  USER objects         %lu\n", static_cast<unsigned long>(GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS)));
		FILETIME created{};
		FILETIME exited{};
		FILETIME kernel_time{};
		FILETIME user_time{};
		if (GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel_time, &user_time) != FALSE) {
			const auto to_seconds = [](const FILETIME& value) noexcept {
				return static_cast<double>((static_cast<uint64_t>(value.dwHighDateTime) << 32) | value.dwLowDateTime) * 1.0e-7;
			};
			std::fprintf(out, "  CPU time             user %.2f s, kernel %.2f s\n", to_seconds(user_time), to_seconds(kernel_time));
		}
#else
		static constexpr const char* prefixes[] = {"VmPeak", "VmSize", "VmRSS", "VmHWM", "Threads"};
		copy_matching_lines(out, "/proc/self/status", prefixes, 5, 8);
#endif
	}

	static void write_modules(std::FILE* out) noexcept {
#if defined(_WIN32)
		const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
		if (snapshot == INVALID_HANDLE_VALUE) {
			std::fprintf(out, "  (module snapshot unavailable)\n");
			return;
		}
		MODULEENTRY32W entry{};
		entry.dwSize = sizeof(entry);
		size_t count = 0;
		if (Module32FirstW(snapshot, &entry) != FALSE) {
			do {
				std::fprintf(
					out, "  0x%016llX %8lu KiB  %s\n",
					static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(entry.modBaseAddr)),
					static_cast<unsigned long>(entry.modBaseSize / 1024UL), narrow_string(entry.szExePath).c_str()
				);
				++count;
			} while (Module32NextW(snapshot, &entry) != FALSE);
		}
		CloseHandle(snapshot);
		std::fprintf(out, "  (%zu modules)\n", count);
#else
		std::FILE* file = std::fopen("/proc/self/maps", "r");
		if (file == nullptr) {
			return;
		}
		char line[512];
		size_t printed = 0;
		while (printed < 600 && std::fgets(line, sizeof(line), file) != nullptr) {
			std::fprintf(out, "  %s", line);
			++printed;
		}
		std::fclose(file);
#endif
	}

	static void write_threads(std::FILE* out, uint64_t faulting_thread) noexcept {
#if defined(_WIN32)
		const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (snapshot == INVALID_HANDLE_VALUE) {
			std::fprintf(out, "  (thread snapshot unavailable)\n");
			return;
		}
		THREADENTRY32 entry{};
		entry.dwSize = sizeof(entry);
		const DWORD process = GetCurrentProcessId();
		size_t count = 0;
		if (Thread32First(snapshot, &entry) != FALSE) {
			do {
				if (entry.th32OwnerProcessID != process) {
					continue;
				}
				++count;
				const ActivitySlot* slot = find_activity_slot(static_cast<uint64_t>(entry.th32ThreadID));
				std::fprintf(
					out, "  thread %lu%s  priority=%ld  name=%s  activity=%s\n",
					static_cast<unsigned long>(entry.th32ThreadID), (static_cast<uint64_t>(entry.th32ThreadID) == faulting_thread) ? " [FAULTING THREAD]" : "",
					static_cast<long>(entry.tpBasePri),
					(slot != nullptr && slot->thread_name[0] != '\0') ? slot->thread_name.data() : "-",
					(slot != nullptr && slot->label[0] != '\0') ? slot->label.data() : "-"
				);
			} while (Thread32Next(snapshot, &entry) != FALSE);
		}
		CloseHandle(snapshot);
		std::fprintf(out, "  (%zu threads)\n", count);
#else
		static_cast<void>(faulting_thread);
		std::fprintf(out, "  (see thread activity table)\n");
#endif
	}

	static void write_all(std::FILE* out) noexcept {
		std::fprintf(out, "\n---- Operating system ----\n");
		write_operating_system(out);
		std::fprintf(out, "\n---- Processor ----\n");
		write_processor(out);
		std::fprintf(out, "\n---- Memory ----\n");
		write_memory(out);
		std::fprintf(out, "\n---- Process ----\n");
		write_process(out);
	}
};

}
