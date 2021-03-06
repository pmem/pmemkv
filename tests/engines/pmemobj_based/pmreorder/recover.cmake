# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2021, Intel Corporation

include(${PARENT_SRC_DIR}/helpers.cmake)
include(${PARENT_SRC_DIR}/engines/pmemobj_based/helpers.cmake)

setup()

if ((${TRACER} STREQUAL "drd") OR (${TRACER} STREQUAL "helgrind"))
    check_is_pmem(${DIR}/testfile)
endif()

if (${ENGINE} STREQUAL "robinhood")
    set(ENV{PMEMKV_ROBINHOOD_SHARDS_NUMBER} 64)
endif()

pmempool_execute(create -l ${LAYOUT} -s ${DB_SIZE} obj ${DIR}/testfile)

make_config({"path":"${DIR}/testfile"})

pmreorder_create_store_log(${DIR}/testfile ${TEST_EXECUTABLE} ${ENGINE} ${CONFIG} create ${PARAMS})
pmreorder_execute(true ReorderPartial ${PARENT_SRC_DIR}/engines/pmemobj_based/pmreorder/pmreorder.conf ${TEST_EXECUTABLE} ${ENGINE} ${CONFIG} open ${PARAMS})

finish()
