#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

#
# run-build.sh - is called inside a Docker container,
#                starts pmemkv build with tests.
#

set -e

./prepare-for-build.sh

EXAMPLE_TEST_DIR="/tmp/build_example"
PREFIX=/usr
TEST_DIR=${PMEMKV_TEST_DIR:-${DEFAULT_TEST_DIR}}
BUILD_JSON_CONFIG=${BUILD_JSON_CONFIG:-ON}
CHECK_CPP_STYLE=${CHECK_CPP_STYLE:-ON}

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
	rm -rf $EXAMPLE_TEST_DIR
	mkdir $EXAMPLE_TEST_DIR
	cd $EXAMPLE_TEST_DIR

	cmake $WORKDIR/examples/$1

	# exit on error
	if [[ $? != 0 ]]; then
		cd -
		return 1
	fi

	make -j$(nproc)
	cd -
}

function run_example_standalone() {
	cd $EXAMPLE_TEST_DIR

	./$1 $2

	# exit on error
	if [[ $? != 0 ]]; then
		cd -
		return 1
	fi

	rm -f $2

	cd -
}

cd $WORKDIR

echo
echo "### Making sure there is no libpmemkv currently installed"
echo "---------------------------- Error expected! ------------------------------"
compile_example_standalone pmemkv_basic_cpp && exit 1
echo "---------------------------------------------------------------------------"

echo
echo "##############################################################"
echo "### Verify build and install (in dir: ${PREFIX})"
echo "##############################################################"

mkdir $WORKDIR/build
cd $WORKDIR/build

cmake .. -DCMAKE_BUILD_TYPE=Debug \
	-DTEST_DIR=$TEST_DIR \
	-DCMAKE_INSTALL_PREFIX=$PREFIX \
	-DCOVERAGE=$COVERAGE \
	-DENGINE_STREE=1 \
	-DBUILD_JSON_CONFIG=${BUILD_JSON_CONFIG} \
	-DCHECK_CPP_STYLE=${CHECK_CPP_STYLE} \
	-DDEVELOPER_MODE=1

make -j$(nproc)
make -j$(nproc) doc
ctest --output-on-failure
sudo_password -S make -j$(nproc) install

if [ "$COVERAGE" == "1" ]; then
	upload_codecov tests
fi

# Verify installed libraries
compile_example_standalone pmemkv_basic_c
run_example_standalone pmemkv_basic_c pool
compile_example_standalone pmemkv_basic_cpp
run_example_standalone pmemkv_basic_cpp pool
if [ "$BUILD_JSON_CONFIG" == "ON" ]; then
	compile_example_standalone pmemkv_config_c
	run_example_standalone pmemkv_config_c pool
fi
compile_example_standalone pmemkv_pmemobj_cpp
run_example_standalone pmemkv_pmemobj_cpp pool

# Poolset example
compile_example_standalone pmemkv_open_cpp
pmempool create -l "pmemkv" obj $WORKDIR/examples/example.poolset
run_example_standalone pmemkv_open_cpp $WORKDIR/examples/example.poolset

# Expect failure - non-existsing path is passed
run_example_standalone pmemkv_open_cpp /non-existing/path && exit 1

# Uninstall libraries
cd $WORKDIR/build
sudo_password -S make uninstall

cd $WORKDIR
rm -rf $WORKDIR/build
