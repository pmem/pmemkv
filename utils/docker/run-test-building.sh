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
# run-test-building.sh - is called inside a Docker container,
#                        starts testing of pmemkv building
#                        and automatic update of the documentation
#

set -e

EXAMPLE_TEST_DIR="/tmp/build_example"
PREFIX=/usr

function sudo_password() {
	echo $USERPASS | sudo -Sk $*
}

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

	make -j$(nproc)
	cd -
}

function run_example_standalone() {
	cd $EXAMPLE_TEST_DIR

	rm -f pool
	./$1 pool
	# exit on error
	if [[ $? != 0 ]]; then
		cd -
		return 1
	fi

	cd -
}

# Resize /dev/shm, since default one is too small
sudo_password -S mount -oremount,size=4G /dev/shm
echo

cd $WORKDIR

echo
echo "##############################################################"
echo "### Checking C++20 support in g++"
echo "##############################################################"
mkdir $WORKDIR/build
cd $WORKDIR/build

CXX=g++ cmake .. -DCMAKE_BUILD_TYPE=Release \
	-DTEST_DIR=/dev/shm \
	-DCMAKE_INSTALL_PREFIX=$PREFIX \
	-DCOVERAGE=$COVERAGE \
	-DDEVELOPER_MODE=1 \
	-DCXX_STANDARD=20

make -j$(nproc)
# Run basic tests
ctest -R "SimpleTest"

cd $WORKDIR
rm -rf $WORKDIR/build

echo
echo "##############################################################"
echo "### Checking C++20 support in clang++"
echo "##############################################################"
mkdir $WORKDIR/build
cd $WORKDIR/build

CXX=clang++ cmake .. -DCMAKE_BUILD_TYPE=Release \
	-DTEST_DIR=/dev/shm \
	-DCMAKE_INSTALL_PREFIX=$PREFIX \
	-DCOVERAGE=$COVERAGE \
	-DDEVELOPER_MODE=1 \
	-DCXX_STANDARD=20

make -j$(nproc)
# Run basic tests
ctest -R "SimpleTest"

cd $WORKDIR
rm -rf $WORKDIR/build

echo
echo "##############################################################"
echo "### Verifying if each engine is building properly"
echo "##############################################################"
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
	mkdir $WORKDIR/build
	cd $WORKDIR/build
	# testing each engine separately; disabling default engines
	echo
	echo "##############################################################"
	echo "### Verifying building of the '$engine_flag' engine"
	echo "##############################################################"
	cmake .. -DENGINE_VSMAP=OFF \
		-DENGINE_VCMAP=OFF \
		-DENGINE_CMAP=OFF \
		-D$engine_flag=ON
	make -j$(nproc)
	# list all tests in this build
	ctest -N
	ctest -R wrong_engine_name_test --output-on-failure

	if [ "$COVERAGE" == "1" ]; then
		upload_codecov tests
	fi

	cd $WORKDIR
	rm -rf $WORKDIR/build
done

echo
echo "##############################################################"
echo "### Verifying building of all engines"
echo "##############################################################"
mkdir $WORKDIR/build
cd $WORKDIR/build

cmake .. -DENGINE_VSMAP=ON \
	-DENGINE_VCMAP=ON \
	-DENGINE_CMAP=ON \
	-DENGINE_STREE=ON \
	-DENGINE_TREE3=ON
make -j$(nproc)
# list all tests in this build
ctest -N

cd $WORKDIR
rm -rf $WORKDIR/build

echo
echo "##############################################################"
echo "### Verifying building of packages"
echo "##############################################################"

# Fetch git history for `git describe` to work,
# so that package has proper 'version' field
[ -f .git/shallow ] && git fetch --unshallow --tags

mkdir $WORKDIR/build
cd $WORKDIR/build

cmake .. -DCMAKE_BUILD_TYPE=Debug \
	-DTEST_DIR=/dev/shm \
	-DCMAKE_INSTALL_PREFIX=$PREFIX \
	-DDEVELOPER_MODE=1 \
	-DCPACK_GENERATOR=$PACKAGE_MANAGER

echo
echo "### Making sure there is no libpmemkv currently installed"
echo "---------------------------- Error expected! ------------------------------"
compile_example_standalone pmemkv_basic_cpp && exit 1
echo "---------------------------------------------------------------------------"

make -j$(nproc) package

if [ $PACKAGE_MANAGER = "deb" ]; then
	sudo_password dpkg -i libpmemkv*.deb
elif [ $PACKAGE_MANAGER = "rpm" ]; then
	sudo_password rpm -i libpmemkv*.rpm
fi

# Verify installed packages
compile_example_standalone pmemkv_basic_c
run_example_standalone pmemkv_basic_c
compile_example_standalone pmemkv_basic_cpp
run_example_standalone pmemkv_basic_cpp

# Trigger auto doc update on master
if [[ "$AUTO_DOC_UPDATE" == "1" ]]; then
	echo "Running auto doc update"

	mkdir -p $WORKDIR/doc_update
	cd $WORKDIR/doc_update

	$SCRIPTSDIR/run-doc-update.sh

	cd $WORKDIR
	rm -rf $WORKDIR/doc_update
fi
