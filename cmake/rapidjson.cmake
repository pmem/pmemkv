# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2017-2021, Intel Corporation

message(STATUS "Checking for module 'RapidJSON'")

if(PKG_CONFIG_FOUND)
    pkg_check_modules(RapidJSON QUIET RapidJSON>=${RAPIDJSON_REQUIRED_VERSION})
endif()

if(NOT RapidJSON_FOUND)
    # find_package (run after pkg-config) without unsetting this var is not working correctly
    unset(RapidJSON_FOUND CACHE)

    find_package(RapidJSON ${RAPIDJSON_REQUIRED_VERSION} QUIET REQUIRED)

    set(RapidJSON_LIBRARIES ${RapidJSON_LIBRARY})
    set(RapidJSON_INCLUDE_DIRS ${RapidJSON_INCLUDE_DIR})
    message(STATUS "  Found in dir '${RapidJSON_DIR}' using CMake's find_package (ver: ${RapidJSON_VERSION})")
else()
    message(STATUS "  Found in dir '${RapidJSON_INCLUDEDIR}' using pkg-config (ver: ${RapidJSON_VERSION})")
endif()

include_directories(${RapidJSON_INCLUDE_DIRS})
link_directories(${RapidJSON_LIBRARY_DIRS})
