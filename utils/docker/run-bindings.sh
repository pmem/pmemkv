#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

#
# run-bindings.sh - checks bindings' building and installation
#

set -e

source `dirname $0`/prepare-for-build.sh

# master: Merge pull request #44 from lukaszstolarczuk/update-tra..., 21.11.2019
RUBY_VERSION="3741e3df698245fc8a15822a1aa85b5c211fd332"

# master: Merge pull request #34 from igchor/add_pmemkv_errormsg, 06.12.2020
JNI_VERSION="fcc8370b230ab3236d062a121e22dcebf37b90ec"

# master: Merge pull request #38 from lukaszstolarczuk/update-tra..., 17.03.2020
JAVA_VERSION="ab8747c3baf4af8cd2ce1985986d7fcc241ccd65"

# master: Merge pull request #49 from how759/buffer-arguments, 02.03.2020
NODEJS_VERSION="12ecc0a9c3205425bf0aa1767eada53834535045"

# master: ver. 1.0, 03.03.2020
PYTHON_VERSION="094bc84fdabff81c2eb2017d32caad2582835f90"

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
python3 setup.py bdist_wheel
pip3 install dist/pmemkv-*.whl --user
cd tests
python3 -m unittest -v pmemkv_tests.py
cd ../examples
PMEM_IS_PMEM_FORCE=1 python3 basic_example.py
