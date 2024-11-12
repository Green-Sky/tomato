cmake_minimum_required(VERSION 3.16...3.24 FATAL_ERROR)

include(FetchContent)

if (NOT TARGET breakpad_client)

	if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Android")
		if (NOT TARGET lss)
			FetchContent_Declare(lss
				GIT_REPOSITORY https://chromium.googlesource.com/linux-syscall-support/
				GIT_TAG 9719c1e1e676814c456b55f5f070eabad6709d31

				FIND_PACKAGE_ARGS # for the future
			)
			FetchContent_GetProperties(lss)
			if(NOT lss_POPULATED)
				FetchContent_Populate(lss)

				# HACK: breakpad expects this at a specific path
				configure_file(
					${lss_SOURCE_DIR}/linux_syscall_support.h
					${CMAKE_CURRENT_BINARY_DIR}/third_party/lss/linux_syscall_support.h
					@ONLY
				)

				add_library(lss INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/third_party/lss/linux_syscall_support.h)
				target_include_directories(lss INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
			endif()
		endif()
	endif()

	FetchContent_Declare(breakpad
		GIT_REPOSITORY https://chromium.googlesource.com/breakpad/breakpad
		GIT_TAG v2023.06.01

		FIND_PACKAGE_ARGS # for the future
	)
	FetchContent_GetProperties(breakpad)
	if(NOT breakpad_POPULATED)
		FetchContent_Populate(breakpad)

		add_library(breakpad_common STATIC
			${breakpad_SOURCE_DIR}/src/common/convert_UTF.h
			${breakpad_SOURCE_DIR}/src/common/convert_UTF.cc
			${breakpad_SOURCE_DIR}/src/common/md5.h
			${breakpad_SOURCE_DIR}/src/common/md5.cc
			${breakpad_SOURCE_DIR}/src/common/string_conversion.h
			${breakpad_SOURCE_DIR}/src/common/string_conversion.cc
		)
		target_include_directories(breakpad_common PUBLIC "${breakpad_SOURCE_DIR}/src")

		if (WIN32)
			target_sources(breakpad_common PUBLIC
				${breakpad_SOURCE_DIR}/src/common/windows/guid_string.h
				${breakpad_SOURCE_DIR}/src/common/windows/guid_string.cc
			)

			add_library(breakpad_client STATIC)
			target_sources(breakpad_client
				PUBLIC
					${breakpad_SOURCE_DIR}/src/client/windows/handler/exception_handler.h
					${breakpad_SOURCE_DIR}/src/client/windows/common/ipc_protocol.h
					${breakpad_SOURCE_DIR}/src/client/windows/crash_generation/crash_generation_client.h
					${breakpad_SOURCE_DIR}/src/client/windows/crash_generation/minidump_generator.h
				PRIVATE
					${breakpad_SOURCE_DIR}/src/client/windows/handler/exception_handler.cc
					${breakpad_SOURCE_DIR}/src/client/windows/crash_generation/crash_generation_client.cc
					${breakpad_SOURCE_DIR}/src/client/windows/crash_generation/minidump_generator.cc
			)
			target_compile_definitions(breakpad_client PRIVATE UNICODE)
		#elseif() # TODO: mac, ios and any other platform
		else() # assume linux
			enable_language(ASM) # mostly to document, needs to be set in parent
			target_sources(breakpad_common PUBLIC
				${breakpad_SOURCE_DIR}/src/common/linux/elf_core_dump.cc
				${breakpad_SOURCE_DIR}/src/common/linux/elfutils.h
				${breakpad_SOURCE_DIR}/src/common/linux/elfutils.cc
				${breakpad_SOURCE_DIR}/src/common/linux/file_id.h
				${breakpad_SOURCE_DIR}/src/common/linux/file_id.cc
				${breakpad_SOURCE_DIR}/src/common/linux/guid_creator.h
				${breakpad_SOURCE_DIR}/src/common/linux/guid_creator.cc
				${breakpad_SOURCE_DIR}/src/common/linux/linux_libc_support.cc
				${breakpad_SOURCE_DIR}/src/common/linux/memory_mapped_file.cc
				${breakpad_SOURCE_DIR}/src/common/linux/safe_readlink.cc
				${breakpad_SOURCE_DIR}/src/common/linux/breakpad_getcontext.h
				${breakpad_SOURCE_DIR}/src/common/linux/breakpad_getcontext.S
			)
			#set_property(SOURCE ${breakpad_SOURCE_DIR}/src/common/linux/breakpad_getcontext.S APPEND PROPERTY COMPILE_OPTIONS "-x" "assembler-with-cpp")

			add_library(breakpad_client STATIC)
			target_sources(breakpad_client
				PUBLIC
					${breakpad_SOURCE_DIR}/src/client/linux/handler/exception_handler.h
					${breakpad_SOURCE_DIR}/src/client/linux/handler/minidump_descriptor.h
					${breakpad_SOURCE_DIR}/src/client/linux/crash_generation/crash_generation_client.h
					${breakpad_SOURCE_DIR}/src/client/linux/log/log.h
					${breakpad_SOURCE_DIR}/src/client/linux/microdump_writer/microdump_writer.h
					${breakpad_SOURCE_DIR}/src/client/linux/minidump_writer/minidump_writer.h
					${breakpad_SOURCE_DIR}/src/client/linux/minidump_writer/pe_file.h
					${breakpad_SOURCE_DIR}/src/client/linux/minidump_writer/pe_structs.h
					${breakpad_SOURCE_DIR}/src/client/linux/minidump_writer/proc_cpuinfo_reader.h
				PRIVATE
					${breakpad_SOURCE_DIR}/src/client/linux/handler/exception_handler.cc
					${breakpad_SOURCE_DIR}/src/client/linux/handler/minidump_descriptor.cc
					${breakpad_SOURCE_DIR}/src/client/linux/crash_generation/crash_generation_client.cc
					${breakpad_SOURCE_DIR}/src/client/linux/crash_generation/crash_generation_server.cc
					${breakpad_SOURCE_DIR}/src/client/linux/log/log.cc
					${breakpad_SOURCE_DIR}/src/client/linux/microdump_writer/microdump_writer.cc
					${breakpad_SOURCE_DIR}/src/client/linux/dump_writer_common/thread_info.cc
					${breakpad_SOURCE_DIR}/src/client/linux/dump_writer_common/ucontext_reader.cc
					${breakpad_SOURCE_DIR}/src/client/linux/minidump_writer/minidump_writer.cc
					${breakpad_SOURCE_DIR}/src/client/linux/minidump_writer/linux_core_dumper.cc
					${breakpad_SOURCE_DIR}/src/client/linux/minidump_writer/linux_dumper.cc
					${breakpad_SOURCE_DIR}/src/client/linux/minidump_writer/linux_ptrace_dumper.cc
					${breakpad_SOURCE_DIR}/src/client/linux/minidump_writer/pe_file.cc
			)
		endif()

		if (TARGET lss)
			target_link_libraries(breakpad_common PUBLIC lss)
			target_link_libraries(breakpad_client PUBLIC lss)
		endif()

		if (TARGET breakpad_client)
			if (NOT WIN32)
				target_sources(breakpad_client PUBLIC
					${breakpad_SOURCE_DIR}/src/client/minidump_file_writer-inl.h
					${breakpad_SOURCE_DIR}/src/client/minidump_file_writer.h
					${breakpad_SOURCE_DIR}/src/client/minidump_file_writer.cc
				)
			endif()
			target_link_libraries(breakpad_client PUBLIC breakpad_common)
			target_include_directories(breakpad_client PUBLIC "${breakpad_SOURCE_DIR}/src")
			target_compile_features(breakpad_client PUBLIC cxx_std_11)
		endif()
	endif()
	#FetchContent_MakeAvailable(breakpad)
endif()

