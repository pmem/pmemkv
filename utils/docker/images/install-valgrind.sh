#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2016-2019, Intel Corporation

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
# valgrind v3.15 with pmemcheck
git checkout c27a8a2f973414934e63f1e94bc84c0a580e3840

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
