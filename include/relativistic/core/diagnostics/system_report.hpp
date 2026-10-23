#pragma once

#include "relativistic/core/diagnostics/diagnostics_registry.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

#include <cctype>
#include <ctime>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#include <psapi.h>
#include <tlhelp32.h>
#else
#include <sys/resource.h>
#include <sys/utsname.h>
#include <unistd.h>
extern "C" char** environ;
#if defined(__GLIBC__)
#include <gnu/libc-version.h>
#endif
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

	#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
	static void query_cpuid(unsigned int leaf, unsigned int subleaf, unsigned int (&registers)[4]) noexcept {
#if defined(_MSC_VER)
		int info[4]{};
		__cpuidex(info, static_cast<int>(leaf), static_cast<int>(subleaf));
		for (size_t lane = 0; lane < 4; ++lane) {
			registers[lane] = static_cast<unsigned int>(info[lane]);
		}
#else
		__cpuid_count(leaf, subleaf, registers[0], registers[1], registers[2], registers[3]);
#endif
	}
#endif

	[[nodiscard]] static bool read_text_file(const char* path, char* destination, size_t capacity) noexcept {
		if (capacity == 0) {
			return false;
		}
		destination[0] = '\0';
		std::FILE* file = std::fopen(path, "r");
		if (file == nullptr) {
			return false;
		}
		const bool read = std::fgets(destination, static_cast<int>(capacity), file) != nullptr;
		std::fclose(file);
		if (!read) {
			destination[0] = '\0';
			return false;
		}
		size_t length = std::strlen(destination);
		while (length > 0 && (destination[length - 1] == '\n' || destination[length - 1] == '\r')) {
			destination[--length] = '\0';
		}
		return true;
	}

	[[nodiscard]] static bool read_first_match(const char* path, const char* prefix, char* destination, size_t capacity) noexcept {
		if (capacity == 0) {
			return false;
		}
		destination[0] = '\0';
		std::FILE* file = std::fopen(path, "r");
		if (file == nullptr) {
			return false;
		}
		char line[512];
		const size_t prefix_length = std::strlen(prefix);
		bool found = false;
		while (std::fgets(line, sizeof(line), file) != nullptr) {
			if (std::strncmp(line, prefix, prefix_length) != 0) {
				continue;
			}
			const char* value = line + prefix_length;
			while (*value == ' ' || *value == '\t' || *value == ':') {
				++value;
			}
			copy_text(destination, capacity, value);
			size_t length = std::strlen(destination);
			while (length > 0 && (destination[length - 1] == '\n' || destination[length - 1] == '\r')) {
				destination[--length] = '\0';
			}
			found = true;
			break;
		}
		std::fclose(file);
		return found;
	}

	[[nodiscard]] static bool starts_with_ignore_case(const char* text, const char* prefix) noexcept {
		for (; *prefix != '\0'; ++text, ++prefix) {
			if (*text == '\0' || std::tolower(static_cast<unsigned char>(*text)) != std::tolower(static_cast<unsigned char>(*prefix))) {
				return false;
			}
		}
		return true;
	}

	static void publish_platform_environment() {
		char text[480];

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
		unsigned int leaf0[4]{};
		unsigned int leaf1[4]{};
		unsigned int leaf7[4]{};
		query_cpuid(0U, 0U, leaf0);
		char vendor[13]{};
		std::memcpy(vendor, &leaf0[1], 4);
		std::memcpy(vendor + 4, &leaf0[3], 4);
		std::memcpy(vendor + 8, &leaf0[2], 4);
		set_environment("cpu.vendor", vendor);
		query_cpuid(1U, 0U, leaf1);
		if (leaf0[0] >= 7U) {
			query_cpuid(7U, 0U, leaf7);
		}
		struct FeatureBit {
			const char* name;
			int leaf;
			size_t lane;
			unsigned int bit;
		};
		static constexpr FeatureBit kFeatures[] = {
			{"sse2", 1, 3, 26}, {"sse3", 1, 2, 0}, {"ssse3", 1, 2, 9}, {"sse4.1", 1, 2, 19}, {"sse4.2", 1, 2, 20},
			{"popcnt", 1, 2, 23}, {"aes", 1, 2, 25}, {"fma", 1, 2, 12}, {"avx", 1, 2, 28}, {"f16c", 1, 2, 29},
			{"bmi1", 7, 1, 3}, {"avx2", 7, 1, 5}, {"bmi2", 7, 1, 8}, {"avx512f", 7, 1, 16}, {"avx512dq", 7, 1, 17},
			{"avx512cd", 7, 1, 28}, {"avx512bw", 7, 1, 30}, {"avx512vl", 7, 1, 31}
		};
		std::string features;
		for (const FeatureBit& feature : kFeatures) {
			const unsigned int* source = (feature.leaf == 1) ? leaf1 : leaf7;
			if (((source[feature.lane] >> feature.bit) & 1U) != 0U) {
				if (!features.empty()) {
					features.push_back(' ');
				}
				features += feature.name;
			}
		}
		set_environment("cpu.features", features);
#elif defined(__aarch64__) || defined(_M_ARM64)
		set_environment("cpu.features", "arm64 (NEON assumed)");
#endif

		format_wall_clock(text, sizeof(text), "%Z (UTC%z)", std::time(nullptr));
		set_environment("os.timezone", text);

#if defined(_WIN32)
		{
			DWORD length = 0;
			GetLogicalProcessorInformation(nullptr, &length);
			if (length >= sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)) {
				std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> info(length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
				if (GetLogicalProcessorInformation(info.data(), &length) != FALSE) {
					unsigned int cores = 0;
					unsigned int packages = 0;
					unsigned int nodes = 0;
					struct CacheEntry {
						unsigned int level;
						unsigned int type;
						unsigned long long size;
						unsigned int count;
					};
					std::vector<CacheEntry> caches;
					for (const auto& entry : info) {
						switch (entry.Relationship) {
							case RelationProcessorCore:
								++cores;
								break;
							case RelationProcessorPackage:
								++packages;
								break;
							case RelationNumaNode:
								++nodes;
								break;
							case RelationCache: {
								bool merged = false;
								for (CacheEntry& cache : caches) {
									if (cache.level == entry.Cache.Level && cache.type == static_cast<unsigned int>(entry.Cache.Type) && cache.size == entry.Cache.Size) {
										++cache.count;
										merged = true;
										break;
									}
								}
								if (!merged) {
									caches.push_back(CacheEntry{entry.Cache.Level, static_cast<unsigned int>(entry.Cache.Type), entry.Cache.Size, 1U});
								}
								break;
							}
							default:
								break;
						}
					}
					set_environment("cpu.physical.cores", std::to_string(cores));
					set_environment("cpu.packages", std::to_string(packages));
					set_environment("cpu.numa.nodes", std::to_string(nodes));
					static constexpr const char* kCacheTypes[] = {"unified", "instruction", "data", "trace"};
					std::string description;
					for (const CacheEntry& cache : caches) {
						if (!description.empty()) {
							description += "; ";
						}
						std::snprintf(text, sizeof(text), "L%u %s %llu KiB x%u", cache.level, kCacheTypes[cache.type < 4U ? cache.type : 0U], cache.size / 1024ULL, cache.count);
						description += text;
					}
					set_environment("cpu.cache", description);
				}
			}
		}
		{
			const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
			using WineVersionFunction = const char* (*)();
			WineVersionFunction wine = nullptr;
			if (ntdll != nullptr) {
				wine = reinterpret_cast<WineVersionFunction>(reinterpret_cast<void*>(GetProcAddress(ntdll, "wine_get_version")));
			}
			set_environment("os.compat.layer", (wine != nullptr) ? (std::string("Wine ") + wine()) : std::string("none"));
		}
		{
			HANDLE token = nullptr;
			if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token) != FALSE) {
				TOKEN_ELEVATION elevation{};
				DWORD size = 0;
				if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size) != FALSE) {
					set_environment("process.elevated", (elevation.TokenIsElevated != 0) ? "yes" : "no");
				}
				CloseHandle(token);
			}
		}
		{
			wchar_t locale[LOCALE_NAME_MAX_LENGTH]{};
			if (GetUserDefaultLocaleName(locale, LOCALE_NAME_MAX_LENGTH) > 0) {
				set_environment("os.locale", narrow_string(locale));
			}
		}
		{
			MEMORYSTATUSEX status{};
			status.dwLength = sizeof(status);
			if (GlobalMemoryStatusEx(&status) != FALSE) {
				std::snprintf(text, sizeof(text), "%.2f GiB physical, %.2f GiB commit limit", static_cast<double>(status.ullTotalPhys) / 1073741824.0, static_cast<double>(status.ullTotalPageFile) / 1073741824.0);
				set_environment("memory.total", text);
			}
		}
		{
			DISPLAY_DEVICEW device{};
			device.cb = sizeof(device);
			for (DWORD index = 0; index < 16 && EnumDisplayDevicesW(nullptr, index, &device, 0) != FALSE; ++index) {
				char key[40];
				std::snprintf(key, sizeof(key), "display.adapter.%lu", static_cast<unsigned long>(index));
				std::snprintf(text, sizeof(text), "%.120s | %.200s | state 0x%lX", narrow_string(device.DeviceString).c_str(), narrow_string(device.DeviceID).c_str(), static_cast<unsigned long>(device.StateFlags));
				set_environment(key, text);
				device = DISPLAY_DEVICEW{};
				device.cb = sizeof(device);
			}
		}
