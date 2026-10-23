#pragma once

#include "relativistic/core/diagnostics/diagnostics_registry.hpp"
#include "relativistic/core/diagnostics/system_report.hpp"
#include "relativistic/core/engine_log.hpp"
#include <array>
#include <atomic>
#include <cctype>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <string>
#include <typeinfo>

#if defined(_WIN32)
#include <dbghelp.h>
#else
#include <execinfo.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ucontext.h>
#include <unistd.h>
#endif

namespace Relativistic::Core::Diagnostics {

struct CrashConfiguration {
	std::string product_name{};
	std::string product_version{};
	std::string session_id{};
	std::string build_description{};
	std::string executable_path{};
	std::string working_directory{};
	std::string command_line{};
	std::string crashes_directory{};
	std::string session_log_path{};
	std::string settings_path{};
	std::string marker_path{};
};

namespace CrashDetail {

inline constexpr size_t kPathCapacity = 1024;
inline constexpr size_t kTextCapacity = 256;
inline constexpr uint32_t kMaximumReports = 4;

struct State {
	std::array<char, kTextCapacity> product_name{};
	std::array<char, kTextCapacity> product_version{};
	std::array<char, kTextCapacity> session_id{};
	std::array<char, 1024> build_description{};
	std::array<char, kPathCapacity> executable_path{};
	std::array<char, kPathCapacity> working_directory{};
	std::array<char, 2048> command_line{};
	std::array<char, kPathCapacity> crashes_directory{};
	std::array<char, kPathCapacity> session_log_path{};
	std::array<char, kPathCapacity> settings_path{};
	std::array<char, kPathCapacity> marker_path{};
	std::array<char, kPathCapacity> last_report_folder{};
	std::array<char, kTextCapacity> manual_reason{};
	std::array<char, 2048> manual_detail{};
	std::atomic<uint32_t> reports_written{0};
	std::atomic<bool> report_in_progress{false};
	std::atomic<bool> terminate_in_progress{false};
	std::atomic<bool> dialog_shown{false};
	std::atomic<unsigned long long> last_code{0};
	std::atomic<unsigned long long> last_address{0};
};

inline State g_state{};
inline std::array<char, 65536> g_copy_buffer{};

inline void copy_file_tail(const char* source, const char* destination, size_t maximum_bytes) noexcept {
	if (source[0] == '\0') {
		return;
	}
	std::FILE* input = std::fopen(source, "rb");
	if (input == nullptr) {
		return;
	}
	std::FILE* output = std::fopen(destination, "wb");
	if (output == nullptr) {
		std::fclose(input);
		return;
	}
	if (std::fseek(input, 0, SEEK_END) == 0) {
		const long size = std::ftell(input);
		if (size > 0 && static_cast<size_t>(size) > maximum_bytes) {
			std::fseek(input, size - static_cast<long>(maximum_bytes), SEEK_SET);
		} else {
			std::fseek(input, 0, SEEK_SET);
		}
	}
	size_t read = 0;
	while ((read = std::fread(g_copy_buffer.data(), 1, g_copy_buffer.size(), input)) > 0) {
		std::fwrite(g_copy_buffer.data(), 1, read, output);
	}
	std::fclose(output);
	std::fclose(input);
}

inline void write_section(std::FILE* out, const char* title) noexcept {
	std::fprintf(out, "\n---- %s ----\n", title);
}

inline void write_report_header(std::FILE* out, uint32_t index, const char* source, const char* stamp, bool first_chance) noexcept {
	std::fprintf(out, "==== %s CRASH REPORT ====\n", g_state.product_name.data());
	std::fprintf(out, "Report number in this process : %u\n", index);
	std::fprintf(out, "Captured                      : %s (local time)\n", stamp);
	std::fprintf(out, "Detection source              : %s\n", source);
	if (first_chance) {
		std::fprintf(out, "Capture stage                 : first chance. If the application kept running afterwards, the exception was handled and this report is informational only.\n");
	}
}

inline void write_application_section(std::FILE* out) noexcept {
	write_section(out, "Application");
	std::fprintf(out, "  Product              %s\n", g_state.product_name.data());
	std::fprintf(out, "  Version              %s\n", g_state.product_version.data());
	std::fprintf(out, "  Session id           %s\n", g_state.session_id.data());
	std::fprintf(out, "  Build                %s\n", g_state.build_description.data());
	std::fprintf(out, "  Executable           %s\n", g_state.executable_path.data());
	std::fprintf(out, "  Working directory    %s\n", g_state.working_directory.data());
	std::fprintf(out, "  Command line         %s\n", g_state.command_line.data());
	std::fprintf(out, "  Session log          %s\n", g_state.session_log_path.data());
	std::fprintf(out, "  Settings file        %s\n", g_state.settings_path.data());
}

inline void write_runtime_state_section(std::FILE* out, uint64_t faulting_thread) noexcept {
	write_section(out, "Runtime state");
	std::fprintf(out, "  Phase                %s\n", app_phase_name(current_phase()));
	std::fprintf(out, "  Uptime               %.3f s\n", uptime_seconds());
	std::fprintf(out, "  UI frames            %llu\n", static_cast<unsigned long long>(g_registry.frame_counter.load(std::memory_order_relaxed)));
	std::fprintf(out, "  Commands processed   %llu\n", static_cast<unsigned long long>(g_registry.command_counter.load(std::memory_order_relaxed)));
	write_section(out, "Startup progress");
	write_snapshot(out, g_registry.startup, "no startup state recorded");
	write_section(out, "Environment and hardware");
	write_environment(out);
	write_section(out, "Thread activity at crash time");
	write_activity_table(out, faulting_thread);
	write_section(out, "Simulation state snapshot");
	write_snapshot(out, g_registry.scene, "no simulation snapshot was published yet");
	write_section(out, "Last render dispatch");
	write_snapshot(out, g_registry.render, "no render dispatch was recorded yet");
	write_section(out, "Recent events (most recent last)");
	write_recent_events(out, 300);
	std::fflush(out);
}

inline const char* classify_module(const char* path) noexcept {
	char lowered[CrashDetail::kPathCapacity];
	size_t length = 0;
	for (; path[length] != '\0' && length + 1 < sizeof(lowered); ++length) {
		lowered[length] = static_cast<char>(std::tolower(static_cast<unsigned char>(path[length])));
	}
	lowered[length] = '\0';
	static constexpr const char* graphics[] = {"nvoglv", "nvwgf2", "nvapi", "nvlddmkm", "atio", "amdvlk", "amdxc", "ig9icd", "igvk", "igd", "vulkan-1", "opengl32", "dxgi", "d3d", "win32u", "gdi32", "nvidia", "mesa", "libvulkan", "libgl"};
	for (const char* token : graphics) {
		if (std::strstr(lowered, token) != nullptr) {
			return "graphics stack or display driver";
		}
	}
	static constexpr const char* runtime[] = {"ucrtbase", "msvcrt", "libstdc++", "libgcc", "vcruntime", "libc.so", "libm.so", "ntdll", "kernelbase", "kernel32"};
	for (const char* token : runtime) {
		if (std::strstr(lowered, token) != nullptr) {
			return "C/C++ runtime or operating system";
		}
	}
	return "application or third-party library";
}

inline void write_diagnosis(std::FILE* out, unsigned long long code, int access_kind, unsigned long long access_address, const char* fault_module) noexcept {
	write_section(out, "Automatic diagnosis hints");
	if (fault_module != nullptr && fault_module[0] != '\0') {
		std::fprintf(out, "  Fault location class : %s\n", classify_module(fault_module));
		if (std::strcmp(classify_module(fault_module), "graphics stack or display driver") == 0) {
			std::fprintf(out, "  Hint                 : the fault occurred inside a graphics module. Check the 'Last render dispatch' and 'Thread activity' sections for the operation in flight, and the session log for slow GPU band warnings (display driver reset).\n");
		}
	}
	if (access_kind >= 0) {
		if (access_address < 0x10000ULL) {
			std::fprintf(out, "  Hint                 : the accessed address is in the null page, which indicates a null pointer dereference.\n");
		} else if (access_address > 0x00007FFFFFFFFFFFULL) {
			std::fprintf(out, "  Hint                 : the accessed address is non-canonical, which indicates a corrupted pointer.\n");
		}
	}
	if (code == 0xC00000FDULL) {
		std::fprintf(out, "  Hint                 : stack overflow, usually unbounded recursion or very large stack arrays.\n");
	}
	if (code == 0xC0000374ULL) {
		std::fprintf(out, "  Hint                 : heap corruption detected by the allocator.\n");
	}
	if (code == 0xE0000001ULL) {
		std::fprintf(out, "  Hint                 : fatal condition reported by the application itself (terminate or abort).\n");
	}
}

}

#if defined(_WIN32)

namespace CrashDetail {

inline thread_local bool g_inside_report = false;

struct DebugHelp {
	HMODULE module = nullptr;
	BOOL(WINAPI* initialize)(HANDLE, PCSTR, BOOL) = nullptr;
	DWORD(WINAPI* set_options)(DWORD) = nullptr;
	BOOL(WINAPI* walk)(DWORD, HANDLE, HANDLE, LPSTACKFRAME64, PVOID, PREAD_PROCESS_MEMORY_ROUTINE64, PFUNCTION_TABLE_ACCESS_ROUTINE64, PGET_MODULE_BASE_ROUTINE64, PTRANSLATE_ADDRESS_ROUTINE64) = nullptr;
	PFUNCTION_TABLE_ACCESS_ROUTINE64 function_table = nullptr;
	PGET_MODULE_BASE_ROUTINE64 module_base = nullptr;
	BOOL(WINAPI* symbol_from_address)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO) = nullptr;
	BOOL(WINAPI* line_from_address)(HANDLE, DWORD64, PDWORD, PIMAGEHLP_LINE64) = nullptr;
	BOOL(WINAPI* write_dump)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION, PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION) = nullptr;
};

