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
# run-bindings.sh - checks bindings' building and installation
#

PREFIX=/usr/local

set -e

export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
export JAVA_TOOL_OPTIONS=-Dfile.encoding=UTF-8

# build and install pmemkv
cd $WORKDIR
# copy Googletest to the current directory
cp /opt/googletest/googletest-*.zip .
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DTBB_DIR=/opt/tbb/cmake \
	-DCMAKE_INSTALL_PREFIX=$PREFIX
make -j2
echo $USERPASS | sudo -S make install

# build and install ruby binding
echo
echo "################################################################"
echo "### Veryfing building and installing of the pmemkv-ruby binding"
echo "################################################################"
cd ~
echo $USERPASS | sudo -S gem install bundler -v '< 2.0'
git clone https://github.com/pmem/pmemkv-ruby.git
cd pmemkv-ruby
echo $USERPASS | sudo -S bundle install
LD_LIBRARY_PATH=$PREFIX/lib/:/opt/tbb/lib/intel64/gcc4.7/ bundle exec rspec

# build and install jni and java bindings
echo
echo "#########################################################################"
echo "### Veryfing building and installing of the pmemkv-jni and java bindings"
echo "#########################################################################"
cd ~
git clone https://github.com/pmem/pmemkv-jni.git
cd pmemkv-jni
# copy Googletest to the current directory
cp /opt/googletest/googletest-*.zip .
make test
echo $USERPASS | sudo -S make install prefix=$PREFIX

cd ~
git clone https://github.com/pmem/pmemkv-java.git
cd pmemkv-java
LD_LIBRARY_PATH=$PREFIX/lib/:/opt/tbb/lib/intel64/gcc4.7/ mvn install

# build and install NodeJS binding
echo
echo "##################################################################"
echo "### Veryfing building and installing of the pmemkv-nodejs binding"
echo "##################################################################"
cd ~
git clone https://github.com/pmem/pmemkv-nodejs.git
cd pmemkv-nodejs
npm install --save
LD_LIBRARY_PATH=$PREFIX/lib/:/opt/tbb/lib/intel64/gcc4.7/ npm test
