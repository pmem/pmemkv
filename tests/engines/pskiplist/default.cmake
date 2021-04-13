# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation

include(${PARENT_SRC_DIR}/helpers.cmake)
include(${PARENT_SRC_DIR}/engines/pmemobj_based/helpers.cmake)

setup()

if ((${TRACER} STREQUAL "drd") OR (${TRACER} STREQUAL "helgrind"))
    check_is_pmem(${DIR}/testfile)
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory /mnt/pmem0/${TEST_NAME})
execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory /mnt/pmem0/${TEST_NAME})
pmempool_execute(create -l ${LAYOUT} -s ${DB_SIZE} obj /mnt/pmem0/${TEST_NAME}/testfile)

make_config({"path":"/mnt/pmem0/${TEST_NAME}/testfile"})
execute(${TEST_EXECUTABLE} ${ENGINE} ${CONFIG} ${PARAMS})

finish()
