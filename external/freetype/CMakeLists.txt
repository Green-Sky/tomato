cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

if (NOT TARGET Freetype::Freetype)
	find_package(Freetype 2.12 CONFIG GLOBAL QUIET)
endif()

if (NOT TARGET Freetype::Freetype)
	find_package(Freetype 2.12 GLOBAL QUIET)
endif()

#if (NOT TARGET Freetype::Freetype)
#    find_package(PkgConfig)
#    if (PKG_CONFIG_FOUND)
#        pkg_check_modules(freetype_PKG IMPORTED_TARGET freetype)

#        if (TARGET PkgConfig::freetype_PKG)
#            add_library(Freetype::Freetype ALIAS PkgConfig::freetype_PKG)
#        endif()
#    endif()
#endif()

if (TARGET Freetype::Freetype)
	message(STATUS "found freetype")
else()
	message(STATUS "freetype not found")
endif()