#else
		{
			struct utsname name{};
			if (::uname(&name) == 0) {
				std::snprintf(text, sizeof(text), "%s %s %s %s", name.sysname, name.release, name.version, name.machine);
				set_environment("os.kernel", text);
			}
		}
		if (read_first_match("/etc/os-release", "PRETTY_NAME=", text, sizeof(text))) {
			set_environment("os.distribution", text);
		}
#if defined(__GLIBC__)
		set_environment("os.libc", std::string("glibc ") + gnu_get_libc_version());
#endif
		if (read_first_match("/proc/cpuinfo", "cpu cores", text, sizeof(text))) {
			set_environment("cpu.physical.cores", text);
		}
		if (read_first_match("/proc/meminfo", "MemTotal:", text, sizeof(text))) {
			set_environment("memory.total", text);
		}
		{
			std::string caches;
			for (int index = 0; index < 8; ++index) {
				char path[112];
				char level[16];
				char type[32];
				char size[32];
				std::snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu0/cache/index%d/level", index);
				if (!read_text_file(path, level, sizeof(level))) {
					break;
				}
				std::snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu0/cache/index%d/type", index);
				static_cast<void>(read_text_file(path, type, sizeof(type)));
				std::snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu0/cache/index%d/size", index);
				static_cast<void>(read_text_file(path, size, sizeof(size)));
				if (!caches.empty()) {
					caches += "; ";
				}
				caches += "L";
				caches += level;
				caches += ' ';
				caches += type;
				caches += ' ';
				caches += size;
			}
			if (!caches.empty()) {
				set_environment("cpu.cache", caches);
			}
		}
		if (read_text_file("/sys/fs/cgroup/memory.max", text, sizeof(text))) {
			set_environment("cgroup.memory.max", text);
		}
		if (read_text_file("/sys/fs/cgroup/cpu.max", text, sizeof(text))) {
			set_environment("cgroup.cpu.max", text);
		}
		set_environment("container.detected", (::access("/.dockerenv", F_OK) == 0 || ::access("/run/.containerenv", F_OK) == 0) ? "yes" : "no");
		if (read_text_file("/proc/driver/nvidia/version", text, sizeof(text))) {
			set_environment("gpu.nvidia.kernel.driver", text);
		}
		const auto publish_limit = [](const char* key, auto resource) {
			struct rlimit limit{};
			if (::getrlimit(resource, &limit) == 0) {
				if (limit.rlim_cur == RLIM_INFINITY) {
					set_environment(key, "unlimited");
				} else {
					set_environment(key, std::to_string(static_cast<unsigned long long>(limit.rlim_cur)));
				}
			}
		};
		publish_limit("limit.open.files", RLIMIT_NOFILE);
		publish_limit("limit.stack.bytes", RLIMIT_STACK);
		publish_limit("limit.core.bytes", RLIMIT_CORE);
		publish_limit("limit.address.space", RLIMIT_AS);
