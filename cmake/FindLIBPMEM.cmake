# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

find_path(LIBPMEM_INCLUDE_DIR libpmem.h)
find_library(LIBPMEM_LIBRARY NAMES pmem libpmem)

set(LIBPMEM_LIBRARIES ${LIBPMEM_LIBRARY})
set(LIBPMEM_INCLUDE_DIRS ${LIBPMEM_INCLUDE_DIR})

set(MSG_NOT_FOUND "libpmem NOT found (set CMAKE_PREFIX_PATH to point the location)")
if(NOT (LIBPMEM_INCLUDE_DIR AND LIBPMEM_LIBRARY))
	if(LIBPMEM_FIND_REQUIRED)
		message(FATAL_ERROR ${MSG_NOT_FOUND})
	else()
		message(WARNING ${MSG_NOT_FOUND})
	endif()
endif()

mark_as_advanced(LIBPMEM_LIBRARY LIBPMEM_INCLUDE_DIR)
