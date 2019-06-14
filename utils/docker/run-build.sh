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
# run-build.sh - is called inside a Docker container,
#                starts pmemkv build with tests.
#

PREFIX=/usr/local

set -e
echo $USERPASS | sudo -S mount -oremount,size=4G /dev/shm

function cleanup() {
	find . -name ".coverage" -exec rm {} \;
	find . -name "coverage.xml" -exec rm {} \;
	find . -name "*.gcov" -exec rm {} \;
	find . -name "*.gcda" -exec rm {} \;
}

function upload_codecov() {
	clang_used=$(cmake -LA -N . | grep CMAKE_CXX_COMPILER | grep clang | wc -c)

	if [[ $clang_used > 0 ]]; then
		gcovexe="llvm-cov gcov"
	else
		gcovexe="gcov"
	fi

	# the output is redundant in this case, i.e. we rely on parsed report from codecov on github
	bash <(curl -s https://codecov.io/bash) -c -F $1 -x "$gcovexe"
	cleanup
}

EXAMPLE_TEST_DIR="/tmp/build_example"

function compile_example_standalone() {
	rm -rf $EXAMPLE_TEST_DIR
	mkdir $EXAMPLE_TEST_DIR
	cd $EXAMPLE_TEST_DIR

	cmake $WORKDIR/examples/$1

	# exit on error
	if [[ $? != 0 ]]; then
		cd -
		return 1
	fi

	make
	cd -
}

function run_example_standalone() {
	cd $EXAMPLE_TEST_DIR

	./$1
	# exit on error
	if [[ $? != 0 ]]; then
		cd -
		return 1
	fi

	cd -
}

cd $WORKDIR

# copy Googletest to the current directory
cp /opt/googletest/googletest-*.zip .

# Make sure there is no libpmemkv currently installed
echo "---------------------------- Error expected! ------------------------------"
compile_example_standalone pmemkv_basic_cpp && exit 1
echo "---------------------------------------------------------------------------"

# make & install
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
	-DTEST_DIR=/dev/shm \
	-DTBB_DIR=/opt/tbb/cmake \
	-DCMAKE_INSTALL_PREFIX=$PREFIX \
	-DCOVERAGE=$COVERAGE \
	-DDEVELOPER_MODE=1

make -j2
ctest --output-on-failure
echo $USERPASS | sudo -S make install

if [ "$COVERAGE" == "1" ]; then
	upload_codecov tests
fi

# Verify installed libraries
compile_example_standalone pmemkv_basic_c
compile_example_standalone pmemkv_basic_cpp
run_example_standalone pmemkv_basic_c
run_example_standalone pmemkv_basic_cpp

# Uninstall libraries
cd $WORKDIR/build
echo $USERPASS | sudo -S make uninstall

# verify if each engine is building properly
engines_flags=(
	ENGINE_VSMAP
	ENGINE_VCMAP
	ENGINE_CMAP
	# XXX: caching engine requires libacl and memcached installed in docker images
	# and firstly we need to remove hardcoded INCLUDE paths (see #244)
	# ENGINE_CACHING
	ENGINE_STREE
	ENGINE_TREE3
	# the last item is to test all engines disabled
	BLACKHOLE_TEST
)

for engine_flag in "${engines_flags[@]}"
do
	cd $WORKDIR/build
	rm -rf *
	# testing each engine separately; disabling default engines
	echo
	echo "##############################################################"
	echo "### Verifying building of the '$engine_flag' engine"
	echo "##############################################################"
	cmake .. -DTBB_DIR=/opt/tbb/cmake \
		-DENGINE_VSMAP=OFF \
		-DENGINE_VCMAP=OFF \
		-DENGINE_CMAP=OFF \
		-D$engine_flag=ON
	make -j2
	# list all tests in this build
	ctest -N
done

echo
echo "##############################################################"
echo "### Verifying building of all engines"
echo "##############################################################"
cd $WORKDIR/build
rm -rf *
cmake .. -DTBB_DIR=/opt/tbb/cmake \
	-DENGINE_VSMAP=ON \
	-DENGINE_VCMAP=ON \
	-DENGINE_CMAP=ON \
	-DENGINE_STREE=ON \
	-DENGINE_TREE3=ON
make -j2
# list all tests in this build
ctest -N

echo
echo "##############################################################"
echo "### Verifying building of packages"
echo "##############################################################"
cd $WORKDIR/build
rm -rf *
cmake .. -DCMAKE_BUILD_TYPE=Debug \
	-DTEST_DIR=/dev/shm \
	-DTBB_DIR=/opt/tbb/cmake \
	-DCMAKE_INSTALL_PREFIX=$PREFIX \
	-DDEVELOPER_MODE=1 \
	-DCPACK_GENERATOR=$PACKAGE_MANAGER

# Make sure there is no libpmemkv currently installed
echo "---------------------------- Error expected! ------------------------------"
compile_example_standalone pmemkv_basic_cpp && exit 1
echo "---------------------------------------------------------------------------"

# Build and install packages
make package

if [ $PACKAGE_MANAGER = "dpkg" ]; then
	sudo_password dpkg -i libpmemkv*.deb
elif [ $PACKAGE_MANAGER = "rpm" ]; then
	sudo_password rpm -i libpmemkv*.rpm
fi

# Verify installed packages
compile_example_standalone pmemkv_basic_c
compile_example_standalone pmemkv_basic_cpp
run_example_standalone pmemkv_basic_c
run_example_standalone pmemkv_basic_cpp
