# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

execute(${TEST_EXECUTABLE} ${DIR}/testfile)

finish()
