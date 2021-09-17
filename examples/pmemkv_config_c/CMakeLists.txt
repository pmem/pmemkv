# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2021, Intel Corporation

cmake_minimum_required(VERSION 3.3)
project(pmemkv_config C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPMEMKV REQUIRED libpmemkv libpmemkv_json_config)

include_directories(${LIBPMEMKV_INCLUDE_DIRS})
link_directories(${LIBPMEMKV_LIBRARY_DIRS})

add_executable(pmemkv_config_c pmemkv_config.c)
target_link_libraries(pmemkv_config_c ${LIBPMEMKV_LIBRARIES})

# Additional executable for basic example not using json helper library.
# It links directly with 'pmemkv' library (not what pkg-config found).
add_executable(pmemkv_basic_config_c pmemkv_basic_config.c)
target_link_libraries(pmemkv_basic_config_c pmemkv)
