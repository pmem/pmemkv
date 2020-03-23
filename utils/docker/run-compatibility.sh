#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation

#
# run-compatibility.sh - verify compatibility between versions
#

set -e

TEST_DIR=${PMEMKV_TEST_DIR:-${DEFAULT_TEST_DIR}}

PREFIX_HEAD=/opt/pmemkv-head
PREFIX_1_0_1=/opt/pmemkv-1.0.1

source `dirname $0`/prepare-for-build.sh

# build and install pmemkv head
mkdir $WORKDIR/build
cd $WORKDIR/build

cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DCMAKE_INSTALL_PREFIX=$PREFIX_HEAD
make -j$(nproc)
sudo_password -S make -j$(nproc) install

cd $WORKDIR
rm -rf $WORKDIR/build

# build and install pmemkv 1.0.1
mkdir $WORKDIR/build
cd $WORKDIR/build

git clone https://github.com/pmem/pmemkv pmemkv-1.0.1
cd pmemkv-1.0.1
git checkout 1.0.1
mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DBUILD_TESTS=OFF \
	-DCMAKE_INSTALL_PREFIX=$PREFIX_1_0_1
make -j$(nproc)
sudo_password -S make -j$(nproc) install

cd $WORKDIR
rm -rf $WORKDIR/build

echo
echo "##################################################################"
echo "### Verifying compatibility with 1.0.1"
echo "##################################################################"

mkdir $WORKDIR/build
cd $WORKDIR/build

mkdir head
cd head
PKG_CONFIG_PATH=$PREFIX_HEAD/lib64/pkgconfig cmake ../../tests/compatibility
make -j$(nproc)
cd ..

mkdir 1.0.1
cd 1.0.1
PKG_CONFIG_PATH=$PREFIX_1_0_1/lib64/pkgconfig cmake ../../tests/compatibility
make -j$(nproc)
cd ..

PMEM_IS_PMEM_FORCE=1 $WORKDIR/tests/compatibility/cmap.sh $WORKDIR/build/head/cmap_compatibility $WORKDIR/build/1.0.1/cmap_compatibility $TEST_DIR/testfile
