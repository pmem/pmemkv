# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

pmempool_execute(create -l "pmemkv" -s 100M obj ${DIR}/testfile)
execute(${TEST_EXECUTABLE} ${ENGINE} ${DIR}/testfile)

finish()
