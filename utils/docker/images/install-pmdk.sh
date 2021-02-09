#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2021, Intel Corporation

#
# install-pmdk.sh <package_type> - installs PMDK
#

set -e

if [ "${SKIP_PMDK_BUILD}" ]; then
	echo "Variable 'SKIP_PMDK_BUILD' is set; skipping building PMDK"
	exit
fi

PACKAGE_TYPE=$1
PREFIX=${2:-/usr}

# common: 1.9.1, 16.09.2020
PMDK_VERSION="1.9.1"

git clone https://github.com/pmem/pmdk
cd pmdk
git checkout $PMDK_VERSION

if [ "$PACKAGE_TYPE" = "" ]; then
	make -j$(nproc) install prefix=$PREFIX
else
	make -j$(nproc) BUILD_PACKAGE_CHECK=n $PACKAGE_TYPE
	if [ "$PACKAGE_TYPE" = "dpkg" ]; then
		sudo dpkg -i dpkg/libpmem_*.deb dpkg/libpmem-dev_*.deb
		sudo dpkg -i dpkg/libpmemobj_*.deb dpkg/libpmemobj-dev_*.deb
		sudo dpkg -i dpkg/pmreorder_*.deb
		sudo dpkg -i dpkg/libpmemblk_*.deb dpkg/libpmemlog_*.deb dpkg/libpmempool_*.deb dpkg/pmempool_*.deb
	elif [ "$PACKAGE_TYPE" = "rpm" ]; then
		sudo rpm -i rpm/*/pmdk-debuginfo-*.rpm
		sudo rpm -i rpm/*/libpmem*-*.rpm
		sudo rpm -i rpm/*/pmreorder-*.rpm
		sudo rpm -i rpm/*/pmempool-*.rpm
	fi
fi

cd ..
rm -r pmdk
