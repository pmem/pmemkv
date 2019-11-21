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
#		so they can be built offline. This step is optional while
#		setting up local environment.
#

set -e

# master: Merge pull request #531 from lukaszstolarczuk/add-skip-build-flags-to-dockerimages, 20.11.2019
PMEMKV_VERSION="6ad5c453f79cb2ddd1ee9ba2a9dff294c6cb7b71"

# master: Merge pull request #44 from lukaszstolarczuk/update-travis-files, 21.11.2019
RUBY_VERSION="3741e3df698245fc8a15822a1aa85b5c211fd332"

# master: Merge pull request #33 from lukaszstolarczuk/update-travis-files, 21.11.2019
JNI_VERSION="5239d6bb3214c56bc45b3296872be50b38bfbab3"

# master: Merge pull request #34 from lukaszstolarczuk/update-offline-de..., 5.12.2019
JAVA_VERSION="47f02b6b52c56ca53fd3dafdff52167719f1e7dd"

# master: Merge pull request #48 from lukaszstolarczuk/update-travis-files, 21.11.2019
NODEJS_VERSION="d19b026207e8a78ebffdccaffb27181a9bdbe51d"

PREFIX=/usr
rm -rf /opt/bindings

WORKDIR=$(pwd)

#
# 1) Build and install PMEMKV - Java will need it
#
git clone https://github.com/pmem/pmemkv.git
cd pmemkv
git checkout $PMEMKV_VERSION
mkdir build
cd build
# only VSMAP engine is enabled, because Java tests need it

cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DCMAKE_INSTALL_PREFIX=$PREFIX \
	-DENGINE_VSMAP=ON \
	-DENGINE_CMAP=OFF \
	-DENGINE_VCMAP=OFF \
	-DENGINE_CACHING=OFF \
	-DENGINE_STREE=OFF \
	-DBUILD_EXAMPLES=OFF \
	-DENGINE_TREE3=OFF
make -j$(nproc)
make -j$(nproc) install

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
make test
make install prefix=$PREFIX

#
# 4) JAVA dependencies - all of the dependencies needed to run
#                        pmemkv-java will be saved
#                        in the /opt/bindings/java directory
cd $WORKDIR
mkdir -p /opt/bindings/java/

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
