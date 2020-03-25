# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

add_common_flag(-Wno-unused-but-set-variable)

add_cppstyle(examples ${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_basic_c/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_basic_cpp/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_pmemobj_cpp/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_config_c/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_open_cpp/*.cpp)

add_check_whitespace(examples ${CMAKE_CURRENT_SOURCE_DIR}/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_basic_c/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_basic_cpp/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_pmemobj_cpp/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_config_c/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_open_cpp/*.*)

add_custom_target(examples)
add_dependencies(examples pmemkv)

function(add_example name)
	set(srcs ${ARGN})
	prepend(srcs ${CMAKE_CURRENT_SOURCE_DIR} ${srcs})
	add_executable(example-${name} ${srcs})
	add_dependencies(examples example-${name})
endfunction()

add_example(pmemkv_basic_cpp pmemkv_basic_cpp/pmemkv_basic.cpp)
target_link_libraries(example-pmemkv_basic_cpp pmemkv)

add_example(pmemkv_basic_c pmemkv_basic_c/pmemkv_basic.c)
target_link_libraries(example-pmemkv_basic_c pmemkv)

# Even though all examples require cmap engine to run, this one needs it
# (or rather libpmemobj++) to compile.
if(ENGINE_CMAP)
	add_example(pmemkv_pmemobj_cpp pmemkv_pmemobj_cpp/pmemkv_pmemobj.cpp)
	target_link_libraries(example-pmemkv_pmemobj_cpp pmemkv ${LIBPMEMOBJ++_LIBRARIES})
endif()

if(BUILD_JSON_CONFIG)
	add_example(pmemkv_config_c pmemkv_config_c/pmemkv_config.c)
	target_link_libraries(example-pmemkv_config_c pmemkv pmemkv_json_config)
endif()

add_example(pmemkv_open_cpp pmemkv_open_cpp/pmemkv_open.cpp)
target_link_libraries(example-pmemkv_open_cpp pmemkv)
