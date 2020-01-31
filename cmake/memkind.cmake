# Copyright 2017-2020, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#
#     * Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
		"#include \"pmem_allocator.h\"
		int main(void) {
		libmemkind::pmem::allocator<int> *alc = nullptr;
		(void)alc;
		}"
		LIBMEMKIND_NAMESPACE_PRESENT)
set(CMAKE_REQUIRED_INCLUDES ${SAVED_CMAKE_REQUIRED_INCLUDES})

if(LIBMEMKIND_NAMESPACE_PRESENT)
	add_definitions(-DUSE_LIBMEMKIND_NAMESPACE)
endif()
