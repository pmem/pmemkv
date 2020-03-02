#
# Copyright 2017-2020, Intel Corporation
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

cmake_minimum_required(VERSION 3.10)
project(pmemkv)

include(cmake/helpers.cmake)
set_version(VERSION)

# set the default build type
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
	set(DEFAULT_BUILD_TYPE "Debug")
else()
	set(DEFAULT_BUILD_TYPE "RelWithDebInfo")
endif()

if(NOT CMAKE_BUILD_TYPE)
	message(STATUS "Setting build type to the default one (${DEFAULT_BUILD_TYPE})")
	set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}"
		CACHE STRING "Choose a type of build (Debug, Release or RelWithDebInfo)" FORCE)
endif()

option(BUILD_DOC "build documentation" ON)
option(BUILD_EXAMPLES "build examples" ON)
option(BUILD_TESTS "build tests" ON)
option(BUILD_JSON_CONFIG "build the 'libpmemkv_json_config' library" ON)

option(TESTS_USE_FORCED_PMEM "run tests with PMEM_IS_PMEM_FORCE=1" OFF)
option(TESTS_USE_VALGRIND "enable tests with valgrind (if found)" ON)

option(COVERAGE "run coverage test" OFF)
if(COVERAGE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -coverage")
endif()

# Each engine can be enabled separately.
# By default all experimental engines are turned off.
option(ENGINE_CMAP "enable cmap engine" ON)
option(ENGINE_VCMAP "enable vcmap engine" ON)
option(ENGINE_VSMAP "enable vsmap engine" ON)
option(ENGINE_CACHING "enable experimental caching engine" OFF)
option(ENGINE_STREE "enable experimental stree engine" OFF)
option(ENGINE_TREE3 "enable experimental tree3 engine" OFF)

option(DEVELOPER_MODE "enable developer's checks" OFF)
option(CHECK_CPP_STYLE "check code style of C++ sources" OFF)

# Do not treat include directories from the interfaces
# of consumed Imported Targets as SYSTEM by default.
set(CMAKE_NO_SYSTEM_FROM_IMPORTED 1)

if(ENGINE_CMAP)
	add_definitions(-DENGINE_CMAP)
	message(STATUS "CMAP engine is ON")
else()
	message(STATUS "CMAP engine is OFF")
endif()
if(ENGINE_VCMAP)
	add_definitions(-DENGINE_VCMAP)
	message(STATUS "VCMAP engine is ON")
else()
	message(STATUS "VCMAP engine is OFF")
endif()
if(ENGINE_VSMAP)
	add_definitions(-DENGINE_VSMAP)
	message(STATUS "VSMAP engine is ON")
else()
	message(STATUS "VSMAP engine is OFF")
endif()
if(ENGINE_CACHING)
	add_definitions(-DENGINE_CACHING)
	message(STATUS "CACHING engine is ON")
else()
	message(STATUS "CACHING engine is OFF")
endif()
if(ENGINE_STREE)
	add_definitions(-DENGINE_STREE)
	message(STATUS "STREE engine is ON")
else()
	message(STATUS "STREE engine is OFF")
endif()
if(ENGINE_TREE3)
	add_definitions(-DENGINE_TREE3)
	message(STATUS "TREE3 engine is ON")
else()
	message(STATUS "TREE3 engine is OFF")
endif()

set(SOURCE_FILES
	src/libpmemkv.cc
	src/libpmemkv.h
	src/engine.cc
	src/engines/blackhole.cc
	src/engines/blackhole.h
	src/out.cc
	src/out.h
)
# Add each engine source separately
if(ENGINE_CMAP)
	list(APPEND SOURCE_FILES
		src/engines/cmap.h
		src/engines/cmap.cc
	)
endif()
if(ENGINE_VCMAP)
	list(APPEND SOURCE_FILES
		src/engines/vcmap.h
		src/engines/vcmap.cc
	)
endif()
if(ENGINE_VSMAP)
	list(APPEND SOURCE_FILES
		src/engines/vsmap.h
		src/engines/vsmap.cc
	)
endif()
if(ENGINE_CACHING)
	list(APPEND SOURCE_FILES
		src/engines-experimental/caching.h
		src/engines-experimental/caching.cc
	)
endif()
if(ENGINE_STREE)
	list(APPEND SOURCE_FILES
		src/engines-experimental/stree.h
		src/engines-experimental/stree.cc
		src/engines-experimental/stree/persistent_b_tree.h
		src/engines-experimental/stree/pstring.h
	)
endif()
if(ENGINE_TREE3)
	list(APPEND SOURCE_FILES
		src/engines-experimental/tree3.h
		src/engines-experimental/tree3.cc
	)
endif()


set(CXX_STANDARD 11 CACHE STRING "C++ language standard")

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD ${CXX_STANDARD})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
set(LIBPMEMOBJ_CPP_REQUIRED_VERSION 1.9)
set(LIBPMEMOBJ_REQUIRED_VERSION 1.8)
set(MEMKIND_REQUIRED_VERSION 1.8.0)

