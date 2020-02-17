#
# Copyright 2018-2020, Intel Corporation
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
# ctest_helpers.cmake - helper functions for tests/CMakeLists.txt
#

set(TEST_ROOT_DIR ${PROJECT_SOURCE_DIR}/tests)

set(GLOBAL_TEST_ARGS
	-DPERL_EXECUTABLE=${PERL_EXECUTABLE}
	-DMATCH_SCRIPT=${PROJECT_SOURCE_DIR}/tests/match
	-DPARENT_DIR=${TEST_DIR}
	-DTESTS_USE_FORCED_PMEM=${TESTS_USE_FORCED_PMEM}
	-DTEST_ROOT_DIR=${TEST_ROOT_DIR})

if(TRACE_TESTS)
	set(GLOBAL_TEST_ARGS ${GLOBAL_TEST_ARGS} --trace-expand)
endif()

set(INCLUDE_DIRS ${LIBPMEMOBJ_INCLUDE_DIRS} ${LIBPMEM_INCLUDE_DIRS} common/ .. .)
set(LIBS_DIRS ${LIBPMEMOBJ_LIBRARY_DIRS} ${LIBPMEM_LIBRARY_DIRS})

include_directories(${INCLUDE_DIRS})
link_directories(${LIBS_DIRS})

function(find_gdb)
	execute_process(COMMAND gdb --help
			RESULT_VARIABLE GDB_RET
			OUTPUT_QUIET
			ERROR_QUIET)
	if(GDB_RET)
		set(GDB_FOUND 0 CACHE INTERNAL "")
		message(WARNING "GDB NOT found, some tests will be skipped")
	else()
		set(GDB_FOUND 1 CACHE INTERNAL "")
	endif()
endfunction()

function(find_pmemcheck)
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

		string(REGEX MATCH ".*pmemcheck-([0-9.]*),.*" PMEMCHECK_OUT "${PMEMCHECK_OUT}")
		set(PMEMCHECK_VERSION ${CMAKE_MATCH_1} CACHE INTERNAL "")
	else()
		message(WARNING "Valgrind pmemcheck NOT found. Pmemcheck tests will not be performed.")
	endif()
endfunction()

function(find_packages)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(CURSES QUIET ncurses)
	else()
		# Specifies that we want FindCurses to find ncurses and not just any
		# curses library
		set(CURSES_NEED_NCURSES TRUE)
		find_package(Curses QUIET)
	endif()

	if(PKG_CONFIG_FOUND)
		pkg_check_modules(LIBUNWIND QUIET libunwind)
	else()
		find_package(LIBUNWIND QUIET)
	endif()
	if(NOT LIBUNWIND_FOUND)
		message(WARNING "libunwind not found. Stack traces from tests will not be reliable")
	endif()

	if(NOT WIN32)
		if(VALGRIND_FOUND)
			if ((NOT(PMEMCHECK_VERSION LESS 1.0)) AND PMEMCHECK_VERSION LESS 2.0)
				find_program(PMREORDER names pmreorder HINTS ${LIBPMEMOBJ_PREFIX}/bin)

				# copy_on_write support since libpmemobj 1.6
				if(PMREORDER AND NOT (LIBPMEMOBJ_VERSION_MINOR LESS 6))
					set(ENV{PATH} ${LIBPMEMOBJ_PREFIX}/bin:$ENV{PATH})
					set(PMREORDER_SUPPORTED true CACHE INTERNAL "pmreorder support")
				endif()
			else()
				message(STATUS "Pmreorder will not be used. Pmemcheck must be installed in version 1.X")
			endif()
		elseif(TESTS_USE_VALGRIND)
			message(WARNING "Valgrind not found. Valgrind tests will not be performed.")
		endif()
	endif()
endfunction()