[[nodiscard]] inline bool load_debug_help(DebugHelp& api) noexcept {
	api.module = LoadLibraryW(L"dbghelp.dll");
	if (api.module == nullptr) {
		return false;
	}
	const auto resolve = [&api](const char* name) noexcept -> void* {
		return reinterpret_cast<void*>(GetProcAddress(api.module, name));
	};
	api.initialize = reinterpret_cast<decltype(api.initialize)>(resolve("SymInitialize"));
	api.set_options = reinterpret_cast<decltype(api.set_options)>(resolve("SymSetOptions"));
	api.walk = reinterpret_cast<decltype(api.walk)>(resolve("StackWalk64"));
	api.function_table = reinterpret_cast<PFUNCTION_TABLE_ACCESS_ROUTINE64>(resolve("SymFunctionTableAccess64"));
	api.module_base = reinterpret_cast<PGET_MODULE_BASE_ROUTINE64>(resolve("SymGetModuleBase64"));
	api.symbol_from_address = reinterpret_cast<decltype(api.symbol_from_address)>(resolve("SymFromAddr"));
	api.line_from_address = reinterpret_cast<decltype(api.line_from_address)>(resolve("SymGetLineFromAddr64"));
	api.write_dump = reinterpret_cast<decltype(api.write_dump)>(resolve("MiniDumpWriteDump"));
	if (api.initialize != nullptr) {
		if (api.set_options != nullptr) {
			api.set_options(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
		}
		api.initialize(GetCurrentProcess(), nullptr, TRUE);
	}
	return true;
}

[[nodiscard]] inline const char* exception_code_name(unsigned long code) noexcept {
	switch (code) {
		case 0xC0000005UL: return "EXCEPTION_ACCESS_VIOLATION";
		case 0xC00000FDUL: return "EXCEPTION_STACK_OVERFLOW";
		case 0xC000001DUL: return "EXCEPTION_ILLEGAL_INSTRUCTION";
		case 0xC000008CUL: return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
		case 0xC0000094UL: return "EXCEPTION_INT_DIVIDE_BY_ZERO";
		case 0xC0000096UL: return "EXCEPTION_PRIV_INSTRUCTION";
		case 0xC0000006UL: return "EXCEPTION_IN_PAGE_ERROR";
		case 0x80000002UL: return "EXCEPTION_DATATYPE_MISALIGNMENT";
		case 0xC0000374UL: return "STATUS_HEAP_CORRUPTION";
		case 0xC0000409UL: return "STATUS_STACK_BUFFER_OVERRUN";
		case 0xC000008EUL: return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
		case 0xC0000090UL: return "EXCEPTION_FLT_INVALID_OPERATION";
		case 0xE0000001UL: return "APPLICATION_FATAL_CONDITION";
		default: return "UNKNOWN_EXCEPTION";
	}
}

[[nodiscard]] inline bool is_fatal_exception(unsigned long code) noexcept {
	switch (code) {
		case 0xC0000005UL:
		case 0xC00000FDUL:
		case 0xC000001DUL:
		case 0xC000008CUL:
		case 0xC0000094UL:
		case 0xC0000096UL:
		case 0xC0000006UL:
		case 0xC0000374UL:
			return true;
		default:
			return false;
	}
}

[[nodiscard]] inline bool is_readable(const void* address, size_t size) noexcept {
	MEMORY_BASIC_INFORMATION info{};
	if (VirtualQuery(address, &info, sizeof(info)) == 0) {
		return false;
	}
	if (info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) {
		return false;
	}
	const uintptr_t end = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
	return reinterpret_cast<uintptr_t>(address) + size <= end;
}

[[nodiscard]] inline bool locate_module(uintptr_t address, char* path, size_t capacity, uintptr_t& base) noexcept {
	path[0] = '\0';
	HMODULE module = nullptr;
	if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(address), &module) == FALSE || module == nullptr) {
		return false;
	}
	wchar_t wide[kPathCapacity];
	const DWORD length = GetModuleFileNameW(module, wide, static_cast<DWORD>(kPathCapacity));
	if (length == 0 || length >= kPathCapacity) {
		return false;
	}
	const int written = WideCharToMultiByte(CP_UTF8, 0, wide, -1, path, static_cast<int>(capacity), nullptr, nullptr);
	if (written <= 0) {
		path[0] = '\0';
		return false;
	}
	base = reinterpret_cast<uintptr_t>(module);
	return true;
}

