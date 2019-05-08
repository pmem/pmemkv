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

cmake_minimum_required(VERSION 3.5)

set(DEFAULT_TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/test)

set(TEST_DIR ${DEFAULT_TEST_DIR}
	CACHE STRING "directory used for tests")

set(GLOBAL_TEST_ARGS -DPARENT_DIR=${TEST_DIR}/)

if(TRACE_TESTS)
	set(GLOBAL_TEST_ARGS ${GLOBAL_TEST_ARGS} --trace-expand)
endif()

set(vg_tracers memcheck helgrind drd pmemcheck)

# Configures test ${name}
function(test name filter tracer)
	if (${tracer} IN_LIST vg_tracers)
		if (NOT VALGRIND_FOUND)
			message(WARNING
				"Valgrind not found, test skipped: ${name}")
			return()
		endif()
		if (COVERAGE_BUILD)
			message(STATUS
				"This is the Coverage build, skipping Valgrind test: ${name}")
			return()
		endif()
	endif()

	add_test(NAME ${name}
		COMMAND ${CMAKE_COMMAND}
		${GLOBAL_TEST_ARGS}
		-DTEST_NAME=${filter}
		-DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}
		-DBIN_DIR=${CMAKE_CURRENT_BINARY_DIR}/${filter}-${tracer}
		-DCONFIG=$<CONFIG>
		-DTRACER=${tracer}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/run-one-test.cmake)

	set_tests_properties(${name} PROPERTIES
		ENVIRONMENT "LC_ALL=C;PATH=$ENV{PATH};PMEM_IS_PMEM_FORCE=1"
		TIMEOUT 300)
endfunction()

set(TEST_FILES
	pmemkv_test.cc
	mock_tx_alloc.cc
	engines/blackhole_test.cc
	engines/vsmap_test.cc
	engines/vcmap_test.cc
	engines/cmap_test.cc
	engines-experimental/tree_test.cc
	engines-experimental/stree_test.cc
	engines-experimental/caching_test.cc
)

include(gtest)

add_executable(pmemkv_test ${TEST_FILES})
target_link_libraries(pmemkv_test pmemkv libgtest ${CMAKE_DL_LIBS})

# parse tests source files to get the list of all tests
set(all_tests "")
foreach(file IN ITEMS ${TEST_FILES})
	file(STRINGS ${file} tests REGEX "^TEST_F")
	string(REPLACE "TEST_F(" "" tests "${tests}")
	string(REPLACE ", " "." tests "${tests}")
	string(REPLACE ") {" " " tests "${tests}")
	string(CONCAT all_tests ${all_tests} ${tests})
endforeach(file)
string(REPLACE " " ";" list_all_tests "${all_tests}")

# set excluded tests
set(excluded_tests STreeTest STreeLargeTest CachingTest)

# set drd tests
set(drd_tests
	BlackholeTest
	CMapTest
	VCMapTest
	VSMapTest)

foreach(test IN ITEMS ${list_all_tests})
	string(REGEX REPLACE "[.].+" "" test_group ${test})
	if(NOT ${test_group} IN_LIST excluded_tests)
		test("${test}" ${test} none)
	endif()
	if(${test_group} IN_LIST drd_tests)
		test("${test}-drd" ${test} drd)
	endif()
endforeach(test)
