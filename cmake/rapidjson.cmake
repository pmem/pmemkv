# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2017-2019, Intel Corporation

if(PKG_CONFIG_FOUND)
    pkg_check_modules(RapidJSON RapidJSON)
endif()

if(NOT RapidJSON_FOUND)
    # try old method

    # find_package without unsetting this var is not working correctly
    unset(RapidJSON_FOUND CACHE)
    find_package(RapidJSON REQUIRED)

    if(RapidJSON_FOUND)
        set(RapidJSON_LIBRARIES ${RapidJSON_LIBRARY})
        set(RapidJSON_INCLUDE_DIRS ${RapidJSON_INCLUDE_DIR})
        message(STATUS "RapidJSON library found the old way (w/o pkg-config)")
    endif()
endif()

if(NOT RapidJSON_FOUND)
    message(FATAL_ERROR "RapidJSON library not found")
endif()

include_directories(${RapidJSON_INCLUDE_DIRS})
link_directories(${RapidJSON_LIBRARY_DIRS})