inline void write_frame(std::FILE* out, const DebugHelp& api, HANDLE process, int index, uint64_t address) noexcept {
	char module_path[kPathCapacity];
	uintptr_t base = 0;
	const bool located = locate_module(static_cast<uintptr_t>(address), module_path, sizeof(module_path), base);
	const char* file_name = module_path;
	for (const char* cursor = module_path; *cursor != '\0'; ++cursor) {
		if (*cursor == '\\' || *cursor == '/') {
			file_name = cursor + 1;
		}
	}
	std::fprintf(out, "  #%02d 0x%016llX  ", index, static_cast<unsigned long long>(address));
	if (located) {
		std::fprintf(out, "%s+0x%llX", file_name, static_cast<unsigned long long>(address - base));
	} else {
		std::fprintf(out, "<unknown module>");
	}
	if (api.symbol_from_address != nullptr) {
		alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + 256]{};
		SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol->MaxNameLen = 255;
		DWORD64 displacement = 0;
		if (api.symbol_from_address(process, address, &displacement, symbol) != FALSE) {
			std::fprintf(out, "  %s+0x%llX", symbol->Name, static_cast<unsigned long long>(displacement));
		}
	}
	if (api.line_from_address != nullptr) {
		IMAGEHLP_LINE64 line{};
		line.SizeOfStruct = sizeof(line);
		DWORD line_displacement = 0;
		if (api.line_from_address(process, address, &line_displacement, &line) != FALSE) {
			std::fprintf(out, "  (%s:%lu)", line.FileName, static_cast<unsigned long>(line.LineNumber));
		}
	}
	std::fprintf(out, "\n");
}

