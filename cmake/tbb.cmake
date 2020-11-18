# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2017-2020, Intel Corporation

message(STATUS "Checking for module 'tbb'")

if(PKG_CONFIG_FOUND)
	pkg_check_modules(TBB QUIET tbb)
endif()

# Now, if needed, try to find it without pkg-config. 'find_package' way will work
# most likely just only for testing and development, not when linking with an actual app.
if(NOT TBB_FOUND)
	# find_package without unsetting this var is not working correctly
	unset(TBB_FOUND CACHE)

	find_package(TBB REQUIRED tbb)
	set(TBB_LIBRARIES ${TBB_IMPORTED_TARGETS})

	message(STATUS "  Found in: ${TBB_DIR} (ver: ${TBB_VERSION})")
	message(WARNING "TBB package found without pkg-config. If you'll compile a standalone "
		"application and link with this libpmemkv, TBB may not be properly linked!")
else()
	message(STATUS "  Found in: ${TBB_LIBDIR} using pkg-config (ver: ${TBB_VERSION})")
endif()
