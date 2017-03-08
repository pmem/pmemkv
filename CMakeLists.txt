cmake_minimum_required(VERSION 3.6)
project(pmemkv)

set(CMAKE_CXX_STANDARD 11)
set(SOURCE_FILES src/pmemkv.cc src/pmemkv.h)
set(3RDPARTY ${PROJECT_SOURCE_DIR}/3rdparty)
include_directories(${3RDPARTY}/gtest)
include_directories(${3RDPARTY}/gtest/include)
include_directories(${3RDPARTY}/nvml/include)

add_library(pmemkv ${SOURCE_FILES})
target_link_libraries(pmemkv ${3RDPARTY}/nvml/lib/libpmem.so ${3RDPARTY}/nvml/lib/libpmemobj.so)

add_executable(pmemkv_example src/pmemkv_example.cc)
target_link_libraries(pmemkv_example pmemkv)

add_executable(pmemkv_stress src/pmemkv_stress.cc)
target_link_libraries(pmemkv_stress pmemkv)

add_executable(pmemkv_test src/pmemkv_test.cc 3rdparty/gtest/src/gtest-all.cc)
target_link_libraries(pmemkv_test pthread pmemkv)
