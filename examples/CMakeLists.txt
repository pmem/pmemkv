# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

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