inline int write_stack_walk(std::FILE* out, const DebugHelp& api, CONTEXT context) noexcept {
	int printed = 0;
#if defined(_M_X64) || defined(__x86_64__)
	if (api.walk == nullptr) {
		return 0;
	}
	STACKFRAME64 frame{};
	frame.AddrPC.Offset = context.Rip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = context.Rbp;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Offset = context.Rsp;
	frame.AddrStack.Mode = AddrModeFlat;
	const HANDLE process = GetCurrentProcess();
	const HANDLE thread = GetCurrentThread();
	for (int index = 0; index < 96; ++index) {
		if (api.walk(IMAGE_FILE_MACHINE_AMD64, process, thread, &frame, &context, nullptr, api.function_table, api.module_base, nullptr) == FALSE) {
			break;
		}
		if (frame.AddrPC.Offset == 0) {
			break;
		}
		write_frame(out, api, process, index, frame.AddrPC.Offset);
		++printed;
	}
#else
	static_cast<void>(out);
	static_cast<void>(api);
	static_cast<void>(context);
#endif
	return printed;
}

inline void write_registers(std::FILE* out, const CONTEXT& context) noexcept {
#if defined(_M_X64) || defined(__x86_64__)
	const auto value = [](DWORD64 register_value) noexcept { return static_cast<unsigned long long>(register_value); };
	std::fprintf(out, "  RAX=%016llX RBX=%016llX RCX=%016llX RDX=%016llX\n", value(context.Rax), value(context.Rbx), value(context.Rcx), value(context.Rdx));
	std::fprintf(out, "  RSI=%016llX RDI=%016llX RBP=%016llX RSP=%016llX\n", value(context.Rsi), value(context.Rdi), value(context.Rbp), value(context.Rsp));
	std::fprintf(out, "  R8 =%016llX R9 =%016llX R10=%016llX R11=%016llX\n", value(context.R8), value(context.R9), value(context.R10), value(context.R11));
	std::fprintf(out, "  R12=%016llX R13=%016llX R14=%016llX R15=%016llX\n", value(context.R12), value(context.R13), value(context.R14), value(context.R15));
	std::fprintf(out, "  RIP=%016llX EFLAGS=%08lX MXCSR=%08lX CS=%04X SS=%04X\n", value(context.Rip), static_cast<unsigned long>(context.EFlags), static_cast<unsigned long>(context.MxCsr), static_cast<unsigned>(context.SegCs), static_cast<unsigned>(context.SegSs));
	const void* code = reinterpret_cast<const void*>(static_cast<uintptr_t>(context.Rip));
	if (is_readable(code, 16)) {
		const unsigned char* bytes = static_cast<const unsigned char*>(code);
		std::fprintf(out, "  Bytes at RIP:");
		for (size_t index = 0; index < 16; ++index) {
			std::fprintf(out, " %02X", static_cast<unsigned>(bytes[index]));
		}
		std::fprintf(out, "\n");
	} else {
		std::fprintf(out, "  Bytes at RIP: <unreadable>\n");
	}
	const void* stack = reinterpret_cast<const void*>(static_cast<uintptr_t>(context.Rsp));
	if (is_readable(stack, 8 * 24)) {
		const uint64_t* words = static_cast<const uint64_t*>(stack);
		std::fprintf(out, "  Stack words at RSP:\n");
		for (size_t index = 0; index < 24; index += 2) {
			std::fprintf(out, "    +%03zX  %016llX  %016llX\n", index * 8, static_cast<unsigned long long>(words[index]), static_cast<unsigned long long>(words[index + 1]));
		}
	}
#else
	static_cast<void>(out);
	static_cast<void>(context);
	std::fprintf(out, "  (register dump is available on x86-64 only)\n");
#endif
}

struct WindowsReportRequest {
	EXCEPTION_POINTERS* pointers;
	DWORD crashing_thread_id;
	const char* source;
	bool first_chance;
};

