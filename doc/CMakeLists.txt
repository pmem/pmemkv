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

add_check_whitespace(man
	${CMAKE_CURRENT_SOURCE_DIR}/*.md)

find_program(PANDOC NAMES pandoc)
if(PANDOC)
	# Prepare C_EXAMPLE content to inject, based on the code
	# of the 'examples/pmemkv_basic_c/pmemkv_basic.c' example;
	# remove comments (with the license and the file description)
	# up to the first preprocessor's directive.
	file(READ
		${CMAKE_SOURCE_DIR}/examples/pmemkv_basic_c/pmemkv_basic.c
		example_content)
	string(REGEX REPLACE "([/]+[*]+).*([*]+[/]+)([\n]+#)" "#"
		example_content "${example_content}")
	set(C_EXAMPLE "${example_content}")
	configure_file(${CMAKE_SOURCE_DIR}/doc/libpmemkv.3.md.in
		${CMAKE_BINARY_DIR}/man/tmp/libpmemkv.3.md)

	# Prepare C_EXAMPLE content to inject, based on the code
	# of the 'examples/pmemkv_config_c/pmemkv_config.c' example;
	# remove comments (with the license and the file description)
	# up to the first preprocessor's directive.
	file(READ
		${CMAKE_SOURCE_DIR}/examples/pmemkv_config_c/pmemkv_config.c
		example_content)
	string(REGEX REPLACE "([/]+[*]+).*([*]+[/]+)([\n]+#)" "#"
		example_content "${example_content}")
	set(C_EXAMPLE "${example_content}")
	configure_file(${CMAKE_SOURCE_DIR}/doc/libpmemkv_config.3.md.in
		${CMAKE_BINARY_DIR}/man/tmp/libpmemkv_config.3.md)

	# convert md files to manpage format
	add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/man/libpmemkv.7
		MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/doc/libpmemkv.7.md
		COMMAND ${CMAKE_SOURCE_DIR}/utils/md2man/md2man.sh
			${CMAKE_SOURCE_DIR}/doc/libpmemkv.7.md
			${CMAKE_SOURCE_DIR}/utils/md2man/default.man
			${CMAKE_BINARY_DIR}/man/libpmemkv.7
			${VERSION})
	add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/man/libpmemkv.3
		MAIN_DEPENDENCY ${CMAKE_BINARY_DIR}/man/tmp/libpmemkv.3.md
		COMMAND ${CMAKE_SOURCE_DIR}/utils/md2man/md2man.sh
			${CMAKE_BINARY_DIR}/man/tmp/libpmemkv.3.md
			${CMAKE_SOURCE_DIR}/utils/md2man/default.man
			${CMAKE_BINARY_DIR}/man/libpmemkv.3
			${VERSION})
	add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/man/libpmemkv_config.3
		MAIN_DEPENDENCY ${CMAKE_BINARY_DIR}/man/tmp/libpmemkv_config.3.md
		COMMAND ${CMAKE_SOURCE_DIR}/utils/md2man/md2man.sh
			${CMAKE_BINARY_DIR}/man/tmp/libpmemkv_config.3.md
			${CMAKE_SOURCE_DIR}/utils/md2man/default.man
			${CMAKE_BINARY_DIR}/man/libpmemkv_config.3
			${VERSION})
	add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/man/libpmemkv_json_config.3
		MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/doc/libpmemkv_json_config.3.md
		COMMAND ${CMAKE_SOURCE_DIR}/utils/md2man/md2man.sh
			${CMAKE_SOURCE_DIR}/doc/libpmemkv_json_config.3.md
			${CMAKE_SOURCE_DIR}/utils/md2man/default.man
			${CMAKE_BINARY_DIR}/man/libpmemkv_json_config.3
			${VERSION})

	# install manpages
	install(FILES ${CMAKE_BINARY_DIR}/man/libpmemkv.7
		DESTINATION ${CMAKE_INSTALL_MANDIR}/man7)
	install(FILES ${CMAKE_BINARY_DIR}/man/libpmemkv.3
		DESTINATION ${CMAKE_INSTALL_MANDIR}/man3)
	install(FILES ${CMAKE_BINARY_DIR}/man/libpmemkv_config.3
		DESTINATION ${CMAKE_INSTALL_MANDIR}/man3)
	install(FILES ${CMAKE_BINARY_DIR}/man/libpmemkv_json_config.3
		DESTINATION ${CMAKE_INSTALL_MANDIR}/man3)
else()
	message(WARNING "pandoc not found - man pages will not be generated")
endif()

include(FindDoxygen)
if(DOXYGEN_FOUND AND DOXYGEN_DOT_FOUND)
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/libpmemkv.Doxyfile.in"
		"${CMAKE_CURRENT_BINARY_DIR}/libpmemkv.Doxyfile" @ONLY)
	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cpp_html/ DESTINATION ${CMAKE_INSTALL_DOCDIR})
elseif(NOT DOXYGEN_FOUND)
	message(WARNING "Doxygen not found - Doxygen documentation will not be generated")
else()
	message(WARNING "Dot tool not found - Doxygen documentation will not be generated")
endif()

if(PANDOC AND DOXYGEN_FOUND AND DOXYGEN_DOT_FOUND)
	add_custom_target(doc ALL
		${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/libpmemkv.Doxyfile"
		DEPENDS ${CMAKE_BINARY_DIR}/man/libpmemkv.7
		DEPENDS ${CMAKE_BINARY_DIR}/man/libpmemkv.3
		DEPENDS ${CMAKE_BINARY_DIR}/man/libpmemkv_config.3
		DEPENDS ${CMAKE_BINARY_DIR}/man/libpmemkv_json_config.3
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
elseif(PANDOC)
	add_custom_target(doc ALL
		DEPENDS ${CMAKE_BINARY_DIR}/man/libpmemkv.7
		DEPENDS ${CMAKE_BINARY_DIR}/man/libpmemkv.3
		DEPENDS ${CMAKE_BINARY_DIR}/man/libpmemkv_config.3
		DEPENDS ${CMAKE_BINARY_DIR}/man/libpmemkv_json_config.3
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
elseif(DOXYGEN_FOUND AND DOXYGEN_DOT_FOUND)
	add_custom_target(doc ALL
		${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/libpmemkv.Doxyfile"
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()
