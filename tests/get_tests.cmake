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

# parse tests source files to get the list of all tests
function(get_tests dir test_files output)
	set(all_tests "")
	foreach(file IN ITEMS ${test_files})
		file(STRINGS ${dir}/${file} tests REGEX "^TEST_F")
		string(REPLACE "TEST_F(" "" tests "${tests}")
		string(REPLACE ", " "." tests "${tests}")
		string(REPLACE ")" " " tests "${tests}")
		string(REGEX MATCH "[^a-zA-Z0-9. ;_]+" forbidden "${tests}")
		if(forbidden)
			message(FATAL_ERROR "One of test names: \"${tests}\"\ncontains forbidden character(s): \"${forbidden}\"")
		endif()
		string(CONCAT all_tests ${all_tests} ${tests})
	endforeach(file)
	string(REPLACE " " ";" list_all_tests "${all_tests}")
	set(${output} "${list_all_tests}" PARENT_SCOPE)
endfunction()
