cmake_minimum_required(VERSION 3.6)
project(pmemkv)

set(CMAKE_CXX_STANDARD 11)
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/gtest)
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/gtest/include)
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/nvml/include)

set(SOURCE_FILES src/pmemkv.cc src/pmemkv.h)

add_executable(pmemkv_example ${SOURCE_FILES} src/pmemkv_example.cc)
target_link_libraries(pmemkv_example
        ${PROJECT_SOURCE_DIR}/3rdparty/nvml/lib/libpmem.so
        ${PROJECT_SOURCE_DIR}/3rdparty/nvml/lib/libpmemobj.so)

add_executable(pmemkv_stress ${SOURCE_FILES} src/pmemkv_stress.cc)
target_link_libraries(pmemkv_stress
        ${PROJECT_SOURCE_DIR}/3rdparty/nvml/lib/libpmem.so
        ${PROJECT_SOURCE_DIR}/3rdparty/nvml/lib/libpmemobj.so)

add_executable(pmemkv_test ${SOURCE_FILES} src/pmemkv_test.cc 3rdparty/gtest/src/gtest-all.cc)
target_link_libraries(pmemkv_test pthread
        ${PROJECT_SOURCE_DIR}/3rdparty/nvml/lib/libpmem.so
        ${PROJECT_SOURCE_DIR}/3rdparty/nvml/lib/libpmemobj.so)
