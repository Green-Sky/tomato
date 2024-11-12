#pragma once

// TODO: if linux/android

#include <client/linux/handler/exception_handler.h>

bool dumpCallback(const google_breakpad::MinidumpDescriptor& descriptor, void*, bool succeeded);

#define BREAKPAD_MAIN_INIT \
	google_breakpad::MinidumpDescriptor bp_descriptor{"/tmp"}; \
	bp_descriptor.set_sanitize_stacks(true); /* *should* remove most PII from stack mem dump */ \
	bp_descriptor.set_size_limit(50*1024*1024); /* limit to 50mb */ \
	google_breakpad::ExceptionHandler bp_eh{ \
		bp_descriptor, \
		nullptr, \
		dumpCallback, \
		nullptr, \
		true, /* install handler */ \
		-1, /* dump in-process (OOP would be better) */ \
	}

// endif linux