# Function to build test with custom build options (e.g. passing defines)
# Example: build_test_ext(NAME ... SRC_FILES ....cpp BUILD_OPTIONS -D...)
function(build_test_ext)
	set(oneValueArgs NAME)
	set(multiValueArgs SRC_FILES BUILD_OPTIONS)
	cmake_parse_arguments(TEST "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	build_test(${TEST_NAME} ${TEST_SRC_FILES})
	target_compile_definitions(${TEST_NAME} PRIVATE ${TEST_BUILD_OPTIONS})
endfunction()

function(build_test name)
	# skip posix tests
	if(${name} MATCHES "posix$" AND WIN32)
		return()
	endif()

	set(srcs ${ARGN})
	prepend(srcs ${CMAKE_CURRENT_SOURCE_DIR} ${srcs})

	add_cppstyle(tests-${name} ${srcs})
	add_check_whitespace(tests-${name} ${srcs})

	add_executable(${name} ${srcs})
	target_link_libraries(${name} ${LIBPMEMOBJ_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} test_backtrace valgrind_internal)
	if(LIBUNWIND_FOUND)
		target_link_libraries(${name} ${LIBUNWIND_LIBRARIES} ${CMAKE_DL_LIBS})
	endif()
	if(WIN32)
		target_link_libraries(${name} dbghelp)
	endif()
	target_compile_definitions(${name} PRIVATE TESTS_LIBPMEMOBJ_VERSION=0x${LIBPMEMOBJ_VERSION_NUM})

	add_dependencies(tests ${name})
endfunction()

# Function to build a test with mocked pmemobj_defrag() function
function(build_test_defrag name)
	build_test(${name} ${ARGN})
	if (NOT WIN32)
		# target_link_options() should be used below,
		# but it is available since CMake v3.13
		target_link_libraries(${name} "-Wl,--wrap=pmemobj_defrag")
	endif()
endfunction()

function(build_test_tbb name)
	build_test(${name} ${ARGN})
	target_link_libraries(${name} ${TBB_LIBRARIES})
endfunction()

# Function to build a TBB test with mocked pmemobj_defrag() function
function(build_test_tbb_defrag name)
	build_test_tbb(${name} ${ARGN})
	if (NOT WIN32)
		# target_link_options() should be used below,
		# but it is available since CMake v3.13
		target_link_libraries(${name} "-Wl,--wrap=pmemobj_defrag")
	endif()
endfunction()

set(vg_tracers memcheck helgrind drd pmemcheck)

# Configures testcase ${name} ${testcase} using tracer ${tracer}, cmake_script is used to run test
function(add_testcase name tracer testcase cmake_script)
	set(executable ${name})
	add_test(NAME ${executable}_${testcase}_${tracer}
			COMMAND ${CMAKE_COMMAND}
			${GLOBAL_TEST_ARGS}
			-DTEST_NAME=${executable}_${testcase}_${tracer}
			-DTESTCASE=${testcase}
			-DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}/${name}
			-DBIN_DIR=${CMAKE_CURRENT_BINARY_DIR}/${executable}_${testcase}_${tracer}
			-DTEST_EXECUTABLE=$<TARGET_FILE:${executable}>
			-DTRACER=${tracer}
			-DLONG_TESTS=${LONG_TESTS}
			-P ${cmake_script})

	set_tests_properties(${name}_${testcase}_${tracer} PROPERTIES
			ENVIRONMENT "LC_ALL=C;PATH=$ENV{PATH};"
			FAIL_REGULAR_EXPRESSION Sanitizer)

	if (${tracer} STREQUAL pmemcheck)
		# XXX: if we use FATAL_ERROR in test.cmake - pmemcheck passes anyway
		set_tests_properties(${name}_${testcase}_${tracer} PROPERTIES
				FAIL_REGULAR_EXPRESSION "CMake Error")
	endif()

	if (${tracer} STREQUAL pmemcheck)
		set_tests_properties(${name}_${testcase}_${tracer} PROPERTIES
				COST 100)
	elseif(${tracer} IN_LIST vg_tracers)
		set_tests_properties(${name}_${testcase}_${tracer} PROPERTIES
				COST 50)
	else()
		set_tests_properties(${name}_${testcase}_${tracer} PROPERTIES
				COST 10)
	endif()
endfunction()

function(skip_test name message)
	add_test(NAME ${name}_${message}
		COMMAND ${CMAKE_COMMAND} -P ${TEST_ROOT_DIR}/true.cmake)

	set_tests_properties(${name}_${message} PROPERTIES COST 0)
endfunction()

# adds testcase with name, tracer, and cmake_script responsible for running it
function(add_test_common name tracer testcase cmake_script)
	if(${tracer} STREQUAL "")
	    set(tracer none)
	endif()

	if (NOT WIN32 AND ((NOT VALGRIND_FOUND) OR (NOT TESTS_USE_VALGRIND)) AND ${tracer} IN_LIST vg_tracers)
		# Only print "SKIPPED_*" message when option is enabled
		if (TESTS_USE_VALGRIND)
			skip_test(${name}_${testcase}_${tracer} "SKIPPED_BECAUSE_OF_MISSING_VALGRIND")
		endif()
		return()
	endif()

	if (NOT WIN32 AND ((NOT VALGRIND_PMEMCHECK_FOUND) OR (NOT TESTS_USE_VALGRIND)) AND ${tracer} STREQUAL "pmemcheck")
		# Only print "SKIPPED_*" message when option is enabled
		if (TESTS_USE_VALGRIND)
			skip_test(${name}_${testcase}_${tracer} "SKIPPED_BECAUSE_OF_MISSING_PMEMCHECK")
		endif()
		return()
	endif()

	if (NOT WIN32 AND (USE_ASAN OR USE_UBSAN) AND ${tracer} IN_LIST vg_tracers)
		skip_test(${name}_${testcase}_${tracer} "SKIPPED_BECAUSE_SANITIZER_USED")
		return()
	endif()

	# if test was not build
	if (NOT TARGET ${name})
		return()
	endif()

	# skip all tests with pmemcheck/memcheck/drd on windows
	if ((NOT ${tracer} STREQUAL none) AND WIN32)
		return()
	endif()

	add_testcase(${name} ${tracer} ${testcase} ${cmake_script})
endfunction()

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
		add_test_common(${TEST_NAME} ${tracer} ${TEST_CASE} ${cmake_script})
	endforeach()
endfunction()
