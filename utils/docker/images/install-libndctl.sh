#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2021, Intel Corporation

#
# install-libndctl.sh - installs libndctl
#

set -e

OS=$1

echo "==== clone ndctl repo ===="
git clone https://github.com/pmem/ndctl.git
cd ndctl
git checkout v69

if [ "$OS" = "fedora" ]; then
	echo "==== setup rpmbuild tree ===="
	rpmdev-setuptree

	RPMDIR=$HOME/rpmbuild/
	VERSION=$(./git-version)
	SPEC=./rhel/ndctl.spec

	echo "==== create source tarball ====="
	git archive --format=tar --prefix="ndctl-${VERSION}/" HEAD | gzip > "$RPMDIR/SOURCES/ndctl-${VERSION}.tar.gz"

	echo "==== build ndctl ===="
	./autogen.sh
	./configure --disable-docs
	make -j$(nproc)

	echo "==== build ndctl packages ===="
	rpmbuild -ba $SPEC

	echo "==== install ndctl packages ===="
	rpm -i $RPMDIR/RPMS/x86_64/*.rpm

	echo "==== cleanup ===="
	rm -rf $RPMDIR
else
	echo "==== set OS-specific options ===="
	OS_SPECIFIC=""
	LIBDIR=/usr/lib
	case $(echo $OS | cut -d'-' -f1) in
		centos|opensuse)
			LIBDIR=/usr/lib64
			;;
		archlinux)
			OS_SPECIFIC="--disable-dependency-tracking"
			;;
	esac

	echo "==== build ndctl ===="
	./autogen.sh
	./configure --libdir=$LIBDIR --disable-docs $OS_SPECIFIC
	make -j$(nproc)

	echo "==== install ndctl ===="
	make -j$(nproc) install

	echo "==== cleanup ===="
fi

cd ..
rm -rf ndctl