if(DEVELOPER_MODE)
	add_common_flag(-Werror)
endif()

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

find_package(PkgConfig QUIET)
include(FindPerl)
include(ExternalProject)
include(FindThreads)
include(CheckCXXSourceCompiles)
include(GNUInstallDirs)

set(PKG_CONFIG_REQUIRES)
set(DEB_DEPENDS)
set(RPM_DEPENDS)

if(ENGINE_VSMAP OR ENGINE_VCMAP OR ENGINE_CMAP OR ENGINE_STREE OR ENGINE_TREE3)
	include(libpmemobj++)
	list(APPEND PKG_CONFIG_REQUIRES "libpmemobj++ >= ${LIBPMEMOBJ_CPP_REQUIRED_VERSION}")
	list(APPEND RPM_DEPENDS "libpmemobj >= ${LIBPMEMOBJ_REQUIRED_VERSION}")
	list(APPEND DEB_DEPENDS "libpmemobj1 (>= ${LIBPMEMOBJ_REQUIRED_VERSION}) | libpmemobj (>= ${LIBPMEMOBJ_REQUIRED_VERSION})")
endif()

if(ENGINE_VSMAP OR ENGINE_VCMAP)
	include(memkind)
	list(APPEND PKG_CONFIG_REQUIRES "memkind >= ${MEMKIND_REQUIRED_VERSION}")
	list(APPEND RPM_DEPENDS "memkind >= ${MEMKIND_REQUIRED_VERSION}")
	list(APPEND DEB_DEPENDS "libmemkind0 (>= ${MEMKIND_REQUIRED_VERSION})")
endif()

if(ENGINE_VCMAP)
	include(tbb)
	list(APPEND PKG_CONFIG_REQUIRES tbb)
	list(APPEND RPM_DEPENDS tbb)
	list(APPEND DEB_DEPENDS libtbb2)
endif()

if(ENGINE_CACHING)
	include(memcached-experimental)
	include(redis-experimental)
	list(APPEND PKG_CONFIG_REQUIRES libmemcached)
	list(APPEND RPM_DEPENDS libmemcached)
	list(APPEND DEB_DEPENDS libmemcached11)
endif()

string(REPLACE ";" " " PKG_CONFIG_REQUIRES "${PKG_CONFIG_REQUIRES}")
string(REPLACE ";" ", " RPM_PACKAGE_REQUIRES "${RPM_DEPENDS}")
string(REPLACE ";" ", " DEB_PACKAGE_REQUIRES "${DEB_DEPENDS}")

add_executable(check_license EXCLUDE_FROM_ALL utils/check_license/check-license.c)

add_custom_target(checkers ALL)
add_custom_target(cppstyle)
add_custom_target(cppformat)
add_custom_target(check-whitespace)
add_custom_target(check-license
	COMMAND ${CMAKE_SOURCE_DIR}/utils/check_license/check-headers.sh
		${CMAKE_SOURCE_DIR}
		${CMAKE_BINARY_DIR}/check_license
		${CMAKE_SOURCE_DIR}/LICENSE)
add_dependencies(check-license check_license)

