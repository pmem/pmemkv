# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019, Intel Corporation

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
		set(VG_OPT "--tool=pmemcheck" "-q" "--log-stores=yes" "--print-summary=no"
			"--log-file=${BIN_DIR}/${TEST_NAME}.storelog"
			"--log-stores-stacktraces=${PMREORDER_STACKTRACE}"
			"--log-stores-stacktraces-depth=${PMREORDER_STACKTRACE_DEPTH}"
			"--expect-fence-after-clflush=yes")
		run_under_valgrind("${VG_OPT}" ${name} ${ARGN})
	else ()
		message(FATAL_ERROR "unknown tracer: ${TRACER}")
	endif ()
endfunction()

#
# pmreorder_create_store_log -- execute the command ${name} under pmemcheck and create a storelog
#
# Arguments:
#	name - path to the checker program with optional parameters
#
function(pmreorder_create_store_log name)
	if (NOT ${TRACER} STREQUAL pmreorder)
		message(FATAL_ERROR "pmreorder_create_store_log() can be called only with the 'pmreorder' tracer.")
	endif()
	execute_tracer(${name} ${ARGN})
endfunction()

#
# pmreorder_execute -- execute pmreorder
#
# Arguments:
#	engine    - pmreorder engine type (for example 'ReorderAccumulative')
#	conf_file - path to the configuration file
#	name      - path to the checker program with optional parameters
#
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