inline void write_windows_report(const WindowsReportRequest& request) noexcept {
	g_inside_report = true;
	const EXCEPTION_RECORD* record = request.pointers->ExceptionRecord;
	const CONTEXT* context = request.pointers->ContextRecord;
	const unsigned long long code = static_cast<unsigned long long>(record->ExceptionCode);
	const unsigned long long address = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(record->ExceptionAddress));

	if (g_state.reports_written.load(std::memory_order_acquire) > 0 && g_state.last_code.load() == code && g_state.last_address.load() == address) {
		return;
	}
	if (g_state.reports_written.load(std::memory_order_acquire) >= kMaximumReports) {
		return;
	}
	bool expected = false;
	if (!g_state.report_in_progress.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
		return;
	}

	const uint32_t index = g_state.reports_written.load(std::memory_order_acquire) + 1;
	char stamp[48];
	format_wall_clock(stamp, sizeof(stamp), "%Y-%m-%d_%H-%M-%S", std::time(nullptr));
	char readable_stamp[48];
	format_wall_clock(readable_stamp, sizeof(readable_stamp), "%Y-%m-%d %H:%M:%S", std::time(nullptr));

	char folder[kPathCapacity];
	std::snprintf(folder, sizeof(folder), "%s/crash_%s_%08llX_pid%llu_n%u", g_state.crashes_directory.data(), stamp, code & 0xFFFFFFFFULL, static_cast<unsigned long long>(current_process_id()), index);
	CreateDirectoryA(folder, nullptr);
	char report_path[kPathCapacity];
	std::snprintf(report_path, sizeof(report_path), "%s/report.txt", folder);
	std::FILE* out = std::fopen(report_path, "wb");
	if (out == nullptr) {
		out = stderr;
	}

	write_report_header(out, index, request.source, readable_stamp, request.first_chance);

	write_section(out, "Exception");
	std::fprintf(out, "  Code                 0x%08llX (%s)\n", code & 0xFFFFFFFFULL, exception_code_name(static_cast<unsigned long>(code)));
	std::fprintf(out, "  Flags                0x%08lX%s\n", static_cast<unsigned long>(record->ExceptionFlags), (record->ExceptionFlags & EXCEPTION_NONCONTINUABLE) != 0 ? " (non-continuable)" : "");
	std::fprintf(out, "  Address              0x%016llX\n", address);
	std::fprintf(out, "  Faulting thread id   %lu\n", static_cast<unsigned long>(request.crashing_thread_id));

	char fault_module[kPathCapacity];
	uintptr_t fault_base = 0;
	if (locate_module(static_cast<uintptr_t>(address), fault_module, sizeof(fault_module), fault_base)) {
		std::fprintf(out, "  Faulting module      %s\n", fault_module);
		std::fprintf(out, "  Module base          0x%016llX\n", static_cast<unsigned long long>(fault_base));
		std::fprintf(out, "  Offset in module     0x%llX (use: addr2line -f -C -e <module> 0x%llX)\n", static_cast<unsigned long long>(address - fault_base), static_cast<unsigned long long>(address - fault_base));
	} else {
		std::fprintf(out, "  Faulting module      <not inside a loaded module (possibly JIT, trampoline or corrupted instruction pointer)>\n");
	}

	int access_kind = -1;
	unsigned long long access_address = 0;
	if ((code == 0xC0000005ULL || code == 0xC0000006ULL) && record->NumberParameters >= 2) {
		access_kind = static_cast<int>(record->ExceptionInformation[0]);
		access_address = static_cast<unsigned long long>(record->ExceptionInformation[1]);
		const char* kind_text = (access_kind == 0) ? "read" : ((access_kind == 1) ? "write" : ((access_kind == 8) ? "execute (DEP)" : "unknown"));
		std::fprintf(out, "  Memory access        %s at 0x%016llX\n", kind_text, access_address);
		MEMORY_BASIC_INFORMATION info{};
		if (VirtualQuery(reinterpret_cast<LPCVOID>(static_cast<uintptr_t>(access_address)), &info, sizeof(info)) != 0) {
			const char* state = (info.State == MEM_COMMIT) ? "committed" : ((info.State == MEM_RESERVE) ? "reserved only" : "free (unmapped)");
			std::fprintf(out, "  Target region        base=0x%016llX size=%llu state=%s protect=0x%lX type=0x%lX allocation_base=0x%016llX\n",
				static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(info.BaseAddress)), static_cast<unsigned long long>(info.RegionSize), state,
				static_cast<unsigned long>(info.Protect), static_cast<unsigned long>(info.Type), static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(info.AllocationBase)));
		}
		if (record->NumberParameters >= 3) {
			std::fprintf(out, "  NTSTATUS (page err)  0x%08llX\n", static_cast<unsigned long long>(record->ExceptionInformation[2]));
		}
	}
	if (code == 0xE0000001ULL) {
		std::fprintf(out, "  Reason               %s\n", g_state.manual_reason.data());
		std::fprintf(out, "  Detail               %s\n", g_state.manual_detail.data());
	}
	int chain = 0;
	for (const EXCEPTION_RECORD* nested = record->ExceptionRecord; nested != nullptr && chain < 4; nested = nested->ExceptionRecord, ++chain) {
		std::fprintf(out, "  Nested exception     0x%08lX at 0x%016llX\n", static_cast<unsigned long>(nested->ExceptionCode), static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(nested->ExceptionAddress)));
	}
	std::fflush(out);

	write_section(out, "CPU registers of the faulting thread");
	write_registers(out, *context);
	std::fflush(out);

	DebugHelp api;
	const bool have_debug_help = load_debug_help(api);
	write_section(out, "Call stack of the faulting thread");
	const int frames = write_stack_walk(out, api, *context);
	if (frames == 0) {
		std::fprintf(out, "  (stack walk produced no frames; raw backtrace of the reporting thread follows)\n");
		void* raw[62]{};
		const USHORT captured = CaptureStackBackTrace(0, 62, raw, nullptr);
		for (USHORT frame_index = 0; frame_index < captured; ++frame_index) {
			write_frame(out, api, GetCurrentProcess(), static_cast<int>(frame_index), static_cast<uint64_t>(reinterpret_cast<uintptr_t>(raw[frame_index])));
		}
	}
	std::fflush(out);

	write_diagnosis(out, code, access_kind, access_address, fault_module);
	write_application_section(out);
	std::fflush(out);
	write_runtime_state_section(out, static_cast<uint64_t>(request.crashing_thread_id));

	write_section(out, "Operating system");
	SystemReport::write_operating_system(out);
	write_section(out, "Processor");
	SystemReport::write_processor(out);
	write_section(out, "Memory");
	SystemReport::write_memory(out);
	write_section(out, "Process");
	SystemReport::write_process(out);
	write_section(out, "Threads");
	SystemReport::write_threads(out, static_cast<uint64_t>(request.crashing_thread_id));
	write_section(out, "Loaded modules");
	SystemReport::write_modules(out);
	write_section(out, "Attachments");
	std::fprintf(out, "  minidump.dmp         process minidump (open with WinDbg or Visual Studio)\n");
	std::fprintf(out, "  session.log          tail of the session log written before the crash\n");
	std::fprintf(out, "  user_settings.cfg    user settings in effect during the crash\n");
	std::fprintf(out, "\n==== END OF REPORT ====\n");
	std::fflush(out);
	if (out != stderr) {
		std::fclose(out);
	}

	char attachment[kPathCapacity];
	std::snprintf(attachment, sizeof(attachment), "%s/session.log", folder);
	copy_file_tail(g_state.session_log_path.data(), attachment, 16U * 1024U * 1024U);
	std::snprintf(attachment, sizeof(attachment), "%s/user_settings.cfg", folder);
	copy_file_tail(g_state.settings_path.data(), attachment, 1024U * 1024U);

	if (have_debug_help && api.write_dump != nullptr) {
		std::snprintf(attachment, sizeof(attachment), "%s/minidump.dmp", folder);
		const HANDLE dump_file = CreateFileA(attachment, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (dump_file != INVALID_HANDLE_VALUE) {
			MINIDUMP_EXCEPTION_INFORMATION information{};
			information.ThreadId = request.crashing_thread_id;
			information.ExceptionPointers = request.pointers;
			information.ClientPointers = FALSE;
			const MINIDUMP_TYPE type = static_cast<MINIDUMP_TYPE>(MiniDumpWithDataSegs | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules | MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithHandleData);
			api.write_dump(GetCurrentProcess(), GetCurrentProcessId(), dump_file, type, &information, nullptr, nullptr);
			CloseHandle(dump_file);
		}
	}

	copy_text(g_state.last_report_folder.data(), kPathCapacity, folder);
	if (g_state.marker_path[0] != '\0') {
		std::FILE* marker = std::fopen(g_state.marker_path.data(), "wb");
		if (marker != nullptr) {
			std::fprintf(marker, "%s\n", folder);
			std::fclose(marker);
		}
	}
	std::fprintf(stderr, "[FATAL] Crash report written to %s\n", folder);
	g_state.last_code.store(code);
	g_state.last_address.store(address);
	g_state.reports_written.fetch_add(1, std::memory_order_acq_rel);
	g_state.report_in_progress.store(false, std::memory_order_release);
}

