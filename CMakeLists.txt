cmake_minimum_required(VERSION 3.16)

cmake_policy(SET CMP0091 NEW) # MSVC_RUNTIME_LIBRARY

project(libenvpp LANGUAGES CXX)
set(LIBENVPP_VERSION "1.4.0")
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

option(BUILD_SHARED_LIBS "Build shared libs" OFF)
option(LIBENVPP_TESTS "Build libenvpp tests." ${LIBENVPP_MASTER_PROJECT})
option(LIBENVPP_EXAMPLES "Build libenvpp examples." ${LIBENVPP_MASTER_PROJECT})
option(LIBENVPP_CHECKS "Enable additional runtime checks in release (always on in debug or when tests are enabled)." OFF)
option(LIBENVPP_INSTALL "Enable installation target for libenvpp." OFF)

include(cmake/libenvpp_mt_utils.cmake)

# Function for setting compiler-specific parameters.
function(libenvpp_set_compiler_parameters TARGET)
	set_target_properties(${TARGET} PROPERTIES
		VERSION ${LIBENVPP_VERSION}
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
		LIBENVPP_CHECKS_ENABLED=$<OR:$<BOOL:${LIBENVPP_CHECKS}>,$<CONFIG:DEBUG>,$<BOOL:${LIBENVPP_TESTS}>>
		LIBENVPP_TESTS_ENABLED=$<BOOL:${LIBENVPP_TESTS}>
		LIBENVPP_PLATFORM_WINDOWS=$<BOOL:${WIN32}>
		LIBENVPP_PLATFORM_UNIX=$<BOOL:${UNIX}>
	)
endfunction()

# External dependencies.
if(LIBENVPP_INSTALL)
	set(FMT_INSTALL ON CACHE BOOL "" FORCE)
endif()
include(FetchContent)
FetchContent_Declare(fmt
	GIT_REPOSITORY https://github.com/fmtlib/fmt.git
	GIT_TAG 9.1.0
)
FetchContent_MakeAvailable(fmt)

if(LIBENVPP_TESTS)
	FetchContent_Declare(Catch2
		GIT_REPOSITORY https://github.com/catchorg/Catch2.git
		GIT_TAG v3.2.1
	)
	FetchContent_MakeAvailable(Catch2)
	list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endif()

# libenvpp library.
set(LIBENVPP_SOURCES
	"source/levenshtein.cpp"
	"source/libenvpp_environment_unix.cpp"
	"source/libenvpp_environment_windows.cpp"
	"source/libenvpp_environment.cpp"
	"source/libenvpp_errors.cpp"
	"source/libenvpp_testing.cpp"
)

# Add includes to library so they show up in IDEs.
file(GLOB_RECURSE LIBENVPP_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

add_library(libenvpp STATIC ${LIBENVPP_SOURCES} ${LIBENVPP_INCLUDES})
add_library(libenvpp::libenvpp ALIAS libenvpp)
libenvpp_set_compiler_parameters(libenvpp)
set_target_properties(libenvpp PROPERTIES PREFIX "")
target_link_libraries(libenvpp PUBLIC fmt::fmt)
target_include_directories(libenvpp PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${LIBENVPP_SOURCES} ${LIBENVPP_INCLUDES})

# Unit tests.
if(LIBENVPP_TESTS)
	include(CTest)
	include(Catch)
	add_executable(libenvpp_tests
		"test/levenshtein_test.cpp"
		"test/libenvpp_environment_test.cpp"
		"test/libenvpp_parser_test.cpp"
		"test/libenvpp_test.cpp"
		"test/libenvpp_testing_test.cpp"
	)
	libenvpp_set_compiler_parameters(libenvpp_tests)
	target_link_libraries(libenvpp_tests PRIVATE libenvpp Catch2::Catch2WithMain)

	# Set test as VS startup if libenvpp is master project.
	if(LIBENVPP_MASTER_PROJECT)
		set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT libenvpp_tests)
	endif()

	catch_discover_tests(libenvpp_tests)
endif()

# Examples.
if(LIBENVPP_EXAMPLES)
	file(GLOB_RECURSE LIBENVPP_EXAMPLES_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp")
	foreach(LIBENVPP_EXAMPLE_SOURCE ${LIBENVPP_EXAMPLES_SOURCES})
		get_filename_component(LIBENVPP_EXAMPLE_TARGET ${LIBENVPP_EXAMPLE_SOURCE} NAME_WE)
		add_executable(${LIBENVPP_EXAMPLE_TARGET} "${LIBENVPP_EXAMPLE_SOURCE}")
		libenvpp_set_compiler_parameters(${LIBENVPP_EXAMPLE_TARGET})
		target_link_libraries(${LIBENVPP_EXAMPLE_TARGET} PRIVATE libenvpp)
	endforeach()
endif()

# Installation target.
if(LIBENVPP_INSTALL)
	# Libenvpp installation.
	set(LIBENVPP_PROJECT_CONFIG_OUT "${CMAKE_CURRENT_BINARY_DIR}/libenvpp-config.cmake")
	set(LIBENVPP_VERSION_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/libenvpp-config-version.cmake")
	install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
		DESTINATION include/
	)
	install(TARGETS libenvpp
		EXPORT libenvpp
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
	)
	install(EXPORT libenvpp
		DESTINATION lib/cmake/libenvpp
		NAMESPACE libenvpp::
		FILE libenvpp-config-targets.cmake
	)
	configure_package_config_file(cmake/libenvpp-config.cmake.in
		"${LIBENVPP_PROJECT_CONFIG_OUT}"
		INSTALL_DESTINATION lib/cmake/libenvpp
	)
	write_basic_package_version_file("${LIBENVPP_VERSION_CONFIG_FILE}"
		VERSION ${LIBENVPP_VERSION}
		COMPATIBILITY SameMajorVersion
	)
	install(FILES "${LIBENVPP_PROJECT_CONFIG_OUT}" "${LIBENVPP_VERSION_CONFIG_FILE}"
		DESTINATION lib/cmake/libenvpp
	)
endif()
