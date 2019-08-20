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

set(DIR ${PARENT_DIR}/${TEST_NAME})

function(setup)
	execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory ${DIR})
	execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${DIR})
	execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory ${BIN_DIR})
	execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${BIN_DIR})
endfunction()

function(cleanup)
	execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory ${DIR})
endfunction()

# Executes test command ${name} and verifies its status matches ${expectation}.
# Optional function arguments are passed as consecutive arguments to
# the command.
function(execute_arg input expectation name)
	message(STATUS "Executing: ${name} ${ARGN}")
	if("${input}" STREQUAL "")
		execute_process(COMMAND ${name} ${ARGN}
			RESULT_VARIABLE RET
			OUTPUT_FILE ${BIN_DIR}/out
			ERROR_FILE ${BIN_DIR}/err)
	else()
		execute_process(COMMAND ${name} ${ARGN}
			RESULT_VARIABLE RET
			INPUT_FILE ${input}
			OUTPUT_FILE ${BIN_DIR}/out
			ERROR_FILE ${BIN_DIR}/err)
	endif()
	message(STATUS "Test ${name}:")
	file(READ ${BIN_DIR}/out OUT)
	message(STATUS "Stdout:\n${OUT}")
	file(READ ${BIN_DIR}/err ERR)
	message(STATUS "Stderr:\n${ERR}")

	if(NOT RET EQUAL expectation)
		message(FATAL_ERROR "${name} ${ARGN} exit code ${RET} doesn't match expectation: ${expectation}")
	endif()
endfunction()

# wrapper of execute_arg()
function(execute name)
	execute_arg("" 0 ${name} ${ARGN})
endfunction()

function(run_under_valgrind vg_opt name)
	message(STATUS "Executing: valgrind ${vg_opt} ${name} ${ARGN}")
	execute_process(COMMAND valgrind ${vg_opt} ${name} ${ARGN}
			RESULT_VARIABLE RET
			OUTPUT_FILE ${BIN_DIR}/out
			ERROR_FILE ${BIN_DIR}/err)
	message(STATUS "Test ${name}:")
	file(READ ${BIN_DIR}/out OUT)
	message(STATUS "Stdout:\n${OUT}")
	file(READ ${BIN_DIR}/err ERR)
	message(STATUS "Stderr:\n${ERR}")

	if(NOT RET EQUAL 0)
		message(FATAL_ERROR
			"command 'valgrind ${name} ${ARGN}' failed:\n${ERR}")
	endif()

	if(${TRACER} STREQUAL pmreorder)
		return(0)
	endif()

	set(text_passed "ERROR SUMMARY: 0 errors")
	string(FIND "${ERR}" "${text_passed}" RET)
	if(RET EQUAL -1)
		message(FATAL_ERROR
			"command 'valgrind ${vg_opt} ${name} ${ARGN}' failed:\n${ERR}")
	endif()
endfunction()

function(execute_tracer name)
	if (${TRACER} STREQUAL "none")
		execute(${name} ${ARGN})
	elseif (${TRACER} STREQUAL memcheck)
		set(MEM_SUPP "${SRC_DIR}/memcheck.supp")
		set(VG_OPT "--leak-check=full" "--suppressions=${MEM_SUPP}")
		run_under_valgrind("${VG_OPT}" ${name} ${ARGN})
	elseif (${TRACER} STREQUAL helgrind)
		set(HEL_SUPP "${SRC_DIR}/helgrind.supp")
		set(VG_OPT "--tool=helgrind" "--suppressions=${HEL_SUPP}")
		run_under_valgrind("${VG_OPT}" ${name} ${ARGN})
	elseif (${TRACER} STREQUAL drd)
		set(DRD_SUPP "${SRC_DIR}/drd.supp")
		set(VG_OPT "--tool=drd" "--suppressions=${DRD_SUPP}")
		run_under_valgrind("${VG_OPT}" ${name} ${ARGN})
	elseif (${TRACER} STREQUAL pmemcheck)
		set(VG_OPT "--tool=pmemcheck")
		run_under_valgrind("${VG_OPT}" ${name} ${ARGN})
	elseif (${TRACER} STREQUAL pmreorder)
		set(ENV{PMREORDER_EMIT_LOG} 1)
		if(DEFINED ENV{PMREORDER_STACKTRACE_DEPTH})
			set(PMREORDER_STACKTRACE_DEPTH $ENV{PMREORDER_STACKTRACE_DEPTH})
			set(PMREORDER_STACKTRACE "yes")
		else()
			set(PMREORDER_STACKTRACE_DEPTH 1)
			set(PMREORDER_STACKTRACE "no")
		endif()
		set(VG_OPT "--tool=pmemcheck" "-q" "--log-stores=yes" "--print-summary=no")
		set(VG_OPT "${VG_OPT}" "--log-file=${BIN_DIR}/${TEST_NAME}.storelog")
		set(VG_OPT "${VG_OPT}" "--log-stores-stacktraces=${PMREORDER_STACKTRACE}")
		set(VG_OPT "${VG_OPT}" "--log-stores-stacktraces-depth=${PMREORDER_STACKTRACE_DEPTH}")
		set(VG_OPT "${VG_OPT}" "--expect-fence-after-clflush=yes")
		run_under_valgrind("${VG_OPT}" ${name} ${ARGN})
	else ()
		message(FATAL_ERROR "unknown tracer: ${TRACER}")
	endif ()
endfunction()

# wrapper of execute_tracer()
function(pmreorder_create_store_log name)
	if (NOT ${TRACER} STREQUAL pmreorder)
		message(FATAL_ERROR "pmreorder_create_store_log() can be called only with the 'pmreorder' tracer.")
	endif()
	execute_tracer(${name} ${ARGN})
endfunction()

# Executes pmreorder.
# First argument is expected result.
# Second argument is engine type.
# Third argument is path to configure file.
# Fourth argument is path to the checker program.
# Optional function arguments are passed as consecutive arguments to
# the command.
function(pmreorder_execute engine conf_file name)
	set(ENV{PMEMOBJ_CONF} "copy_on_write.at_open=1")

	set(cmd pmreorder
		-l ${BIN_DIR}/${TEST_NAME}.storelog
		-o ${BIN_DIR}/${TEST_NAME}.pmreorder
		-r ${engine}
		-x ${conf_file}
		-p "${name} ${ARGN}")

	execute(${cmd})

	unset(ENV{PMEMOBJ_CONF})
endfunction()
