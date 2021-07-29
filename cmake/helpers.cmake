# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2021, Intel Corporation

#
# helpers.cmake - helper functions for top-level CMakeLists.txt
#

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

# Sets project's version based on git output, in ${VERSION}
function(set_version VERSION)
	set(VERSION_FILE ${PMEMKV_ROOT_DIR}/VERSION)
	if(EXISTS ${VERSION_FILE})
		file(STRINGS ${VERSION_FILE} FILE_VERSION)
		set(VERSION ${FILE_VERSION} PARENT_SCOPE)
		return()
	endif()

	execute_process(COMMAND git describe
			OUTPUT_VARIABLE GIT_VERSION
			WORKING_DIRECTORY ${PMEMKV_ROOT_DIR}
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET)
	if(GIT_VERSION)
		# 1.5-rc1-19-gb8f78a329 -> 1.5-rc1.git19.gb8f78a329
		string(REGEX MATCHALL
			"([0-9.]*)-rc([0-9]*)-([0-9]*)-([0-9a-g]*)"
			MATCHES
			${GIT_VERSION})
		if(MATCHES)
			set(VERSION
				"${CMAKE_MATCH_1}-rc${CMAKE_MATCH_2}.git${CMAKE_MATCH_3}.${CMAKE_MATCH_4}"
				PARENT_SCOPE)
			return()
		endif()

		# 1.5-19-gb8f78a329 -> 1.5-git19.gb8f78a329
		string(REGEX MATCHALL
			"([0-9.]*)-([0-9]*)-([0-9a-g]*)"
			MATCHES
			${GIT_VERSION})
		if(MATCHES)
			set(VERSION
				"${CMAKE_MATCH_1}-git${CMAKE_MATCH_2}.${CMAKE_MATCH_3}"
				PARENT_SCOPE)
		endif()
	else()
		execute_process(COMMAND git log -1 --format=%h
				OUTPUT_VARIABLE GIT_COMMIT
				WORKING_DIRECTORY ${PMEMKV_ROOT_DIR}
				OUTPUT_STRIP_TRAILING_WHITESPACE)
		set(VERSION ${GIT_COMMIT} PARENT_SCOPE)
	endif()
endfunction()

# Generates cppstyle-$name and cppformat-$name targets and attaches them
# as dependencies of global "cppformat" target.
# cppstyle-$name target verifies C++ style of files in current source dir.
# cppformat-$name target reformats files in current source dir.
# If more arguments are used, then they are used as files to be checked
# instead.
# ${name} must be unique.
function(add_cppstyle name)
	if(NOT CLANG_FORMAT OR NOT (CLANG_FORMAT_VERSION VERSION_EQUAL CLANG_FORMAT_REQUIRED))
		return()
	endif()

	if(${ARGC} EQUAL 1)
		add_custom_target(cppstyle-${name}
			COMMAND ${PERL_EXECUTABLE}
				${PMEMKV_ROOT_DIR}/utils/cppstyle
				${CLANG_FORMAT}
				check
				${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
				${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
			)
		add_custom_target(cppformat-${name}
			COMMAND ${PERL_EXECUTABLE}
				${PMEMKV_ROOT_DIR}/utils/cppstyle
				${CLANG_FORMAT}
				format
				${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
				${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
			)
	else()
		add_custom_target(cppstyle-${name}
			COMMAND ${PERL_EXECUTABLE}
				${PMEMKV_ROOT_DIR}/utils/cppstyle
				${CLANG_FORMAT}
				check
				${ARGN}
			)
		add_custom_target(cppformat-${name}
			COMMAND ${PERL_EXECUTABLE}
				${PMEMKV_ROOT_DIR}/utils/cppstyle
				${CLANG_FORMAT}
				format
				${ARGN}
			)
	endif()

	add_dependencies(cppstyle cppstyle-${name})
	add_dependencies(cppformat cppformat-${name})
endfunction()

# Generates check-whitespace-$name target and attaches it as a dependency
# of global "check-whitespace" target.
# ${name} must be unique.
function(add_check_whitespace name)
	if(NOT DEVELOPER_MODE)
		return()
	endif()

	add_custom_target(check-whitespace-${name}
		COMMAND ${PERL_EXECUTABLE}
			${PMEMKV_ROOT_DIR}/utils/check_whitespace ${ARGN})

	add_dependencies(check-whitespace check-whitespace-${name})
endfunction()

# Sets ${ret} to version of program specified by ${name} in major.minor format
function(get_program_version_major_minor name ret)
	execute_process(COMMAND ${name} --version
		OUTPUT_VARIABLE cmd_ret
		ERROR_QUIET)
	STRING(REGEX MATCH "([0-9]+)\.([0-9]+)" VERSION ${cmd_ret})
	SET(${ret} ${VERSION} PARENT_SCOPE)
endfunction()

# prepends prefix to list of strings
function(prepend var prefix)
	set(listVar "")
	foreach(f ${ARGN})
		list(APPEND listVar "${prefix}/${f}")
	endforeach(f)
	set(${var} "${listVar}" PARENT_SCOPE)
endfunction()

# Checks whether flag is supported by current C++ compiler and appends
# it to the relevant cmake variable.
# 1st argument is a flag
# 2nd (optional) argument is a build type (debug, release)
macro(add_cxx_flag flag)
	string(REPLACE - _ flag2 ${flag})
	string(REPLACE " " _ flag2 ${flag2})
	string(REPLACE = "_" flag2 ${flag2})
	set(check_name "CXX_HAS_${flag2}")

	check_cxx_compiler_flag(${flag} ${check_name})

	if (${${check_name}})
		if (${ARGC} EQUAL 1)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
		else()
			set(CMAKE_CXX_FLAGS_${ARGV1} "${CMAKE_CXX_FLAGS_${ARGV1}} ${flag}")
		endif()
	endif()
endmacro()

# Checks whether flag is supported by current C compiler and appends
# it to the relevant cmake variable.
# 1st argument is a flag
# 2nd (optional) argument is a build type (debug, release)
macro(add_c_flag flag)
	string(REPLACE - _ flag2 ${flag})
	string(REPLACE " " _ flag2 ${flag2})
	string(REPLACE = "_" flag2 ${flag2})
	set(check_name "C_HAS_${flag2}")

	check_c_compiler_flag(${flag} ${check_name})

	if (${${check_name}})
		if (${ARGC} EQUAL 1)
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}")
		else()
			set(CMAKE_C_FLAGS_${ARGV1} "${CMAKE_C_FLAGS_${ARGV1}} ${flag}")
		endif()
	endif()
endmacro()

# Checks whether flag is supported by both C and C++ compiler and appends
# it to the relevant cmake variables.
# 1st argument is a flag
# 2nd (optional) argument is a build type (debug, release)
macro(add_common_flag flag)
	add_c_flag(${flag} ${ARGV1})
	add_cxx_flag(${flag} ${ARGV1})
endmacro()

# Adds sanitizer flag for CXX compiler.
# It's used to check i.a. for USAN/UBSAN sanitizers, as an additional static analysis.
macro(add_sanitizer_flag flag)
	set(SAVED_CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
	set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES} -fsanitize=${flag}")

	check_cxx_compiler_flag("-fsanitize=${flag}" CXX_HAS_ASAN_UBSAN)
	if(CXX_HAS_ASAN_UBSAN)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=${flag}")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=${flag}")
	else()
		message("${flag} sanitizer not supported")
	endif()

	set(CMAKE_REQUIRED_LIBRARIES ${SAVED_CMAKE_REQUIRED_LIBRARIES})
endmacro()
