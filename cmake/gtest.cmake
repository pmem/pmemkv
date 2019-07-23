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

set(GTEST_VERSION 1.7.0)
set(GTEST_SHA256HASH b58cb7547a28b2c718d1e38aee18a3659c9e3ff52440297e965f5edffe34b6d0)

if (EXISTS ${CMAKE_SOURCE_DIR}/googletest-${GTEST_VERSION}.zip)
    set(GTEST_URL ${CMAKE_SOURCE_DIR}/googletest-${GTEST_VERSION}.zip)
else ()
    set(GTEST_URL https://github.com/google/googletest/archive/release-${GTEST_VERSION}.zip)
endif ()

ExternalProject_Add(
    gtest
    URL ${GTEST_URL}
    URL_HASH SHA256=${GTEST_SHA256HASH}
    DOWNLOAD_NAME googletest-${GTEST_VERSION}.zip
    DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
    INSTALL_COMMAND ""
    CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
)

ExternalProject_Get_Property(gtest source_dir binary_dir)
add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)
set_target_properties(libgtest PROPERTIES "IMPORTED_LOCATION" "${binary_dir}/libgtest.a"
                      "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")
include_directories(SYSTEM "${source_dir}/include")
