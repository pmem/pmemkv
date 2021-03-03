# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2021, Intel Corporation

include(${PARENT_SRC_DIR}/helpers.cmake)

setup()

make_config({})
execute(${TEST_EXECUTABLE} ${ENGINE} ${CONFIG} ${PARAMS})

finish()
