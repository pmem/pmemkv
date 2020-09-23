#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

#
# install-bindings-dependencies.sh - installs dependencies for bindings
#		so they can be built offline. This step is optional while
#		setting up local environment.
#

set -e

# master: Merge pull request #737 from lukaszstolarczuk/fix-CI-p..., 07.08.2020
PMEMKV_VERSION="88cd43e7e3e6b62f67e73a48650e27e0fb90d7ea"

# master: Merge pull request #44 from lukaszstolarczuk/update-tr..., 21.11.2019
RUBY_VERSION="3741e3df698245fc8a15822a1aa85b5c211fd332"

# master: Merge pull request #90 from lukaszstolarczuk/merge-sta..., 09.09.2020
JAVA_VERSION="e04ca43b5e5590c653cbdec35a87e0279a4f1a7a"

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
# NodeJS dependencies - all of the dependencies needed to run
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
