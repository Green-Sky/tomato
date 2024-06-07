#include "./sys_check.hpp"

// use message boxes to notify the user of system failure
#include <SDL3/SDL.h>

#include <cstdlib>

#if defined(_WIN32) || defined(WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

typedef LONG NTSTATUS, *PNTSTATUS;
#define STATUS_SUCCESS (0x00000000)

void runSysCheck(void) {
	NTSTATUS(WINAPI *RtlGetVersion)(LPOSVERSIONINFOEXW);
	OSVERSIONINFOEXW osInfo;

	*(FARPROC*)&RtlGetVersion = GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion");

	if (NULL != RtlGetVersion) {
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
}

#else

void runSysCheck(void) {
	// do nothing
}

#endif

