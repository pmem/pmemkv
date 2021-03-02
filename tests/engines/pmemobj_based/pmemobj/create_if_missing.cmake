# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020-2021, Intel Corporation

include(${PARENT_SRC_DIR}/helpers.cmake)

setup()

make_config({"path":"${DIR}/testfile","create_if_missing":1,"size":83886080}) #80MB
execute(${TEST_EXECUTABLE} ${ENGINE} ${CONFIG} ${PARAMS})

finish()
