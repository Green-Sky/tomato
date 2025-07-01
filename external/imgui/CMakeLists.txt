cmake_minimum_required(VERSION 3.14...3.24 FATAL_ERROR)

include(FetchContent)

if (TARGET imgui)
	return()
endif()

if (NOT TARGET imgui)
	FetchContent_Declare(imgui
		GIT_REPOSITORY https://github.com/ocornut/imgui.git
		GIT_TAG f5befd2d29e66809cd1110a152e375a7f1981f06 # v1.91.9b
		EXCLUDE_FROM_ALL
	)

	FetchContent_GetProperties(imgui)
	if(NOT imgui_POPULATED)
		FetchContent_MakeAvailable(imgui)

		add_library(imgui STATIC
			${imgui_SOURCE_DIR}/imgui.h
			${imgui_SOURCE_DIR}/imgui_internal.h

			${imgui_SOURCE_DIR}/imgui.cpp
			${imgui_SOURCE_DIR}/imgui_demo.cpp
			${imgui_SOURCE_DIR}/imgui_draw.cpp
			${imgui_SOURCE_DIR}/imgui_tables.cpp
			${imgui_SOURCE_DIR}/imgui_widgets.cpp
			${imgui_SOURCE_DIR}/imstb_rectpack.h
			${imgui_SOURCE_DIR}/imstb_textedit.h
			${imgui_SOURCE_DIR}/imstb_truetype.h

			${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.h
			${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
		)
		target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
		target_compile_features(imgui PUBLIC cxx_std_11)
		target_compile_definitions(imgui PUBLIC IMGUI_USE_WCHAR32)


		if(DEFINED CMAKE_CXX_BYTE_ORDER)
			if(CMAKE_CXX_BYTE_ORDER STREQUAL "BIG_ENDIAN")
				# for opengl and sdlrenderer
				target_compile_definitions(imgui PUBLIC IM_COL32_R_SHIFT=24)
				target_compile_definitions(imgui PUBLIC IM_COL32_G_SHIFT=16)
				target_compile_definitions(imgui PUBLIC IM_COL32_B_SHIFT=8)
				target_compile_definitions(imgui PUBLIC IM_COL32_A_SHIFT=0)
				target_compile_definitions(imgui PUBLIC IM_COL32_A_MASK=0x000000ff)
			endif()
		endif()

		if (TARGET Freetype::Freetype)
			message(STATUS "Freetype detected, enabling support in imgui")
			target_sources(imgui PUBLIC
				${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.h
				${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp
			)
			target_link_libraries(imgui PUBLIC Freetype::Freetype) # public in case of static
			target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_FREETYPE)

			if (TARGET plutosvg::plutosvg)
				message(STATUS "plutosvg detected, enabling support in imgui")
				target_link_libraries(imgui PUBLIC plutosvg::plutosvg) # public in case of static
				target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_FREETYPE_PLUTOSVG)
			endif()
		endif()

		add_library(imgui_backend_sdlrenderer3
			${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.h
			${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
		)
		target_link_libraries(imgui_backend_sdlrenderer3 PUBLIC imgui SDL3::SDL3)

		add_library(imgui_backend_sdl3
			${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.h
			${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
		)
		target_link_libraries(imgui_backend_sdl3 PUBLIC imgui SDL3::SDL3)

	endif()
endif()

