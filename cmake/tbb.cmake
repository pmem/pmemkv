# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2017-2021, Intel Corporation

message(STATUS "Checking for module 'tbb'")

# find_package without unsetting this var is not working correctly
unset(TBB_FOUND CACHE)

set(find_package_msg "TBB package was found without pkg-config. If you'll compile a standalone application and link with this libpmemkv, TBB may not be properly linked!")

# CMake param TBB_DIR is priortized. This is the best to use
# while developing/testing pmemkv with custom TBB installation.
if(TBB_DIR)
	message(STATUS "  CMake param TBB_DIR was set, look in there (${TBB_DIR})")
	find_package(TBB REQUIRED tbb NO_DEFAULT_PATH) # XXX or should it be quiet? right now it fails if TBB_DIR does not contain TBB
	set(TBB_LIBRARIES ${TBB_IMPORTED_TARGETS})

	message(STATUS "  Found in dir '${TBB_DIR}' (ver: ${TBB_VERSION})")
	message(WARNING "${find_package_msg}")
else()
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(TBB QUIET tbb)
		if(TBB_FOUND)
			message(STATUS "  Found in dir '${TBB_LIBDIR}' using pkg-config (ver: ${TBB_VERSION})")
			return()
		endif()
	endif()

	unset(TBB_FOUND CACHE)
	find_package(TBB REQUIRED tbb)
	set(TBB_LIBRARIES ${TBB_IMPORTED_TARGETS})

	message(STATUS "  Found in dir '${TBB_DIR}' (ver: ${TBB_VERSION})")
	message(WARNING "${find_package_msg}")
endif()
