# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2021, Intel Corporation

#
# helpers.cmake - helper functions for cmake scripts used in tests.
#

# Sets testing path, based on TEST_DIR (as 'PARENT_DIR' here) passed to the top-level CMake.
set(DIR ${PARENT_DIR}/${TEST_NAME})

string(REPLACE "|PARAM|" ";" PARAMS "${RAW_PARAMS}")

# ----------------------------------------------------------------- #
## Define functions to control tests' flow and prepare environment
# ----------------------------------------------------------------- #

# setup the env, by removing old binary and testfile
function(setup)
	execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory ${PARENT_DIR}/${TEST_NAME})
	execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${PARENT_DIR}/${TEST_NAME})
	execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory ${BIN_DIR})
	execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${BIN_DIR})
endfunction()

function(print_logs)
	message(STATUS "Test ${TEST_NAME}:")
	if(EXISTS ${BIN_DIR}/${TEST_NAME}.out)
		file(READ ${BIN_DIR}/${TEST_NAME}.out OUT)
		message(STATUS "Stdout:\n${OUT}")
	endif()
	if(EXISTS ${BIN_DIR}/${TEST_NAME}.err)
		file(READ ${BIN_DIR}/${TEST_NAME}.err ERR)
		message(STATUS "Stderr:\n${ERR}")
	endif()
	if(EXISTS ${BIN_DIR}/${TEST_NAME}.pmreorder)
	   file(READ ${BIN_DIR}/${TEST_NAME}.pmreorder PMEMREORDER)
	   message(STATUS "Pmreorder:\n${PMEMREORDER}")
	endif()
endfunction()

# Performs cleanup and log matching.
function(finish)
	print_logs()

	execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory ${PARENT_DIR}/${TEST_NAME})
endfunction()

# Verifies ${log_file} matches ${match_file} using "match".
function(match log_file match_file)
	execute_process(COMMAND
			${PERL_EXECUTABLE} ${MATCH_SCRIPT} -o ${log_file} ${match_file}
			RESULT_VARIABLE MATCH_ERROR)

	if(MATCH_ERROR)
		message(FATAL_ERROR "Log does not match: ${MATCH_ERROR}")
	endif()
endfunction()

# Verifies file exists
function(check_file_exists file)
	if(NOT EXISTS ${file})
		message(FATAL_ERROR "${file} doesn't exist")
	endif()
endfunction()

# Verifies file doesn't exist
function(check_file_doesnt_exist file)
	if(EXISTS ${file})
		message(FATAL_ERROR "${file} exists")
	endif()
endfunction()

# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=810295
# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=780173
# https://bugs.kde.org/show_bug.cgi?id=303877
#
# valgrind issues an unsuppressable warning when exceeding
# the brk segment, causing matching failures. We can safely
# ignore it because malloc() will fallback to mmap() anyway.
#
# list of ignored warnings should match the list provided by PMDK:
# https://github.com/pmem/pmdk/blob/master/src/test/unittest/unittest.sh
function(valgrind_ignore_warnings valgrind_log)
	execute_process(COMMAND bash "-c" "cat ${valgrind_log} | grep -v \
	-e \"WARNING: Serious error when reading debug info\" \
	-e \"When reading debug info from \" \
	-e \"Ignoring non-Dwarf2/3/4 block in .debug_info\" \
	-e \"Last block truncated in .debug_info; ignoring\" \
	-e \"parse_CU_Header: is neither DWARF2 nor DWARF3 nor DWARF4\" \
	-e \"brk segment overflow\" \
	-e \"see section Limitations in user manual\" \
	-e \"Warning: set address range perms: large range\"\
	-e \"further instances of this message will not be shown\"\
	>  ${valgrind_log}.tmp
mv ${valgrind_log}.tmp ${valgrind_log}")
endfunction()

function(execute_common expect_success output_file name)
	if(TESTS_USE_FORCED_PMEM)
		set(ENV{PMEM_IS_PMEM_FORCE} 1)
	endif()

	if(${TRACER} STREQUAL pmemcheck)
		if(TESTS_USE_FORCED_PMEM)
			# pmemcheck runs really slow with pmem, disable it
			set(ENV{PMEM_IS_PMEM_FORCE} 0)
		endif()
		set(TRACE valgrind --error-exitcode=99 --tool=pmemcheck)
	elseif(${TRACER} STREQUAL memcheck)
		set(TRACE valgrind --error-exitcode=99 --tool=memcheck --leak-check=full --max-threads=3000
		   --suppressions=${TEST_ROOT_DIR}/ld.supp --suppressions=${TEST_ROOT_DIR}/memcheck-stdcpp.supp --suppressions=${TEST_ROOT_DIR}/memcheck-libunwind.supp
		   --suppressions=${TEST_ROOT_DIR}/memcheck-ndctl.supp)
	elseif(${TRACER} STREQUAL helgrind)
		set(TRACE valgrind --error-exitcode=99 --tool=helgrind --max-threads=3000)
	elseif(${TRACER} STREQUAL drd)
		set(TRACE valgrind --error-exitcode=99 --tool=drd --max-threads=3000 --suppressions=${TEST_ROOT_DIR}/drd.supp)
	elseif(${TRACER} STREQUAL gdb)
		set(TRACE gdb --batch --command=${GDB_BATCH_FILE} --args)
	elseif(${TRACER} MATCHES "none.*")
		# nothing
	else()
		message(FATAL_ERROR "Unknown tracer '${TRACER}'")
	endif()

	if (NOT $ENV{CGDB})
		if (NOT WIN32)
			set(TRACE timeout -s SIGALRM -k 200s 180s ${TRACE})
		endif()
	endif()

	string(REPLACE ";" " " TRACE_STR "${TRACE}")
	message(STATUS "Executing: ${TRACE_STR} ${name} ${ARGN}")

	set(cmd ${TRACE} ${name} ${ARGN})

	if($ENV{CGDB})
		find_program(KONSOLE NAMES konsole)
		find_program(GNOME_TERMINAL NAMES gnome-terminal)
		find_program(CGDB NAMES cgdb)

		if (NOT KONSOLE AND NOT GNOME_TERMINAL)
			message(FATAL_ERROR "konsole or gnome-terminal not found.")
		elseif (NOT CGDB)
			message(FATAL_ERROR "cdgb not found.")
		elseif(NOT (${TRACER} STREQUAL none))
			message(FATAL_ERROR "Cannot use cgdb with ${TRACER}")
		else()
			if (KONSOLE)
				set(cmd konsole -e cgdb --args ${cmd})
			elseif(GNOME_TERMINAL)
				set(cmd gnome-terminal --tab --active --wait -- cgdb --args ${cmd})
			endif()
		endif()
	endif()

	if(${output_file} STREQUAL none)
		execute_process(COMMAND ${cmd}
			OUTPUT_QUIET
			RESULT_VARIABLE res)
	else()
		execute_process(COMMAND ${cmd}
			RESULT_VARIABLE res
			OUTPUT_FILE ${BIN_DIR}/${TEST_NAME}.out
			ERROR_FILE ${BIN_DIR}/${TEST_NAME}.err)
	endif()

	print_logs()

	# pmemcheck is a special snowflake and it doesn't set exit code when
	# it detects an error, so we have to look at its output if match file
	# was not found.
	if(${TRACER} STREQUAL pmemcheck)
		if(NOT EXISTS ${BIN_DIR}/${TEST_NAME}.err)
			message(FATAL_ERROR "${TEST_NAME}.err not found.")
		endif()

		file(READ ${BIN_DIR}/${TEST_NAME}.err PMEMCHECK_ERR)
		message(STATUS "Stderr:\n${PMEMCHECK_ERR}\nEnd of stderr")
		if(NOT PMEMCHECK_ERR MATCHES "ERROR SUMMARY: 0")
			message(FATAL_ERROR "${TRACE} ${name} ${ARGN} failed: ${res}")
		endif()
	endif()

	if(res AND expect_success)
		message(FATAL_ERROR "${TRACE} ${name} ${ARGN} failed: ${res}")
	endif()

	if(NOT res AND NOT expect_success)
		message(FATAL_ERROR "${TRACE} ${name} ${ARGN} unexpectedly succeeded: ${res}")
	endif()

	if(TESTS_USE_FORCED_PMEM)
		unset(ENV{PMEM_IS_PMEM_FORCE})
	endif()
endfunction()

# Checks if target (test binary) was built
function(check_target name)
	if(NOT EXISTS ${name})
		message(FATAL_ERROR "Test file: \"${name}\" was not found! If test wasn't built, run make first.")
	endif()
endfunction()

# Generic command executor which handles failures and prints command output
# to specified file.
function(execute_with_output out name)
	check_target(${name})

	execute_common(true ${out} ${name} ${ARGN})
endfunction()

# Generic command executor which handles failures but ignores output.
function(execute_ignore_output name)
	check_target(${name})

	execute_common(true none ${name} ${ARGN})
endfunction()

# Executes test command ${name} and verifies its status.
# First argument of the command is test directory name.
# Optional function arguments are passed as consecutive arguments to
# the command.
function(execute name)
	check_target(${name})

	execute_common(true ${TRACER}_${TESTCASE} ${name} ${ARGN})
endfunction()

# Trim '{' and '}' characters from the beginning and end
function(trim_config config_string out_config)
	string(LENGTH ${config_string} config_len)
	math(EXPR config_len "${config_len}-2")
	string(SUBSTRING ${config_string} 1 ${config_len} out)
	set(${out_config} ${out} PARENT_SCOPE)
endfunction()

# Expects config in form of a json-like list, e.g.
# make_config({"path":"/path/to/file"})
function(make_config)
	string(REPLACE " " "" config ${ARGN})

	if ("${EXTRA_CONFIG_PARAMS}" STREQUAL "")
		set(CONFIG "${config}" CACHE INTERNAL "")
	else()
		trim_config(${config} trimmed_config)
		trim_config(${EXTRA_CONFIG_PARAMS} extra_config_params)
		set(CONFIG "{${trimmed_config},${extra_config_params}}" CACHE INTERNAL "")
	endif()
endfunction()

# Executes command ${name} and creates a storelog.
# First argument is pool file.
# Second argument is test executable.
# Optional function arguments are passed as consecutive arguments to
# the command.
function(pmreorder_create_store_log pool name)
	check_target(${name})

	if(NOT (${TRACER} STREQUAL none))
		message(FATAL_ERROR "Pmreorder test must be run without any tracer.")
	endif()

	configure_file(${pool} ${pool}.copy COPYONLY)

	set(ENV{PMREORDER_EMIT_LOG} 1)

	if(DEFINED ENV{PMREORDER_STACKTRACE_DEPTH})
		set(PMREORDER_STACKTRACE_DEPTH $ENV{PMREORDER_STACKTRACE_DEPTH})
		set(PMREORDER_STACKTRACE "yes")
	else()
		set(PMREORDER_STACKTRACE_DEPTH 1)
		set(PMREORDER_STACKTRACE "no")
	endif()

	set(cmd valgrind --tool=pmemcheck -q
		--log-stores=yes
		--print-summary=no
		--log-file=${BIN_DIR}/${TEST_NAME}.storelog
		--log-stores-stacktraces=${PMREORDER_STACKTRACE}
		--log-stores-stacktraces-depth=${PMREORDER_STACKTRACE_DEPTH}
		--expect-fence-after-clflush=yes
		${name} ${ARGN})

	execute_common(true ${TRACER}_${TESTCASE} ${cmd})

	file(READ ${BIN_DIR}/${TEST_NAME}.storelog STORELOG)
	string(REPLACE "operator|=" "operator_OR" FIXED_STORELOG "${STORELOG}")
	file(WRITE ${BIN_DIR}/${TEST_NAME}.storelog "${FIXED_STORELOG}")

	unset(ENV{PMREORDER_EMIT_LOG})

	file(REMOVE ${pool})
	configure_file(${pool}.copy ${pool} COPYONLY)
endfunction()

# Executes pmreorder.
# First argument is expected result.
# Second argument is engine type.
# Third argument is path to configure file.
# Fourth argument is path to the checker program.
# Optional function arguments are passed as consecutive arguments to
# the command.
function(pmreorder_execute expect_success engine conf_file name)
	check_target(${name})

	if(NOT (${TRACER} STREQUAL none))
		message(FATAL_ERROR "Pmreorder test must be run without any tracer.")
	endif()

	set(ENV{PMEMOBJ_CONF} "copy_on_write.at_open=1")

	string(REPLACE "\"" "\\\"" ESCAPED_ARGN "${ARGN}")

	set(cmd pmreorder -l ${BIN_DIR}/${TEST_NAME}.storelog
					-o ${BIN_DIR}/${TEST_NAME}.pmreorder
					-r ${engine}
					-p "${name} ${ESCAPED_ARGN}"
					-x ${conf_file})

	execute_common(${expect_success} ${TRACER}_${TESTCASE} ${cmd})

	unset(ENV{PMEMOBJ_CONF})
endfunction()

function(pmempool_execute)
	set(ENV{LD_LIBRARY_PATH} ${LIBPMEMOBJ++_LIBRARY_DIRS})

	execute_process(COMMAND pmempool ${ARGN})

	unset(ENV{LD_LIBRARY_PATH})
endfunction()

# Executes test command ${name} under GDB.
# First argument of the command is a gdb batch file.
# Second argument of the command is the test command.
# Optional function arguments are passed as consecutive arguments to
# the command.
function(crash_with_gdb gdb_batch_file name)
	check_target(${name})

	set(PREV_TRACER ${TRACER})
	set(TRACER gdb)
	set(GDB_BATCH_FILE ${gdb_batch_file})

	execute_common(true ${TRACER}_${TESTCASE} ${name} ${ARGN})

	set(TRACER ${PREV_TRACER})
endfunction()

# Checks whether specified filename is located on persistent memory and emits
# FATAL_ERROR in case it's not.
function(check_is_pmem filename)
	execute_process(COMMAND ${BIN_DIR}/../check_is_pmem ${filename} RESULT_VARIABLE is_pmem)

	if (${is_pmem} EQUAL 2)
		message(FATAL_ERROR "check_is_pmem failed.")
	elseif ((${is_pmem} EQUAL 1) AND (NOT TESTS_USE_FORCED_PMEM))
		# Return value 1 means that path points to non-pmem
		message(FATAL_ERROR "${TEST_NAME} can only be run on PMEM.")
	endif()
endfunction()
