# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019, Intel Corporation

include(${SRC_DIR}/helpers.cmake)

setup()

# create and initialize the pool
execute(${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME})

# create the store log while doing inserts
pmreorder_create_store_log(
	${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME})

# execute pmreorder: open the pool and check consistency
pmreorder_execute(ReorderAccumulative ${SRC_DIR}/pmreorder.conf
	${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME})

cleanup()
