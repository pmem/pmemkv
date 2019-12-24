#
# Copyright 2019-2020, Intel Corporation
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

add_common_flag(-Wno-unused-but-set-variable)

add_cppstyle(examples-pmemkv_basic_c ${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_basic_c/*.c)
add_check_whitespace(examples-pmemkv_basic_c ${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_basic_c/*.c)

add_cppstyle(examples-pmemkv_basic_cpp ${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_basic_cpp/*.cpp)
add_check_whitespace(examples-pmemkv_basic_cpp ${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_basic_cpp/*.cpp)

add_cppstyle(examples-pmemkv_pmemobj_cpp ${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_pmemobj_cpp/*.cpp)
add_check_whitespace(examples-pmemkv_pmemobj_cpp ${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_pmemobj_cpp/*.cpp)

add_cppstyle(examples-pmemkv_config_c ${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_config_c/*.c)
add_check_whitespace(examples-pmemkv_config_c ${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_config_c/*.c)

add_cppstyle(examples-pmemkv_open_cpp ${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_open_cpp/*.cpp)
add_check_whitespace(examples-pmemkv_open_cpp ${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_open_cpp/*.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

function(add_example name)
	set(srcs ${ARGN})
	prepend(srcs ${CMAKE_CURRENT_SOURCE_DIR} ${srcs})
	add_executable(example-${name} ${srcs})
endfunction()

add_example(pmemkv_basic_cpp pmemkv_basic_cpp/pmemkv_basic.cpp)
target_link_libraries(example-pmemkv_basic_cpp pmemkv)

add_example(pmemkv_basic_c pmemkv_basic_c/pmemkv_basic.c)
target_link_libraries(example-pmemkv_basic_c pmemkv)

if(ENGINE_CMAP)
	add_example(pmemkv_pmemobj_cpp pmemkv_pmemobj_cpp/pmemkv_pmemobj.cpp)
	target_link_libraries(example-pmemkv_pmemobj_cpp pmemkv ${LIBPMEMOBJ++_LIBRARIES})
endif()

if(BUILD_JSON_CONFIG)
	add_example(pmemkv_config_c pmemkv_config_c/pmemkv_config.c)
	target_link_libraries(example-pmemkv_config_c pmemkv pmemkv_json_config)
endif()

if(ENGINE_CMAP)
	add_example(pmemkv_open_cpp pmemkv_open_cpp/pmemkv_open.cpp)
	target_link_libraries(example-pmemkv_open_cpp pmemkv)
endif()
