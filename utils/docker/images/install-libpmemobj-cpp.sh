#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

#
# install-libpmemobj-cpp.sh <package_type>
#		- installs PMDK C++ bindings (libpmemobj-cpp)
#

set -e

if [ "${SKIP_LIBPMEMOBJCPP_BUILD}" ]; then
	echo "Variable 'SKIP_LIBPMEMOBJCPP_BUILD' is set; skipping building libpmemobj-cpp"
	exit
fi

PREFIX=/usr
PACKAGE_TYPE=$1

# common: 1.10 release; 28.05.2020
LIBPMEMOBJ_CPP_VERSION="1.10"

git clone https://github.com/pmem/libpmemobj-cpp --shallow-since=2019-10-02
cd libpmemobj-cpp
git checkout $LIBPMEMOBJ_CPP_VERSION

mkdir build
cd build

cmake .. -DCPACK_GENERATOR="$PACKAGE_TYPE" -DCMAKE_INSTALL_PREFIX=$PREFIX

if [ "$PACKAGE_TYPE" = "" ]; then
	make -j$(nproc) install
else
	make -j$(nproc) package
	if [ "$PACKAGE_TYPE" = "DEB" ]; then
		sudo dpkg -i libpmemobj++*.deb
	elif [ "$PACKAGE_TYPE" = "RPM" ]; then
		sudo rpm -i libpmemobj++*.rpm
	fi
fi

cd ../..
rm -r libpmemobj-cpp
