# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2017-2020, Intel Corporation

if(PKG_CONFIG_FOUND)
	pkg_check_modules(MEMKIND memkind>=${MEMKIND_REQUIRED_VERSION})
endif()

if(NOT MEMKIND_FOUND)
	unset(MEMKIND_LIBRARY CACHE)
	unset(MEMKIND_INCLUDEDIR CACHE)

	# try old method
	include(FindPackageHandleStandardArgs)
	find_path(MEMKIND_INCLUDEDIR pmem_allocator.h)
	find_library(MEMKIND_LIBRARY NAMES memkind libmemkind)
	mark_as_advanced(MEMKIND_LIBRARY MEMKIND_INCLUDEDIR)
	find_package_handle_standard_args(MEMKIND DEFAULT_MSG MEMKIND_INCLUDEDIR MEMKIND_LIBRARY)

	if(MEMKIND_FOUND)
		set(MEMKIND_LIBRARIES ${MEMKIND_LIBRARY})
		set(MEMKIND_INCLUDE_DIRS ${MEMKIND_INCLUDEDIR})
		message(STATUS "Memkind library found the old way (w/o pkg-config)")
	else()
		message(FATAL_ERROR "Memkind library (>=${MEMKIND_REQUIRED_VERSION}) not found")
	endif()
endif()

link_directories(${MEMKIND_LIBRARY_DIRS})
include_directories(${MEMKIND_INCLUDE_DIRS})

# XXX To be removed when memkind updated to ver. >= 1.10
# Check if libmemkind namespace is available, if not
# the old namespace will be used for 'pmem::allocator'.
# ref: https://github.com/pmem/pmemkv/issues/429
set(SAVED_CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES})
set(CMAKE_REQUIRED_INCLUDES ${MEMKIND_INCLUDE_DIRS})
CHECK_CXX_SOURCE_COMPILES(
		"#include <stdexcept>
		#include \"pmem_allocator.h\"
		int main(void) {
		libmemkind::pmem::allocator<int> *alc = nullptr;
		(void)alc;
		}"
		LIBMEMKIND_NAMESPACE_PRESENT)
set(CMAKE_REQUIRED_INCLUDES ${SAVED_CMAKE_REQUIRED_INCLUDES})

if(LIBMEMKIND_NAMESPACE_PRESENT)
	add_definitions(-DUSE_LIBMEMKIND_NAMESPACE)
endif()
