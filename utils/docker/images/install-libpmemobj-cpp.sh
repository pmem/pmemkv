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

# v1.9; 31.01.2020
LIBPMEMOBJ_CPP_VERSION="1.9"

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