inline DWORD WINAPI report_thread_entry(LPVOID parameter) noexcept {
	write_windows_report(*static_cast<WindowsReportRequest*>(parameter));
	return 0;
}

inline void dispatch_windows_report(EXCEPTION_POINTERS* pointers, const char* source, bool first_chance) noexcept {
	if (g_inside_report) {
		return;
	}
	for (int attempt = 0; attempt < 2400 && g_state.report_in_progress.load(std::memory_order_acquire); ++attempt) {
		Sleep(25);
	}
	WindowsReportRequest request{pointers, GetCurrentThreadId(), source, first_chance};
	DWORD thread_id = 0;
	const HANDLE worker = CreateThread(nullptr, 2U * 1024U * 1024U, report_thread_entry, &request, 0, &thread_id);
	if (worker == nullptr) {
		write_windows_report(request);
		g_inside_report = false;
		return;
	}
	WaitForSingleObject(worker, 90000);
	CloseHandle(worker);
}

inline void show_crash_dialog() noexcept {
	bool expected = false;
	if (!g_state.dialog_shown.compare_exchange_strong(expected, true)) {
		return;
	}
	char text[2048];
	if (g_state.last_report_folder[0] != '\0') {
		std::snprintf(text, sizeof(text), "%s has crashed.\n\nA detailed crash report was saved to:\n%s\n\nPlease send this folder to the developer.", g_state.product_name.data(), g_state.last_report_folder.data());
	} else {
		std::snprintf(text, sizeof(text), "%s has crashed and no crash report could be written.", g_state.product_name.data());
	}
	MessageBoxA(nullptr, text, "Application crash", MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
}

inline void report_manual_windows(const char* reason, const char* detail) noexcept {
	copy_text(g_state.manual_reason.data(), g_state.manual_reason.size(), reason);
	copy_text(g_state.manual_detail.data(), g_state.manual_detail.size(), detail);
	CONTEXT context{};
	RtlCaptureContext(&context);
	EXCEPTION_RECORD record{};
	record.ExceptionCode = 0xE0000001UL;
#if defined(_M_X64) || defined(__x86_64__)
	record.ExceptionAddress = reinterpret_cast<PVOID>(static_cast<uintptr_t>(context.Rip));
#endif
	EXCEPTION_POINTERS pointers{&record, &context};
	dispatch_windows_report(&pointers, reason, false);
}

inline LONG WINAPI vectored_handler(EXCEPTION_POINTERS* info) noexcept {
	if (info == nullptr || info->ExceptionRecord == nullptr || !is_fatal_exception(static_cast<unsigned long>(info->ExceptionRecord->ExceptionCode))) {
		return EXCEPTION_CONTINUE_SEARCH;
	}
	dispatch_windows_report(info, "vectored exception handler", true);
	return EXCEPTION_CONTINUE_SEARCH;
}

inline LONG WINAPI unhandled_filter(EXCEPTION_POINTERS* info) noexcept {
	if (info != nullptr && info->ExceptionRecord != nullptr) {
		dispatch_windows_report(info, "unhandled exception filter", false);
	}
	show_crash_dialog();
	return EXCEPTION_EXECUTE_HANDLER;
}

inline void describe_active_exception(char* destination, size_t capacity) noexcept {
	copy_text(destination, capacity, "no active exception object");
	try {
		if (const std::exception_ptr active = std::current_exception()) {
			std::rethrow_exception(active);
		}
	} catch (const std::exception& ex) {
		std::snprintf(destination, capacity, "std::exception of type %s: %s", typeid(ex).name(), ex.what());
	} catch (...) {
		copy_text(destination, capacity, "non-standard exception object");
	}
}

inline void terminate_handler() noexcept {
	g_state.terminate_in_progress.store(true);
	char detail[1024];
	describe_active_exception(detail, sizeof(detail));
	try {
		log_error(std::string("std::terminate called: ") + detail);
	} catch (...) {
	}
	report_manual_windows("std::terminate", detail);
	show_crash_dialog();
	std::_Exit(3);
}

inline void abort_handler(int) noexcept {
	if (!g_state.terminate_in_progress.load()) {
		report_manual_windows("abort() or SIGABRT", "The process raised SIGABRT, typically through abort() or a failed assertion.");
	}
	show_crash_dialog();
	std::_Exit(3);
}

}