add_custom_target(check-whitespace-main
		COMMAND ${PERL_EXECUTABLE}
			${CMAKE_SOURCE_DIR}/utils/check_whitespace
			${CMAKE_SOURCE_DIR}/utils/check_license/*.sh
			${CMAKE_SOURCE_DIR}/*.md)

add_dependencies(check-whitespace check-whitespace-main)

if(CHECK_CPP_STYLE)
	find_program(CLANG_FORMAT NAMES clang-format clang-format-9.0)
	set(CLANG_FORMAT_REQUIRED "9.0")
	if(CLANG_FORMAT)
		get_program_version_major_minor(${CLANG_FORMAT} CLANG_FORMAT_VERSION)
		if(NOT (CLANG_FORMAT_VERSION VERSION_EQUAL CLANG_FORMAT_REQUIRED))
			message(FATAL_ERROR "required clang-format version is ${CLANG_FORMAT_REQUIRED} (version ${CLANG_FORMAT_VERSION} is installed)")
		endif()
	else()
		message(WARNING "clang-format not found - C++ sources will not be checked (required version: ${CLANG_FORMAT_REQUIRED})")
	endif()
endif()

if(DEVELOPER_MODE)
	if(NOT PERL_FOUND)
		message(FATAL_ERROR "Perl not found")
	endif()
	if (PERL_VERSION_STRING VERSION_LESS 5.16)
		message(FATAL_ERROR "Minimum required version of Perl is 5.16)")
	endif()

	execute_process(COMMAND ${PERL_EXECUTABLE} -MText::Diff -e ""
			ERROR_QUIET
			RESULT_VARIABLE PERL_TEXT_DIFF_STATUS)
	if (PERL_TEXT_DIFF_STATUS)
		message(FATAL_ERROR "Text::Diff Perl module not found (install libtext-diff-perl or perl-Text-Diff)")
	endif()

	add_dependencies(checkers cppstyle)
	add_dependencies(checkers check-whitespace)
	add_dependencies(checkers check-license)
endif()

add_library(pmemkv SHARED ${SOURCE_FILES})
set_target_properties(pmemkv PROPERTIES SOVERSION 1)
target_link_libraries(pmemkv PRIVATE
	-Wl,--version-script=${CMAKE_SOURCE_DIR}/src/libpmemkv.map)

if(ENGINE_VSMAP OR ENGINE_VCMAP OR ENGINE_CMAP OR ENGINE_STREE OR ENGINE_TREE3)
	target_link_libraries(pmemkv PRIVATE ${LIBPMEMOBJ++_LIBRARIES})
endif()
if(ENGINE_VSMAP OR ENGINE_VCMAP)
	target_link_libraries(pmemkv PRIVATE ${MEMKIND_LIBRARIES})
endif()
if(ENGINE_VCMAP)
	target_link_libraries(pmemkv PRIVATE ${TBB_LIBRARIES})
endif()
if(ENGINE_CACHING)
	target_link_libraries(pmemkv PRIVATE ${CMAKE_THREAD_LIBS_INIT} memcached)
	target_link_libraries(pmemkv PRIVATE acl_cpp protocol acl)
endif()

target_include_directories(pmemkv PRIVATE src/valgrind)
# Enable libpmemobj-cpp valgrind annotations
target_compile_options(pmemkv PRIVATE -DLIBPMEMOBJ_CPP_VG_ENABLED=1)

set(TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/test
	CACHE STRING "working directory for tests")

if(BUILD_TESTS)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(VALGRIND QUIET valgrind)
	else()
		find_package(VALGRIND QUIET)
	endif()

	if(NOT VALGRIND_FOUND AND TESTS_USE_VALGRIND)
		message(FATAL_ERROR "Valgrind not found, but flag TESTS_USE_VALGRIND was set.")
	endif()

	if(VALGRIND_FOUND)
		find_pmemcheck()
	endif()

	if(PKG_CONFIG_FOUND)
		pkg_check_modules(LIBPMEMOBJ REQUIRED libpmemobj>=${LIBPMEMOBJ_REQUIRED_VERSION})
	else()
		find_package(LIBPMEMOBJ REQUIRED ${LIBPMEMOBJ_REQUIRED_VERSION})
	endif()

	enable_testing()
	add_subdirectory(tests)
endif()

configure_file(libpmemkv.pc.in libpmemkv.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/libpmemkv.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

set_target_properties(pmemkv PROPERTIES PUBLIC_HEADER "src/libpmemkv.h;src/libpmemkv.hpp")

if(BUILD_DOC)
	add_subdirectory(doc)
endif()

install(TARGETS pmemkv
		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(BUILD_JSON_CONFIG)
	include(rapidjson)
	add_definitions(-DBUILD_JSON_CONFIG)

	set(SOURCE_FILES_JSON_CONFIG
		src/libpmemkv_json_config.cc
		src/libpmemkv_json_config.h
		src/out.cc
		src/out.h
	)

	add_library(pmemkv_json_config SHARED ${SOURCE_FILES_JSON_CONFIG})
	set_target_properties(pmemkv_json_config  PROPERTIES SOVERSION 1)
	target_link_libraries(pmemkv_json_config PRIVATE pmemkv ${RapidJSON_LIBRARIES}
		-Wl,--version-script=${CMAKE_SOURCE_DIR}/src/libpmemkv_json_config.map)
	set_target_properties(pmemkv_json_config
		PROPERTIES PUBLIC_HEADER "src/libpmemkv_json_config.h")

	configure_file(libpmemkv_json_config.pc.in libpmemkv_json_config.pc @ONLY)

	install(FILES ${CMAKE_BINARY_DIR}/libpmemkv_json_config.pc
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

	install(TARGETS pmemkv_json_config
		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

configure_file(
	"${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY)

add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

add_cppstyle(src ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/src/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines-experimental/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines-experimental/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines-experimental/stree/*.h*)

add_check_whitespace(src ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/src/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines-experimental/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines-experimental/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/src/engines-experimental/stree/*.h*)

if(BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

if(NOT "${CPACK_GENERATOR}" STREQUAL "")
	include(${CMAKE_SOURCE_DIR}/cmake/packages.cmake)
endif()
