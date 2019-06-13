#
# Copyright 2019, Intel Corporation
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

add_common_flag(-Wall)
add_common_flag(-Wpointer-arith)
add_common_flag(-Wsign-compare)
add_common_flag(-Wunreachable-code-return)
add_common_flag(-Wmissing-variable-declarations)
add_common_flag(-fno-common)
add_common_flag(-Wunused-macros)
add_common_flag(-Wsign-conversion)

add_common_flag(-ggdb DEBUG)
add_common_flag(-DDEBUG DEBUG)

add_common_flag("-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2" RELEASE)

include_directories(${LIBPMEMKV_INCLUDE_DIRS} . ..)
link_directories(${LIBPMEMKV_LIBRARY_DIRS})

function(add_example name)
	set(srcs ${ARGN})
	prepend(srcs ${CMAKE_CURRENT_SOURCE_DIR} ${srcs})
	add_executable(example-${name} ${srcs})
endfunction()

add_example(pmemkv_basic_cpp pmemkv_basic_cpp/pmemkv_basic.cpp)
target_link_libraries(example-pmemkv_basic_cpp pmemkv ${CMAKE_THREAD_LIBS_INIT})

add_example(pmemkv_basic_c pmemkv_basic_c/pmemkv_basic.c)
target_link_libraries(example-pmemkv_basic_c pmemkv ${CMAKE_THREAD_LIBS_INIT})
