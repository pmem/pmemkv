# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020-2021, Intel Corporation

cmake_minimum_required(VERSION 3.3)
project(pmemkv_iterator_cpp CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPMEMKV REQUIRED libpmemkv)

include_directories(${LIBPMEMKV_INCLUDE_DIRS})
link_directories(${LIBPMEMKV_LIBRARY_DIRS})
add_executable(pmemkv_iterator_cpp pmemkv_iterator.cpp)
target_link_libraries(pmemkv_iterator_cpp ${LIBPMEMKV_LIBRARIES} pthread)
