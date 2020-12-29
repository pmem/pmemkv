# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

cmake_minimum_required(VERSION 3.3)
project(pmemkv_basic C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPMEMKV REQUIRED libpmemkv)

include_directories(${LIBPMEMKV_INCLUDE_DIRS})
link_directories(${LIBPMEMKV_LIBRARY_DIRS})
add_executable(pmemkv_basic_c pmemkv_basic.c)
target_link_libraries(pmemkv_basic_c ${LIBPMEMKV_LIBRARIES})
