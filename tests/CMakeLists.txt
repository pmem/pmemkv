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

include(ctest_helpers.cmake)

add_cppstyle(tests ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
		${CMAKE_CURRENT_SOURCE_DIR}/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/common/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/common/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/compatibility/*.cc
		${CMAKE_CURRENT_SOURCE_DIR}/config/*.cc
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/all/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/concurrent/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/persistent/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/pmemobj/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/sorted/*.c*)

add_check_whitespace(tests ${CMAKE_CURRENT_SOURCE_DIR}/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/cmake/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/common/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/compatibility/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/config/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/all/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/concurrent/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/persistent/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/pmemobj/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/sorted/*.*)

add_custom_target(tests)

add_library(test_backtrace STATIC common/test_backtrace.c)
if(LIBUNWIND_FOUND)
	target_compile_definitions(test_backtrace PUBLIC USE_LIBUNWIND=1)
endif()

find_pmreorder()
find_pmempool()

if (COVERAGE AND VALGRIND_FOUND)
	message(STATUS "This is the Coverage build, skipping Valgrind tests")
endif()

function(ignore_unused_but_set_variable_cpp target)
	check_cxx_compiler_flag(-Wno-unused-but-set-variable wno_unused_but_set_variable_flag_cpp)
	if (wno_unused_but_set_variable_flag_cpp)
		target_compile_options(${target} PUBLIC -Wno-unused-but-set-variable)
	endif()
endfunction()

function(ignore_unused_but_set_variable_c target)
	check_c_compiler_flag(-Wno-unused-but-set-variable wno_unused_but_set_variable_flag_c)
	if (wno_unused_but_set_variable_flag_c)
		target_compile_options(${target} PUBLIC -Wno-unused-but-set-variable)
	endif()
endfunction()

function(build_example_pmemkv_basic_cpp)
	add_executable(ex_pmemkv_basic_cpp ../examples/pmemkv_basic_cpp/pmemkv_basic.cpp)
	target_include_directories(ex_pmemkv_basic_cpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
	target_link_libraries(ex_pmemkv_basic_cpp pmemkv)
	ignore_unused_but_set_variable_cpp(ex_pmemkv_basic_cpp)
endfunction()

function(build_example_pmemkv_basic_c)
	add_executable(ex_pmemkv_basic_c ../examples/pmemkv_basic_c/pmemkv_basic.c)
	target_include_directories(ex_pmemkv_basic_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
	target_link_libraries(ex_pmemkv_basic_c pmemkv)
	ignore_unused_but_set_variable_c(ex_pmemkv_basic_c)
endfunction()

function(build_example_pmemkv_pmemobj_cpp)
	if(NOT("${LIBPMEMOBJ++_LIBRARIES}" STREQUAL ""))
		add_executable(ex_pmemkv_pmemobj_cpp ../examples/pmemkv_pmemobj_cpp/pmemkv_pmemobj.cpp)
		target_include_directories(ex_pmemkv_pmemobj_cpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
		target_link_libraries(ex_pmemkv_pmemobj_cpp pmemkv ${LIBPMEMOBJ++_LIBRARIES})
		ignore_unused_but_set_variable_cpp(ex_pmemkv_pmemobj_cpp)
	endif()
endfunction()

function(build_wrong_engine_name_test)
	build_test(wrong_engine_name_test wrong_engine_name_test.cc)

	if(ENGINE_VSMAP)
		target_compile_definitions(wrong_engine_name_test PRIVATE -DENGINE_VSMAP)
	endif()
	if(ENGINE_VCMAP)
		target_compile_definitions(wrong_engine_name_test PRIVATE -DENGINE_VCMAP)
	endif()
	if(ENGINE_CMAP)
		target_compile_definitions(wrong_engine_name_test PRIVATE -DENGINE_CMAP)
	endif()
	if(ENGINE_CACHING)
		target_compile_definitions(wrong_engine_name_test PRIVATE -DENGINE_CACHING)
	endif()
	if(ENGINE_STREE)
		target_compile_definitions(wrong_engine_name_test PRIVATE -DENGINE_STREE)
	endif()
	if(ENGINE_TREE3)
		target_compile_definitions(wrong_engine_name_test PRIVATE -DENGINE_TREE3)
	endif()
endfunction()

if(ENGINE_CACHING)
	if(ENGINE_TREE3 AND BUILD_JSON_CONFIG)
		build_test(caching_test engines/experimental/caching_test.cc)
	elseif(NOT BUILD_JSON_CONFIG)
		message(WARNING
			"Caching tests require the 'libpmemkv_json_config' library, which is not built, "
			"hence they are disabled. If you want to run them use -DBUILD_JSON_CONFIG=ON option.")
	else()
		message(WARNING
			"Caching tests are set to work with TREE3 engine, which is disabled, hence "
			"they are also disabled. If you want to run them use -DENGINE_TREE3=ON option.")
	endif()
endif()
# XXX
# if(ENGINE_STREE)
# build_test(stree_test engines-experimental/stree_test.cc)
# endif()
# if(ENGINE_TREE3)
# build_test(tree3_test engines-experimental/tree3_test.cc)
# endif()

build_wrong_engine_name_test()
add_test_generic(NAME wrong_engine_name_test TRACERS none)

if(BUILD_EXAMPLES AND ENGINE_CMAP)
	build_example_pmemkv_basic_c()
	build_example_pmemkv_basic_cpp()
	build_example_pmemkv_pmemobj_cpp()

	add_test_generic(NAME ex_pmemkv_basic_c TRACERS none)
	add_test_generic(NAME ex_pmemkv_basic_cpp TRACERS none)
	add_test_generic(NAME ex_pmemkv_pmemobj_cpp TRACERS none)
elseif(BUILD_EXAMPLES AND NOT ENGINE_CMAP)
	message(WARNING
		"Examples use cmap engine, which is disabled, hence their execution "
		"is also disabled. If you want to run them use -DENGINE_CMAP=ON option.")
endif()

build_test(config_c config/config_c.c)
add_test_generic(NAME config_c TRACERS none memcheck)

build_test(config_cpp config/config_cpp.cc)
add_test_generic(NAME config_cpp TRACERS none memcheck)

build_test(blackhole_test engines/blackhole/blackhole_test.cc)
add_test_generic(NAME blackhole_test TRACERS none memcheck)

# Tests for all engines
build_test(put_get_remove engine_scenarios/all/put_get_remove.cc)
build_test(put_get_remove_params engine_scenarios/all/put_get_remove_params.cc)
build_test(put_get_std_map engine_scenarios/all/put_get_std_map.cc)
build_test(iterate engine_scenarios/all/iterate.cc)
build_test(error_handling engine_scenarios/all/error_handling.cc)

# Tests for concurrent engines
build_test(concurrent_put_get_remove engine_scenarios/concurrent/put_get_remove.cc)

# Tests for peristent engines
build_test(persistent_not_found_verify engine_scenarios/persistent/not_found_verify.cc)
build_test(persistent_overwrite_verify engine_scenarios/persistent/overwrite_verify.cc)
build_test(persistent_put_remove_verify engine_scenarios/persistent/put_remove_verify.cc)
build_test(persistent_put_verify_asc_params engine_scenarios/persistent/put_verify_asc_params.cc)
build_test(persistent_put_verify_desc_params engine_scenarios/persistent/put_verify_desc_params.cc)
build_test(persistent_put_verify engine_scenarios/persistent/put_verify.cc)

# Tests for sorted engines
build_test(sorted_iterate engine_scenarios/sorted/iterate.cc)

# Tests for pmemobj engines
build_test(pmemobj_error_handling_create engine_scenarios/pmemobj/error_handling_create.cc)
build_test(pmemobj_error_handling_tx_oid engine_scenarios/pmemobj/error_handling_tx_oid.cc)
build_test(pmemobj_error_handling_tx_path engine_scenarios/pmemobj/error_handling_tx_path.cc)
build_test(pmemobj_error_handling_defrag engine_scenarios/pmemobj/error_handling_defrag.cc)
build_test(pmemobj_put_get_std_map_defrag engine_scenarios/pmemobj/put_get_std_map_defrag.cc)
build_test(pmemobj_put_get_std_map_oid engine_scenarios/pmemobj/put_get_std_map_oid.cc)

# Tests for C API
build_test(c_api_null_db_config c_api/null_db_config.c)

add_engine_test(ENGINE cmap BINARY c_api_null_db_config
		TRACERS none memcheck SCRIPT c_api/null_db_config.cmake)
add_engine_test(ENGINE cmap BINARY put_get_remove
		TRACERS none memcheck pmemcheck SCRIPT all/put_get_remove.cmake)
# Not run under pmemcheck because of timeout
add_engine_test(ENGINE cmap BINARY put_get_remove_params
		TRACERS none memcheck SCRIPT all/put_get_remove_params.cmake)
add_engine_test(ENGINE cmap BINARY put_get_std_map
		TRACERS none memcheck pmemcheck SCRIPT all/put_get_std_map.cmake)
add_engine_test(ENGINE cmap BINARY error_handling
		TRACERS none memcheck SCRIPT all/error_handling.cmake)
add_engine_test(ENGINE cmap BINARY iterate
		TRACERS none memcheck SCRIPT all/iterate.cmake)

add_engine_test(ENGINE cmap BINARY concurrent_put_get_remove
		TRACERS none memcheck helgrind drd SCRIPT concurrent/put_get_remove.cmake)

add_engine_test(ENGINE cmap BINARY persistent_not_found_verify
		TRACERS none memcheck pmemcheck SCRIPT persistent/not_found_verify.cmake)
add_engine_test(ENGINE cmap BINARY persistent_overwrite_verify
		TRACERS none memcheck pmemcheck SCRIPT persistent/overwrite_verify.cmake)
add_engine_test(ENGINE cmap BINARY persistent_put_remove_verify
		TRACERS none memcheck pmemcheck SCRIPT persistent/put_remove_verify.cmake)
add_engine_test(ENGINE cmap BINARY persistent_put_verify_asc_params
		TRACERS none memcheck pmemcheck SCRIPT persistent/put_verify_asc_params.cmake)
add_engine_test(ENGINE cmap BINARY persistent_put_verify_desc_params
		TRACERS none memcheck pmemcheck SCRIPT persistent/put_verify_desc_params.cmake)
add_engine_test(ENGINE cmap BINARY persistent_put_verify
		TRACERS none memcheck pmemcheck SCRIPT persistent/put_verify.cmake)

add_engine_test(ENGINE cmap BINARY pmemobj_error_handling_create
		TRACERS none memcheck SCRIPT pmemobj/error_handling_create.cmake)
add_engine_test(ENGINE cmap BINARY pmemobj_error_handling_tx_oid
		TRACERS none memcheck SCRIPT pmemobj/error_handling_tx_oid.cmake)
add_engine_test(ENGINE cmap BINARY pmemobj_error_handling_tx_path
		TRACERS none memcheck SCRIPT pmemobj/error_handling_tx_path.cmake)
add_engine_test(ENGINE cmap BINARY pmemobj_error_handling_defrag
		TRACERS none memcheck SCRIPT pmemobj/error_handling_defrag.cmake)
add_engine_test(ENGINE cmap BINARY pmemobj_put_get_std_map_defrag
		TRACERS none memcheck pmemcheck SCRIPT pmemobj/put_get_std_map_defrag.cmake)
add_engine_test(ENGINE cmap BINARY pmemobj_put_get_std_map_oid
		TRACERS none memcheck pmemcheck SCRIPT pmemobj/put_get_std_map_oid.cmake)
add_engine_test(ENGINE cmap BINARY put_get_std_map
		TRACERS none memcheck pmemcheck SCRIPT pmemobj/put_get_std_map_force_create.cmake)

# add_engine_test(ENGINE vcmap BINARY c_api_null_db_config
# TRACERS none memcheck SCRIPT c_api/null_db_config.cmake)
# add_engine_test(ENGINE vcmap BINARY put_get_remove
# TRACERS none memcheck SCRIPT all/put_get_remove.cmake)
# add_engine_test(ENGINE vcmap BINARY put_get_remove_params
# TRACERS none memcheck SCRIPT all/put_get_remove_params.cmake)
# add_engine_test(ENGINE vcmap BINARY put_get_std_map
# TRACERS none memcheck SCRIPT all/put_get_std_map.cmake)
# add_engine_test(ENGINE vcmap BINARY error_handling
# TRACERS none memcheck SCRIPT all/error_handling.cmake)
# add_engine_test(ENGINE vcmap BINARY iterate
# TRACERS none memcheck SCRIPT all/iterate.cmake)
# add_engine_test(ENGINE vcmap BINARY concurrent_put_get_remove
# TRACERS none memcheck helgrind drd SCRIPT concurrent/put_get_remove.cmake)

# add_engine_test(ENGINE vsmap BINARY c_api_null_db_config
# TRACERS none memcheck SCRIPT c_api/null_db_config.cmake)
# add_engine_test(ENGINE vsmap BINARY put_get_remove
# TRACERS none memcheck SCRIPT all/put_get_remove.cmake)
# add_engine_test(ENGINE vsmap BINARY put_get_remove_params
# TRACERS none memcheck SCRIPT all/put_get_remove_params.cmake)
# add_engine_test(ENGINE vsmap BINARY put_get_std_map
# TRACERS none memcheck SCRIPT all/put_get_std_map.cmake)
# add_engine_test(ENGINE vsmap BINARY error_handling
# TRACERS none memcheck SCRIPT all/error_handling.cmake)
# add_engine_test(ENGINE vsmap BINARY iterate
# TRACERS none memcheck SCRIPT all/iterate.cmake)
# add_engine_test(ENGINE vsmap BINARY concurrent_put_get_remove
# TRACERS none memcheck helgrind drd SCRIPT concurrent/put_get_remove.cmake)
# add_engine_test(ENGINE vsmap BINARY sorted_iterate
# TRACERS none memcheck SCRIPT sorted/iterate.cmake)

if(BUILD_JSON_CONFIG)
	build_test(json_to_config config/json_to_config.cc)
	target_link_libraries(json_to_config pmemkv_json_config)

	add_test_generic(NAME json_to_config TRACERS none memcheck)
endif()
