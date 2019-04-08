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

option(EXPERIMENTAL "use experimental features" OFF)
if(EXPERIMENTAL)
    add_definitions(-DEXPERIMENTAL)
endif(EXPERIMENTAL)

set(SOURCE_FILES src/pmemkv.cc src/pmemkv.h
    src/engines/blackhole.h src/engines/blackhole.cc
    src/engines/vsmap.h src/engines/vsmap.cc
    src/engines/vcmap.h src/engines/vcmap.cc
    src/engines/cmap.h src/engines/cmap.cc
    src/engines-experimental/tree3.h src/engines-experimental/tree3.cc
    src/engines-experimental/stree.h src/engines-experimental/stree.cc
    src/engines-experimental/stree/persistent_b_tree.h src/engines-experimental/stree/pstring.h
    src/engines-experimental/caching.h src/engines-experimental/caching.cc
)

set(TEST_FILES tests/pmemkv_test.cc tests/mock_tx_alloc.cc
    tests/engines/blackhole_test.cc
    tests/engines/vsmap_test.cc
    tests/engines/vcmap_test.cc
    tests/engines/cmap_test.cc
    tests/engines-experimental/tree_test.cc
    tests/engines-experimental/stree_test.cc
    tests/engines-experimental/caching_test.cc
)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
find_package(PkgConfig QUIET)
include(ExternalProject)
include(FindThreads)
include(CheckCXXSourceCompiles)

include(memkind)
include(pmemobj++)
include(rapidjson)
include(tbb)
include(gtest)
if(EXPERIMENTAL)
    include(memcached-experimental)
    include(redis-experimental)
endif(EXPERIMENTAL)

add_library(pmemkv SHARED ${SOURCE_FILES})
target_link_libraries(pmemkv ${PMEMOBJ++_LIBRARIES})
target_link_libraries(pmemkv ${MEMKIND_LIBRARIES})
target_link_libraries(pmemkv ${RapidJSON_LIBRARIES})
target_link_libraries(pmemkv ${TBB_IMPORTED_TARGETS})

if(EXPERIMENTAL)
    target_link_libraries(pmemkv ${CMAKE_THREAD_LIBS_INIT} memcached)
    target_link_libraries(pmemkv acl_cpp protocol acl)
endif(EXPERIMENTAL)

add_executable(pmemkv_test ${TEST_FILES})
target_link_libraries(pmemkv_test pmemkv libgtest ${CMAKE_DL_LIBS})

unset(EXPERIMENTAL CACHE)
