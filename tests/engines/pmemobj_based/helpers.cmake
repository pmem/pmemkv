# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation

#
# helpers.cmake - helper functions for tests' (cmake) scripts (pmemobj_based engines only)
#

set(LAYOUT "pmemkv")
if (NOT ${ENGINE} STREQUAL "cmap") 
    string(CONCAT LAYOUT "pmemkv_" ${ENGINE})
endif()
