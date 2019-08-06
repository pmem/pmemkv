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

PREFIX=/usr
rm -rf /opt/bindings

#
# 1) RUBY dependencies - all of the dependencies (gems) needed to run
#                        pmemkv-ruby will be saved
#                        in the /opt/bindings/ruby directory
mkdir -p /opt/bindings/ruby/
gem install bundler -v '< 2.0'
git clone https://github.com/pmem/pmemkv-ruby.git
cd pmemkv-ruby
git checkout 0.8
# bundle package command copies all of the .gem files needed to run
# the application into the vendor/cache directory
bundle package
mv -v vendor/cache/* /opt/bindings/ruby/
cd ..
rm -rf pmemkv-ruby

#
# 2) JAVA dependencies - all of the dependencies needed to run
#                        pmemkv-java will be saved
#                        in the /opt/bindings/java directory
mkdir -p /opt/bindings/java/
export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
export JAVA_TOOL_OPTIONS=-Dfile.encoding=UTF-8

# build and install pmemkv
git clone https://github.com/pmem/pmemkv.git
cd pmemkv
git checkout 0.8
cp /opt/googletest/googletest-*.zip .
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DCMAKE_INSTALL_PREFIX=$PREFIX
make -j2
make install
cd ../..

# build and install pmemkv-jni
git clone https://github.com/pmem/pmemkv-jni.git
cd pmemkv-jni
git checkout 0.8
cp /opt/googletest/googletest-*.zip .
make
make install prefix=$PREFIX
cd ..

# build and install pmemkv-java
git clone https://github.com/pmem/pmemkv-java.git
cd pmemkv-java
git checkout 0.8
mvn dependency:go-offline
mvn install
mv -v ~/.m2/repository /opt/bindings/java/
cd ..

# uninstall all installed things
cd pmemkv/build
make uninstall
cd ../../pmemkv-jni
make uninstall
cd ..
rm -rf pmemkv pmemkv-jni pmemkv-java

# make the /opt/bindings directory world-readable
chmod -R a+r /opt/bindings
