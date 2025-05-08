# based on code from SDLAndroidFunctions.cmake

# target name is name of file without .keystore
function(tomato_load_android_keystore TARGET)
	set(output "${CMAKE_SOURCE_DIR}/${TARGET}.keystore")
	add_custom_target(${TARGET} DEPENDS "${output}")
	set_property(TARGET ${TARGET} PROPERTY OUTPUT "${output}")
endfunction()

