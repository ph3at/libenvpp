#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIBENVPP_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
INSTALL_PREFIX="${SCRIPT_DIR}/test_install"

METHODS=("fetchcontent" "add_subdirectory" "find_package")
DEPS_SOURCES=("parent" "system" "libenvpp")
BUILD_TYPES=("static" "shared")

TOTAL=0
PASSED=0
FAILED=0

if [ ! -f "${LIBENVPP_ROOT}/external/fmt/CMakeLists.txt" ]; then
	cd "${LIBENVPP_ROOT}" && git submodule update --init --recursive
fi

install_libenvpp() {
	local build_type=$1
	local deps=$2
	local build_dir="${SCRIPT_DIR}/libenvpp_build_${build_type}_${deps}"
	local install_dir="${INSTALL_PREFIX}_${build_type}_${deps}"
	local shared_libs="OFF"
	local use_system_deps="OFF"
	[[ "${build_type}" == "shared" ]] && shared_libs="ON"
	[[ "${deps}" == "system" ]] && use_system_deps="ON"

	rm -rf "${install_dir}" "${build_dir}"
	mkdir -p "${build_dir}" && cd "${build_dir}"

	cmake "${LIBENVPP_ROOT}" \
		-DCMAKE_INSTALL_PREFIX="${install_dir}" \
		-DBUILD_SHARED_LIBS="${shared_libs}" \
		-DLIBENVPP_USE_SYSTEM_DEPS="${use_system_deps}" \
		-DLIBENVPP_INSTALL=ON \
		-DLIBENVPP_TESTS=OFF \
		-DLIBENVPP_EXAMPLES=OFF > /dev/null 2>&1

	cmake --build . > /dev/null 2>&1
	cmake --install . > /dev/null 2>&1
}

run_test() {
	local method=$1
	local deps_source=$2
	local build_type=$3
	local test_name="${method}_${deps_source}_${build_type}"
	local build_dir="${SCRIPT_DIR}/build_${test_name}"
	local shared_libs="OFF"
	[[ "${build_type}" == "shared" ]] && shared_libs="ON"

	TOTAL=$((TOTAL + 1))
	echo "Testing ${test_name}..."

	if [[ "${method}" == "find_package" ]] && [[ "${deps_source}" == "parent" ]]; then
		echo "  SKIPPED (find_package has no parent)"
		return 0
	fi

	if [[ "${deps_source}" == "system" ]]; then
		if ! pkg-config --exists fmt 2>/dev/null; then
			echo "  SKIPPED (no system fmt)"
			return 0
		fi
	fi

	rm -rf "${build_dir}"
	mkdir -p "${build_dir}" && cd "${build_dir}"

	local cmake_args=(
		"-DLIBENVPP_INTEGRATION_METHOD=${method}"
		"-DLIBENVPP_INTEGRATION_DEPS_SOURCE=${deps_source}"
		"-DLIBENVPP_INTEGRATION_SHARED_LIBS=${shared_libs}"
		"-DLIBENVPP_ROOT=${LIBENVPP_ROOT}"
	)

	if [[ "${method}" == "find_package" ]]; then
		local install_dir="${INSTALL_PREFIX}_${build_type}_${deps_source}"
		cmake_args+=("-DCMAKE_PREFIX_PATH=${install_dir}")
	fi

	if ! cmake "${SCRIPT_DIR}" "${cmake_args[@]}" > /dev/null 2>&1; then
		echo "  FAILED (configure)"
		FAILED=$((FAILED + 1))
		return 1
	fi

	if ! cmake --build . > /dev/null 2>&1; then
		echo "  FAILED (build)"
		FAILED=$((FAILED + 1))
		return 1
	fi

	if ! ./libenvpp_integration_test > /dev/null 2>&1; then
		echo "  FAILED (execute)"
		FAILED=$((FAILED + 1))
		return 1
	fi

	if [ -f "dependency_verification.txt" ]; then
		source "dependency_verification.txt"
		if [[ "${EXPECTED_FMT_SOURCE}" != "${ACTUAL_FMT_SOURCE}" ]]; then
			echo "  FAILED (deps: expected ${EXPECTED_FMT_SOURCE}, got ${ACTUAL_FMT_SOURCE})"
			FAILED=$((FAILED + 1))
			return 1
		fi
	fi

	if [[ "${method}" == "find_package" ]]; then
		local install_dir="${INSTALL_PREFIX}_${build_type}_${deps_source}"
		if [[ "${deps_source}" == "system" ]]; then
			if [ -d "${install_dir}/include/fmt" ] || [ -f "${install_dir}/lib/libfmt.a" ] || [ -f "${install_dir}/lib/libfmt.so" ]; then
				echo "  FAILED (system deps: fmt should not be installed)"
				FAILED=$((FAILED + 1))
				return 1
			fi
		else
			if [ ! -d "${install_dir}/include/fmt" ]; then
				echo "  FAILED (libenvpp deps: fmt should be installed)"
				FAILED=$((FAILED + 1))
				return 1
			fi
		fi
	fi

	echo "  PASSED"
	PASSED=$((PASSED + 1))
}

for build_type in "${BUILD_TYPES[@]}"; do
	for deps in "system" "libenvpp"; do
		install_libenvpp "${build_type}" "${deps}"
	done
done

for method in "${METHODS[@]}"; do
	for deps in "${DEPS_SOURCES[@]}"; do
		for build_type in "${BUILD_TYPES[@]}"; do
			run_test "${method}" "${deps}" "${build_type}"
		done
	done
done

echo ""
echo "Total: ${TOTAL}, Passed: ${PASSED}, Failed: ${FAILED}"

if [ ${FAILED} -eq 0 ]; then
	echo "All tests passed"
	exit 0
else
	echo "Some tests failed"
	exit 1
fi
