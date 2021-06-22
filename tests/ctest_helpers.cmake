# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2021, Intel Corporation

#
# ctest_helpers.cmake - helper functions for building and adding
#		new testcases (in tests/CMakeLists.txt)
#

set(TEST_ROOT_DIR ${PROJECT_SOURCE_DIR}/tests)

set(GLOBAL_TEST_ARGS
	-DLIBPMEMOBJ++_LIBRARY_DIRS=${LIBPMEMOBJ++_LIBRARY_DIRS}
	-DPERL_EXECUTABLE=${PERL_EXECUTABLE}
	-DMATCH_SCRIPT=${PROJECT_SOURCE_DIR}/tests/match
	-DPARENT_DIR=${TEST_DIR}
	-DTESTS_USE_FORCED_PMEM=${TESTS_USE_FORCED_PMEM}
	-DTEST_ROOT_DIR=${TEST_ROOT_DIR})
if(TRACE_TESTS)
	set(GLOBAL_TEST_ARGS ${GLOBAL_TEST_ARGS} --trace-expand)
endif()

# List of supported Valgrind tracers
set(vg_tracers memcheck helgrind drd pmemcheck)

# ----------------------------------------------------------------- #
## Include and link required dirs/libs for tests
# ----------------------------------------------------------------- #
set(INCLUDE_DIRS ${LIBPMEMOBJ++_INCLUDE_DIRS} common/ ../src .)
set(LIBS_DIRS ${LIBPMEMOBJ++_LIBRARY_DIRS})

include_directories(${INCLUDE_DIRS})
link_directories(${LIBS_DIRS})

# ----------------------------------------------------------------- #
## Define functions to use in tests/CMakeLists.txt
# ----------------------------------------------------------------- #
function(find_gdb)
	execute_process(COMMAND gdb --help
			RESULT_VARIABLE GDB_RET
			OUTPUT_QUIET
			ERROR_QUIET)
	if(GDB_RET)
		set(GDB_FOUND 0 CACHE INTERNAL "")
		message(WARNING "gdb not found, some tests will be skipped")
	else()
		set(GDB_FOUND 1 CACHE INTERNAL "")
		message(STATUS "Found gdb")
	endif()
endfunction()

function(find_pmemcheck)
	if(WIN32)
		return()
	endif()

	if(NOT VALGRIND_FOUND)
		return()
	endif()

	set(ENV{PATH} ${VALGRIND_PREFIX}/bin:$ENV{PATH})
	execute_process(COMMAND valgrind --tool=pmemcheck --help
			RESULT_VARIABLE VALGRIND_PMEMCHECK_RET
			OUTPUT_QUIET
			ERROR_QUIET)
	if(VALGRIND_PMEMCHECK_RET)
		set(VALGRIND_PMEMCHECK_FOUND 0 CACHE INTERNAL "")
	else()
		set(VALGRIND_PMEMCHECK_FOUND 1 CACHE INTERNAL "")
	endif()

	if(VALGRIND_PMEMCHECK_FOUND)
		execute_process(COMMAND valgrind --tool=pmemcheck true
				ERROR_VARIABLE PMEMCHECK_OUT
				OUTPUT_QUIET)

		string(REGEX MATCH ".*pmemcheck-([0-9.]+),.*" PMEMCHECK_OUT "${PMEMCHECK_OUT}")
		set(PMEMCHECK_VERSION ${CMAKE_MATCH_1} CACHE INTERNAL "")
		message(STATUS "Valgrind pmemcheck found, version: ${PMEMCHECK_VERSION}")
	else()
		message(WARNING "Valgrind pmemcheck not found. Pmemcheck tests will not be performed.")
	endif()
endfunction()

function(find_libunwind)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(LIBUNWIND QUIET libunwind)
	else()
		find_package(LIBUNWIND QUIET)
	endif()

	if(NOT LIBUNWIND_FOUND)
		message(WARNING "libunwind not found. Stack traces from tests will not be reliable")
	else()
		message(STATUS "Found libunwind, version ${LIBUNWIND_VERSION}")
	endif()
endfunction()

