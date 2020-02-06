# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

set(DEFAULT_TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/test)

set(TEST_DIR ${DEFAULT_TEST_DIR}
	CACHE STRING "working directory for tests")

set(GLOBAL_TEST_ARGS -DPARENT_DIR=${TEST_DIR}/)

option(TRACE_TESTS "enable expanded tests' tracing" OFF)

if(TRACE_TESTS)
	set(GLOBAL_TEST_ARGS ${GLOBAL_TEST_ARGS} --trace-expand)
endif()

set(vg_tracers memcheck helgrind drd pmemcheck pmreorder)
set(pmemcheck_tracers pmemcheck pmreorder)

file(MAKE_DIRECTORY ${DEFAULT_TEST_DIR})

#
# test -- add a test 'test_name'
#
# Arguments:
#	cmake_file  - cmake file to run the test
#	test_name   - name of a test to be printed out by ctest (must be unique)
#	test_filter - name of a test in the gtest binary (used as a gtest filter)
#	tracer      - Valgrind tool (memcheck/helgrind/drd/pmemcheck)
#	              or pmreorder used to trace the test
#
function(test cmake_file test_name test_filter tracer)
	if (${tracer} IN_LIST vg_tracers)
		if (NOT VALGRIND_FOUND)
			return()
		endif()
		if (COVERAGE)
			return()
		endif()
	endif()
	if (${tracer} IN_LIST pmemcheck_tracers)
		if (NOT VALGRIND_PMEMCHECK_FOUND)
			return()
		endif()
	endif()

	add_test(NAME ${test_name}
		COMMAND ${CMAKE_COMMAND}
			${GLOBAL_TEST_ARGS}
			-DTEST_NAME=${test_filter}
			-DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}
			-DBIN_DIR=${TEST_DIR}/${test_filter}-${tracer}
			-DCONFIG=$<CONFIG>
			-DTRACER=${tracer}
			-DFILE_TEST_FILES=${FILE_TEST_FILES}
			-DFILE_ALL_TESTS=${FILE_ALL_TESTS}
			${ARGN}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/${cmake_file}.cmake)

	set_tests_properties(${test_name} PROPERTIES
		ENVIRONMENT "LC_ALL=C;PATH=$ENV{PATH};PMEM_IS_PMEM_FORCE=1"
		TIMEOUT 300)
endfunction()
