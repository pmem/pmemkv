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
option(ENGINE_VSMAP "enable VSMap engine" ON)
option(ENGINE_VCMAP "enable VCMap engine" ON)
option(ENGINE_CMAP "enable CMap engine" ON)
option(ENGINE_CACHING "enable experimental CachingEngine" OFF)
option(ENGINE_STREE "enable experimental STree engine" OFF)
option(ENGINE_TREE3 "enable experimental Tree engine" OFF)

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
include(ExternalProject)
include(FindThreads)
include(CheckCXXSourceCompiles)
include(GNUInstallDirs)

include(rapidjson)
set(PKG_REQUIRES "Requires.private: RapidJSON")

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

set(PKG_CONFIG_FILE libpmemkv.pc)
configure_file(
	libpmemkv.pc.in
	${PKG_CONFIG_FILE}
	@ONLY)
file(APPEND
	${CMAKE_CURRENT_BINARY_DIR}/${PKG_CONFIG_FILE}
	"${PKG_REQUIRES}\n")
install(FILES ${CMAKE_BINARY_DIR}/libpmemkv.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

set_target_properties(pmemkv PROPERTIES PUBLIC_HEADER src/libpmemkv.h)

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
