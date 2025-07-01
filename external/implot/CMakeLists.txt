cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

include(FetchContent)

if (NOT TARGET implot)
	FetchContent_Declare(implot
		GIT_REPOSITORY https://github.com/epezent/implot.git
		#GIT_TAG 47522f47054d33178e7defa780042bd2a06b09f9 # 22-01-2025
		#GIT_TAG 193b9d8f92c4a437e84182b171f1ae266e72321f # 05-03-2025
		GIT_TAG 3da8bd34299965d3b0ab124df743fe3e076fa222 # 19-03-2025
		EXCLUDE_FROM_ALL
		CONFIGURE_COMMAND "" # no cmake
	)

	FetchContent_GetProperties(implot)
	if(NOT implot_POPULATED)
		FetchContent_MakeAvailable(implot)

		add_library(implot STATIC
			${implot_SOURCE_DIR}/implot.h
			${implot_SOURCE_DIR}/implot_internal.h

			${implot_SOURCE_DIR}/implot.cpp
			${implot_SOURCE_DIR}/implot_demo.cpp
			${implot_SOURCE_DIR}/implot_items.cpp
		)
		target_include_directories(implot PUBLIC ${implot_SOURCE_DIR})
		target_compile_features(implot PUBLIC cxx_std_11)
		target_link_libraries(implot PUBLIC imgui)
		#target_compile_definitions(implot PUBLIC IMGUI_USE_WCHAR32)
	endif()
endif()