function(find_pmreorder)
	if(NOT VALGRIND_FOUND OR NOT VALGRIND_PMEMCHECK_FOUND)
		message(WARNING "Pmreorder will not be used. Valgrind with pmemcheck must be installed")
		return()
	endif()

	if((NOT(PMEMCHECK_VERSION VERSION_LESS 1.0)) AND PMEMCHECK_VERSION VERSION_LESS 2.0)
		find_program(PMREORDER names pmreorder HINTS ${LIBPMEMOBJ_PREFIX}/bin)

		if(PMREORDER)
			get_program_version_major_minor(${PMREORDER} PMREORDER_VERSION)
			message(STATUS "Found pmreorder: ${PMREORDER}, in version: ${PMREORDER_VERSION}")

			set(ENV{PATH} ${LIBPMEMOBJ_PREFIX}/bin:$ENV{PATH})
			set(PMREORDER_SUPPORTED true CACHE INTERNAL "pmreorder support")
		else()
			message(WARNING "Pmreorder not found - pmreorder tests will not be performed.")
		endif()
	else()
		message(WARNING "Pmemcheck must be installed in version 1.X for pmreorder to work - pmreorder tests will not be performed.")
	endif()
endfunction()

function(find_pmempool)
	find_program(PMEMPOOL names pmempool HINTS ${LIBPMEMOBJ_PREFIX}/bin)
	if(PMEMPOOL)
		set(ENV{PATH} ${LIBPMEMOBJ_PREFIX}/bin:$ENV{PATH})
		message(STATUS "Found pmempool: ${PMEMPOOL}")
	else()
		message(FATAL_ERROR "Pmempool not found.")
	endif()
endfunction()

# Function to build test with custom build options (e.g. passing defines),
# link it with custom library/-ies and compile options. It calls build_test function.
# Usage: build_test_ext(NAME .. SRC_FILES .. .. LIBS .. .. BUILD_OPTIONS .. .. OPTS .. ..)
function(build_test_ext)
	set(oneValueArgs NAME)
	set(multiValueArgs SRC_FILES LIBS BUILD_OPTIONS OPTS)
	cmake_parse_arguments(TEST "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	set(LIBS_TO_LINK "")

	if(${TEST_NAME} MATCHES "posix$" AND WIN32)
		return()
	endif()

	foreach(lib ${TEST_LIBS})
		if("${lib}" STREQUAL "json")
			if(NOT BUILD_JSON_CONFIG)
				return()
			else()
				list(APPEND LIBS_TO_LINK pmemkv_json_config)
				list(APPEND TEST_BUILD_OPTIONS -DJSON_TESTS_SUPPORT)
			endif()
		elseif("${lib}" STREQUAL "libpmemobj_cpp")
			if("${LIBPMEMOBJ++_LIBRARIES}" STREQUAL "")
				return()
			else()
				list(APPEND LIBS_TO_LINK ${LIBPMEMOBJ++_LIBRARIES})
			endif()
		elseif("${lib}" STREQUAL "dl_libs")
			list(APPEND LIBS_TO_LINK ${CMAKE_DL_LIBS})
		elseif("${lib}" STREQUAL "memkind")
			list(APPEND LIBS_TO_LINK ${MEMKIND_LIBRARIES})
		endif()
	endforeach()

	build_test(${TEST_NAME} ${TEST_SRC_FILES})
	target_link_libraries(${TEST_NAME} ${LIBS_TO_LINK})
	target_compile_definitions(${TEST_NAME} PRIVATE ${TEST_BUILD_OPTIONS})
	target_compile_options(${TEST_NAME} PRIVATE ${TEST_OPTS})
endfunction()

function(build_test name)
	if(${name} MATCHES "posix$" AND WIN32)
		return()
	endif()

	set(srcs ${ARGN})
	prepend(srcs ${CMAKE_CURRENT_SOURCE_DIR} ${srcs})

	add_executable(${name} ${srcs})
	target_link_libraries(${name} ${LIBPMEMOBJ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} pmemkv test_backtrace)
	if(LIBUNWIND_FOUND)
		target_link_libraries(${name} ${LIBUNWIND_LIBRARIES} ${CMAKE_DL_LIBS})
	endif()
	if(WIN32)
		target_link_libraries(${name} dbghelp)
	endif()

	add_dependencies(tests ${name})
endfunction()

# Configures testcase ${test_name}_${testcase} with ${tracer}
# and cmake_script used to execute test
function(add_testcase executable test_name tracer testcase cmake_script)
	add_test(NAME ${test_name}_${testcase}_${tracer}
			COMMAND ${CMAKE_COMMAND}
			${GLOBAL_TEST_ARGS}
			-DTEST_NAME=${test_name}_${testcase}_${tracer}
			-DTESTCASE=${testcase}
			-DPARENT_SRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}
			-DBIN_DIR=${CMAKE_CURRENT_BINARY_DIR}/${test_name}_${testcase}_${tracer}
			-DTEST_EXECUTABLE=$<TARGET_FILE:${executable}>
			-DTRACER=${tracer}
			-DLONG_TESTS=${LONG_TESTS}
			${ARGN}
			-P ${cmake_script})

	set_tests_properties(${test_name}_${testcase}_${tracer} PROPERTIES
			ENVIRONMENT "LC_ALL=C;PATH=$ENV{PATH};"
			FAIL_REGULAR_EXPRESSION Sanitizer)

	# XXX: if we use FATAL_ERROR in test.cmake - pmemcheck passes anyway
	# workaround: look for "CMake Error" in output and fail if found
	if (${tracer} STREQUAL pmemcheck)
		set_tests_properties(${test_name}_${testcase}_${tracer} PROPERTIES
				FAIL_REGULAR_EXPRESSION "CMake Error")
	endif()

	if (${tracer} STREQUAL pmemcheck)
		set_tests_properties(${test_name}_${testcase}_${tracer} PROPERTIES
				COST 100)
	elseif(${tracer} IN_LIST vg_tracers)
		set_tests_properties(${test_name}_${testcase}_${tracer} PROPERTIES
				COST 50)
	else()
		set_tests_properties(${test_name}_${testcase}_${tracer} PROPERTIES
				COST 10)
	endif()
