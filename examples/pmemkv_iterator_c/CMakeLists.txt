# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation

cmake_minimum_required(VERSION 3.3)
project(pmemkv_iterator_c C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPMEMKV REQUIRED libpmemkv)

include_directories(${LIBPMEMKV_INCLUDE_DIRS})
link_directories(${LIBPMEMKV_LIBRARY_DIRS})
add_executable(pmemkv_iterator_c pmemkv_iterator.c)
target_link_libraries(pmemkv_iterator_c ${LIBPMEMKV_LIBRARIES})
