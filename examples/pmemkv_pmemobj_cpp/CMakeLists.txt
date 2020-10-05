# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

cmake_minimum_required(VERSION 3.3)
project(pmemkv_pmemobj CXX)

set(LIBPMEMOBJ_CPP_REQUIRED_VERSION 1.9)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(LIBPMEMKV REQUIRED libpmemkv)
	pkg_check_modules(LIBPMEMOBJ++ REQUIRED libpmemobj++>=${LIBPMEMOBJ_CPP_REQUIRED_VERSION})
else()
	message(FATAL_ERROR "pkg-config not found")
endif()

include_directories(${LIBPMEMKV_INCLUDE_DIRS})
link_directories(${LIBPMEMKV_LIBRARY_DIRS})
add_executable(pmemkv_pmemobj_cpp pmemkv_pmemobj.cpp)
target_link_libraries(pmemkv_pmemobj_cpp ${LIBPMEMKV_LIBRARIES} ${LIBPMEMOBJ++_LIBRARIES})
