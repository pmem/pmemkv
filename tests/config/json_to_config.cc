// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019, Intel Corporation */

#include "../../src/libpmemkv.hpp"
#include "../../src/libpmemkv_json_config.h"
#include "gtest/gtest.h"

class JsonToConfigTest : public testing::Test {
public:
	pmemkv_config *config;

	JsonToConfigTest()
	{
		config = pmemkv_config_new();
	}

	~JsonToConfigTest()
	{
		pmemkv_config_delete(config);
	}
};

TEST_F(JsonToConfigTest, SimpleTest_TRACERS_M)
{
	auto ret = pmemkv_config_from_json(
		config, "{\"string\": \"abc\", \"int\": 123, \"bool\": true}");
	// XXX: extend by adding "false", subconfig, negative value
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);

	const char *value_string;
	ret = pmemkv_config_get_string(config, "string", &value_string);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);
	ASSERT_TRUE(std::string(value_string) == "abc");

	int64_t value_int;
	ret = pmemkv_config_get_int64(config, "int", &value_int);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);
	ASSERT_EQ(value_int, 123);

	int64_t value_bool;
	ret = pmemkv_config_get_int64(config, "bool", &value_bool);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);
	ASSERT_EQ(value_bool, 1);

	ret = pmemkv_config_get_int64(config, "string", &value_int);
	ASSERT_EQ(ret, PMEMKV_STATUS_CONFIG_TYPE_ERROR);
}

TEST_F(JsonToConfigTest, DoubleTest_TRACERS_M)
{
	auto ret = pmemkv_config_from_json(config, "{\"double\": 12.34}");
	ASSERT_EQ(ret, PMEMKV_STATUS_CONFIG_PARSING_ERROR);
	ASSERT_EQ(
		std::string(pmemkv_config_from_json_errormsg()),
		"[pmemkv_config_from_json] Unsupported data type in JSON string: Number");
}

TEST_F(JsonToConfigTest, MalformedInput_TRACERS_M)
{
	auto ret = pmemkv_config_from_json(config, "{\"int\": 12");
	ASSERT_EQ(ret, PMEMKV_STATUS_CONFIG_PARSING_ERROR);
	ASSERT_EQ(std::string(pmemkv_config_from_json_errormsg()),
		  "[pmemkv_config_from_json] Config parsing failed");
}
