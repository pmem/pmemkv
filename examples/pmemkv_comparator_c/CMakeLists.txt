# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation

cmake_minimum_required(VERSION 3.3)
project(pmemkv_basic C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPMEMKV REQUIRED libpmemkv)

include_directories(${LIBPMEMKV_INCLUDE_DIRS})
link_directories(${LIBPMEMKV_LIBRARY_DIRS})
add_executable(pmemkv_comparator_c pmemkv_comparator.c)
target_link_libraries(pmemkv_comparator_c ${LIBPMEMKV_LIBRARIES})
