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

set(DEFAULT_TEST_DIR ${CMAKE_CURRENT_BINARY_DIR})

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

set(vg_tracers memcheck helgrind drd pmemcheck pmreorder)
set(pmemcheck_tracers pmemcheck pmreorder)

#
# test -- add a test 'test_name'
#
# Arguments:
#	cmake_file  - cmake file to run the test
#	test_name   - name of a test to be printed out by ctest (must be uniqe)
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
			-P ${CMAKE_CURRENT_SOURCE_DIR}/${cmake_file}.cmake)

	set_tests_properties(${test_name} PROPERTIES
		ENVIRONMENT "LC_ALL=C;PATH=$ENV{PATH};PMEM_IS_PMEM_FORCE=1"
		TIMEOUT 300)
endfunction()

if (COVERAGE AND VALGRIND_FOUND)
	message(STATUS "This is the Coverage build, skipping Valgrind tests")
endif()

function(ignore_unused_but_set_variable_cpp target)
	check_cxx_compiler_flag(-Wno-unused-but-set-variable wno_unused_but_set_variable_flag_cpp)
	if (wno_unused_but_set_variable_flag_cpp)
		target_compile_options(${target} PUBLIC -Wno-unused-but-set-variable)
	endif()
endfunction()

function(ignore_unused_but_set_variable_c target)
	check_c_compiler_flag(-Wno-unused-but-set-variable wno_unused_but_set_variable_flag_c)
	if (wno_unused_but_set_variable_flag_c)
		target_compile_options(${target} PUBLIC -Wno-unused-but-set-variable)
	endif()
endfunction()

