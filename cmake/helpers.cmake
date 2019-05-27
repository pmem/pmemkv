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

function(set_version VERSION)
	execute_process(COMMAND git describe
			OUTPUT_VARIABLE GIT_VERSION)
	if(GIT_VERSION)
		# 1.5-rc1-19-gb8f78a329 -> 1.5-rc1+git19.gb8f78a329
		string(REGEX MATCHALL
			"([0-9.]*)-rc([0-9]*)-([0-9]*)-([0-9a-g]*)"
			MATCHES
			${GIT_VERSION})
		if(MATCHES)
			set(VERSION
				"${CMAKE_MATCH_1}-rc${CMAKE_MATCH_2}+git${CMAKE_MATCH_3}.${CMAKE_MATCH_4}"
				PARENT_SCOPE)
		endif()

		# 1.5-19-gb8f78a329 -> 1.5+git19.gb8f78a329
		string(REGEX MATCHALL
			"\([0-9.]*\)-\([0-9]*\)-\([0-9a-g]*\)"
			MATCHES
			${GIT_VERSION})
		if(MATCHES)
			set(VERSION
				"${CMAKE_MATCH_1}+git${CMAKE_MATCH_2}.${CMAKE_MATCH_3}"
				PARENT_SCOPE)
		endif()
	else()
		execute_process(COMMAND git log -1 --format=%h
				OUTPUT_VARIABLE GIT_COMMIT)
		set(VERSION
			${GIT_COMMIT}
			PARENT_SCOPE)
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

		string(REGEX MATCH ".*pmemcheck-([0-9.]+),.*" PMEMCHECK_OUT "${PMEMCHECK_OUT}")
		set(PMEMCHECK_VERSION ${CMAKE_MATCH_1} CACHE INTERNAL "")
	else()
		message(WARNING "Valgrind pmemcheck NOT found.")
	endif()
endfunction()
