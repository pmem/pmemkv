# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020-2021, Intel Corporation

include(${PARENT_SRC_DIR}/helpers.cmake)
include(${PARENT_SRC_DIR}/engines/pmemobj_based/helpers.cmake)

setup()

if ((${TRACER} STREQUAL "drd") OR (${TRACER} STREQUAL "helgrind"))
    check_is_pmem(${DIR}/testfile)
endif()

set(PATH "${DIR}/testfile")
pmempool_execute(create -l ${LAYOUT} -s ${DB_SIZE} obj ${PATH})

execute(${TEST_EXECUTABLE} ${ENGINE} ${PATH} ${DB_SIZE} ${PARAMS})

finish()
