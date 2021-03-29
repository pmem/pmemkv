# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2021, Intel Corporation

#
# doc/CMakeLists.txt - prepares 'make doc' command for documentation
#

# Prepares example code to inject into man page:
# Read file pointed by ${PATH}, remove comments (with license and description)
# up to the first preprocessor's directive and place it in ${VAL} in a caller scope.
function(strip_example PATH VAL)
	file(READ ${PATH} example_content)
	string(REGEX REPLACE "([/]+[*/]+).*([*]+[/]+)([\n]+#)" "#"
		example_content "${example_content}")
	set(${VAL} "${example_content}" PARENT_SCOPE)
endfunction()

# Converts md files into manpage format. Requires pandoc command.
# NAME is output manpage name, INPUT is a path to the source md file
function(configure_man NAME INPUT)
	add_custom_command(OUTPUT ${MAN_DIR}/${NAME}
		MAIN_DEPENDENCY ${INPUT}
		COMMAND ${PMEMKV_ROOT_DIR}/utils/md2man/md2man.sh
			${INPUT}
			${PMEMKV_ROOT_DIR}/utils/md2man/default.man
			${MAN_DIR}/${NAME}
			${VERSION})
	list(APPEND MANPAGE_OUTFILES ${MAN_DIR}/${NAME})
	set(MANPAGE_OUTFILES ${MANPAGE_OUTFILES} PARENT_SCOPE)
endfunction()

# Generate files (to be installed) - links of C API functions (passed as {ARGN})
# to chosen (3) manpage.
function(add_manpage_links MANPAGE)
	foreach(function ${ARGN})
		set(CONTENT ".so ${MANPAGE}")
		file(WRITE ${MAN_DIR}/${function}.3 "${CONTENT}")
	endforeach()
endfunction()

