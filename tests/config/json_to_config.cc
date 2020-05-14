// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include <libpmemkv.hpp>
#include <libpmemkv_json_config.h>

#include "unittest.hpp"

/**
 * Tests pmemkv_config_from_json method in C API
 */

static void simple_test()
{
	/**
	 * TEST: basic data types put into json, to be read using
	 * pmemkv_config_from_json()
	 */
	auto config = pmemkv_config_new();
	UT_ASSERT(config != nullptr);

	auto ret = pmemkv_config_from_json(
		config, "{\"string\": \"abc\", \"int\": 123, \"bool\": true}");
	// XXX: extend by adding "false", subconfig, negative value
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);

	const char *value_string;
	ret = pmemkv_config_get_string(config, "string", &value_string);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERT(std::string(value_string) == "abc");

	int64_t value_int;
	ret = pmemkv_config_get_int64(config, "int", &value_int);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERTeq(value_int, 123);

	int64_t value_bool;
	ret = pmemkv_config_get_int64(config, "bool", &value_bool);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERTeq(value_bool, 1);

	ret = pmemkv_config_get_int64(config, "string", &value_int);
	UT_ASSERTeq(ret, PMEMKV_STATUS_CONFIG_TYPE_ERROR);

	pmemkv_config_delete(config);
}

static void double_test()
{
	/**
	 * TEST: floating point numbers are not supported
	 */
	auto config = pmemkv_config_new();
	UT_ASSERT(config != nullptr);

	auto ret = pmemkv_config_from_json(config, "{\"double\": 12.34}");
	UT_ASSERTeq(ret, PMEMKV_STATUS_CONFIG_PARSING_ERROR);
	UT_ASSERT(
		std::string(pmemkv_config_from_json_errormsg()) ==
		"[pmemkv_config_from_json] Unsupported data type in JSON string: Number");

	pmemkv_config_delete(config);
}

static void malformed_input_test()
{
	/**
	 * TEST: improperly formatted/malformed json string should return an error
	 */
	auto config = pmemkv_config_new();
	UT_ASSERT(config != nullptr);

	auto ret = pmemkv_config_from_json(config, "{\"int\": 12");
	UT_ASSERTeq(ret, PMEMKV_STATUS_CONFIG_PARSING_ERROR);
	UT_ASSERT(std::string(pmemkv_config_from_json_errormsg()) ==
		  "[pmemkv_config_from_json] Config parsing failed");

	pmemkv_config_delete(config);
}

static void test(int argc, char *argv[])
{
	simple_test();
	double_test();
	malformed_input_test();
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
