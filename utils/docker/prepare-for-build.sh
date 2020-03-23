#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

#
# prepare-for-build.sh - prepare the Docker image for the build
#

set -e

EXAMPLE_TEST_DIR="/tmp/build_example"
PREFIX=/usr

function sudo_password() {
	echo $USERPASS | sudo -Sk $*
}

function cleanup() {
	find . -name ".coverage" -exec rm {} \;
	find . -name "coverage.xml" -exec rm {} \;
	find . -name "*.gcov" -exec rm {} \;
	find . -name "*.gcda" -exec rm {} \;
}

function upload_codecov() {
	clang_used=$(cmake -LA -N . | grep CMAKE_CXX_COMPILER | grep clang | wc -c)

	if [[ $clang_used > 0 ]]; then
		gcovexe="llvm-cov gcov"
	else
		gcovexe="gcov"
	fi

	# the output is redundant in this case, i.e. we rely on parsed report from codecov on github
	bash <(curl -s https://codecov.io/bash) -c -F $1 -x "$gcovexe"
	cleanup
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
