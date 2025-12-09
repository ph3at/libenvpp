cmake_minimum_required(VERSION 3.16)
project(libenvpp_integration_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIBENVPP_INTEGRATION_METHOD "fetchcontent" CACHE STRING "Method to obtain libenvpp")
set_property(CACHE LIBENVPP_INTEGRATION_METHOD PROPERTY STRINGS "fetchcontent" "add_subdirectory" "find_package")

set(LIBENVPP_INTEGRATION_DEPS_SOURCE "libenvpp" CACHE STRING "Source of dependencies")
set_property(CACHE LIBENVPP_INTEGRATION_DEPS_SOURCE PROPERTY STRINGS "parent" "system" "libenvpp")

set(LIBENVPP_INTEGRATION_SHARED_LIBS "OFF" CACHE STRING "Build shared libs")
set_property(CACHE LIBENVPP_INTEGRATION_SHARED_LIBS PROPERTY STRINGS "ON" "OFF")

set(LIBENVPP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "Path to libenvpp root directory")

include(FetchContent)

set(BUILD_SHARED_LIBS ${LIBENVPP_INTEGRATION_SHARED_LIBS})

if(LIBENVPP_INTEGRATION_METHOD STREQUAL "find_package")
	set(EXPECTED_FMT_SOURCE "system")
else()
	if(LIBENVPP_INTEGRATION_DEPS_SOURCE STREQUAL "parent")
		FetchContent_Declare(fmt
			GIT_REPOSITORY https://github.com/fmtlib/fmt.git
			GIT_TAG 10.2.1
			GIT_SHALLOW TRUE
		)
		FetchContent_MakeAvailable(fmt)
		set(EXPECTED_FMT_SOURCE "parent")
	elseif(LIBENVPP_INTEGRATION_DEPS_SOURCE STREQUAL "system")
		find_package(fmt REQUIRED)
		set(EXPECTED_FMT_SOURCE "system")
	else()
		set(EXPECTED_FMT_SOURCE "submodule")
	endif()
endif()

set(FMT_WAS_AVAILABLE_BEFORE_LIBENVPP FALSE)

if(TARGET fmt OR TARGET fmt::fmt)
	set(FMT_WAS_AVAILABLE_BEFORE_LIBENVPP TRUE)
endif()

if(LIBENVPP_INTEGRATION_METHOD STREQUAL "fetchcontent")
	FetchContent_Declare(libenvpp SOURCE_DIR ${LIBENVPP_ROOT})
	set(LIBENVPP_TESTS OFF CACHE BOOL "" FORCE)
	set(LIBENVPP_EXAMPLES OFF CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable(libenvpp)
elseif(LIBENVPP_INTEGRATION_METHOD STREQUAL "add_subdirectory")
	add_subdirectory(${LIBENVPP_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/libenvpp)
elseif(LIBENVPP_INTEGRATION_METHOD STREQUAL "find_package")
	find_package(libenvpp REQUIRED)
else()
	message(FATAL_ERROR "Unknown LIBENVPP_INTEGRATION_METHOD: ${LIBENVPP_INTEGRATION_METHOD}")
endif()

add_executable(libenvpp_integration_test ${CMAKE_CURRENT_SOURCE_DIR}/common/test_app.cpp)
target_link_libraries(libenvpp_integration_test PRIVATE libenvpp::libenvpp)

function(detect_fmt_source OUTPUT_VAR)
	if(TARGET fmt::fmt)
		if(FMT_WAS_AVAILABLE_BEFORE_LIBENVPP)
			get_target_property(FMT_IMPORTED fmt::fmt IMPORTED)

			if(FMT_IMPORTED)
				set(${OUTPUT_VAR} "system" PARENT_SCOPE)
			else()
				set(${OUTPUT_VAR} "parent" PARENT_SCOPE)
			endif()
			return()
		endif()

		get_target_property(FMT_IMPORTED fmt::fmt IMPORTED)

		if(FMT_IMPORTED)
			set(${OUTPUT_VAR} "system" PARENT_SCOPE)
		else()
			set(${OUTPUT_VAR} "submodule" PARENT_SCOPE)
		endif()
		return()
	endif()

	set(${OUTPUT_VAR} "unknown" PARENT_SCOPE)
endfunction()

detect_fmt_source(ACTUAL_FMT_SOURCE)

if(NOT ACTUAL_FMT_SOURCE STREQUAL EXPECTED_FMT_SOURCE)
	message(WARNING "Dependency source mismatch! Expected '${EXPECTED_FMT_SOURCE}' but got '${ACTUAL_FMT_SOURCE}'")
endif()

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/dependency_verification.txt"
	"EXPECTED_FMT_SOURCE=${EXPECTED_FMT_SOURCE}\nACTUAL_FMT_SOURCE=${ACTUAL_FMT_SOURCE}\n")
