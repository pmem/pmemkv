# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020-2021, Intel Corporation

include(${PARENT_SRC_DIR}/helpers.cmake)

setup()

set(PATH "${DIR}")
execute(${TEST_EXECUTABLE} ${ENGINE} ${PATH} ${DB_SIZE} ${PARAMS})

finish()
