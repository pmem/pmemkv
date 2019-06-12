#
# Copyright 2017-2019, Intel Corporation
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

cmake_minimum_required(VERSION 3.5)
project(pmemkv)

include(cmake/helpers.cmake)
set_version(VERSION)

# set the default build type
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
	set(DEFAULT_BUILD_TYPE "Debug")
else()
	set(DEFAULT_BUILD_TYPE "RelWithDebInfo")
endif()

if(NOT CMAKE_BUILD_TYPE)
	message(STATUS "Setting build type to the default one (${DEFAULT_BUILD_TYPE})")
	set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}"
		CACHE STRING "Choose a type of build (Debug, Release or RelWithDebInfo)" FORCE)
endif()

option(COVERAGE "run coverage test" OFF)
if(COVERAGE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -coverage")
endif()

# Each engine can be enabled separately.
# By default all experimental engines are turned off.
option(ENGINE_VSMAP "enable vsmap engine" ON)
option(ENGINE_VCMAP "enable vcmap engine" ON)
option(ENGINE_CMAP "enable cmap engine" ON)
option(ENGINE_CACHING "enable experimental caching engine" OFF)
option(ENGINE_STREE "enable experimental stree engine" OFF)
option(ENGINE_TREE3 "enable experimental tree3 engine" OFF)

option(DEVELOPER_MODE "enable developer's checks" OFF)

if(ENGINE_VSMAP)
	add_definitions(-DENGINE_VSMAP)
endif()
if(ENGINE_VCMAP)
	add_definitions(-DENGINE_VCMAP)
endif()
if(ENGINE_CMAP)
	add_definitions(-DENGINE_CMAP)
endif()
if(ENGINE_CACHING)
	add_definitions(-DENGINE_CACHING)
endif()
if(ENGINE_STREE)
	add_definitions(-DENGINE_STREE)
endif()
if(ENGINE_TREE3)
	add_definitions(-DENGINE_TREE3)
endif()

set(SOURCE_FILES
	src/libpmemkv.cc
	src/libpmemkv.h
	src/engines/blackhole.cc
	src/engines/blackhole.h
)
# Add each engine source separately
if(ENGINE_VSMAP)
	list(APPEND SOURCE_FILES
		src/engines/vsmap.h
		src/engines/vsmap.cc
	)
endif()
if(ENGINE_VCMAP)
	list(APPEND SOURCE_FILES
		src/engines/vcmap.h
		src/engines/vcmap.cc
	)
endif()
if(ENGINE_CMAP)
	list(APPEND SOURCE_FILES
		src/engines/cmap.h
		src/engines/cmap.cc
	)
endif()
if(ENGINE_CACHING)
	list(APPEND SOURCE_FILES
		src/engines-experimental/caching.h
		src/engines-experimental/caching.cc
	)
endif()
if(ENGINE_STREE)
	list(APPEND SOURCE_FILES
		src/engines-experimental/stree.h
		src/engines-experimental/stree.cc
		src/engines-experimental/stree/persistent_b_tree.h
		src/engines-experimental/stree/pstring.h
	)
endif()
if(ENGINE_TREE3)
	list(APPEND SOURCE_FILES
		src/engines-experimental/tree3.h
		src/engines-experimental/tree3.cc
	)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
set(LIBPMEMOBJ_CPP_REQUIRED_VERSION 1.6)

find_package(PkgConfig QUIET)
include(FindPerl)
include(ExternalProject)
include(FindThreads)
include(CheckCXXSourceCompiles)
include(GNUInstallDirs)

include(rapidjson)
set(PKG_REQUIRES "RapidJSON")

if(ENGINE_VSMAP OR ENGINE_VCMAP OR ENGINE_CMAP OR ENGINE_STREE OR ENGINE_TREE3 OR ENGINE_CACHING)
	include(libpmemobj++)
	set(PKG_REQUIRES "${PKG_REQUIRES} libpmemobj++ >= ${LIBPMEMOBJ_CPP_REQUIRED_VERSION}")
endif()
if(ENGINE_VSMAP OR ENGINE_VCMAP)
	include(memkind)
	set(PKG_REQUIRES "${PKG_REQUIRES} memkind")
endif()
if(ENGINE_VCMAP OR ENGINE_CMAP)
	include(tbb)
	set(PKG_REQUIRES "${PKG_REQUIRES} tbb")
endif()
if(ENGINE_CACHING)
	include(memcached-experimental)
	include(redis-experimental)
	# XXX consider adding
	# set(PKG_REQUIRES "${PKG_REQUIRES} libmemcached")
endif()

add_executable(check_license EXCLUDE_FROM_ALL utils/check_license/check-license.c)

add_custom_target(checkers ALL)
add_custom_target(cppstyle)
add_custom_target(cppformat)
add_custom_target(check-whitespace)
add_custom_target(check-license
	COMMAND ${CMAKE_SOURCE_DIR}/utils/check_license/check-headers.sh
		${CMAKE_SOURCE_DIR}
		${CMAKE_BINARY_DIR}/check_license
		${CMAKE_SOURCE_DIR}/LICENSE
		-a)
add_dependencies(check-license check_license)

add_custom_target(check-whitespace-main
		COMMAND ${PERL_EXECUTABLE}
			${CMAKE_SOURCE_DIR}/utils/check_whitespace
			${CMAKE_SOURCE_DIR}/utils/check_license/*.sh
			${CMAKE_SOURCE_DIR}/*.md)

add_dependencies(check-whitespace check-whitespace-main)

if(DEVELOPER_MODE)
	find_program(CLANG_FORMAT NAMES clang-format clang-format-6.0)
	set(CLANG_FORMAT_REQUIRED "6.0")
	if(CLANG_FORMAT)
		get_program_version(${CLANG_FORMAT} CLANG_FORMAT_VERSION)
		if(NOT (CLANG_FORMAT_VERSION VERSION_EQUAL CLANG_FORMAT_REQUIRED))
			message(WARNING "required clang-format version is ${CLANG_FORMAT_REQUIRED} (version ${CLANG_FORMAT_VERSION} is installed)")
			unset(CLANG_FORMAT)
		endif()
	else()
		message(WARNING "clang-format not found - C++ sources will not be checked (needed version: ${CLANG_FORMAT_REQUIRED})")
	endif()

	if(NOT PERL_FOUND)
		message(FATAL_ERROR "Perl not found")
	endif()
	if (PERL_VERSION_STRING VERSION_LESS 5.16)
		message(FATAL_ERROR "Minimum required version of Perl is 5.16)")
	endif()

	execute_process(COMMAND ${PERL_EXECUTABLE} -MText::Diff -e ""
			ERROR_QUIET
			RESULT_VARIABLE PERL_TEXT_DIFF_STATUS)
	if (PERL_TEXT_DIFF_STATUS)
		message(FATAL_ERROR "Text::Diff Perl module not found (install libtext-diff-perl or perl-Text-Diff)")
	endif()

	add_dependencies(checkers cppstyle)
	add_dependencies(checkers check-whitespace)
	add_dependencies(checkers check-license)
endif()

if(PKG_CONFIG_FOUND)
	pkg_check_modules(VALGRIND QUIET valgrind)
else()
	find_package(VALGRIND QUIET)
endif()

if(VALGRIND_FOUND)
	find_pmemcheck()

	if(NOT VALGRIND_PMEMCHECK_FOUND)
		message(WARNING "Valgrind pmemcheck not found. Some tests will be skipped.")
	endif()
else()
	message(WARNING "Valgrind not found. Some tests will be skipped.")
endif()

add_library(pmemkv SHARED ${SOURCE_FILES})

target_link_libraries(pmemkv PRIVATE ${RapidJSON_LIBRARIES})
if(ENGINE_VSMAP OR ENGINE_VCMAP OR ENGINE_CMAP OR ENGINE_STREE OR ENGINE_TREE3)
	target_link_libraries(pmemkv PRIVATE ${LIBPMEMOBJ++_LIBRARIES})
endif()
if(ENGINE_VSMAP OR ENGINE_VCMAP)
	target_link_libraries(pmemkv PRIVATE ${MEMKIND_LIBRARIES})
endif()
if(ENGINE_VCMAP OR ENGINE_CMAP)
	target_link_libraries(pmemkv PRIVATE ${TBB_LIBRARIES})
endif()
if(ENGINE_CACHING)
	target_link_libraries(pmemkv PRIVATE ${CMAKE_THREAD_LIBS_INIT} memcached)
	target_link_libraries(pmemkv PRIVATE acl_cpp protocol acl)
endif()

target_include_directories(pmemkv PRIVATE src/valgrind)
# Enable libpmemobj-cpp valgrind annotations
target_compile_options(pmemkv PRIVATE -DLIBPMEMOBJ_CPP_VG_ENABLED=1)

enable_testing()
add_subdirectory(tests)

configure_file(libpmemkv.pc.in libpmemkv.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/libpmemkv.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

set_target_properties(pmemkv PROPERTIES PUBLIC_HEADER "src/libpmemkv.h;src/libpmemkv.hpp")

install(TARGETS pmemkv
		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

configure_file(
	"${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY)

add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

add_cppstyle(src ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/src/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines-experimental/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines-experimental/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines-experimental/stree/*.h*)

add_check_whitespace(src ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/src/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines-experimental/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines-experimental/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines-experimental/stree/*.h*)
