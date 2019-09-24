#!/usr/bin/env bash
#
# Copyright 2019, Intel Corporation
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
# install-bindings-dependencies.sh - installs dependencies for bindings
# that they can be built offline
#

set -e

# Merge pull request #442 from ldorau/Install-pmemkv-nodejs-dependencies-in-the-Docker-image, 23.09.2019
PMEMKV_VERSION=a7746b7d7f2d40ebd1272cebffdc037c449c3915

# Merge pull request #36 from ldorau/Replace-PMEMKV_STATUS_FAILED-with-PMEMKV_STATUS_UNKNOWN_ERROR, 18.09.2019
RUBY_VERSION=99d1bfc05d116d35d0e96541ece9b9df831d95a0

# Merge pull request #29 from ldorau/Replace-PMEMKV_STATUS_FAILED-with-PMEMKV_STATUS_UNKNOWN_ERROR, 18.09.2019
JNI_VERSION=78b81de8266ec690fb41b5f4e62948e200640cbe

# Merge pull request #26 from ldorau/Update-gitignore, 16.09.2019
JAVA_VERSION=30c2a897574aa2552bd3e651e4e57f2469da5767

# Merge pull request #34 from ldorau/Replace-PMEMKV_STATUS_FAILED-with-PMEMKV_STATUS_UNKNOWN_ERROR, 18.09.2019
NODEJS_VERSION=5cf32b58839617618fa4c40af620686a403564c6

PREFIX=/usr
rm -rf /opt/bindings

WORKDIR=$(pwd)

#
# 1) Build and install PMEMKV - Java will need it
#
git clone https://github.com/pmem/pmemkv.git
cd pmemkv
git checkout $PMEMKV_VERSION
cp /opt/googletest/googletest-*.zip .
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DCMAKE_INSTALL_PREFIX=$PREFIX \
	-DENGINE_CMAP=OFF \
	-DENGINE_VCMAP=OFF \
	-DENGINE_VSMAP=OFF \
	-DENGINE_CACHING=OFF \
	-DENGINE_STREE=OFF \
	-DENGINE_TREE3=OFF
make -j2
make -j2 install

#
# 2) RUBY dependencies - all of the dependencies (gems) needed to run
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
# 3) Build and install JNI
#
cd $WORKDIR
git clone https://github.com/pmem/pmemkv-jni.git
cd pmemkv-jni
git checkout $JNI_VERSION
cp /opt/googletest/googletest-*.zip .
make
make install prefix=$PREFIX

#
# 4) JAVA dependencies - all of the dependencies needed to run
#                        pmemkv-java will be saved
#                        in the /opt/bindings/java directory
cd $WORKDIR
mkdir -p /opt/bindings/java/
export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
export JAVA_TOOL_OPTIONS=-Dfile.encoding=UTF-8

git clone https://github.com/pmem/pmemkv-java.git
cd pmemkv-java
git checkout $JAVA_VERSION
mvn dependency:go-offline
mvn install
mv -v ~/.m2/repository /opt/bindings/java/

#
# 5) NodeJS dependencies - all of the dependencies needed to run
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

cd $WORKDIR/pmemkv-jni
make uninstall

cd $WORKDIR/pmemkv-nodejs
npm uninstall

cd $WORKDIR
rm -r pmemkv pmemkv-ruby pmemkv-jni pmemkv-java pmemkv-nodejs


# make the /opt/bindings directory world-readable
chmod -R a+r /opt/bindings
