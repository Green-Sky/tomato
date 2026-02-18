#include "./breakpad_client.hpp"

#ifdef WIN32

bool dumpCallback(const wchar_t* dump_path, const wchar_t* minidump_id, void* context, EXCEPTION_POINTERS* exinfo, MDRawAssertionInfo* assertion, bool succeeded) {
	//TCHAR* text = new TCHAR[kMaximumLineLength];
	//text[0] = _T('\0');
	//int result = swprintf_s(text,
	//                        kMaximumLineLength,
	//                        TEXT("Dump generation request %s\r\n"),
	//                        succeeded ? TEXT("succeeded") : TEXT("failed"));
	//if (result == -1) {
	//  delete [] text;
	//}

	//QueueUserWorkItem(AppendTextWorker, text, WT_EXECUTEDEFAULT);

	if (succeeded) {
		fprintf(stderr, "Crash detected, MiniDump written to: %ls\n", dump_path);
	} else {
		fprintf(stderr, "Crash detected, failed to write MiniDump. (path: %ls)\n", dump_path);
	}
	return succeeded;
}

#else
// linux

bool dumpCallback(const google_breakpad::MinidumpDescriptor& descriptor, void*, bool succeeded) {
	if (succeeded) {
		fprintf(stderr, "Crash detected, MiniDump written to: %s\n", descriptor.path());
	} else {
		fprintf(stderr, "Crash detected, FAILED to write MiniDump! (would have been path: %s)\n", descriptor.path());
	}
	return succeeded;
}

#endif
