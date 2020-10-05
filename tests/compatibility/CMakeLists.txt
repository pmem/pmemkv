# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

project(pmemkv_compatibility_test)

cmake_minimum_required(VERSION 3.13)

include(../ctest_helpers.cmake)

find_package(PkgConfig REQUIRED)

enable_testing()

pkg_check_modules(LIBPMEMKV REQUIRED libpmemkv)

function(build_binary name)
	add_executable(${name} ${ARGN})

	target_include_directories(${name} PRIVATE ${LIBPMEMKV_INCLUDE_DIRS})
	target_link_directories(${name} PRIVATE ${LIBPMEMKV_LIBRARY_DIRS})
	target_link_libraries(${name} pmemkv)
endfunction()

build_binary(cmap_compatibility cmap.cc)
