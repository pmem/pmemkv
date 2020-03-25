#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

#
# run-build.sh - is called inside a Docker container,
#                starts pmemkv build with tests.
#

set -e

source `dirname $0`/prepare-for-build.sh

TEST_DIR=${PMEMKV_TEST_DIR:-${DEFAULT_TEST_DIR}}
BUILD_JSON_CONFIG=${BUILD_JSON_CONFIG:-ON}
CHECK_CPP_STYLE=${CHECK_CPP_STYLE:-ON}

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
	-DDEVELOPER_MODE=1 \
	-DTESTS_USE_FORCED_PMEM=1 \
	-DTESTS_PMEMOBJ_DRD_HELGRIND=1

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

# Expect failure - non-existing path is passed
run_example_standalone pmemkv_open_cpp /non-existing/path && exit 1

# Uninstall libraries
cd $WORKDIR/build
sudo_password -S make uninstall

cd $WORKDIR
rm -rf $WORKDIR/build
