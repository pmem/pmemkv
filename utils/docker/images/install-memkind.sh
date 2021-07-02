#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2021, Intel Corporation

#
# install-memkind.sh <OS> - installs memkind from sources; depends on
#		the system it uses proper installation paramaters
#

set -e

OS=$1

# contains better error handling in pmem_allocator
MEMKIND_VERSION=64a37d4ca46a8a2419871e859d611fff85d0e843

WORKDIR=$(pwd)

git clone https://github.com/memkind/memkind
cd $WORKDIR/memkind
git checkout $MEMKIND_VERSION

# set OS-specific configure options
OS_SPECIFIC=""
case $(echo $OS | cut -d'-' -f1) in
	centos|opensuse)
		OS_SPECIFIC="--libdir=/usr/lib64"
		;;
esac

./autogen.sh
./configure --prefix=/usr $OS_SPECIFIC
make -j$(nproc)
make -j$(nproc) install

# cleanup
cd $WORKDIR
rm -r memkind