function(build_example_pmemkv_basic_cpp)
	add_executable(ex_pmemkv_basic_cpp ../examples/pmemkv_basic_cpp/pmemkv_basic.cpp)
	target_include_directories(ex_pmemkv_basic_cpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
	target_link_libraries(ex_pmemkv_basic_cpp pmemkv)
	ignore_unused_but_set_variable_cpp(ex_pmemkv_basic_cpp)
endfunction()

function(build_example_pmemkv_basic_c)
	add_executable(ex_pmemkv_basic_c ../examples/pmemkv_basic_c/pmemkv_basic.c)
	target_include_directories(ex_pmemkv_basic_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
	target_link_libraries(ex_pmemkv_basic_c pmemkv)
	ignore_unused_but_set_variable_c(ex_pmemkv_basic_c)
endfunction()

function(build_example_pmemkv_pmemobj_basic_cpp)
	if(NOT("${LIBPMEMOBJ++_LIBRARIES}" STREQUAL ""))
		add_executable(ex_pmemkv_pmemobj_cpp ../examples/pmemkv_pmemobj_cpp/pmemkv_pmemobj.cpp)
		target_include_directories(ex_pmemkv_pmemobj_cpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
		target_link_libraries(ex_pmemkv_pmemobj_cpp pmemkv ${LIBPMEMOBJ++_LIBRARIES})
		ignore_unused_but_set_variable_cpp(ex_pmemkv_pmemobj_cpp)
	endif()
endfunction()

function(build_wrong_engine_name_test)
	add_executable(wrong_engine_name_test wrong_engine_name_test.cc)
	target_include_directories(wrong_engine_name_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
	target_link_libraries(wrong_engine_name_test pmemkv)

	if(ENGINE_VSMAP)
		target_compile_definitions(wrong_engine_name_test PRIVATE -DENGINE_VSMAP)
	endif()
	if(ENGINE_VCMAP)
		target_compile_definitions(wrong_engine_name_test PRIVATE -DENGINE_VCMAP)
	endif()
	if(ENGINE_CMAP)
		target_compile_definitions(wrong_engine_name_test PRIVATE -DENGINE_CMAP)
	endif()
	if(ENGINE_CACHING)
		target_compile_definitions(wrong_engine_name_test PRIVATE -DENGINE_CACHING)
	endif()
	if(ENGINE_STREE)
		target_compile_definitions(wrong_engine_name_test PRIVATE -DENGINE_STREE)
	endif()
	if(ENGINE_TREE3)
		target_compile_definitions(wrong_engine_name_test PRIVATE -DENGINE_TREE3)
	endif()
endfunction()

set(TEST_FILES
	pmemkv_test.cc
	pmemkv_c_api_test.cc
	mock_tx_alloc.cc
	engines/blackhole_test.cc
	config/config_c.cc
	config/config_cpp.cc
)

if(BUILD_JSON_CONFIG)
	list(APPEND TEST_FILES config/json_to_config.cc)
endif()

# add each engine's tests source files separately
if(ENGINE_VSMAP)
	list(APPEND TEST_FILES engines/vsmap_test.cc)
endif()
if(ENGINE_VCMAP)
	list(APPEND TEST_FILES engines/vcmap_test.cc)
endif()
if(ENGINE_CMAP)
	list(APPEND TEST_FILES engines/cmap_test.cc)
	list(APPEND TEST_FILES engines/cmap_pmemobj_test.cc)
endif()
if(ENGINE_CACHING)
	if(ENGINE_TREE3 AND BUILD_JSON_CONFIG)
		list(APPEND TEST_FILES engines-experimental/caching_test.cc)
	elseif(NOT BUILD_JSON_CONFIG)
		message(WARNING
			"Caching tests require the 'libpmemkv_json_config' library, which is not built, "
			"hence they are disabled. If you want to run them use -DBUILD_JSON_CONFIG=ON option.")
	else()
		message(WARNING
			"Caching tests are set to work with TREE3 engine, which is disabled, hence "
			"they are also disabled. If you want to run them use -DENGINE_TREE3=ON option.")
	endif()
endif()
if(ENGINE_STREE)
	list(APPEND TEST_FILES engines-experimental/stree_test.cc)
	list(APPEND TEST_FILES engines-experimental/stree_pmemobj_test.cc)
endif()
if(ENGINE_TREE3)
	list(APPEND TEST_FILES engines-experimental/tree3_test.cc)
	list(APPEND TEST_FILES engines-experimental/tree3_pmemobj_test.cc)
endif()

# CMake option 'CMAKE_PREFIX_PATH' will be prioritized
# over system paths in find_library and find_path calls
find_library(GTEST NAMES gtest)
if(GTEST)
	message(STATUS "Gtest found in ${GTEST}")

	find_path(GTEST_INCLUDEDIR "gtest/gtest.h")
	if (GTEST_INCLUDEDIR)
		message(STATUS "Gtest headers found in ${GTEST_INCLUDEDIR}")
		include_directories(${GTEST_INCLUDEDIR})
	endif()

	add_library(libgtest IMPORTED STATIC GLOBAL)
	set_target_properties(libgtest PROPERTIES "IMPORTED_LOCATION" "${GTEST}"
		"IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")
else()
	message(FATAL_ERROR "Gtest is not installed or couldn't be found. Try using cmake option "
						"-DCMAKE_PREFIX_PATH=<dir with lib and include dirs for gtest>. "
						"Use e.g. like 'cmake .. -DCMAKE_PREFIX_PATH=/install/path/gtest/'")
endif()

include(Dart)
include(GoogleTest)
add_executable(pmemkv_test ${TEST_FILES})
gtest_discover_tests(pmemkv_test EXTRA_ARGS --test_dir ${TEST_DIR} NO_PRETTY_VALUES)

if (VALGRIND_FOUND AND NOT COVERAGE)
	add_executable(memcheck valgrind_wrapper.cc)
	add_dependencies(memcheck pmemkv_test)
	gtest_discover_tests(memcheck EXTRA_ARGS --test_dir ${TEST_DIR} NO_PRETTY_VALUES TEST_SUFFIX _MEMCHECK)

	add_executable(drd valgrind_wrapper.cc)
	add_dependencies(drd pmemkv_test)
	gtest_discover_tests(drd EXTRA_ARGS --test_dir ${TEST_DIR} NO_PRETTY_VALUES TEST_SUFFIX _DRD)

	add_executable(helgrind valgrind_wrapper.cc)
	add_dependencies(helgrind pmemkv_test)
	gtest_discover_tests(helgrind EXTRA_ARGS --test_dir ${TEST_DIR} NO_PRETTY_VALUES TEST_SUFFIX _HELGRIND)
endif()

if (VALGRIND_PMEMCHECK_FOUND AND NOT COVERAGE)
	add_executable(pmemcheck valgrind_wrapper.cc)
	add_dependencies(pmemcheck pmemkv_test)
	gtest_discover_tests(pmemcheck EXTRA_ARGS --test_dir ${TEST_DIR} NO_PRETTY_VALUES TEST_SUFFIX _PMEMCHECK)
endif()

target_link_libraries(pmemkv_test pmemkv libgtest ${CMAKE_DL_LIBS})

if(BUILD_JSON_CONFIG)
	target_link_libraries(pmemkv_test pmemkv_json_config)
endif()
if(ENGINE_CMAP)
	target_link_libraries(pmemkv_test ${LIBPMEMOBJ++_LIBRARIES})
endif()
if(ENGINE_CACHING)
	# caching tests require lib_acl and memcached included, so we need to link
	# them to test binary itself
	target_link_libraries(pmemkv_test memcached)
	target_link_libraries(pmemkv_test acl_cpp protocol acl)
endif()
if(ENGINE_STREE)
	target_link_libraries(pmemkv_test ${LIBPMEMOBJ++_LIBRARIES})
endif()
if(ENGINE_TREE3)
	target_link_libraries(pmemkv_test ${LIBPMEMOBJ++_LIBRARIES})
endif()

# save lists of source files and all tests in files to check them in the first test
set(FILE_TEST_FILES ${CMAKE_CURRENT_BINARY_DIR}/test_files.txt)
set(FILE_ALL_TESTS ${CMAKE_CURRENT_BINARY_DIR}/list_all_tests.txt)
file(WRITE ${FILE_TEST_FILES} "${TEST_FILES}")
file(WRITE ${FILE_ALL_TESTS} "${list_all_tests}")

build_wrong_engine_name_test()

test("run-binary" wrong_engine_name_test wrong_engine_name_test none)

if(BUILD_EXAMPLES AND ENGINE_CMAP)
	build_example_pmemkv_basic_c()
	build_example_pmemkv_basic_cpp()
	build_example_pmemkv_pmemobj_basic_cpp()
	test("run-binary" ex_pmemkv_basic_c ex_pmemkv_basic_c none)
	test("run-binary" ex_pmemkv_basic_cpp ex_pmemkv_basic_cpp none)
	test("run-binary" ex_pmemkv_pmemobj_basic_cpp ex_pmemkv_pmemobj_basic_cpp none)
elseif(BUILD_EXAMPLES AND NOT ENGINE_CMAP)
	message(WARNING
		"Examples use cmap engine, which is disabled, hence their execution "
		"is also disabled. If you want to run them use -DENGINE_CMAP=ON option.")
endif()

# test pmreorder framework
if(PMREORDER_SUPPORTED)
	test("run-pmreorder-test" wrong_engine_name_test-pmreorder wrong_engine_name_test pmreorder)
endif()
