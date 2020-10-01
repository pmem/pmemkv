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

PREFIX="/usr"
PACKAGE_TYPE=${1^^} #To uppercase
echo "PACKAGE_TYPE: ${PACKAGE_TYPE}"

# 1.11; 30.09.2020
LIBPMEMOBJ_CPP_VERSION="1.11"
echo "LIBPMEMOBJ_CPP_VERSION: ${LIBPMEMOBJ_CPP_VERSION}"

build_dir=$(mktemp -d -t libpmemobj-cpp-XXX)

git clone https://github.com/pmem/libpmemobj-cpp --shallow-since=2019-10-02 ${build_dir}

pushd ${build_dir}
git checkout ${LIBPMEMOBJ_CPP_VERSION}

mkdir build
cd build

cmake .. -DCPACK_GENERATOR="${PACKAGE_TYPE}" -DCMAKE_INSTALL_PREFIX=${PREFIX}

if [ "${PACKAGE_TYPE}" = "" ]; then
	make -j$(nproc) install
else
	make -j$(nproc) package
	if [ "${PACKAGE_TYPE}" = "DEB" ]; then
		sudo dpkg -i libpmemobj++*.deb
	elif [ "${PACKAGE_TYPE}" = "RPM" ]; then
		sudo rpm -iv libpmemobj++*.rpm
	fi
fi

popd
rm -r ${build_dir} 
