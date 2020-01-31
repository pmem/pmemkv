#!/usr/bin/env bash
#
# Copyright 2019-2020, Intel Corporation
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
# install-pmdk.sh <package_type> - installs PMDK
#

set -e

if [ "${SKIP_PMDK_BUILD}" ]; then
	echo "Variable 'SKIP_PMDK_BUILD' is set; skipping building PMDK"
	exit
fi

PACKAGE_TYPE=$1
PREFIX=${2:-/usr}

# v1.8, 31.01.2020
PMDK_VERSION="1.8"

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
