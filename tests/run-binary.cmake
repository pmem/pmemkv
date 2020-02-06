# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019, Intel Corporation

include(${SRC_DIR}/helpers.cmake)

setup()

execute_process(COMMAND ${CMAKE_COMMAND} -E remove pool_${TEST_NAME})

execute(${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME} pool_${TEST_NAME})

cleanup()