endfunction()

function(skip_test name message)
	add_test(NAME ${name}_${message}
		COMMAND ${CMAKE_COMMAND} -P ${TEST_ROOT_DIR}/true.cmake)

	set_tests_properties(${name}_${message} PROPERTIES COST 0)
endfunction()

# adds testcase only if tracer is found and target is build, skips otherwise
function(add_test_common executable test_name tracer testcase cmake_script)
	if(${tracer} STREQUAL "")
	    set(tracer none)
	endif()

	if (NOT WIN32 AND ((NOT VALGRIND_FOUND) OR (NOT TESTS_USE_VALGRIND)) AND ${tracer} IN_LIST vg_tracers)
		# Only print "SKIPPED_*" message when option is enabled
		if (TESTS_USE_VALGRIND)
			skip_test(${test_name}_${testcase}_${tracer} "SKIPPED_BECAUSE_OF_MISSING_VALGRIND")
		endif()
		return()
	endif()

	if (NOT WIN32 AND ((NOT VALGRIND_PMEMCHECK_FOUND) OR (NOT TESTS_USE_VALGRIND)) AND ${tracer} STREQUAL "pmemcheck")
		# Only print "SKIPPED_*" message when option is enabled
		if (TESTS_USE_VALGRIND)
			skip_test(${test_name}_${testcase}_${tracer} "SKIPPED_BECAUSE_OF_MISSING_PMEMCHECK")
		endif()
		return()
	endif()

	if (NOT WIN32 AND (USE_ASAN OR USE_UBSAN) AND ${tracer} IN_LIST vg_tracers)
		skip_test(${test_name}_${testcase}_${tracer} "SKIPPED_BECAUSE_SANITIZER_USED")
		return()
	endif()

	# if test was not build
	if (NOT TARGET ${executable})
		message(WARNING "${executable} not build. Skipping.")
		return()
	endif()

	# skip all valgrind tests on windows
	if ((NOT ${tracer} STREQUAL none) AND WIN32)
		return()
	endif()

	if (COVERAGE AND ${tracer} IN_LIST vg_tracers)
		return()
	endif()

	add_testcase(${executable} ${test_name} ${tracer} ${testcase} ${cmake_script} ${ARGN})
endfunction()

# adds testcase with optional SCRIPT and TEST_CASE parameters
function(add_test_generic)
	set(oneValueArgs NAME CASE SCRIPT)
	set(multiValueArgs TRACERS)
	cmake_parse_arguments(TEST "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	if("${TEST_SCRIPT}" STREQUAL "")
		if("${TEST_CASE}" STREQUAL "")
			set(TEST_CASE "0")
			set(cmake_script ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_default.cmake)
		else()
			set(cmake_script ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}/${TEST_NAME}_${TEST_CASE}.cmake)
		endif()
	else()
		if("${TEST_CASE}" STREQUAL "")
			set(TEST_CASE "0")
		endif()
		set(cmake_script ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SCRIPT})
	endif()

	foreach(tracer ${TEST_TRACERS})
		add_test_common(${TEST_NAME} ${TEST_NAME} ${tracer} ${TEST_CASE} ${cmake_script})
	endforeach()
