#!/usr/bin/env bash
#
# Copyright 2019, Intel Corporation
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
# install-memkind.sh - installs newest memkind (master)
#

set -e

# v1.9.0-73-g1ece023; 25.09.2019, contains new libmemkind namespace
MEMKIND_MASTER_VERSION=1ece023b9c06a68d3f329786d1c4c0e65cef390f

# v1.9.0; latest release
MEMKIND_STABLE_VERSION=v1.9.0

WORKDIR=$(pwd)

git clone https://github.com/memkind/memkind memkind_git
# copy is made because `make clean` (after one installation) may omit some files
cp -R memkind_git memkind_git_stable

# install (in home's subdirectory) current master
cd $WORKDIR/memkind_git
git checkout $MEMKIND_MASTER_VERSION

mkdir /opt/memkind-master
./autogen.sh
./configure --prefix=/opt/memkind-master
make -j$(nproc)
make install

cd $WORKDIR
rm -r memkind_git

# install (in home's subdirectory) latest stable release
cd $WORKDIR/memkind_git_stable
git checkout $MEMKIND_STABLE_VERSION

mkdir /opt/memkind-stable
./build_jemalloc.sh
./autogen.sh
./configure --prefix=/opt/memkind-stable
make -j$(nproc)
make install

cd $WORKDIR
rm -r memkind_git_stable
