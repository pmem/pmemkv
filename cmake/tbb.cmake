# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2017-2019, Intel Corporation

if(PKG_CONFIG_FOUND)
	pkg_check_modules(TBB tbb)
endif()

if(NOT TBB_FOUND)
	# find_package without unsetting this var is not working correctly
	unset(TBB_FOUND CACHE)
	find_package(TBB COMPONENTS tbb)

	if(TBB_FOUND)
		message(STATUS "TBB package found without pkg-config")
	endif()

	set(TBB_LIBRARIES ${TBB_IMPORTED_TARGETS})
endif()

if(NOT TBB_FOUND)
	message(FATAL_ERROR "TBB not found. Please set TBB_DIR CMake variable if TBB \
is installed in a non-standard directory, like: -DTBB_DIR=<path_to_tbb_cmake_dir>")
endif()
