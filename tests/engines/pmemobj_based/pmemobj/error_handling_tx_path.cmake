# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)
include(${SRC_DIR}/../engines/pmemobj_based/helpers.cmake)

setup()

pmempool_execute(create -l ${LAYOUT} -s 100M obj ${DIR}/testfile)
pmempool_execute(create -l ${LAYOUT} -s 100M obj ${DIR}/testfile2)

make_config({"path":"${DIR}/testfile"})
execute(${TEST_EXECUTABLE} ${ENGINE} ${CONFIG} ${DIR}/testfile2)

finish()
