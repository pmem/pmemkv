cmake_minimum_required(VERSION 3.6)
project(pmemkv)

set(CMAKE_CXX_STANDARD 11)
include_directories(/usr/local/include/libpmemobj)
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/third-party/gtest-1.7.0/fused-src)

set(SOURCE_FILES pmemkv.cc pmemkv.h)

add_executable(pmemkv_example ${SOURCE_FILES} pmemkv_example.cc)
target_link_libraries(pmemkv_example LINK_PUBLIC pmemobj)

add_executable(pmemkv_stress ${SOURCE_FILES} pmemkv_stress.cc)
target_link_libraries(pmemkv_stress LINK_PUBLIC pmemobj)

add_executable(pmemkv_test ${SOURCE_FILES} pmemkv_test.cc third-party/gtest-1.7.0/fused-src/gtest/gtest-all.cc)
target_link_libraries(pmemkv_test LINK_PUBLIC gflags pmemobj pthread)
