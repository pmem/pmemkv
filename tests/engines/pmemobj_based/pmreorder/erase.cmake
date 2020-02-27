# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

if ((${TRACER} STREQUAL "drd") OR (${TRACER} STREQUAL "helgrind"))
    check_is_pmem(${DIR}/testfile)
endif()

pmempool_execute(create -l "pmemkv" -s ${DB_SIZE} obj ${DIR}/testfile)

make_config({"path":"${DIR}/testfile"})

execute(${TEST_EXECUTABLE} ${ENGINE} ${CONFIG} create ${PARAMS})
pmreorder_create_store_log(${DIR}/testfile ${TEST_EXECUTABLE} ${ENGINE} ${CONFIG} erase ${PARAMS})
pmreorder_execute(true ReorderAccumulative ${SRC_DIR}/pmreorder.conf ${TEST_EXECUTABLE} ${ENGINE} ${CONFIG} open ${PARAMS})

finish()
