cmake_minimum_required(VERSION 3.9...3.24 FATAL_ERROR)

########################################

add_library(font_loading STATIC)

target_sources(font_loading PUBLIC
	./font_finder.hpp
	./font_finder.cpp
	./font_finder_i.hpp
	./font_finder_fs.hpp
	./font_finder_fs.cpp
	./font_finder_fc_sdl.hpp
	./font_finder_fc_sdl.cpp
)

# TODO: conditionally compile fontconfig
#if (TOMATO_BREAKPAD)
#    target_sources(tomato PUBLIC
#        ./breakpad_client.hpp
#        ./breakpad_client.cpp
#    )

#    target_link_libraries(tomato PUBLIC breakpad_client)
#    target_compile_definitions(tomato PUBLIC TOMATO_BREAKPAD)
#endif()

target_compile_features(font_loading PUBLIC cxx_std_17)
target_link_libraries(font_loading PUBLIC
	SDL3::SDL3
	imgui
)

set_target_properties(font_loading PROPERTIES POSITION_INDEPENDENT_CODE ON)

########################################

if (BUILD_TESTING)
	add_executable(test_font_loaders
		./test_ff.cpp
	)

	target_link_libraries(test_font_loaders
		font_loading
	)

	add_test(NAME test_font_loaders COMMAND test_font_loaders)
endif()

