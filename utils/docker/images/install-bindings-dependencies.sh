#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

#
# install-bindings-dependencies.sh - installs dependencies for bindings
#		so they can be built offline. This step is optional while
#		setting up local environment.
#

set -e

# master: Merge pull request #624 from igchor/catch_std_exception, 17.03.2020
PMEMKV_VERSION="a2a70b994d57dd3fa9edbf0c7c9bbebb01dc2f5f"

# master: Merge pull request #44 from lukaszstolarczuk/update-tra..., 21.11.2019
RUBY_VERSION="3741e3df698245fc8a15822a1aa85b5c211fd332"

# master: common: pmemkv-java 1.0 release, 30.06.2020
JAVA_VERSION="bada69f43447d7a664171458e0ca6d5d535feeb3"

# master: Merge pull request #49 from how759/buffer-arguments, 02.03.2020
NODEJS_VERSION="12ecc0a9c3205425bf0aa1767eada53834535045"

PREFIX=/usr
rm -rf /opt/bindings

WORKDIR=$(pwd)

#
# Build and install PMEMKV - Java will need it
#
git clone https://github.com/pmem/pmemkv.git
cd pmemkv
git checkout $PMEMKV_VERSION
mkdir build
cd build

# Pmemkv at this point is needed only to be linked with jni. As tests are skipped,
# engines are not needed.
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DCMAKE_INSTALL_PREFIX=$PREFIX \
	-DENGINE_VSMAP=OFF \
	-DENGINE_CMAP=OFF \
	-DENGINE_CSMAP=OFF \
	-DENGINE_VCMAP=OFF \
	-DENGINE_CACHING=OFF \
	-DENGINE_STREE=OFF \
	-DBUILD_EXAMPLES=OFF \
	-DENGINE_TREE3=OFF
make -j$(nproc)
make -j$(nproc) install

#
# RUBY dependencies - all of the dependencies (gems) needed to run
#                        pmemkv-ruby will be saved
#                        in the /opt/bindings/ruby directory
cd $WORKDIR
mkdir -p /opt/bindings/ruby/
gem install bundler -v '< 2.0'
git clone https://github.com/pmem/pmemkv-ruby.git
cd pmemkv-ruby
git checkout $RUBY_VERSION
# bundle package command copies all of the .gem files needed to run
# the application into the vendor/cache directory
bundle package
mv -v vendor/cache/* /opt/bindings/ruby/

#
# JAVA dependencies - all of the dependencies needed to run
#                        pmemkv-java will be saved
#                        in the /opt/bindings/java directory
cd $WORKDIR
mkdir -p /opt/bindings/java/

git clone https://github.com/pmem/pmemkv-java.git
cd pmemkv-java
git checkout $JAVA_VERSION
mvn install -Dmaven.test.skip=true
mvn dependency:go-offline
rm -r ~/.m2/repository/io/pmem
mv -v ~/.m2/repository /opt/bindings/java/

#
#  NodeJS dependencies - all of the dependencies needed to run
#                          pmemkv-nodejs will be saved
#                          in the /opt/bindings/nodejs/ directory
cd $WORKDIR
mkdir -p /opt/bindings/nodejs/
git clone https://github.com/pmem/pmemkv-nodejs.git
cd pmemkv-nodejs
git checkout $NODEJS_VERSION
npm install --save
cp -rv ./node_modules /opt/bindings/nodejs/

#
# Uninstall all installed stuff
#
cd $WORKDIR/pmemkv/build
make uninstall

cd $WORKDIR/pmemkv-nodejs
npm uninstall

cd $WORKDIR
rm -r pmemkv pmemkv-ruby pmemkv-java pmemkv-nodejs


# make the /opt/bindings directory world-readable
chmod -R a+r /opt/bindings
