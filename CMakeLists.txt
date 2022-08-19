cmake_minimum_required(VERSION 3.18)

cmake_policy(SET CMP0091 NEW) # MSVC_RUNTIME_LIBRARY

project(libenvpp LANGUAGES CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Determine if libenvpp is built as a subproject (using add_subdirectory).
if (NOT DEFINED LIBENVPP_MASTER_PROJECT)
	set(LIBENVPP_MASTER_PROJECT OFF)
	if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
		set(LIBENVPP_MASTER_PROJECT ON)
	endif()
endif()

# Set CMAKE_MSVC_RUNTIME_LIBRARY to its supposed default value. This is needed
# by external dependencies to correctly support builds on Windows using Clang.
if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
	set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>DLL)
endif()

option(LIBENVPP_TESTS "Build libenvpp tests." ${LIBENVPP_MASTER_PROJECT})
option(LIBENVPP_CHECKS "Enable additional runtime checks in release (always on in debug)." OFF)

include(cmake/libenvpp_mt_utils.cmake)

# Function for setting compiler-specific parameters.
function(libenvpp_set_compiler_parameters TARGET)
	set_target_properties(${TARGET} PROPERTIES
		CXX_STANDARD 17
		CXX_EXTENSIONS OFF
	)
	target_compile_options(${TARGET} PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>:/D_CRT_SECURE_NO_WARNINGS /MP /W4 /permissive- /bigobj /Zi /utf-8>
		$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -pedantic -g>
		$<$<CXX_COMPILER_ID:GNU>:-fdiagnostics-color>
		$<$<CXX_COMPILER_ID:Clang,AppleClang>:-fcolor-diagnostics>
	)
	target_link_options(${TARGET} PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>:/DEBUG /ignore:4099>
	)
	target_compile_definitions(${TARGET} PUBLIC
		$<$<OR:$<BOOL:${LIBENVPP_CHECKS}>,$<CONFIG:DEBUG>>:LIBENVPP_CHECKS_ENABLED>
		$<$<BOOL:${LIBENVPP_TESTS}>:LIBENVPP_TESTS_ENABLED>
		$<$<BOOL:${WIN32}>:LIBENVPP_PLATFORM_WINDOWS>
		$<$<BOOL:${UNIX}>:LIBENVPP_PLATFORM_UNIX>
	)
endfunction()

# External dependencies.
option(BUILD_SHARED_LIBS "Build shared libs" OFF)
set(CMAKE_FOLDER "external/fmt")
	add_subdirectory(external/fmt)
if(LIBENVPP_TESTS)
	set(CMAKE_FOLDER "external/catch2")
		add_subdirectory(external/catch2)
		include("external/catch2/extras/Catch.cmake")
endif()
unset(CMAKE_FOLDER)

# libenvpp library.
set(LIBENVPP_SOURCES
	"source/levenshtein.cpp"
	"source/libenvpp_env_unix.cpp"
	"source/libenvpp_env_windows.cpp"
	"source/libenvpp.cpp"
)

# Add includes to library so they show up in IDEs.
file(GLOB_RECURSE LIBENVPP_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

add_library(libenvpp STATIC ${LIBENVPP_SOURCES} ${LIBENVPP_INCLUDES})
libenvpp_set_compiler_parameters(libenvpp)
set_target_properties(libenvpp PROPERTIES PREFIX "")
target_link_libraries(libenvpp PUBLIC fmt::fmt)
target_include_directories(libenvpp PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${LIBENVPP_SOURCES} ${LIBENVPP_INCLUDES})

# Unit tests.
if(LIBENVPP_TESTS)
	include(CTest)
	add_executable(libenvpp_tests
		"test/levenshtein_test.cpp"
		"test/libenvpp_env_test.cpp"
		"test/libenvpp_parser_test.cpp"
		"test/libenvpp_test.cpp"
		"test/libenvpp_util_test.cpp"
	)
	libenvpp_set_compiler_parameters(libenvpp_tests)
	target_link_libraries(libenvpp_tests PRIVATE libenvpp Catch2::Catch2WithMain)

	# Set test as VS startup if libenvpp is master project.
	if(LIBENVPP_MASTER_PROJECT)
		set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT libenvpp_tests)
	endif()

	catch_discover_tests(libenvpp_tests)
endif()
