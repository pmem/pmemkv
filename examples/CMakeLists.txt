# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

add_common_flag(-Wno-unused-but-set-variable)

add_cppstyle(examples ${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_basic_c/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_basic_cpp/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_pmemobj_cpp/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_config_c/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_open_cpp/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_comparator_cpp/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_comparator_c/*.c)

add_check_whitespace(examples ${CMAKE_CURRENT_SOURCE_DIR}/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_basic_c/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_basic_cpp/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_pmemobj_cpp/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_config_c/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_open_cpp/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_comparator_cpp/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/pmemkv_comparator_c/*.c)

if(NOT ENGINE_CMAP)
	message(WARNING
		"Examples use cmap engine, which is disabled, hence they are "
		"not built. If you want to build them use -DENGINE_CMAP=ON option.")
	return()
endif()

add_custom_target(examples)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
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

add_example(pmemkv_pmemobj_cpp pmemkv_pmemobj_cpp/pmemkv_pmemobj.cpp)
target_link_libraries(example-pmemkv_pmemobj_cpp pmemkv ${LIBPMEMOBJ++_LIBRARIES})

if(BUILD_JSON_CONFIG)
	add_example(pmemkv_config_c pmemkv_config_c/pmemkv_config.c)
	target_link_libraries(example-pmemkv_config_c pmemkv pmemkv_json_config)
endif()

add_example(pmemkv_open_cpp pmemkv_open_cpp/pmemkv_open.cpp)
target_link_libraries(example-pmemkv_open_cpp pmemkv)

if (ENGINE_CSMAP)
	add_example(pmemkv_comparator_cpp pmemkv_comparator_cpp/pmemkv_comparator.cpp)
	target_link_libraries(example-pmemkv_comparator_cpp pmemkv)

	add_example(pmemkv_comparator_c pmemkv_comparator_c/pmemkv_comparator.c)
	target_link_libraries(example-pmemkv_comparator_c pmemkv)
endif()
