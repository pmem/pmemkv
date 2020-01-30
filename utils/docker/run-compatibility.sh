#!/usr/bin/env bash
#
# Copyright 2020, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#
#     * Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# run-compatibility.sh - verify compatibility
#

set -e

TEST_DIR=${PMEMKV_TEST_DIR:-${DEFAULT_TEST_DIR}}

PREFIX_HEAD=/opt/pmemkv-head
PREFIX_1_0_1=/opt/pmemkv-1.0.1

function sudo_password() {
	echo $USERPASS | sudo -Sk $*
}

./prepare-for-build.sh

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
