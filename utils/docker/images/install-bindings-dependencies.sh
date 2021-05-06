#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2021, Intel Corporation

#
# install-bindings-dependencies.sh - installs dependencies for bindings
#		so they can be built offline. This step is optional while
#		setting up local environment.
#

set -e

# master: Merge pull request #987 from igchor/num_of_elems, 22.04.2021
PMEMKV_VERSION="09a24d2f456a062a02a190c8d81777d80295337c"

# master: Merge pull request #44 from lukaszstolarczuk/update-tr..., 21.11.2019
RUBY_VERSION="3741e3df698245fc8a15822a1aa85b5c211fd332"

# common: 1.2.0 release, 02.07.2021
JAVA_VERSION="9a32f9f518198ae575242b448f61514c231b5a60"

# master: Merge pull request #55 from lukaszstolarczuk/fix-comment, 03.04.2020
NODEJS_VERSION="76600e002b9d9105d3f46b7cc2bf991931286cec"

PREFIX=/usr
rm -rf /opt/bindings

WORKDIR=$(pwd)

#
# Build and install PMEMKV - Java will need it
#
git clone https://github.com/pmem/pmemkv.git
cd pmemkv
git checkout ${PMEMKV_VERSION}
mkdir build
cd build
# Pmemkv at this point is needed only to be linked with JNI. As tests are skipped,
# engines are not needed.
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DCMAKE_INSTALL_PREFIX=${PREFIX} \
	-DBUILD_DOC=OFF \
	-DBUILD_EXAMPLES=OFF \
	-DBUILD_TESTS=OFF \
	-DENGINE_CMAP=OFF \
	-DENGINE_VSMAP=OFF \
	-DENGINE_VCMAP=OFF \
	-DENGINE_STREE=OFF
make -j$(nproc)
make -j$(nproc) install

#
# RUBY dependencies - all of the dependencies (gems) needed to run
#                        pmemkv-ruby will be saved
#                        in the /opt/bindings/ruby directory
cd ${WORKDIR}
mkdir -p /opt/bindings/ruby/
gem install bundler -v '< 2.0'
git clone https://github.com/pmem/pmemkv-ruby.git
cd pmemkv-ruby
git checkout ${RUBY_VERSION}
# bundle package command copies all of the .gem files needed to run
# the application into the vendor/cache directory
bundle package
mv -v vendor/cache/* /opt/bindings/ruby/

#
# JAVA dependencies - all of the dependencies needed to run
#                        pmemkv-java will be saved
#                        in the /opt/bindings/java directory
cd ${WORKDIR}
mkdir -p /opt/bindings/java/

git clone https://github.com/pmem/pmemkv-java.git
cd pmemkv-java
git checkout ${JAVA_VERSION}
mvn install -Dmaven.test.skip=true
mvn dependency:go-offline
rm -r ~/.m2/repository/io/pmem
mv -v ~/.m2/repository /opt/bindings/java/

#
# NodeJS dependencies - all of the dependencies needed to run
#                          pmemkv-nodejs will be saved
#                          in the /opt/bindings/nodejs/ directory
cd ${WORKDIR}
mkdir -p /opt/bindings/nodejs/
git clone https://github.com/pmem/pmemkv-nodejs.git
cd pmemkv-nodejs
git checkout ${NODEJS_VERSION}
npm install --save
cp -rv ./node_modules /opt/bindings/nodejs/

#
# Uninstall all installed stuff
#
cd ${WORKDIR}/pmemkv/build
make uninstall

cd ${WORKDIR}/pmemkv-nodejs
npm uninstall

cd ${WORKDIR}
rm -r pmemkv pmemkv-ruby pmemkv-java pmemkv-nodejs

# make the /opt/bindings directory world-readable
chmod -R a+r /opt/bindings
