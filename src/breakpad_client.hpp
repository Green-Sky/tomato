#pragma once

// TODO: require msvc
#ifdef WIN32

#include <client/windows/handler/exception_handler.h>

bool dumpCallback(const wchar_t* dump_path, const wchar_t* minidump_id, void* context, EXCEPTION_POINTERS* exinfo, MDRawAssertionInfo* assertion, bool succeeded);

#define BREAKPAD_MAIN_INIT \
	google_breakpad::ExceptionHandler bp_eh{ \
		L".\\", /* path */ \
		nullptr, \
		dumpCallback, \
		nullptr, \
		google_breakpad::ExceptionHandler::HANDLER_ALL, \
	}

#else

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

#endif
