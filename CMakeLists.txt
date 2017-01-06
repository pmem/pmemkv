cmake_minimum_required(VERSION 3.6)
project(pmemkv)

set(CMAKE_CXX_STANDARD 11)
include_directories(/usr/local/include/libpmemobj)
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/third-party/gtest-1.7.0/fused-src)

set(SOURCE_FILES
        pmemkv.cc
        pmemkv.h
        pmemkv_example.cc
        pmemkv_stress.cc
        pmemkv_test.cc)

add_executable(pmemkv ${SOURCE_FILES})
