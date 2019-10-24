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

project(pmemkv_compatibility_test)

cmake_minimum_required(VERSION 3.10)

include(../ctest_helpers.cmake)

find_package(PkgConfig REQUIRED)

enable_testing()

set(PMEMKV_PATHS "" CACHE STRING "List of pmemkv libraries to be tested (semicolon separated list of paths).")

# Compile with first stable release
pkg_check_modules(LIBPMEMKV_1.0 REQUIRED libpmemkv=1.0)
set(FIRST_RELEASE_LIB ${LIBPMEMKV_1.0_LIBRARY_DIRS})

# Do not set rpath in executable - we will use LD_LIBRARY_PATH to select pmemkv version
SET(CMAKE_SKIP_RPATH TRUE)

function(build_binary name)
	add_executable(${name} ${ARGN})

	target_include_directories(${name} PRIVATE ${LIBPMEMKV_1.0_INCLUDE_DIRS})
	target_link_directories(${name} PRIVATE ${LIBPMEMKV_1.0_LIBRARY_DIRS})
	target_link_libraries(${name} pmemkv)
endfunction()

function(add_compatibility_test_version name binary lib_create lib_open)
	set(EXTRA_CARGS
		-DLIB_CREATE=${lib_create}
		-DLIB_OPEN=${lib_open}
	)

	test(test_graceful ${name}_graceful ${binary} none ${EXTRA_CARGS})
	test(test_ungraceful ${name}_ungraceful ${binary} none ${EXTRA_CARGS})
endfunction()

function(add_compatibility_test name)
	# Test each version from PMEMKV_PATHS with pmemkv-1.0
	foreach(PATH ${PMEMKV_PATHS})
		get_filename_component(LIB_NAME ${PATH} NAME)
		set(LIB_DIR "${PATH}/lib64:${PATH}/lib")

		add_compatibility_test_version(${name}_forward_${LIB_NAME} ${name} ${FIRST_RELEASE_LIB} ${LIB_DIR})
		add_compatibility_test_version(${name}_backward_${LIB_NAME} ${name} ${LIB_DIR} ${FIRST_RELEASE_LIB})
	endforeach()
endfunction()

build_binary(cmap_compatibility cmap.cc)
add_compatibility_test(cmap_compatibility)
