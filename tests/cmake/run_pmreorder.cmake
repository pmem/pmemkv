# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

execute(${TEST_EXECUTABLE} c ${DIR}/testfile)
pmreorder_create_store_log(${DIR}/testfile ${TEST_EXECUTABLE} i ${DIR}/testfile)
pmreorder_execute(true ReorderAccumulative ${SRC_DIR}/pmreorder.conf ${TEST_EXECUTABLE} o)

finish()
