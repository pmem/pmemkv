# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

include(ctest_helpers.cmake)

## Setup
add_custom_target(tests)

# Checks for when DEVELOPER_MODE is ON
add_cppstyle(tests ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
		${CMAKE_CURRENT_SOURCE_DIR}/c_api/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/common/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/common/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/compatibility/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/config/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/all/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/all/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/concurrent/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/memkind/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/persistent/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/pmemobj/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/pmemobj/*.h*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/sorted/*.c*
		${CMAKE_CURRENT_SOURCE_DIR}/engines-experimental/*.c*)

add_check_whitespace(tests ${CMAKE_CURRENT_SOURCE_DIR}/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/c_api/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/cmake/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/common/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/compatibility/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/config/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/all/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/concurrent/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/memkind/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/persistent/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/pmemobj/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engine_scenarios/sorted/*.*
		${CMAKE_CURRENT_SOURCE_DIR}/engines-experimental/*.*)

if(TESTS_JSON AND NOT BUILD_JSON_CONFIG)
	message(FATAL_ERROR "Tests requiring 'libpmemkv_json_config' library "
		"are enabled (with TESTS_JSON option), but the library is not built. "
		"If you want to run them use BUILD_JSON_CONFIG=ON option.")
elseif(NOT TESTS_JSON)
	message(WARNING
		"Most of pmemkv's tests require the 'libpmemkv_json_config' library. "
		"To enable these tests use TESTS_JSON option (and turn BUILD_JSON_CONFIG on.")
endif()

add_library(test_backtrace STATIC common/test_backtrace.c)
if(LIBUNWIND_FOUND)
	target_compile_definitions(test_backtrace PUBLIC USE_LIBUNWIND=1)
endif()

find_pmreorder()
find_pmempool()

add_executable(check_is_pmem common/check_is_pmem.cpp)
target_link_libraries(check_is_pmem ${LIBPMEM_LIBRARIES})

if(COVERAGE AND VALGRIND_FOUND)
	message(STATUS "This is the Coverage build, skipping Valgrind tests")
endif()


## Common tests
build_test(wrong_engine_name_test wrong_engine_name_test.cc)
add_test_generic(NAME wrong_engine_name_test TRACERS none)

if(BUILD_EXAMPLES AND ENGINE_CMAP)
	add_dependencies(tests example-pmemkv_basic_c
		example-pmemkv_basic_cpp
		example-pmemkv_pmemobj_cpp)
	add_test_generic(NAME example-pmemkv_basic_c TRACERS none)
	add_test_generic(NAME example-pmemkv_basic_cpp TRACERS none)
	add_test_generic(NAME example-pmemkv_pmemobj_cpp TRACERS none)

	if(BUILD_JSON_CONFIG)
		add_dependencies(tests example-pmemkv_config_c)
		add_test_generic(NAME example-pmemkv_config_c TRACERS none)
	endif()
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

build_test_ext(NAME json_to_config SRC_FILES config/json_to_config.cc LIBS json)
add_test_generic(NAME json_to_config TRACERS none memcheck)

## Test scenarios (parametrized at least with engine name)
# Tests for all engines
build_test_ext(NAME put_get_remove SRC_FILES engine_scenarios/all/put_get_remove.cc LIBS json)
build_test_ext(NAME put_get_remove_long_key SRC_FILES engine_scenarios/all/put_get_remove_long_key.cc LIBS json)
build_test_ext(NAME put_get_remove_params SRC_FILES engine_scenarios/all/put_get_remove_params.cc LIBS json)
build_test_ext(NAME put_get_std_map SRC_FILES engine_scenarios/all/put_get_std_map.cc LIBS json)
build_test_ext(NAME iterate SRC_FILES engine_scenarios/all/iterate.cc LIBS json)
build_test_ext(NAME error_handling_oom SRC_FILES engine_scenarios/all/error_handling_oom.cc LIBS json)

# Tests for concurrent engines
build_test_ext(NAME concurrent_put_get_remove_params SRC_FILES engine_scenarios/concurrent/put_get_remove_params.cc LIBS json)
build_test_ext(NAME concurrent_put_get_remove_gen_params SRC_FILES engine_scenarios/concurrent/put_get_remove_gen_params.cc LIBS json)
build_test_ext(NAME concurrent_put_get_remove_single_op_params SRC_FILES engine_scenarios/concurrent/put_get_remove_single_op_params.cc LIBS json)

# Tests for peristent engines
build_test_ext(NAME persistent_not_found_verify SRC_FILES engine_scenarios/persistent/not_found_verify.cc LIBS json)
build_test_ext(NAME persistent_overwrite_verify SRC_FILES engine_scenarios/persistent/overwrite_verify.cc LIBS json)
build_test_ext(NAME persistent_put_remove_verify SRC_FILES engine_scenarios/persistent/put_remove_verify.cc LIBS json)
build_test_ext(NAME persistent_put_verify_asc_params SRC_FILES engine_scenarios/persistent/put_verify_asc_params.cc LIBS json)
build_test_ext(NAME persistent_put_verify_desc_params SRC_FILES engine_scenarios/persistent/put_verify_desc_params.cc LIBS json)
build_test_ext(NAME persistent_put_verify SRC_FILES engine_scenarios/persistent/put_verify.cc LIBS json)
build_test_ext(NAME persistent_put_get_std_map_multiple_reopen SRC_FILES engine_scenarios/persistent/put_get_std_map_multiple_reopen.cc LIBS json)

# Tests for sorted engines
build_test_ext(NAME sorted_iterate SRC_FILES engine_scenarios/sorted/iterate.cc LIBS json)

# Tests for pmemobj engines
build_test_ext(NAME pmemobj_error_handling_create SRC_FILES engine_scenarios/pmemobj/error_handling_create.cc LIBS json)
build_test_ext(NAME pmemobj_error_handling_defrag SRC_FILES engine_scenarios/pmemobj/error_handling_defrag.cc LIBS json)
build_test_ext(NAME pmemobj_error_handling_tx_path SRC_FILES engine_scenarios/pmemobj/error_handling_tx_path.cc LIBS json)
build_test_ext(NAME pmemobj_put_get_std_map_defrag SRC_FILES engine_scenarios/pmemobj/put_get_std_map_defrag.cc LIBS json)
build_test_ext(NAME pmemobj_error_handling_tx_oom SRC_FILES engine_scenarios/pmemobj/error_handling_tx_oom.cc engine_scenarios/pmemobj/mock_tx_alloc.cc LIBS json dl_libs)
build_test_ext(NAME pmemobj_error_handling_tx_oid SRC_FILES engine_scenarios/pmemobj/error_handling_tx_oid.cc LIBS json libpmemobj_cpp)
build_test_ext(NAME pmemobj_put_get_std_map_oid SRC_FILES engine_scenarios/pmemobj/put_get_std_map_oid.cc LIBS json libpmemobj_cpp)

# Tests for memkind engines
build_test_ext(NAME memkind_error_handling SRC_FILES engine_scenarios/memkind/error_handling.cc LIBS json)

# Tests for C API
build_test(c_api_null_db_config c_api/null_db_config.c)

###################################### CMAP ####################################
if(ENGINE_CMAP)
	add_engine_test(ENGINE cmap
			BINARY c_api_null_db_config
			TRACERS none memcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE cmap
			BINARY put_get_remove
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE cmap
			BINARY put_get_remove_long_key
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE cmap
			BINARY put_get_remove_params
			TRACERS none
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 4G PARAMS 400000)

	add_engine_test(ENGINE cmap
			BINARY put_get_remove_params
			TRACERS memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 1G PARAMS 4000)

	add_engine_test(ENGINE cmap
			BINARY put_get_std_map
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE cmap
			BINARY error_handling_oom
			TRACERS none
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 50M)

	add_engine_test(ENGINE cmap
			BINARY iterate
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE cmap
			BINARY concurrent_put_get_remove_params
			TRACERS none memcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 8 50)

	if (TESTS_PMEMOBJ_DRD_HELGRIND)
		add_engine_test(ENGINE cmap
				BINARY concurrent_put_get_remove_params
				TRACERS drd helgrind
				SCRIPT pmemobj_based/default.cmake
				PARAMS 8 50)
	endif()

	add_engine_test(ENGINE cmap
			BINARY concurrent_put_get_remove_gen_params
			TRACERS none memcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 8 50 100)

	if (TESTS_PMEMOBJ_DRD_HELGRIND)
		add_engine_test(ENGINE cmap
				BINARY concurrent_put_get_remove_gen_params
				TRACERS drd helgrind
				SCRIPT pmemobj_based/default.cmake
				PARAMS 8 50 100)
	endif()

	add_engine_test(ENGINE cmap
			BINARY concurrent_put_get_remove_single_op_params
			TRACERS none memcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000)

	if (TESTS_PMEMOBJ_DRD_HELGRIND)
		add_engine_test(ENGINE cmap
				BINARY concurrent_put_get_remove_single_op_params
				TRACERS drd helgrind
				SCRIPT pmemobj_based/default.cmake
				PARAMS 250)
	endif()

	add_engine_test(ENGINE cmap
			BINARY persistent_put_get_std_map_multiple_reopen
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE cmap
			BINARY persistent_not_found_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE cmap
			BINARY persistent_overwrite_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE cmap
			BINARY persistent_put_remove_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE cmap
			BINARY persistent_put_verify
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE cmap
			BINARY persistent_put_verify_asc_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 4000)

	add_engine_test(ENGINE cmap
			BINARY persistent_put_verify_desc_params
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 4000)

	add_engine_test(ENGINE cmap
			BINARY pmemobj_error_handling_tx_oom
			TRACERS none memcheck
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 200M)

	add_engine_test(ENGINE cmap
			BINARY pmemobj_error_handling_create
			TRACERS none memcheck
			SCRIPT pmemobj_based/pmemobj/error_handling_create.cmake)

	add_engine_test(ENGINE cmap
			BINARY pmemobj_error_handling_tx_oid
			TRACERS none memcheck
			SCRIPT pmemobj_based/pmemobj/error_handling_tx_oid.cmake)

	add_engine_test(ENGINE cmap
			BINARY pmemobj_error_handling_tx_path
			TRACERS none memcheck
			SCRIPT pmemobj_based/pmemobj/error_handling_tx_path.cmake)

	add_engine_test(ENGINE cmap
			BINARY pmemobj_error_handling_defrag
			TRACERS none memcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE cmap
			BINARY pmemobj_put_get_std_map_defrag
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE cmap
			BINARY pmemobj_put_get_std_map_oid
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/put_get_std_map_oid.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE cmap
			BINARY put_get_std_map
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/put_get_std_map_force_create.cmake
			PARAMS 1000 100 200)
endif(ENGINE_CMAP)
################################################################################
###################################### VCMAP ###################################
if(ENGINE_VCMAP)
	add_engine_test(ENGINE vcmap
			BINARY c_api_null_db_config
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vcmap
			BINARY put_get_remove
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vcmap
			BINARY put_get_remove_long_key
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vcmap
			BINARY put_get_remove_params
			TRACERS none
			SCRIPT memkind_based/default.cmake
			DB_SIZE 4294967296 PARAMS 400000)

	add_engine_test(ENGINE vcmap
			BINARY put_get_remove_params
			TRACERS memcheck
			SCRIPT memkind_based/default.cmake
			DB_SIZE 4294967296 PARAMS 4000)

	add_engine_test(ENGINE vcmap
			BINARY put_get_std_map
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake
			PARAMS 1000 100 200)

	# XXX: https://github.com/pmem/pmemkv/issues/623
	# add_engine_test(ENGINE vcmap
	# BINARY error_handling_oom
	# TRACERS none memcheck
	# SCRIPT memkind_based/default.cmake
	# DB_SIZE 50485760)

	add_engine_test(ENGINE vcmap
			BINARY iterate
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vcmap
			BINARY concurrent_put_get_remove_params
			TRACERS none memcheck # XXX - tbb lock does not work well with drd or helgrind
			SCRIPT memkind_based/default.cmake
			PARAMS 8 50)

	add_engine_test(ENGINE vcmap
			BINARY concurrent_put_get_remove_gen_params
			TRACERS none memcheck # XXX - tbb lock does not work well with drd or helgrind
			SCRIPT memkind_based/default.cmake
			PARAMS 8 50 100)

	add_engine_test(ENGINE vcmap
			BINARY concurrent_put_get_remove_single_op_params
			TRACERS none memcheck # XXX - tbb lock does not work well with drd or helgrind
			SCRIPT memkind_based/default.cmake
			PARAMS 1000)

	add_engine_test(ENGINE vcmap
			BINARY memkind_error_handling
			TRACERS none memcheck
			SCRIPT memkind_based/memkind/error_handling.cmake)
endif(ENGINE_VCMAP)
################################################################################
###################################### VSMAP ###################################
if(ENGINE_VSMAP)
	add_engine_test(ENGINE vsmap
			BINARY c_api_null_db_config
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vsmap
			BINARY put_get_remove
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vsmap
			BINARY put_get_remove_long_key
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vsmap
			BINARY put_get_remove_params
			TRACERS none
			SCRIPT memkind_based/default.cmake
			DB_SIZE 4294967296 PARAMS 400000)

	add_engine_test(ENGINE vsmap
			BINARY put_get_remove_params
			TRACERS memcheck
			SCRIPT memkind_based/default.cmake
			DB_SIZE 4294967296 PARAMS 4000)

	add_engine_test(ENGINE vsmap
			BINARY put_get_std_map
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE vsmap
			BINARY error_handling_oom
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake
			DB_SIZE 50485760)

	add_engine_test(ENGINE vsmap
			BINARY iterate
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vsmap
			BINARY sorted_iterate
			TRACERS none memcheck
			SCRIPT memkind_based/default.cmake)

	add_engine_test(ENGINE vsmap
			BINARY memkind_error_handling
			TRACERS none memcheck
			SCRIPT memkind_based/memkind/error_handling.cmake)
endif(ENGINE_VSMAP)
################################################################################
###################################### TREE3 ###################################
if(ENGINE_TREE3)
	# XXX - all memcheck and pmemcheck tests are disabled due to failures
	# Need to investigate

	add_engine_test(ENGINE tree3
			BINARY c_api_null_db_config
			TRACERS none #memcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE tree3
			BINARY put_get_remove
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE tree3
			BINARY put_get_remove_long_key
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE tree3
			BINARY put_get_remove_params
			TRACERS none #memcheck
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 4G PARAMS 400000)

	add_engine_test(ENGINE tree3
			BINARY put_get_std_map
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE tree3
			BINARY error_handling_oom
			TRACERS none #memcheck
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 20M)

	# XXX - inverstigate failure
	# add_engine_test(ENGINE tree3
	# BINARY iterate
	# TRACERS none memcheck pmemcheck
	# SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE tree3
			BINARY persistent_put_get_std_map_multiple_reopen
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE tree3
			BINARY persistent_not_found_verify
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE tree3
			BINARY persistent_overwrite_verify
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE tree3
			BINARY persistent_put_remove_verify
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE tree3
			BINARY persistent_put_verify
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE tree3
			BINARY persistent_put_verify_asc_params
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 6000)

	add_engine_test(ENGINE tree3
			BINARY persistent_put_verify_asc_params
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 144) # 144 is limit for one inner node

	add_engine_test(ENGINE tree3
			BINARY persistent_put_verify_desc_params
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 6000)

	add_engine_test(ENGINE tree3
			BINARY persistent_put_verify_desc_params
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 144) # 144 is limit for one inner node

	add_engine_test(ENGINE tree3
			BINARY pmemobj_error_handling_tx_oom
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE tree3
			BINARY pmemobj_error_handling_create
			TRACERS none #memcheck
			SCRIPT pmemobj_based/pmemobj/error_handling_create.cmake)

	# XXX - defrag not supported
	# add_engine_test(ENGINE tree3
	# BINARY pmemobj_error_handling_tx_oid
	# TRACERS none memcheck
	# SCRIPT pmemobj_based/pmemobj/error_handling_tx_oid.cmake)

	# XXX - defrag not supported
	# add_engine_test(ENGINE tree3
	# BINARY pmemobj_error_handling_tx_path
	# TRACERS none memcheck
	# SCRIPT pmemobj_based/pmemobj/error_handling_tx_path.cmake)

	# XXX - defrag not supported
	# add_engine_test(ENGINE tree3
	# BINARY pmemobj_error_handling_defrag
	# TRACERS none memcheck
	# SCRIPT pmemobj_based/default.cmake)

	# XXX - defrag not supported
	# add_engine_test(ENGINE tree3
	# BINARY pmemobj_put_get_std_map_defrag
	# TRACERS none memcheck pmemcheck
	# SCRIPT pmemobj_based/default.cmake
	# PARAMS 1000 100 200)

	add_engine_test(ENGINE tree3
			BINARY pmemobj_put_get_std_map_oid
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/put_get_std_map_oid.cmake
			PARAMS 1000 100 200)

	add_engine_test(ENGINE tree3
			BINARY put_get_std_map
			TRACERS none #memcheck pmemcheck
			SCRIPT pmemobj_based/pmemobj/put_get_std_map_force_create.cmake
			PARAMS 1000 100 200)

	# XXX fail to investigate
	# add_engine_test(ENGINE tree3
	# BINARY sorted_iterate
	# TRACERS none memcheck pmemcheck
	# SCRIPT pmemobj_based/default.cmake)
endif(ENGINE_TREE3)
################################################################################
###################################### STREE ###################################
if(ENGINE_STREE)
	# XXX - all memcheck and pmemcheck tests are disabled due to failures
	# Need to investigate

	add_engine_test(ENGINE stree
			BINARY c_api_null_db_config
			TRACERS none memcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE stree
			BINARY put_get_remove
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE stree
			BINARY put_get_remove_long_key
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE stree
			BINARY put_get_remove_params
			TRACERS none memcheck
			SCRIPT pmemobj_based/default.cmake
			DB_SIZE 4G PARAMS 40000) # 400000 elements test runs > 10 minutes (travis restriction)

	add_engine_test(ENGINE stree
			BINARY put_get_std_map
			TRACERS none memcheck #pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 20 200)

	# XXX - oom test requires support for strings longer than stree max size
	# add_engine_test(ENGINE stree
	# BINARY error_handling_oom
	# TRACERS none memcheck
	# SCRIPT pmemobj_based/default.cmake
	# DB_SIZE 20M)

	add_engine_test(ENGINE stree
			BINARY iterate
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE stree
			BINARY persistent_put_get_std_map_multiple_reopen
			TRACERS none memcheck #pmemcheck
			SCRIPT pmemobj_based/default.cmake
			PARAMS 1000 20 200)

	add_engine_test(ENGINE stree
			BINARY persistent_not_found_verify
			TRACERS none memcheck #pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE stree
			BINARY persistent_overwrite_verify
			TRACERS none memcheck #pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE stree
			BINARY persistent_put_remove_verify
			TRACERS none memcheck #pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE stree
			BINARY persistent_put_verify
			TRACERS none memcheck #pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake)

	add_engine_test(ENGINE stree
			BINARY persistent_put_verify_asc_params
			TRACERS none memcheck #pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 6000)

	add_engine_test(ENGINE stree
			BINARY persistent_put_verify_asc_params
			TRACERS none memcheck #pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 3906) # 3906 is limit for one inner node

	add_engine_test(ENGINE stree
			BINARY persistent_put_verify_desc_params
			TRACERS none memcheck #pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 6000)

	add_engine_test(ENGINE stree
			BINARY persistent_put_verify_desc_params
			TRACERS none memcheck #pmemcheck
			SCRIPT pmemobj_based/persistent/insert_check.cmake
			DB_SIZE 1G PARAMS 3906) # 3906 is limit for one inner node

	# XXX: it uses atomic allocations, enable when make_peristent will be used
	# add_engine_test(ENGINE stree
	# BINARY pmemobj_error_handling_oom
	# TRACERS none memcheck pmemcheck
	# SCRIPT pmemobj_based/default.cmake)

	add_engine_test(ENGINE stree
			BINARY pmemobj_error_handling_create
			TRACERS none memcheck
			SCRIPT pmemobj_based/pmemobj/error_handling_create.cmake)

	# XXX - defrag not supported
	# add_engine_test(ENGINE stree
	# BINARY pmemobj_error_handling_tx_oid
	# TRACERS none memcheck
	# SCRIPT pmemobj_based/pmemobj/error_handling_tx_oid.cmake)

	# XXX - defrag not supported
	# add_engine_test(ENGINE stree
	# BINARY pmemobj_error_handling_tx_path
	# TRACERS none memcheck
	# SCRIPT pmemobj_based/pmemobj/error_handling_tx_path.cmake)

	# XXX - defrag not supported
	# add_engine_test(ENGINE stree
	# BINARY pmemobj_error_handling_defrag
	# TRACERS none memcheck
	# SCRIPT pmemobj_based/default.cmake)

	# XXX - defrag not supported
	# add_engine_test(ENGINE stree
	# BINARY pmemobj_put_get_std_map_defrag
	# TRACERS none memcheck pmemcheck
	# SCRIPT pmemobj_based/default.cmake
	# PARAMS 1000 100 200)

	add_engine_test(ENGINE stree
			BINARY pmemobj_put_get_std_map_oid
			TRACERS none memcheck #pmemcheck
			SCRIPT pmemobj_based/pmemobj/put_get_std_map_oid.cmake
			PARAMS 1000 20 200)

	add_engine_test(ENGINE stree
			BINARY put_get_std_map
			TRACERS none memcheck #pmemcheck
			SCRIPT pmemobj_based/pmemobj/put_get_std_map_force_create.cmake
			PARAMS 1000 20 200)

	add_engine_test(ENGINE stree
			BINARY sorted_iterate
			TRACERS none memcheck pmemcheck
			SCRIPT pmemobj_based/default.cmake)
endif(ENGINE_STREE)
################################################################################
###################################### CACHING #################################
if(ENGINE_CACHING)
	# Test scenarios are not ported for caching engine
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
################################################################################
