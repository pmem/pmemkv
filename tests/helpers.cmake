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

set(DIR ${PARENT_DIR}/${TEST_NAME})

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

    if(EXISTS ${SRC_DIR}/${TEST_NAME}.err.match)
        match(${BIN_DIR}/${TEST_NAME}.err ${SRC_DIR}/${TEST_NAME}.err.match)
    endif()
    if(EXISTS ${SRC_DIR}/${TEST_NAME}.out.match)
        match(${BIN_DIR}/${TEST_NAME}.out ${SRC_DIR}/${TEST_NAME}.out.match)
    endif()

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
        set(ENV{LIBPMEMOBJ_CPP_TRACER_PMEMCHECK} 1)
    elseif(${TRACER} STREQUAL memcheck)
        set(TRACE valgrind --error-exitcode=99 --tool=memcheck --leak-check=full
           --suppressions=${TEST_ROOT_DIR}/ld.supp --suppressions=${TEST_ROOT_DIR}/memcheck-stdcpp.supp --suppressions=${TEST_ROOT_DIR}/memcheck-libunwind.supp)
        set(ENV{LIBPMEMOBJ_CPP_TRACER_MEMCHECK} 1)
    elseif(${TRACER} STREQUAL helgrind)
        set(TRACE valgrind --error-exitcode=99 --tool=helgrind)
        set(ENV{LIBPMEMOBJ_CPP_TRACER_HELGRIND} 1)
    elseif(${TRACER} STREQUAL drd)
        set(TRACE valgrind --error-exitcode=99 --tool=drd)
        set(ENV{LIBPMEMOBJ_CPP_TRACER_DRD} 1)
    elseif(${TRACER} STREQUAL gdb)
        set(TRACE gdb --batch --command=${GDB_BATCH_FILE} --args)
        set(ENV{LIBPMEMOBJ_CPP_TRACER_GDB} 1)
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

    # memcheck and pmemcheck match files should follow name pattern:
    # testname_testcasenr_memcheck/pmemcheck.err.match
    # If they do exist, ignore test result - it will be verified during
    # log matching in finish() function.
    if(EXISTS ${SRC_DIR}/${TEST_NAME}.err.match)
        valgrind_ignore_warnings(${BIN_DIR}/${TEST_NAME}.err)
    # pmemcheck is a special snowflake and it doesn't set exit code when
    # it detects an error, so we have to look at its output if match file
    # was not found.
    else()
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
    endif()

    if(${TRACER} STREQUAL pmemcheck)
        unset(ENV{LIBPMEMOBJ_CPP_TRACER_PMEMCHECK})
    elseif(${TRACER} STREQUAL memcheck)
        unset(ENV{LIBPMEMOBJ_CPP_TRACER_MEMCHECK})
    elseif(${TRACER} STREQUAL helgrind)
        unset(ENV{LIBPMEMOBJ_CPP_TRACER_HELGRIND})
    elseif(${TRACER} STREQUAL drd)
        unset(ENV{LIBPMEMOBJ_CPP_TRACER_DRD})
    elseif(${TRACER} STREQUAL gdb)
        unset(ENV{LIBPMEMOBJ_CPP_TRACER_GDB})
    endif()

    if(TESTS_USE_FORCED_PMEM)
        unset(ENV{PMEM_IS_PMEM_FORCE})
    endif()
endfunction()

function(check_target name)
    if(NOT EXISTS ${name})
        message(FATAL_ERROR "Tests were not found! If not built, run make first.")
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

    set(cmd pmreorder -l ${BIN_DIR}/${TEST_NAME}.storelog
                    -o ${BIN_DIR}/${TEST_NAME}.pmreorder
                    -r ${engine}
                    -p "${name} ${ARGN}"
                    -x ${conf_file})

    execute_common(${expect_success} ${TRACER}_${TESTCASE} ${cmd})

    unset(ENV{PMEMOBJ_CONF})
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