endfunction()

# adds testcase with additional parameters, required by "engine scenario" tests
# EXTRA_CONFIG_PARAMS should be supplied in form of a json-like list, e.g. {"path":"/path/to/file"}
function(add_engine_test)
	set(oneValueArgs BINARY ENGINE SCRIPT DB_SIZE)
	set(multiValueArgs TRACERS PARAMS EXTRA_CONFIG_PARAMS)
	cmake_parse_arguments(TEST "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	set(cmake_script ${CMAKE_CURRENT_SOURCE_DIR}/engines/${TEST_SCRIPT})

	if("${TEST_DB_SIZE}" STREQUAL "")
		set(TEST_DB_SIZE 104857600) # 100MB
	elseif("${TEST_DB_SIZE}" STREQUAL "MIN_JEMALLOC_ARENA_SIZE")
		execute_process(COMMAND nproc OUTPUT_VARIABLE NPROC OUTPUT_STRIP_TRAILING_WHITESPACE)
		# By default jemalloc creates 4 arenas for each logical CPU with 2MB chunks
		math(EXPR TEST_DB_SIZE "${NPROC} * 4 * 2 * 1024 * 1024")

		# Limit size of the test database with arbitrary threshold: almost 2GB
		# (value for nproc=256 minus 2 bytes).
		# Can be overwritten in CMake cache using build param: -DTEST_DB_SIZE_THRESHOLD=<size>
		set(TEST_DB_SIZE_THRESHOLD 2147483646 CACHE STRING "Limit calculated size of the TEST_DB_SIZE for some of the memkind tests")
		if(${TEST_DB_SIZE} GREATER ${TEST_DB_SIZE_THRESHOLD})
			message(WARNING "Calculated TEST_DB_SIZE (${TEST_DB_SIZE}) reached threshold: ${TEST_DB_SIZE_THRESHOLD}. "
				"This may cause failures in some tests of memkind based engines.")
			set(TEST_DB_SIZE ${TEST_DB_SIZE_THRESHOLD})
		endif()

		# Set minimum arbitrary test database to avoid test failures; this should never happen
		set(TEST_DB_SIZE_MIN 8388608) # 8 MB
		if(${TEST_DB_SIZE} LESS ${TEST_DB_SIZE_MIN})
			message(FATAL_ERROR "DB_SIZE (${TEST_DB_SIZE}) was calculated below the minimum: (${TEST_DB_SIZE_MIN}).")
		endif()
	endif()

	get_filename_component(script_name ${cmake_script} NAME)
	set(parsed_script_name ${script_name})
	string(REGEX REPLACE ".cmake" "" parsed_script_name ${parsed_script_name})

	set(TEST_NAME "${TEST_ENGINE}__${TEST_BINARY}__${parsed_script_name}")
	if(NOT "${TEST_PARAMS}" STREQUAL "")
		string(REPLACE ";" "_" parsed_params "${TEST_PARAMS}")
		set(TEST_NAME "${TEST_NAME}_${parsed_params}")
	endif()

	if (NOT ("${TEST_EXTRA_CONFIG_PARAMS}" STREQUAL ""))
		# Parse TEST_EXTRA_CONFIG_PARAMS so it can be used in test name
		string(REPLACE " " "" extra_config_params ${TEST_EXTRA_CONFIG_PARAMS})
		string(REPLACE ":" "_" parsed_extra_config_params ${extra_config_params})
		string(REPLACE "\"" "" parsed_extra_config_params ${parsed_extra_config_params})
		set(TEST_NAME "${TEST_NAME}__${parsed_extra_config_params}" CACHE INTERNAL "")
	endif()

	# Use "|PARAM|" as list separator so that CMake does not expand it
	# when passing to the test script
	string(REPLACE ";" "|PARAM|" raw_params "${TEST_PARAMS}")

	foreach(tracer ${TEST_TRACERS})
		add_test_common(${TEST_BINARY} ${TEST_NAME} ${tracer} 0 ${cmake_script}
			-DENGINE=${TEST_ENGINE}
			-DDB_SIZE=${TEST_DB_SIZE}
			-DRAW_PARAMS=${raw_params}
			-DEXTRA_CONFIG_PARAMS=${extra_config_params})
	endforeach()
endfunction()
