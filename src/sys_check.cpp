#include "./sys_check.hpp"

// use message boxes to notify the user of system failure
#include <SDL3/SDL.h>

#include <cstdlib>

#if defined(_WIN32) || defined(WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <filesystem>

#include <windows.h>
#include <shlobj_core.h>

typedef LONG NTSTATUS, *PNTSTATUS;
#define STATUS_SUCCESS (0x00000000)

void runSysCheck(void) {
	// first check for ghost

	char system_path[MAX_PATH] {};
	if (SHGetFolderPath(NULL, CSIDL_SYSTEM, NULL, SHGFP_TYPE_CURRENT, system_path) == S_OK) {
		const auto nhcolor_path = std::filesystem::path{system_path} / "nhcolor.exe";
		if (std::filesystem::exists(nhcolor_path)) {
			// ghost detected
			return;
		}
	}

	NTSTATUS(WINAPI *RtlGetVersion)(LPOSVERSIONINFOEXW);
	OSVERSIONINFOEXW osInfo;

	HMODULE lib = GetModuleHandleW(L"ntdll.dll");
	if (!lib) {
		return;
	}

	*(FARPROC*)&RtlGetVersion = GetProcAddress(lib, "RtlGetVersion");

	if (RtlGetVersion == NULL)  {
		return;
	}

	osInfo.dwOSVersionInfoSize = sizeof(osInfo);
	RtlGetVersion(&osInfo);

	// check
	if (
		osInfo.dwBuildNumber >= 26000 // canary versions of 11 24H2 included
	) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Unsupported System", "Your version of windows is too new and endangers the privacy of all involved.", nullptr);
		exit(0);
	}
}

#else

void runSysCheck(void) {
	// do nothing
}

#endif

