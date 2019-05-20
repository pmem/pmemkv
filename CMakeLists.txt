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

# Each engine is switched on separately
# By default all experimental engines are turned off
option(ENABLE_VSMAP_ENGINE "enable installation and testing of VSMap engine" ON)
option(ENABLE_VCMAP_ENGINE "enable installation and testing of VCMap engine" ON)
option(ENABLE_CMAP_ENGINE "enable installation and testing of CMap engine" ON)
option(ENABLE_CACHING_ENGINE "enable installation and testing of experimental CachingEngine" OFF)
option(ENABLE_STREE_ENGINE "enable installation and testing of experimental STree engine" OFF)
option(ENABLE_TREE3_ENGINE "enable installation and testing of experimental Tree engine" OFF)

if(ENABLE_VSMAP_ENGINE)
    add_definitions(-DENABLE_VSMAP_ENGINE)
endif()
if(ENABLE_VCMAP_ENGINE)
    add_definitions(-DENABLE_VCMAP_ENGINE)
endif()
if(ENABLE_CMAP_ENGINE)
    add_definitions(-DENABLE_CMAP_ENGINE)
endif()
if(ENABLE_CACHING_ENGINE)
    add_definitions(-DENABLE_CACHING_ENGINE)
endif()
if(ENABLE_STREE_ENGINE)
    add_definitions(-DENABLE_STREE_ENGINE)
endif()
if(ENABLE_TREE3_ENGINE)
    add_definitions(-DENABLE_TREE3_ENGINE)
endif()

set(SOURCE_FILES
	src/libpmemkv.cc
	src/libpmemkv.h
	src/engines/blackhole.cc
	src/engines/blackhole.h
)
# Add each engine source separately
if(ENABLE_VSMAP_ENGINE)
	list(APPEND SOURCE_FILES
		src/engines/vsmap.h
		src/engines/vsmap.cc
	)
endif()
if(ENABLE_VCMAP_ENGINE)
	list(APPEND SOURCE_FILES
		src/engines/vcmap.h
		src/engines/vcmap.cc
	)
endif()
if(ENABLE_CMAP_ENGINE)
	list(APPEND SOURCE_FILES
		src/engines/cmap.h
		src/engines/cmap.cc
	)
endif()
if(ENABLE_CACHING_ENGINE)
	list(APPEND SOURCE_FILES
		src/engines-experimental/caching.h
		src/engines-experimental/caching.cc
	)
endif()
if(ENABLE_STREE_ENGINE)
	list(APPEND SOURCE_FILES
		src/engines-experimental/stree.h
		src/engines-experimental/stree.cc
		src/engines-experimental/stree/persistent_b_tree.h
		src/engines-experimental/stree/pstring.h
	)
endif()
if(ENABLE_TREE3_ENGINE)
	list(APPEND SOURCE_FILES
		src/engines-experimental/tree3.h
		src/engines-experimental/tree3.cc
	)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
find_package(PkgConfig QUIET)
include(ExternalProject)
include(FindThreads)
include(CheckCXXSourceCompiles)
include(GNUInstallDirs)

include(rapidjson)
if(ENABLE_VSMAP_ENGINE OR ENABLE_VCMAP_ENGINE OR ENABLE_CMAP_ENGINE OR ENABLE_STREE_ENGINE OR ENABLE_TREE3_ENGINE)
	include(libpmemobj++)
endif()
if(ENABLE_VSMAP_ENGINE OR ENABLE_VCMAP_ENGINE)
	include(memkind)
endif()
if(ENABLE_VCMAP_ENGINE OR ENABLE_CMAP_ENGINE)
	include(tbb)
endif()
if(ENABLE_CACHING_ENGINE)
    include(memcached-experimental)
    include(redis-experimental)
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
if(ENABLE_VSMAP_ENGINE OR ENABLE_VCMAP_ENGINE OR ENABLE_CMAP_ENGINE OR ENABLE_STREE_ENGINE OR ENABLE_TREE3_ENGINE)
       target_link_libraries(pmemkv PRIVATE ${LIBPMEMOBJ++_LIBRARIES})
endif()
if(ENABLE_VSMAP_ENGINE OR ENABLE_VCMAP_ENGINE)
	target_link_libraries(pmemkv PRIVATE ${MEMKIND_LIBRARIES})
endif()
if(ENABLE_VCMAP_ENGINE OR ENABLE_CMAP_ENGINE)
	target_link_libraries(pmemkv PRIVATE ${TBB_LIBRARIES})
endif()
if(ENABLE_CACHING_ENGINE)
	target_link_libraries(pmemkv PRIVATE ${CMAKE_THREAD_LIBS_INIT} memcached)
	target_link_libraries(pmemkv PRIVATE acl_cpp protocol acl)
endif()
target_link_libraries(pmemkv PRIVATE ${LIBPMEMOBJ++_LIBRARIES} ${RapidJSON_LIBRARIES})

target_include_directories(pmemkv PRIVATE src/valgrind)
# Enable libpmemobj-cpp valgrind annotations
target_compile_options(pmemkv PRIVATE -DLIBPMEMOBJ_CPP_VG_ENABLED=1)

enable_testing()
add_subdirectory(tests)

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
