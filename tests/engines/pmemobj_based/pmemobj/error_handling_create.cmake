# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation

include(${PARENT_SRC_DIR}/helpers.cmake)

setup()

execute(${TEST_EXECUTABLE} ${ENGINE} ${DIR}/testfile ${DIR}/nope/nope)

finish()
