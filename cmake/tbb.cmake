# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2017-2021, Intel Corporation

set(TBB_DIR "" CACHE PATH "dir with TBBConfig.cmake, to be set if using custom TBB installation")
message(STATUS "Checking for module 'tbb'")

# TBB was found without pkg-config, warning must be printed
# and global variable with tbb libs has to be set.
function(handle_tbb_found_no_pkgconf)
	set(TBB_LIBRARIES ${TBB_IMPORTED_TARGETS} PARENT_SCOPE)

	message(STATUS "  Found in dir '${TBB_DIR}' using CMake's find_package (ver: ${TBB_VERSION})")
	message(WARNING "TBB package was found without pkg-config. If you'll compile a "
		"standalone application and link with this libpmemkv, TBB may not be properly linked!")
endfunction()

# CMake param TBB_DIR is priortized. This is the best to use
# while developing/testing pmemkv with custom TBB installation.
if(TBB_DIR)
	message(STATUS "  CMake param TBB_DIR is set, look ONLY in there (${TBB_DIR})!")
	find_package(TBB QUIET NAMES TBB tbb NO_DEFAULT_PATH)
	if(TBB_FOUND)
		handle_tbb_found_no_pkgconf()
	else()
		message(FATAL_ERROR "TBB_DIR is set, but does not contain a path to TBB. "
			"Either set this var to a dir with TBBConfig.cmake (or tbb-config.cmake), "
			"or unset it and let CMake find TBB in the system (using e.g. pkg-config).")
	endif()
else()
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(TBB QUIET tbb)
		if(TBB_FOUND)
			message(STATUS "  Found in dir '${TBB_LIBDIR}' using pkg-config (ver: ${TBB_VERSION})")
			return()
		endif()
	endif()

	# find_package (run after pkg-config) without unsetting this var is not working correctly
	unset(TBB_FOUND CACHE)

	find_package(TBB REQUIRED tbb)
	handle_tbb_found_no_pkgconf()
endif()
