# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation

include(${PARENT_SRC_DIR}/helpers.cmake)

setup()

make_config({"path":"${DIR}","size":${DB_SIZE}})
execute(${TEST_EXECUTABLE} ${ENGINE} ${CONFIG} ${PARAMS})

finish()