# ----------------------------------------------------------------- #
## Setup custom targets and useful variables
# ----------------------------------------------------------------- #
set(MANPAGE_OUTFILES "")
set(DOXYGEN_COMMAND "")
set(MAN_DIR ${CMAKE_CURRENT_BINARY_DIR}/man)
add_check_whitespace(man ${CMAKE_CURRENT_SOURCE_DIR}/*.*)

# ----------------------------------------------------------------- #
## Prepare C documentation (using manpage format)
# ----------------------------------------------------------------- #
find_program(PANDOC NAMES pandoc)
if(PANDOC)
	message(STATUS "Found pandoc: ${PANDOC}")

	## Manpages without any examples
	# libpmemkv.7
	configure_man(libpmemkv.7 ${CMAKE_CURRENT_SOURCE_DIR}/libpmemkv.7.md)

	# libpmemkv_json_config.3
	configure_man(libpmemkv_json_config.3 ${CMAKE_CURRENT_SOURCE_DIR}/libpmemkv_json_config.3.md)
	add_manpage_links(libpmemkv_json_config.3
		pmemkv_config_from_json pmemkv_config_from_json_errormsg)

	## Manpages with example(s) injected
	# libpmemkv.3
	strip_example(${PMEMKV_ROOT_DIR}/examples/pmemkv_basic_c/pmemkv_basic.c C_EXAMPLE)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libpmemkv.3.md.in
		${MAN_DIR}/tmp/libpmemkv.3.md)
	configure_man(libpmemkv.3 ${MAN_DIR}/tmp/libpmemkv.3.md)
	add_manpage_links(libpmemkv.3
		pmemkv_get_kv_callback pmemkv_get_v_callback
		pmemkv_open pmemkv_close pmemkv_count_all pmemkv_count_above pmemkv_count_below
		pmemkv_count_between pmemkv_get_all pmemkv_get_above pmemkv_get_below pmemkv_get_between
		pmemkv_exists pmemkv_get pmemkv_get_copy pmemkv_put pmemkv_remove pmemkv_defrag pmemkv_errormsg)

	# libpmemkv_config.3
	strip_example(
		${PMEMKV_ROOT_DIR}/examples/pmemkv_config_c/pmemkv_config.c
		CONFIG_TYPE_BASED_EXAMPLE)
	strip_example(
		${PMEMKV_ROOT_DIR}/examples/pmemkv_config_c/pmemkv_basic_config.c
		CONFIG_BASIC_EXAMPLE)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libpmemkv_config.3.md.in
		${MAN_DIR}/tmp/libpmemkv_config.3.md)
	configure_man(libpmemkv_config.3 ${MAN_DIR}/tmp/libpmemkv_config.3.md)
	add_manpage_links(libpmemkv_config.3
		pmemkv_config_new pmemkv_config_delete pmemkv_config_put_size pmemkv_config_put_path pmemkv_config_put_force_create
		pmemkv_config_put_create_or_error_if_exists pmemkv_config_put_create_if_missing pmemkv_config_put_comparator pmemkv_config_put_oid
		pmemkv_config_put_data pmemkv_config_put_object pmemkv_config_put_object_cb pmemkv_config_put_uint64
		pmemkv_config_put_int64 pmemkv_config_put_string pmemkv_config_get_data pmemkv_config_get_object pmemkv_config_get_uint64
		pmemkv_config_get_int64 pmemkv_config_get_string pmemkv_comparator_new pmemkv_comparator_delete)

	# libpmemkv_tx.3
	strip_example(
		${PMEMKV_ROOT_DIR}/examples/pmemkv_transaction_c/pmemkv_transaction.c
		TRANSACTION_BASIC_C_EXAMPLE)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libpmemkv_tx.3.md.in
		${MAN_DIR}/tmp/libpmemkv_tx.3.md)
	configure_man(libpmemkv_tx.3 ${MAN_DIR}/tmp/libpmemkv_tx.3.md)
	add_manpage_links(libpmemkv_tx.3
		pmemkv_tx_begin pmemkv_tx_put pmemkv_tx_remove pmemkv_tx_commit pmemkv_tx_abort pmemkv_tx_end)

	# libpmemkv_iterator.3
	strip_example(
		${PMEMKV_ROOT_DIR}/examples/pmemkv_iterator_c/pmemkv_iterator.c
		ITERATOR_BASIC_EXAMPLE)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libpmemkv_iterator.3.md.in
		${MAN_DIR}/tmp/libpmemkv_iterator.3.md)
	configure_man(libpmemkv_iterator.3 ${MAN_DIR}/tmp/libpmemkv_iterator.3.md)
	add_manpage_links(libpmemkv_iterator.3
		pmemkv_iterator_new pmemkv_write_iterator_new pmemkv_iterator_delete pmemkv_write_iterator_delete
		pmemkv_iterator_seek pmemkv_iterator_seek_lower pmemkv_iterator_seek_lower_eq pmemkv_iterator_seek_higher
		pmemkv_iterator_seek_higher_eq pmemkv_iterator_seek_to_first pmemkv_iterator_seek_to_last
		pmemkv_iterator_is_next pmemkv_iterator_next pmemkv_iterator_prev pmemkv_iterator_key pmemkv_iterator_read_range
		pmemkv_write_iterator_write_range pmemkv_write_iterator_commit pmemkv_write_iterator_abort)

	# install manpages
	install(FILES ${MAN_DIR}/libpmemkv.7
		DESTINATION ${CMAKE_INSTALL_MANDIR}/man7)
	install(DIRECTORY ${MAN_DIR}/
		DESTINATION ${CMAKE_INSTALL_MANDIR}/man3
		FILES_MATCHING
		PATTERN "*.3"
		PATTERN "tmp" EXCLUDE)
else()
	message(WARNING "pandoc not found - man pages (C documentation) will not be generated")
endif()

# ----------------------------------------------------------------- #
## Prepare C++ documentation (using doxygen format)
# ----------------------------------------------------------------- #
include(FindDoxygen)
if(DOXYGEN_FOUND AND DOXYGEN_DOT_FOUND)
	if(DEVELOPER_MODE)
		set(DOXYGEN_WARN_AS_ERROR "YES")
	else()
		set(DOXYGEN_WARN_AS_ERROR "NO")
	endif()

	set(DOXYGEN_OUTFILE "${CMAKE_CURRENT_BINARY_DIR}/libpmemkv.Doxyfile")
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/libpmemkv.Doxyfile.in"
		${DOXYGEN_OUTFILE} @ONLY)
	set(DOXYGEN_COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUTFILE})

	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cpp_html/ DESTINATION ${CMAKE_INSTALL_DOCDIR})
elseif(NOT DOXYGEN_FOUND)
	message(WARNING "Doxygen not found - Doxygen (C++) documentation will not be generated")
else()
	message(WARNING "Dot tool not found - Doxygen (C++) documentation will not be generated")
endif()

# ----------------------------------------------------------------- #
## Prepare the actual 'make doc' command
# ----------------------------------------------------------------- #
if(MANPAGE_OUTFILES OR DOXYGEN_COMMAND)
	add_custom_target(doc ALL
		${DOXYGEN_COMMAND}
		DEPENDS ${MANPAGE_OUTFILES}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
else()
	message(WARNING "Neither C or C++ docs could be built. "
		"If you wish to build them install pandoc (for C) and/or doxygen (for C++), "
		"otherwise disable it using CMake option -DBUILD_DOC=OFF")
endif()
