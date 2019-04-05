# Copyright 2017-2019, Intel Corporation
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
    pkg_check_modules(MEMKIND memkind)
endif()

if(NOT MEMKIND_FOUND)
    # try old method
    include(FindPackageHandleStandardArgs)
    find_path(MEMKIND_INCLUDE_DIR pmem_allocator.h)
    find_library(MEMKIND_LIBRARY NAMES memkind libmemkind)
    mark_as_advanced(MEMKIND_LIBRARY MEMKIND_INCLUDE_DIR)
    find_package_handle_standard_args(MEMKIND DEFAULT_MSG MEMKIND_INCLUDE_DIR MEMKIND_LIBRARY)

    if(MEMKIND_FOUND)
        set(MEMKIND_LIBRARIES ${MEMKIND_LIBRARY})
        set(MEMKIND_INCLUDE_DIRS ${MEMKIND_INCLUDE_DIR})
    endif()
endif()

if(NOT MEMKIND_FOUND)
    message(FATAL_ERROR "Memkind library not found")
endif()

link_directories(${MEMKIND_LIBRARY_DIRS})
include_directories(${MEMKIND_INCLUDE_DIRS})
target_link_libraries(pmemkv ${MEMKIND_LIBRARIES})
