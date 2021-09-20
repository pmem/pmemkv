#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2021, Intel Corporation

#
# prepare-for-build.sh - prepare the Docker image for the builds
#                        and defines functions for other scripts.
#

set -e

EXAMPLE_TEST_DIR="/tmp/build_example"
PREFIX=/usr

### Helper functions, used in run-*.sh scripts
function sudo_password() {
	echo ${USERPASS} | sudo -Sk $*
}

function upload_codecov() {
	printf "\n$(tput setaf 1)$(tput setab 7)COVERAGE ${FUNCNAME[0]} START$(tput sgr 0)\n"

	# set proper gcov command
	clang_used=$(cmake -LA -N . | grep CMAKE_CXX_COMPILER | grep clang | wc -c)
	if [[ ${clang_used} -gt 0 ]]; then
		gcovexe="llvm-cov gcov"
	else
		gcovexe="gcov"
	fi

	# run gcov exe, using their bash (remove parsed coverage files, set flag and exit 1 if not successful)
	# we rely on parsed report on codecov.io; the output is quite long, hence it's disabled using -X flag
	/opt/scripts/codecov -c -F ${1} -Z -x "${gcovexe}" -X "gcovout"

	printf "check for any leftover gcov files\n"
	leftover_files=$(find . -name "*.gcov")
	if [[ -n "${leftover_files}" ]]; then
		# display found files and exit with error (they all should be parsed)
		echo "${leftover_files}"
		return 1
	fi

	printf "$(tput setaf 1)$(tput setab 7)COVERAGE ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

# compile all examples (it looks up for examples/ sub-directories)
# and run them as long as they are not on the block_list
function compile_run_examples_standalone() {
	# Examples may have some special execution requirement.
	# They should be run in the function `test_release_installation` (in run-build.sh).
	block_list="pmemkv_config_c pmemkv_fill_cpp pmemkv_open_cpp pmemkv_transaction_c pmemkv_transaction_cpp"

	pushd ${WORKDIR}/examples
	examples=$(find * -prune -type d)
	echo "Found examples to compile: ${examples}"
	for example in ${examples}; do
		compile_example_standalone ${example}

		if ! echo ${block_list} | grep -w -q ${example}; then
			run_example_standalone ${example} pool
		fi
	done
	popd
}

function compile_example_standalone() {
	example_name=${1}
	echo "Compile standalone example: ${example_name}"

	rm -rf ${EXAMPLE_TEST_DIR}
	mkdir ${EXAMPLE_TEST_DIR}
	pushd ${EXAMPLE_TEST_DIR}

	cmake ${WORKDIR}/examples/${example_name}

	# exit on error
	if [[ $? != 0 ]]; then
		popd
		return 1
	fi

	make -j$(nproc)
	popd
}

function run_example_standalone() {
	example_name=${1}
	pool_path=${2}
	optional_args=${@:3}
	echo "Run standalone example: ${example_name} with pool path: ${pool_path}"

	pushd ${EXAMPLE_TEST_DIR}

	PMEM_IS_PMEM_FORCE=${TESTS_USE_FORCED_PMEM} ./${example_name} ${pool_path} ${optional_args}

	# exit on error
	if [[ $? != 0 ]]; then
		popd
		return 1
	fi

	rm -f ${pool_path}
	popd
}

function workspace_cleanup() {
	echo "Cleanup build dirs and example poolset:"

	pushd ${WORKDIR}
	rm -rf ${WORKDIR}/build
	rm -rf ${EXAMPLE_TEST_DIR}
	pmempool rm -f ${WORKDIR}/examples/example.poolset
}

### Additional checks, to be run, when this file is sourced
if [[ -z "${WORKDIR}" ]]; then
	echo "ERROR: The variable WORKDIR has to contain a path to the root " \
		"of this project - 'build' sub-directory will be created there."
	exit 1
fi

# this should be run only on CIs
if [ "${CI_RUN}" == "YES" ]; then
	sudo_password chown -R $(id -u).$(id -g) ${WORKDIR}
fi || true

echo "CMake version:"
cmake --version

# assign CMake's version to variable(s)  - a single number representation for easier comparison
CMAKE_VERSION=$(cmake --version | head -n1 | grep -P -o "\d+\.\d+")
CMAKE_VERSION_MAJOR=$(echo ${CMAKE_VERSION} | cut -d. -f1)
CMAKE_VERSION_MINOR=$(echo ${CMAKE_VERSION} | cut -d. -f2)
CMAKE_VERSION_NUMBER=${CMAKE_VERSION_MAJOR}${CMAKE_VERSION_MINOR}
