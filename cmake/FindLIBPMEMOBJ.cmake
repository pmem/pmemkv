# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2020, Intel Corporation

find_path(LIBPMEMOBJ_INCLUDE_DIR libpmemobj.h)
find_library(LIBPMEMOBJ_LIBRARY NAMES pmemobj libpmemobj)

set(LIBPMEMOBJ_LIBRARIES ${LIBPMEMOBJ_LIBRARY})
set(LIBPMEMOBJ_INCLUDE_DIRS ${LIBPMEMOBJ_INCLUDE_DIR})

set(MSG_NOT_FOUND "libpmemobj NOT found (set CMAKE_PREFIX_PATH to point the location)")
if(NOT (LIBPMEMOBJ_INCLUDE_DIR AND LIBPMEMOBJ_LIBRARY))
	if(LIBPMEMOBJ_FIND_REQUIRED)
		message(FATAL_ERROR ${MSG_NOT_FOUND})
	else()
		message(WARNING ${MSG_NOT_FOUND})
	endif()
endif()

mark_as_advanced(LIBPMEMOBJ_LIBRARY LIBPMEMOBJ_INCLUDE_DIR)
