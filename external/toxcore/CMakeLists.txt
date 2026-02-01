cmake_minimum_required(VERSION 3.13...3.16 FATAL_ERROR)

set(EXPERIMENTAL_API ON CACHE BOOL "" FORCE)
set(UNITTEST OFF CACHE BOOL "" FORCE)
set(BOOTSTRAP_DAEMON OFF CACHE BOOL "" FORCE)
if (TOMATO_TOX_AV)
	set(BUILD_TOXAV ON CACHE BOOL "" FORCE)
	set(MUST_BUILD_TOXAV ON CACHE BOOL "" FORCE)
endif()

add_subdirectory(./c-toxcore)

#message("II BUILD_TOXAV: ${BUILD_TOXAV}")

# the sad case
add_library(toxcore INTERFACE)

if (TARGET toxcore_static)
	target_link_libraries(toxcore INTERFACE toxcore_static)

	# the ideal case
	#add_library(toxcore ALIAS toxcore_static)
else()
	target_link_libraries(toxcore INTERFACE toxcore_shared)

	# the ideal case
	#add_library(toxcore ALIAS toxcore_shared)
endif()

# HACK: "install" api headers into binary dir
configure_file(
	./c-toxcore/toxcore/tox.h
	${CMAKE_CURRENT_BINARY_DIR}/include/tox/tox.h
	@ONLY
)
configure_file(
	./c-toxcore/toxcore/tox_events.h
	${CMAKE_CURRENT_BINARY_DIR}/include/tox/tox_events.h
	@ONLY
)
configure_file(
	./c-toxcore/toxcore/tox_log_level.h
	${CMAKE_CURRENT_BINARY_DIR}/include/tox/tox_log_level.h
	@ONLY
)
configure_file(
	./c-toxcore/toxcore/tox_options.h
	${CMAKE_CURRENT_BINARY_DIR}/include/tox/tox_options.h
	@ONLY
)
configure_file(
	./c-toxcore/toxcore/tox_private.h
	${CMAKE_CURRENT_BINARY_DIR}/include/tox/tox_private.h
	@ONLY
)
configure_file(
	./c-toxcore/toxcore/tox_attributes.h
	${CMAKE_CURRENT_BINARY_DIR}/include/tox/tox_attributes.h
	@ONLY
)
configure_file(
	./c-toxcore/toxencryptsave/toxencryptsave.h
	${CMAKE_CURRENT_BINARY_DIR}/include/tox/toxencryptsave.h
	@ONLY
)
configure_file(
	./c-toxcore/toxav/toxav.h
	${CMAKE_CURRENT_BINARY_DIR}/include/tox/toxav.h
	@ONLY
)

target_include_directories(toxcore INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/include/)

# HACK: support old libsodium find
# libs should handle this case themselfs
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(sodium QUIET)
if(unofficial-sodium_FOUND) # vcpkg
	if(TARGET unofficial-sodium::sodium)
		target_link_libraries(toxcore INTERFACE unofficial-sodium::sodium)
	endif()
	if(TARGET unofficial-sodium::sodium_config_public)
		target_link_libraries(toxcore INTERFACE unofficial-sodium::sodium_config_public)
	endif()
elseif(sodium_FOUND)
	target_link_libraries(toxcore INTERFACE sodium)
else()
	message(SEND_ERROR "missing libsodium")
endif()

#if(BUILD_TOXAV)
#    set_target_properties(toxcore PROPERTIES TOX_HAS_TOXAV ON)
#endif()

