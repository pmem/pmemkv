cmake_minimum_required(VERSION 3.6)
project(pmemkv)

set(CMAKE_CXX_STANDARD 11)
set(SOURCE_FILES src/pmemkv.cc src/pmemkv.h)
set(3RDPARTY ${PROJECT_SOURCE_DIR}/3rdparty)
set(GTEST_VERSION 1.7.0)

find_package(PkgConfig QUIET)
include(ExternalProject)
include(FindThreads)

if(EXISTS ${CMAKE_SOURCE_DIR}/googletest-${GTEST_VERSION}.zip)
set(GTEST_URL ${CMAKE_SOURCE_DIR}/googletest-${GTEST_VERSION}.zip)
else()
set(GTEST_URL https://github.com/google/googletest/archive/release-${GTEST_VERSION}.zip)
endif()
ExternalProject_Add(
	gtest
	URL ${GTEST_URL}
	DOWNLOAD_NAME googletest-${GTEST_VERSION}.zip
	DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}
	PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
	INSTALL_COMMAND ""
	CMAKE_ARGS -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
)
ExternalProject_Get_Property(gtest source_dir binary_dir)
add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)
set_target_properties(libgtest PROPERTIES
	"IMPORTED_LOCATION" "${binary_dir}/libgtest.a"
	"IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
)
include_directories("${source_dir}/include")

if(PKG_CONFIG_FOUND)
	pkg_check_modules(PMEMOBJ++ REQUIRED libpmemobj++)
else()
	find_package(PMEMOBJ++ REQUIRED)
endif()

include_directories(${PMEMOBJ++_INCLUDE_DIRS})
link_directories(${PMEMOBJ++_LIBRARY_DIRS})

add_library(pmemkv ${SOURCE_FILES})
target_link_libraries(pmemkv ${PMEMOBJ++_LIBRARIES})

add_executable(pmemkv_example src/pmemkv_example.cc)
target_link_libraries(pmemkv_example pmemkv)

add_executable(pmemkv_stress src/pmemkv_stress.cc)
target_link_libraries(pmemkv_stress pmemkv)

add_executable(pmemkv_test src/pmemkv_test.cc src/mock_tx_alloc.cc)
target_link_libraries(pmemkv_test pmemkv libgtest ${CMAKE_DL_LIBS})
