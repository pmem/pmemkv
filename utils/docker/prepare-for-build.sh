#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

#
# prepare-for-build.sh - prepare the Docker image for the builds
#                        and defines functions for other scripts.
#

set -e

EXAMPLE_TEST_DIR="/tmp/build_example"
PREFIX=/usr

# CMake's version assigned to variable(s) (a single number representation for easier comparison)
CMAKE_VERSION=$(cmake --version | head -n1 | grep -oE '[0-9].[0-9]*')
CMAKE_VERSION_MAJOR=$(echo $CMAKE_VERSION | cut -d. -f1)
CMAKE_VERSION_MINOR=$(echo $CMAKE_VERSION | cut -d. -f2)
CMAKE_VERSION_NUMBER=$((100 * $CMAKE_VERSION_MAJOR + $CMAKE_VERSION_MINOR))

function sudo_password() {
	echo $USERPASS | sudo -Sk $*
}

function upload_codecov() {
	printf "\n$(tput setaf 1)$(tput setab 7)COVERAGE ${FUNCNAME[0]} START$(tput sgr 0)\n"

	# set proper gcov command
	clang_used=$(cmake -LA -N . | grep CMAKE_CXX_COMPILER | grep clang | wc -c)
	if [[ $clang_used > 0 ]]; then
		gcovexe="llvm-cov gcov"
	else
		gcovexe="gcov"
	fi

	# run gcov exe, using their bash (remove parsed coverage files, set flag and exit 1 if not successful)
	# we rely on parsed report on codecov.io; the output is quite long, hence it's disabled using -X flag
	/opt/scripts/codecov -c -F $1 -Z -x "$gcovexe" -X "gcovout"

	printf "check for any leftover gcov files\n"
	leftover_files=$(find . -name "*.gcov")
	if [[ -n "$leftover_files" ]]; then
		# display found files and exit with error (they all should be parsed)
		echo "$leftover_files"
		return 1
	fi

	printf "$(tput setaf 1)$(tput setab 7)COVERAGE ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

function compile_example_standalone() {
	example_name=$1

	rm -rf $EXAMPLE_TEST_DIR
	mkdir $EXAMPLE_TEST_DIR
	cd $EXAMPLE_TEST_DIR

	cmake $WORKDIR/examples/$example_name

	# exit on error
	if [[ $? != 0 ]]; then
		cd -
		return 1
	fi

	make -j$(nproc)
	cd -
}

function run_example_standalone() {
	example_name=$1
	pool_path=$2

	cd $EXAMPLE_TEST_DIR

	./$example_name $pool_path

	# exit on error
	if [[ $? != 0 ]]; then
		cd -
		return 1
	fi

	rm -f $pool_path
	cd -
}

# this should be run only on CIs
if [ "$CI_RUN" == "YES" ]; then
	sudo_password chown -R $(id -u).$(id -g) $WORKDIR
fi || true
