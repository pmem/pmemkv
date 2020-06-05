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
	clang_used=$(cmake -LA -N . | grep CMAKE_CXX_COMPILER | grep clang | wc -c)

	if [[ $clang_used > 0 ]]; then
		gcovexe="llvm-cov gcov"
	else
		gcovexe="gcov"
	fi

	# run gcov exe, using their bash (set flag and remove parsed coverage files)
	bash <(curl -s https://codecov.io/bash) -c -F $1 -x "$gcovexe"

	find . -name ".coverage" -exec rm {} \;
	find . -name "coverage.xml" -exec rm {} \;
	find . -name "*.gcov" -exec rm {} \;
	find . -name "*.gcda" -exec rm {} \;
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
