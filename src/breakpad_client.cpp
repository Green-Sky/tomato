#include "./breakpad_client.hpp"

// if linux

bool dumpCallback(const google_breakpad::MinidumpDescriptor& descriptor, void*, bool succeeded) {
	fprintf(stderr, "Crash detected, MiniDump written to: %s\n", descriptor.path());
	return succeeded;
}

// endif linux
