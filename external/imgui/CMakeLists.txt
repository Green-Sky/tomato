cmake_minimum_required(VERSION 3.14...3.24 FATAL_ERROR)

project(imgui C CXX)

add_library(imgui
	imgui/imgui.h
	imgui/imgui.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_widgets.cpp
	imgui/imgui_tables.cpp
	imgui/imgui_demo.cpp

	imgui/imstb_rectpack.h
	imgui/imstb_textedit.h
	imgui/imstb_truetype.h

	imgui/misc/cpp/imgui_stdlib.h
	imgui/misc/cpp/imgui_stdlib.cpp
)

target_compile_definitions(imgui PUBLIC IMGUI_USE_WCHAR32)
target_compile_features(imgui PUBLIC cxx_std_11)

if(DEFINED CMAKE_CXX_BYTE_ORDER)
	if(CMAKE_CXX_BYTE_ORDER STREQUAL "BIG_ENDIAN")
		# for opengl and sdlrenderer
		target_compile_definitions(imgui PUBLIC IMGUI_USE_BGRA_PACKED_COLOR)
	endif()
endif()

if (TARGET Freetype::Freetype)
	message(STATUS "Freetype detected, enabling support in imgui")
	target_sources(imgui PUBLIC
		imgui/misc/freetype/imgui_freetype.h
		imgui/misc/freetype/imgui_freetype.cpp
	)
	target_link_libraries(imgui PUBLIC Freetype::Freetype) # public in case of static
	target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_FREETYPE)

	if (TARGET plutosvg::plutosvg)
		message(STATUS "plutosvg detected, enabling support in imgui")
		target_link_libraries(imgui PUBLIC plutosvg::plutosvg) # public in case of static
		target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_FREETYPE_PLUTOSVG)
	endif()
endif()

target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui") # im sad

#add_library(imgui_backend_opengl3
	#"imgui/backends/imgui_impl_opengl3.h"
	#"imgui/backends/imgui_impl_opengl3.cpp"
#)
#target_link_libraries(imgui_backend_opengl3 PUBLIC imgui)

add_library(imgui_backend_sdlrenderer3
	imgui/backends/imgui_impl_sdlrenderer3.h
	imgui/backends/imgui_impl_sdlrenderer3.cpp
)
target_link_libraries(imgui_backend_sdlrenderer3 PUBLIC imgui SDL3::SDL3)

#add_library(imgui_backend_glfw
	#"imgui/backends/imgui_impl_glfw.h"
	#"imgui/backends/imgui_impl_glfw.cpp"
#)
#target_link_libraries(imgui_backend_glfw PUBLIC imgui glfw)

add_library(imgui_backend_sdl3
	imgui/backends/imgui_impl_sdl3.h
	imgui/backends/imgui_impl_sdl3.cpp
)
target_link_libraries(imgui_backend_sdl3 PUBLIC imgui SDL3::SDL3)
