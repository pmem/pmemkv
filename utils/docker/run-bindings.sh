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

PREFIX=/usr

# Merge pull request #42 from ldorau/Fix-repo-name-in-CONTRIBUTING.md, 22.10.2019
RUBY_VERSION=972057c8c8481485b1be4b9f73ba39c869197bf8

# Merge pull request #32 from ldorau/Fix-repo-name-in-CONTRIBUTING.md, 22.10.2019
JNI_VERSION=85b49234e9ec35e83259eb8f7c14f52f431a74b0

# Merge pull request #31 from ldorau/Fix-repo-name-in-CONTRIBUTING.md, 22.10.2019
JAVA_VERSION=e83a99b9a764bd961103ceef1b2c971022ce0827

# Merge pull request #46 from ldorau/Add-j$nproc-to-make-command, 22.10.2019
NODEJS_VERSION=06dc6313d2a20a8dd6d4b4d30544b32d9d0e8d90

# Merge pull request #13 from ldorau/Fix-repo-name-in-CONTRIBUTING.md, 22.10.2019
PYTHON_VERSION=4b10e5230d79cb2d440d015248c6ddb3cd2502de

set -e

export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
export JAVA_TOOL_OPTIONS=-Dfile.encoding=UTF-8

function sudo_password() {
	echo $USERPASS | sudo -Sk $*
}

# build and install pmemkv
cd $WORKDIR
mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DCMAKE_INSTALL_PREFIX=$PREFIX
make -j$(nproc)
sudo_password -S make -j$(nproc) install

echo
echo "##################################################################"
echo "### Verifying building and installing of the pmemkv-ruby bindings "
echo "##################################################################"
cd ~
git clone https://github.com/pmem/pmemkv-ruby.git
cd pmemkv-ruby
git checkout $RUBY_VERSION
mkdir -p vendor/cache/
cp -r /opt/bindings/ruby/* vendor/cache/
bundle install --local
bundle exec rspec

echo
echo "#################################################################"
echo "### Verifying building and installing of the pmemkv-jni bindings "
echo "#################################################################"
cd ~
git clone https://github.com/pmem/pmemkv-jni.git
cd pmemkv-jni
git checkout $JNI_VERSION

# XXX remove when bindings switched to using gtest installed in system
# Copy Googletest to the current directory
cp /opt/googletest/googletest-*.zip .
make test
sudo_password -S make install prefix=$PREFIX

echo
echo "##################################################################"
echo "### Verifying building and installing of the pmemkv-java bindings "
echo "##################################################################"
cd ~
git clone https://github.com/pmem/pmemkv-java.git
cd pmemkv-java
git checkout $JAVA_VERSION
mkdir -p ~/.m2/repository
cp -r /opt/bindings/java/repository ~/.m2/
mvn --offline install

echo
echo "####################################################################"
echo "### Verifying building and installing of the pmemkv-nodejs bindings "
echo "####################################################################"
cd ~
git clone https://github.com/pmem/pmemkv-nodejs.git
cd pmemkv-nodejs
git checkout $NODEJS_VERSION
cp -r /opt/bindings/nodejs/node_modules .
npm install --save
npm test

echo
echo "####################################################################"
echo "### Verifying building and installing of the pmemkv-python bindings "
echo "####################################################################"
cd ~
git clone https://github.com/pmem/pmemkv-python.git
cd pmemkv-python
git checkout $PYTHON_VERSION
python3 setup.py install --user
cd tests
python3 -m unittest -v pmemkv_tests.py
cd ../examples
python3 basic_example.py
