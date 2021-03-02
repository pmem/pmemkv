# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020-2021, Intel Corporation

include(${PARENT_SRC_DIR}/helpers.cmake)
include(${PARENT_SRC_DIR}/engines/pmemobj_based/helpers.cmake)

setup()

if ((${TRACER} STREQUAL "drd") OR (${TRACER} STREQUAL "helgrind"))
    check_is_pmem(${DIR}/testfile)
endif()

set(TEST_PATH "${DIR}/testfile")
pmempool_execute(create -l ${LAYOUT} -s ${DB_SIZE} obj ${TEST_PATH})

execute(${TEST_EXECUTABLE} ${ENGINE} ${TEST_PATH} ${DB_SIZE} ${PARAMS})

finish()
