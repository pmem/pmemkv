# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020-2021, Intel Corporation

include(${PARENT_SRC_DIR}/helpers.cmake)

setup()

execute(${TEST_EXECUTABLE} ${ENGINE} ${DIR}/nope/nope ${DIR} ${PARAMS})

finish()
