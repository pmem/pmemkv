# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

cmake_minimum_required(VERSION 3.3)
project(pmemkv_config C)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(LIBPMEMKV REQUIRED libpmemkv libpmemkv_json_config)
else()
	message(FATAL_ERROR "pkg-config not found")
endif()

include_directories(${LIBPMEMKV_INCLUDE_DIRS})
link_directories(${LIBPMEMKV_LIBRARY_DIRS})
add_executable(pmemkv_config_c pmemkv_config.c)
target_link_libraries(pmemkv_config_c ${LIBPMEMKV_LIBRARIES})