#endif
	}

	static void publish_environment() noexcept {
		try {
			publish_platform_environment();
		} catch (...) {
		}
	}

	static void write_environment_variables(std::FILE* out) noexcept {
#if defined(_WIN32)
		char** variables = _environ;
#else
		char** variables = environ;
#endif
		static constexpr const char* kPrefixes[] = {
			"PATH=", "VK_", "VULKAN", "MESA", "__GL", "__NV", "__VK", "GLFW", "DXVK", "VKD3D", "LD_", "XDG_", "WAYLAND",
			"DISPLAY=", "SDL_", "NVIDIA", "AMD", "RADV", "INTEL", "PROCESSOR_", "NUMBER_OF_PROCESSORS", "OS=", "LANG",
			"LC_", "TZ=", "DRI_", "ENABLE_", "WINE", "PROTON", "STEAM", "GTK_", "QT_", "OMP_", "MKL_", "SESSIONNAME"
		};
		size_t omitted = 0;
		for (char** variable = variables; variable != nullptr && *variable != nullptr; ++variable) {
			bool included = false;
			for (const char* prefix : kPrefixes) {
				if (starts_with_ignore_case(*variable, prefix)) {
					included = true;
					break;
				}
			}
			if (included) {
				std::fprintf(out, "  %.400s\n", *variable);
			} else {
				++omitted;
			}
		}
		std::fprintf(out, "  (%zu other variables omitted)\n", omitted);
	}

	static void write_extended(std::FILE* out) noexcept {
		try {
			std::fprintf(out, "\n---- System state ----\n");
#if defined(_WIN32)
			std::fprintf(out, "  System uptime        %.2f hours\n", static_cast<double>(GetTickCount64()) / 3.6e6);
			SYSTEM_POWER_STATUS power{};
			if (GetSystemPowerStatus(&power) != FALSE) {
				std::fprintf(out, "  Power                AC line %s, battery %d%% (flags 0x%02X)\n", (power.ACLineStatus == 1) ? "online" : ((power.ACLineStatus == 0) ? "offline" : "unknown"), (power.BatteryLifePercent == 255) ? -1 : static_cast<int>(power.BatteryLifePercent), static_cast<unsigned>(power.BatteryFlag));
			}
			DWORD_PTR process_affinity = 0;
			DWORD_PTR system_affinity = 0;
			if (GetProcessAffinityMask(GetCurrentProcess(), &process_affinity, &system_affinity) != FALSE) {
				std::fprintf(out, "  Affinity mask        process 0x%llX, system 0x%llX\n", static_cast<unsigned long long>(process_affinity), static_cast<unsigned long long>(system_affinity));
			}
			std::fprintf(out, "  Priority class       0x%lX\n", static_cast<unsigned long>(GetPriorityClass(GetCurrentProcess())));
#else
			char value[256];
			if (read_text_file("/proc/uptime", value, sizeof(value))) {
				std::fprintf(out, "  System uptime        %s (seconds, idle seconds)\n", value);
			}
			if (read_text_file("/proc/loadavg", value, sizeof(value))) {
				std::fprintf(out, "  Load average         %s\n", value);
			}
			if (read_text_file("/proc/pressure/memory", value, sizeof(value))) {
				std::fprintf(out, "  Memory pressure      %s\n", value);
			}
#endif
			std::error_code ec;
			const auto report_space = [&](const char* label, const std::filesystem::path& path) {
				if (path.empty()) {
					return;
				}
				const auto info = std::filesystem::space(path, ec);
				if (!ec) {
					std::fprintf(out, "  %-20s %.2f GiB free of %.2f GiB (%s)\n", label, static_cast<double>(info.available) / 1073741824.0, static_cast<double>(info.capacity) / 1073741824.0, path.string().c_str());
				}
				ec.clear();
			};
			report_space("Working directory", std::filesystem::current_path(ec));
			ec.clear();
			report_space("Temporary files", std::filesystem::temp_directory_path(ec));
			ec.clear();
			char logs[512];
			if (copy_environment_value("logs.directory", logs, sizeof(logs)) && logs[0] != '\0') {
				report_space("Logs directory", std::filesystem::path(logs));
			}
			std::fprintf(out, "\n---- Relevant environment variables ----\n");
			write_environment_variables(out);
		} catch (...) {
		}
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
