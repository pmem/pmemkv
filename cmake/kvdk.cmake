# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2017-2021, Intel Corporation

message(STATUS "Checking for module 'kvdk'")

if(PKG_CONFIG_FOUND)
    pkg_check_modules(kvdk kvdk)
endif()

include_directories(${kvdk_INCLUDE_DIRS})
link_directories(${kvdk_LIBRARY_DIRS})
message(INFO ${kvdk_LIBRARY_DIRS})
