#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

#
# run-test-building.sh - is called inside a Docker container,
#                        starts testing of pmemkv building
#                        and automatic update of the documentation
#

set -e

source `dirname $0`/prepare-for-build.sh

TEST_DIR=${PMEMKV_TEST_DIR:-${DEFAULT_TEST_DIR}}
TEST_PACKAGES=${TEST_PACKAGES:-ON}
BUILD_JSON_CONFIG=${BUILD_JSON_CONFIG:-ON}

function run_test_check_support_cpp20_gcc() {
	CWD=$(pwd)
	echo
	echo "##############################################################"
	echo "### Checking C++20 support in g++"
	echo "##############################################################"
	mkdir $WORKDIR/build
	cd $WORKDIR/build

	CC=gcc CXX=g++ cmake .. -DCMAKE_BUILD_TYPE=Release \
		-DTEST_DIR=$TEST_DIR \
		-DCMAKE_INSTALL_PREFIX=$PREFIX \
		-DBUILD_JSON_CONFIG=${BUILD_JSON_CONFIG} \
		-DCOVERAGE=$COVERAGE \
		-DDEVELOPER_MODE=1 \
		-DCXX_STANDARD=20

	make -j$(nproc)
	# Run basic tests
	ctest -R "SimpleTest" --output-on-failure

	cd $CWD
	rm -rf $WORKDIR/build
}

function run_test_check_support_cpp20_clang() {
	CWD=$(pwd)
	echo
	echo "##############################################################"
	echo "### Checking C++20 support in clang++"
	echo "##############################################################"
	mkdir $WORKDIR/build
	cd $WORKDIR/build

	CC=clang CXX=clang++ cmake .. -DCMAKE_BUILD_TYPE=Release \
		-DTEST_DIR=$TEST_DIR \
		-DCMAKE_INSTALL_PREFIX=$PREFIX \
		-DBUILD_JSON_CONFIG=${BUILD_JSON_CONFIG} \
		-DCOVERAGE=$COVERAGE \
		-DDEVELOPER_MODE=1 \
		-DCXX_STANDARD=20

	make -j$(nproc)
	# Run basic tests
	ctest -R "SimpleTest" --output-on-failure

	cd $CWD
	rm -rf $WORKDIR/build
}

function verify_building_of_packages() {
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
		-DTEST_DIR=$TEST_DIR \
		-DCMAKE_INSTALL_PREFIX=$PREFIX \
		-DDEVELOPER_MODE=1 \
		-DBUILD_JSON_CONFIG=${BUILD_JSON_CONFIG} \
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
	run_example_standalone pmemkv_basic_c pool
	compile_example_standalone pmemkv_basic_cpp
	run_example_standalone pmemkv_basic_cpp pool

	# Clean after installation
	if [ $PACKAGE_MANAGER = "deb" ]; then
		sudo_password dpkg -r libpmemkv-dev
	elif [ $PACKAGE_MANAGER = "rpm" ]; then
		sudo_password rpm -e --nodeps libpmemkv-devel
	fi

	cd $WORKDIR
	rm -rf $WORKDIR/build
}

function build_with_flags() {
	CMAKE_FLAGS_AND_SETTINGS=$@
	echo
	echo "##############################################################"
	echo "### Verifying building with flag: ${CMAKE_FLAGS_AND_SETTINGS}"
	echo "##############################################################"
	mkdir $WORKDIR/build
	cd $WORKDIR/build

	cmake .. ${CMAKE_FLAGS_AND_SETTINGS}
	make -j$(nproc)
	# list all tests in this build
	ctest -N

	cd $WORKDIR
	rm -rf $WORKDIR/build
}

cd $WORKDIR

CMAKE_VERSION=$(cmake --version | head -n1 | grep -oE '[0-9].[0-9]*')
CMAKE_VERSION_MAJOR=$(echo $CMAKE_VERSION | cut -d. -f1)
CMAKE_VERSION_MINOR=$(echo $CMAKE_VERSION | cut -d. -f2)
CMAKE_VERSION_NUMBER=$((100 * $CMAKE_VERSION_MAJOR + $CMAKE_VERSION_MINOR))

# CXX_STANDARD==20 is supported since CMake 3.12
if [ $CMAKE_VERSION_NUMBER -ge 312 ]; then
	run_test_check_support_cpp20_gcc
	run_test_check_support_cpp20_clang
fi

echo
echo "##############################################################"
echo "### Verifying if each engine is building properly"
echo "##############################################################"
engines_flags=(
	ENGINE_VSMAP
	ENGINE_VCMAP
	ENGINE_CMAP
	ENGINE_CSMAP
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
	cmake .. -DCXX_STANDARD=14 \
		-DENGINE_VSMAP=OFF \
		-DENGINE_VCMAP=OFF \
		-DENGINE_CMAP=OFF \
		-DENGINE_CSMAP=OFF \
		-DBUILD_JSON_CONFIG=${BUILD_JSON_CONFIG} \
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

cmake .. -DCXX_STANDARD=14 \
	-DENGINE_VSMAP=ON \
	-DENGINE_VCMAP=ON \
	-DENGINE_CMAP=ON \
	-DENGINE_CSMAP=ON \
	-DENGINE_STREE=ON \
	-DENGINE_TREE3=ON \
	-DBUILD_JSON_CONFIG=${BUILD_JSON_CONFIG}
make -j$(nproc)
# list all tests in this build
ctest -N

cd $WORKDIR
rm -rf $WORKDIR/build

# test build with specific CMake flags set
build_with_flags -DBUILD_JSON_CONFIG=OFF -DTESTS_JSON=OFF

# building of packages should be verified only if PACKAGE_MANAGER equals 'rpm' or 'deb'
case $PACKAGE_MANAGER in
	rpm|deb)
		[ "$TEST_PACKAGES" == "ON" ] && verify_building_of_packages
		;;
	*)
		echo "Notice: skipping building of packages because PACKAGE_MANAGER is not equal 'rpm' nor 'deb' ..."
		;;
esac

# Trigger auto doc update on master
if [[ "$AUTO_DOC_UPDATE" == "1" ]]; then
	echo "Running auto doc update"

	mkdir -p $WORKDIR/doc_update
	cd $WORKDIR/doc_update

	$SCRIPTSDIR/run-doc-update.sh

	cd $WORKDIR
	rm -rf $WORKDIR/doc_update
fi
