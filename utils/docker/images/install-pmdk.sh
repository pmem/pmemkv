#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2022, Intel Corporation

#
# install-pmdk.sh [package_type] - installs PMDK
#		from DEB/RPM packages if possible.
#

set -e

if [ "${SKIP_PMDK_BUILD}" ]; then
	echo "Variable 'SKIP_PMDK_BUILD' is set; skipping building PMDK"
	exit
fi

PACKAGE_TYPE=${1}
PREFIX=${2:-/usr}

# master: 1.12.1, 25.08.2022
PMDK_VERSION="786098a024c6fe60e746f2cb1041bcfcd21386c9"

git clone https://github.com/pmem/pmdk
cd pmdk
git checkout ${PMDK_VERSION}

if [ "${PACKAGE_TYPE}" = "" ]; then
	make DOC=n -j$(nproc) install prefix=${PREFIX}
else
	make -j$(nproc) BUILD_PACKAGE_CHECK=n ${PACKAGE_TYPE}
	if [ "${PACKAGE_TYPE}" = "dpkg" ]; then
		sudo dpkg -i dpkg/libpmem_*.deb dpkg/libpmem-dev_*.deb \
			dpkg/libpmemobj_*.deb dpkg/libpmemobj-dev_*.deb \
			dpkg/pmreorder_*.deb dpkg/libpmempool_*.deb dpkg/libpmempool-dev_*.deb \
			dpkg/libpmemblk_*.deb dpkg/libpmemlog_*.deb dpkg/pmempool_*.deb
	elif [ "${PACKAGE_TYPE}" = "rpm" ]; then
		sudo rpm -i rpm/*/pmdk-debuginfo-*.rpm \
			rpm/*/libpmem*-*.rpm \
			rpm/*/pmreorder-*.rpm \
			rpm/*/pmempool-*.rpm
	fi
fi

cd ..
rm -r pmdk