#else

namespace CrashDetail {

inline std::array<char, 65536> g_alternate_stack{};

[[nodiscard]] inline const char* signal_name(int number) noexcept {
	switch (number) {
		case SIGSEGV: return "SIGSEGV";
		case SIGBUS: return "SIGBUS";
		case SIGFPE: return "SIGFPE";
		case SIGILL: return "SIGILL";
		case SIGABRT: return "SIGABRT";
		default: return "UNKNOWN_SIGNAL";
	}
}

inline void write_posix_report(int number, const siginfo_t* info, const void* context, const char* reason, const char* detail) noexcept {
	const uint32_t index = g_state.reports_written.load() + 1;
	char stamp[48];
	format_wall_clock(stamp, sizeof(stamp), "%Y-%m-%d_%H-%M-%S", std::time(nullptr));
	char readable_stamp[48];
	format_wall_clock(readable_stamp, sizeof(readable_stamp), "%Y-%m-%d %H:%M:%S", std::time(nullptr));
	char folder[kPathCapacity];
	std::snprintf(folder, sizeof(folder), "%s/crash_%s_%s_pid%llu_n%u", g_state.crashes_directory.data(), stamp, signal_name(number), static_cast<unsigned long long>(current_process_id()), index);
	::mkdir(folder, 0755);
	char report_path[kPathCapacity];
	std::snprintf(report_path, sizeof(report_path), "%s/report.txt", folder);
	std::FILE* out = std::fopen(report_path, "wb");
	if (out == nullptr) {
		out = stderr;
	}
	write_report_header(out, index, reason, readable_stamp, false);
	write_section(out, "Signal");
	std::fprintf(out, "  Signal               %d (%s)\n", number, signal_name(number));
	if (info != nullptr) {
		std::fprintf(out, "  Code                 %d\n", info->si_code);
		std::fprintf(out, "  Fault address        %p\n", info->si_addr);
	}
	std::fprintf(out, "  Faulting thread id   %llu\n", static_cast<unsigned long long>(current_thread_id()));
	if (detail != nullptr) {
		std::fprintf(out, "  Detail               %s\n", detail);
	}
#if defined(__linux__) && defined(__x86_64__)
	if (context != nullptr) {
		const ucontext_t* user = static_cast<const ucontext_t*>(context);
		write_section(out, "CPU registers");
		std::fprintf(out, "  RIP=%016llX RSP=%016llX RBP=%016llX\n", static_cast<unsigned long long>(user->uc_mcontext.gregs[REG_RIP]), static_cast<unsigned long long>(user->uc_mcontext.gregs[REG_RSP]), static_cast<unsigned long long>(user->uc_mcontext.gregs[REG_RBP]));
		std::fprintf(out, "  RAX=%016llX RBX=%016llX RCX=%016llX RDX=%016llX\n", static_cast<unsigned long long>(user->uc_mcontext.gregs[REG_RAX]), static_cast<unsigned long long>(user->uc_mcontext.gregs[REG_RBX]), static_cast<unsigned long long>(user->uc_mcontext.gregs[REG_RCX]), static_cast<unsigned long long>(user->uc_mcontext.gregs[REG_RDX]));
	}
#else
	static_cast<void>(context);
#endif
	write_section(out, "Call stack of the faulting thread");
	std::fflush(out);
	void* frames[96];
	const int count = ::backtrace(frames, 96);
	::backtrace_symbols_fd(frames, count, ::fileno(out));
	write_diagnosis(out, 0, -1, 0, nullptr);
	write_application_section(out);
	write_runtime_state_section(out, current_thread_id());
	write_section(out, "Operating system");
	SystemReport::write_operating_system(out);
	write_section(out, "Processor");
	SystemReport::write_processor(out);
	write_section(out, "Memory");
	SystemReport::write_memory(out);
	write_section(out, "Process");
	SystemReport::write_process(out);
	write_section(out, "Memory map");
	SystemReport::write_modules(out);
	std::fprintf(out, "\n==== END OF REPORT ====\n");
	std::fflush(out);
	if (out != stderr) {
		std::fclose(out);
	}
	char attachment[kPathCapacity];
	std::snprintf(attachment, sizeof(attachment), "%s/session.log", folder);
	copy_file_tail(g_state.session_log_path.data(), attachment, 16U * 1024U * 1024U);
	std::snprintf(attachment, sizeof(attachment), "%s/user_settings.cfg", folder);
	copy_file_tail(g_state.settings_path.data(), attachment, 1024U * 1024U);
	copy_text(g_state.last_report_folder.data(), kPathCapacity, folder);
	if (g_state.marker_path[0] != '\0') {
		std::FILE* marker = std::fopen(g_state.marker_path.data(), "wb");
		if (marker != nullptr) {
			std::fprintf(marker, "%s\n", folder);
			std::fclose(marker);
		}
	}
	std::fprintf(stderr, "[FATAL] Crash report written to %s\n", folder);
	g_state.reports_written.fetch_add(1);
}

inline void signal_handler(int number, siginfo_t* info, void* context) {
	bool expected = false;
	if (!g_state.report_in_progress.compare_exchange_strong(expected, true)) {
		::_exit(128 + number);
	}
	if (!g_state.terminate_in_progress.load()) {
		write_posix_report(number, info, context, "POSIX signal handler", nullptr);
	}
	std::signal(number, SIG_DFL);
	::raise(number);
	::_exit(128 + number);
}

inline void terminate_handler() noexcept {
	g_state.terminate_in_progress.store(true);
	char detail[1024];
	copy_text(detail, sizeof(detail), "no active exception object");
	try {
		if (const std::exception_ptr active = std::current_exception()) {
			std::rethrow_exception(active);
		}
	} catch (const std::exception& ex) {
		std::snprintf(detail, sizeof(detail), "std::exception of type %s: %s", typeid(ex).name(), ex.what());
	} catch (...) {
		copy_text(detail, sizeof(detail), "non-standard exception object");
	}
	try {
		log_error(std::string("std::terminate called: ") + detail);
	} catch (...) {
	}
	bool expected = false;
	if (g_state.report_in_progress.compare_exchange_strong(expected, true)) {
		write_posix_report(SIGABRT, nullptr, nullptr, "std::terminate", detail);
	}
	::_exit(3);
}

}

