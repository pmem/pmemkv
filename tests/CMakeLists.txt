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

set(DEFAULT_TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/test)

set(TEST_DIR ${DEFAULT_TEST_DIR}
	CACHE STRING "working directory for tests")

set(GLOBAL_TEST_ARGS -DPARENT_DIR=${TEST_DIR}/)

option(TRACE_TESTS "enable expanded tests' tracing" OFF)

if(TRACE_TESTS)
	set(GLOBAL_TEST_ARGS ${GLOBAL_TEST_ARGS} --trace-expand)
endif()

add_cppstyle(tests ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
		${CMAKE_CURRENT_SOURCE_DIR}/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/engines/*.cc
		${CMAKE_CURRENT_SOURCE_DIR}/engines-experimental/*.cc
		${CMAKE_CURRENT_SOURCE_DIR}/config/*.cc)

add_check_whitespace(tests ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
		${CMAKE_CURRENT_SOURCE_DIR}/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/engines/*.cc
		${CMAKE_CURRENT_SOURCE_DIR}/engines-experimental/*.cc
		${CMAKE_CURRENT_SOURCE_DIR}/config/*.cc)

set(vg_tracers memcheck helgrind drd pmemcheck)

#
# test -- add a test 'test_name'
#
# Arguments:
#	cmake_file  - cmake file to run the test
#	test_name   - name of a test to be printed out by ctest (must be uniqe)
#	test_filter - name of a test in the gtest binary (used as a gtest filter)
#	tracer      - Valgrind tool (memcheck/helgrind/drd/pmemcheck) used
#	              to trace the test
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
	if (${tracer} STREQUAL pmemcheck)
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
			-P ${CMAKE_CURRENT_SOURCE_DIR}/${cmake_file}.cmake)

	set_tests_properties(${test_name} PROPERTIES
		ENVIRONMENT "LC_ALL=C;PATH=$ENV{PATH};PMEM_IS_PMEM_FORCE=1"
		TIMEOUT 300)
endfunction()

if (COVERAGE AND VALGRIND_FOUND)
	message(STATUS "This is the Coverage build, skipping Valgrind tests")
endif()

function(build_example_pmemkv_basic_cpp)
	add_executable(ex_pmemkv_basic_cpp ../examples/pmemkv_basic_cpp/pmemkv_basic.cpp)
	target_include_directories(ex_pmemkv_basic_cpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
	target_link_libraries(ex_pmemkv_basic_cpp pmemkv)

	check_cxx_compiler_flag(-Wno-unused-but-set-variable wno_unused_but_set_variable_flag_cpp)
	if (wno_unused_but_set_variable_flag_cpp)
		target_compile_options(ex_pmemkv_basic_cpp PUBLIC -Wno-unused-but-set-variable)
	endif()
endfunction()

function(build_example_pmemkv_basic_c)
	add_executable(ex_pmemkv_basic_c ../examples/pmemkv_basic_c/pmemkv_basic.c)
	target_include_directories(ex_pmemkv_basic_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
	target_link_libraries(ex_pmemkv_basic_c pmemkv)

	check_c_compiler_flag(-Wno-unused-but-set-variable wno_unused_but_set_variable_flag_c)
	if (wno_unused_but_set_variable_flag_c)
		target_compile_options(ex_pmemkv_basic_c PUBLIC -Wno-unused-but-set-variable)
	endif()
endfunction()

set(TEST_FILES
	pmemkv_test.cc
	mock_tx_alloc.cc
	engines/blackhole_test.cc
	config/config.cc
	config/json_to_config.cc
)
# add each engine's tests source files separately
if(ENGINE_VSMAP)
	list(APPEND TEST_FILES engines/vsmap_test.cc)
endif()
if(ENGINE_VCMAP)
	list(APPEND TEST_FILES engines/vcmap_test.cc)
endif()
if(ENGINE_CMAP)
	list(APPEND TEST_FILES engines/cmap_test.cc)
endif()
if(ENGINE_CACHING)
	if(ENGINE_TREE3)
		list(APPEND TEST_FILES engines-experimental/caching_test.cc)
	else()
		message(WARNING
			"Caching tests are set to work with TREE3 engine, which is disabled, hence "
			"they are also disabled. If you want to run them use -DENGINE_TREE3=ON option.")
	endif()
endif()
if(ENGINE_STREE)
	list(APPEND TEST_FILES engines-experimental/stree_test.cc)
endif()
if(ENGINE_TREE3)
	list(APPEND TEST_FILES engines-experimental/tree3_test.cc)
endif()

find_library(GTEST NAMES gtest gtest.a)
if(GTEST)
	message(STATUS "Gtest found at ${GTEST}")
	add_library(libgtest IMPORTED STATIC GLOBAL)
	set_target_properties(libgtest PROPERTIES "IMPORTED_LOCATION" "${GTEST}"
		"IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")
else()
	message(FATAL_ERROR "Gtest not installed in system")
endif()

include(Dart)
include(GoogleTest)
add_executable(pmemkv_test ${TEST_FILES})

gtest_discover_tests(pmemkv_test NO_PRETTY_VALUES)

target_link_libraries(pmemkv_test pmemkv libgtest ${CMAKE_DL_LIBS})
if(ENGINE_CACHING)
	target_link_libraries(pmemkv_test memcached)
	target_link_libraries(pmemkv_test acl_cpp protocol acl)
endif()

# save lists of source files and all tests in files to check them in the first test
set(FILE_TEST_FILES ${CMAKE_CURRENT_BINARY_DIR}/test_files.txt)
set(FILE_ALL_TESTS ${CMAKE_CURRENT_BINARY_DIR}/list_all_tests.txt)
file(WRITE ${FILE_TEST_FILES} "${TEST_FILES}")
file(WRITE ${FILE_ALL_TESTS} "${list_all_tests}")

build_example_pmemkv_basic_c()
build_example_pmemkv_basic_cpp()

test("run-binary" ex_pmemkv_basic_c ex_pmemkv_basic_c none)
test("run-binary" ex_pmemkv_basic_cpp ex_pmemkv_basic_cpp none)
