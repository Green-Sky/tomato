cmake_minimum_required(VERSION 3.14...3.24 FATAL_ERROR)

project(tomato_android)

# here be dragons

list(APPEND CMAKE_MODULE_PATH "${SDL3_SOURCE_DIR}/cmake/android")

find_package(SdlAndroid MODULE)
find_package(Java)
find_package(SdlAndroidPlatform MODULE)
# the existence of SDL3::Jar usually implies platform
if(SdlAndroid_FOUND)
	include(SdlAndroidFunctions)
	sdl_create_android_debug_keystore(tomato-debug-keystore)
	sdl_android_compile_resources(tomato-resources RESFOLDER app/res)


	set(ANDROID_MANIFEST_PACKAGE "org.libsdl.app.tomato")
	set(generated_manifest_path "${CMAKE_CURRENT_BINARY_DIR}/android/app/AndroidManifest.xml")
	configure_file("app/AndroidManifest.xml.in" generated_manifest_path @ONLY)
	string(REPLACE "." "/" JAVA_PACKAGE_DIR "${ANDROID_MANIFEST_PACKAGE}")
	#set(GENERATED_SRC_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/android/${TEST}-src")
	#set(GENERATED_RES_FOLDER "${GENERATED_SRC_FOLDER}/res")
	#set(JAVA_PACKAGE_DIR "${GENERATED_SRC_FOLDER}/${JAVA_PACKAGE_DIR}")
	set(JAVA_PACKAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/app/java/${JAVA_PACKAGE_DIR}")

	sdl_android_link_resources(tomato-apk-linked
		MANIFEST ${generated_manifest_path}
		PACKAGE ${ANDROID_MANIFEST_PACKAGE}
		RES_TARGETS tomato-resources
		TARGET_SDK_VERSION 31
	)

	set(CMAKE_JAVA_COMPILE_FLAGS "-encoding;utf-8")
	set(classes_path "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/tomato-java.dir/classes")
	# Some CMake versions have a slow `cmake -E make_directory` implementation
	if(NOT IS_DIRECTORY "${classes_path}")
		execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${classes_path}")
	endif()
	set(OUT_JAR "${CMAKE_CURRENT_BINARY_DIR}/tomato.jar")
	# TODO: convert to cmake's add_jar
	add_custom_command(
		OUTPUT "${OUT_JAR}"
		COMMAND ${CMAKE_COMMAND} -E rm -rf "${classes_path}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${classes_path}"
		COMMAND ${Java_JAVAC_EXECUTABLE}
			-source 1.8 -target 1.8
			-bootclasspath "$<TARGET_PROPERTY:SDL3::Jar,JAR_FILE>"
			"${JAVA_PACKAGE_DIR}/TomatoActivity.java"
			$<TARGET_PROPERTY:tomato-apk-linked,JAVA_R>
			-cp "$<TARGET_PROPERTY:SDL3::Jar,JAR_FILE>:${SDL_ANDROID_PLATFORM_ANDROID_JAR}"
			-d "${classes_path}"
		COMMAND ${Java_JAR_EXECUTABLE} cf "${OUT_JAR}" -C "${classes_path}" .
		DEPENDS $<TARGET_PROPERTY:tomato-apk-linked,OUTPUTS> "$<TARGET_PROPERTY:SDL3::Jar,JAR_FILE>"
	)
	add_custom_target(tomato-jar DEPENDS "${OUT_JAR}")
	add_dependencies(tomato-jar SDL3::Jar) # HACK: somehow their jar is not registered as an output
	set_property(TARGET tomato-jar PROPERTY OUTPUT "${OUT_JAR}")

	set(dexworkdir "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/tomato-dex.dir")
	# Some CMake versions have a slow `cmake -E make_directory` implementation
	if(NOT IS_DIRECTORY "${dexworkdir}")
		execute_process(COMMAND "${CMAKE_COMMAND}" -E make_directory "${dexworkdir}")
	endif()
	set(classes_dex_base_name "classes.dex")
	set(classes_dex "${dexworkdir}/${classes_dex_base_name}")
	add_custom_command(
		OUTPUT "${classes_dex}"
		COMMAND SdlAndroid::d8
			$<TARGET_PROPERTY:tomato-jar,OUTPUT>
			$<TARGET_PROPERTY:SDL3::Jar,JAR_FILE>
			--lib "${SDL_ANDROID_PLATFORM_ANDROID_JAR}"
			--output "${dexworkdir}"
		DEPENDS $<TARGET_PROPERTY:tomato-jar,OUTPUT> $<TARGET_PROPERTY:SDL3::Jar,JAR_FILE>
	)
	add_custom_target(tomato-dex DEPENDS "${classes_dex}")
	set_property(TARGET tomato-dex PROPERTY OUTPUT "${classes_dex}")
	set_property(TARGET tomato-dex PROPERTY OUTPUT_BASE_NAME "${classes_dex_base_name}")

	# file(GLOB RESOURCE_FILES *.bmp *.wav *.hex moose.dat utf8.txt)

	sdl_add_to_apk_unaligned(tomato-unaligned-apk
		APK_IN tomato-apk-linked
		OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/intermediates"
		#ASSETS ${RESOURCE_FILES}
		#NATIVE_LIBS SDL3::SDL3-shared tomato
		NATIVE_LIBS tomato
		DEX tomato-dex
	)

	sdl_apk_align(tomato-aligned-apk tomato-unaligned-apk
		OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/intermediates"
	)
	sdl_apk_sign(tomato-apk tomato-aligned-apk
		KEYSTORE tomato-debug-keystore
	)
else()
	message("EE SdlAndroid module not found")
endif()