#endif

class CrashReporter {
public:
	static void configure(const CrashConfiguration& configuration) noexcept {
		using namespace CrashDetail;
		copy_text(g_state.product_name.data(), g_state.product_name.size(), configuration.product_name);
		copy_text(g_state.product_version.data(), g_state.product_version.size(), configuration.product_version);
		copy_text(g_state.session_id.data(), g_state.session_id.size(), configuration.session_id);
		copy_text(g_state.build_description.data(), g_state.build_description.size(), configuration.build_description);
		copy_text(g_state.executable_path.data(), g_state.executable_path.size(), configuration.executable_path);
		copy_text(g_state.working_directory.data(), g_state.working_directory.size(), configuration.working_directory);
		copy_text(g_state.command_line.data(), g_state.command_line.size(), configuration.command_line);
		copy_text(g_state.crashes_directory.data(), g_state.crashes_directory.size(), configuration.crashes_directory);
		copy_text(g_state.session_log_path.data(), g_state.session_log_path.size(), configuration.session_log_path);
		copy_text(g_state.settings_path.data(), g_state.settings_path.size(), configuration.settings_path);
		copy_text(g_state.marker_path.data(), g_state.marker_path.size(), configuration.marker_path);
	}

	static void install() noexcept {
#if defined(_WIN32)
		AddVectoredExceptionHandler(1, &CrashDetail::vectored_handler);
		SetUnhandledExceptionFilter(&CrashDetail::unhandled_filter);
		std::signal(SIGABRT, &CrashDetail::abort_handler);
		std::set_terminate(&CrashDetail::terminate_handler);
#else
		stack_t alternate{};
		alternate.ss_sp = CrashDetail::g_alternate_stack.data();
		alternate.ss_size = CrashDetail::g_alternate_stack.size();
		alternate.ss_flags = 0;
		::sigaltstack(&alternate, nullptr);
		struct sigaction action{};
		action.sa_sigaction = &CrashDetail::signal_handler;
		action.sa_flags = SA_SIGINFO | SA_ONSTACK;
		sigemptyset(&action.sa_mask);
		for (const int number : {SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT}) {
			::sigaction(number, &action, nullptr);
		}
		std::set_terminate(&CrashDetail::terminate_handler);
#endif
	}

	static void report_fatal(const char* reason, const char* detail = nullptr) noexcept {
#if defined(_WIN32)
		CrashDetail::report_manual_windows(
			reason != nullptr ? reason : "fatal_condition",
			detail != nullptr ? detail : ""
		);
		CrashDetail::show_crash_dialog();
		std::_Exit(3);
#else
		bool expected = false;
		if (CrashDetail::g_state.report_in_progress.compare_exchange_strong(expected, true)) {
			CrashDetail::write_posix_report(
				SIGABRT,
				nullptr,
				nullptr,
				reason != nullptr ? reason : "fatal_condition",
				detail != nullptr ? detail : ""
			);
		}
		::_exit(3);
#endif
	}

	[[nodiscard]] static const char* last_report_folder() noexcept {
		return CrashDetail::g_state.last_report_folder.data();
	}

	[[nodiscard]] static uint32_t reports_written() noexcept {
		return CrashDetail::g_state.reports_written.load(std::memory_order_acquire);
	}
};

} 
