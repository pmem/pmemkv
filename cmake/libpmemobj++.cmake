# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2017-2019, Intel Corporation

if(PKG_CONFIG_FOUND)
	pkg_check_modules(LIBPMEMOBJ++ REQUIRED libpmemobj++>=${LIBPMEMOBJ_CPP_REQUIRED_VERSION})
else()
	find_package(LIBPMEMOBJ++ ${LIBPMEMOBJ_CPP_REQUIRED_VERSION} REQUIRED libpmemobj++)
	message(STATUS "libpmemobj++ found the old way (w/o pkg-config)")
endif()

include_directories(${LIBPMEMOBJ++_INCLUDE_DIRS})
link_directories(${LIBPMEMOBJ++_LIBRARY_DIRS})
