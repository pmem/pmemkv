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

# Merge pull request #36 from ldorau/Replace-PMEMKV_STATUS_FAILED-with-PMEMKV_STATUS_UNKNOWN_ERROR, 18.09.2019
RUBY_VERSION=99d1bfc05d116d35d0e96541ece9b9df831d95a0

# Merge pull request #29 from ldorau/Replace-PMEMKV_STATUS_FAILED-with-PMEMKV_STATUS_UNKNOWN_ERROR, 18.09.2019
JNI_VERSION=78b81de8266ec690fb41b5f4e62948e200640cbe

# Merge pull request #26 from ldorau/Update-gitignore, 16.09.2019
JAVA_VERSION=30c2a897574aa2552bd3e651e4e57f2469da5767

# Merge pull request #38 from lukaszstolarczuk/replace-json-config, 01.10.2019
NODEJS_VERSION=9e3f0edd8f10d0d6f011fd22354ce1b4efe36f84

# Merge pull request #10 from ldorau/Throw-an-exception-in-pmemkv_NI_Get, 01.10.2019
PYTHON_VERSION=824bfec5976cb19e387b32d82fc19c763000ef88

set -e

./prepare-for-build.sh

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
PMEM_IS_PMEM_FORCE=1 python3 basic_example.py
