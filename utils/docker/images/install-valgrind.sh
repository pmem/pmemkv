#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2016-2020, Intel Corporation

#
# install-valgrind.sh - installs valgrind for persistent memory
#

set -e

if [ "${SKIP_VALGRIND_BUILD}" ]; then
	echo "Variable 'SKIP_VALGRIND_BUILD' is set; skipping building valgrind (pmem's fork)"
	exit
fi

OS=$1

git clone https://github.com/pmem/valgrind.git
cd valgrind
# pmem-3.15: Merge pull request #81 from marcinslusarz/pmem-3.15
git checkout 09f75f69683d862f8456f75484fcdc0dc5508900

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
sudo make -j$(nproc) install
cd ..
rm -r valgrind
