// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019, Intel Corporation */

#include "../../src/libpmemkv.hpp"
#include "gtest/gtest.h"

class ConfigCTest : public testing::Test {
public:
	pmemkv_config *config;

	ConfigCTest()
	{
		config = pmemkv_config_new();
	}

	~ConfigCTest()
	{
		if (config)
			pmemkv_config_delete(config);
	}
};

struct custom_type {
	int a;
	char b;
};

static void deleter(custom_type *ct_ptr)
{
	ct_ptr->a = -1;
	ct_ptr->b = '0';
}

TEST_F(ConfigCTest, SimpleTest_TRACERS_M)
{
	auto ret = pmemkv_config_put_string(config, "string", "abc");
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);

	ret = pmemkv_config_put_int64(config, "int", 123);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);

	custom_type *ptr = new custom_type;
	ptr->a = 10;
	ptr->b = 'a';
	ret = pmemkv_config_put_object(config, "object_ptr", ptr, nullptr);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);

	ret = pmemkv_config_put_data(config, "object", ptr, sizeof(*ptr));
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);

	custom_type *ptr_deleter = new custom_type;
	ptr_deleter->a = 11;
	ptr_deleter->b = 'b';
	ret = pmemkv_config_put_object(config, "object_ptr_with_deleter", ptr_deleter,
				       (void (*)(void *)) & deleter);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);

	const char *value_string;
	ret = pmemkv_config_get_string(config, "string", &value_string);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);
	ASSERT_TRUE(std::string(value_string) == "abc");

	int64_t value_int;
	ret = pmemkv_config_get_int64(config, "int", &value_int);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);
	ASSERT_EQ(value_int, 123);

	custom_type *value_custom_ptr;
	ret = pmemkv_config_get_object(config, "object_ptr", (void **)&value_custom_ptr);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);
	ASSERT_EQ(value_custom_ptr->a, 10);
	ASSERT_EQ(value_custom_ptr->b, 'a');

	custom_type *value_custom_ptr_deleter;
	ret = pmemkv_config_get_object(config, "object_ptr_with_deleter",
				       (void **)&value_custom_ptr_deleter);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);
	ASSERT_EQ(value_custom_ptr_deleter->a, 11);
	ASSERT_EQ(value_custom_ptr_deleter->b, 'b');

	custom_type *value_custom;
	size_t value_custom_size;
	ret = pmemkv_config_get_data(config, "object", (const void **)&value_custom,
				     &value_custom_size);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);
	ASSERT_EQ(value_custom_size, sizeof(value_custom));
	ASSERT_EQ(value_custom->a, 10);
	ASSERT_EQ(value_custom->b, 'a');

	int64_t none;
	ASSERT_EQ(pmemkv_config_get_int64(config, "non-existent", &none),
		  PMEMKV_STATUS_NOT_FOUND);

	delete ptr;

	pmemkv_config_delete(config);
	config = nullptr;

	ASSERT_EQ(value_custom_ptr_deleter->a, -1);
	ASSERT_EQ(value_custom_ptr_deleter->b, '0');

	delete ptr_deleter;
}

TEST_F(ConfigCTest, IntegralConversionTest_TRACERS_M)
{
	auto ret = pmemkv_config_put_int64(config, "int", 123);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);

	ret = pmemkv_config_put_uint64(config, "uint", 123);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);

	ret = pmemkv_config_put_int64(config, "negative-int", -123);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);

	ret = pmemkv_config_put_uint64(config, "uint-max",
				       std::numeric_limits<size_t>::max());
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);

	int64_t int_s;
	ret = pmemkv_config_get_int64(config, "int", &int_s);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);
	ASSERT_EQ(int_s, 123);

	size_t int_us;
	ret = pmemkv_config_get_uint64(config, "int", &int_us);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);
	ASSERT_EQ(int_us, 123U);

	int64_t uint_s;
	ret = pmemkv_config_get_int64(config, "uint", &uint_s);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);
	ASSERT_EQ(uint_s, 123);

	size_t uint_us;
	ret = pmemkv_config_get_uint64(config, "uint", &uint_us);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);
	ASSERT_EQ(uint_us, 123U);

	int64_t neg_int_s;
	ret = pmemkv_config_get_int64(config, "negative-int", &neg_int_s);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);
	ASSERT_EQ(neg_int_s, -123);

	size_t neg_int_us;
	ret = pmemkv_config_get_uint64(config, "negative-int", &neg_int_us);
	ASSERT_EQ(ret, PMEMKV_STATUS_CONFIG_TYPE_ERROR);

	int64_t uint_max_s;
	ret = pmemkv_config_get_int64(config, "uint-max", &uint_max_s);
	ASSERT_EQ(ret, PMEMKV_STATUS_CONFIG_TYPE_ERROR);

	size_t uint_max_us;
	ret = pmemkv_config_get_uint64(config, "uint-max", &uint_max_us);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);
	ASSERT_EQ(uint_max_us, std::numeric_limits<size_t>::max());
}

TEST_F(ConfigCTest, NotFoundTest_TRACERS_M)
{
	/* all gets should return NotFound when looking for non-existing key */
	const char *my_string;
	int ret = pmemkv_config_get_string(config, "non-existent-string", &my_string);
	ASSERT_EQ(ret, PMEMKV_STATUS_NOT_FOUND);

	int64_t my_int;
	ret = pmemkv_config_get_int64(config, "non-existent-int", &my_int);
	ASSERT_EQ(ret, PMEMKV_STATUS_NOT_FOUND);

	size_t my_uint;
	ret = pmemkv_config_get_uint64(config, "non-existent-uint", &my_uint);
	ASSERT_EQ(ret, PMEMKV_STATUS_NOT_FOUND);

	custom_type *my_object;
	ret = pmemkv_config_get_object(config, "non-existent-object",
				       (void **)&my_object);
	ASSERT_EQ(ret, PMEMKV_STATUS_NOT_FOUND);

	size_t my_object_size = 0;
	ret = pmemkv_config_get_data(config, "non-existent-data",
				     (const void **)&my_object, &my_object_size);
	ASSERT_EQ(ret, PMEMKV_STATUS_NOT_FOUND);
	ASSERT_EQ(my_object_size, 0U);
}

/* Test if null can be passed as config to pmemkv_config_* functions */
TEST_F(ConfigCTest, NullConfigTest)
{
	auto ret = pmemkv_config_put_string(nullptr, "string", "abc");
	ASSERT_EQ(ret, PMEMKV_STATUS_INVALID_ARGUMENT);

	ret = pmemkv_config_put_int64(nullptr, "int", 123);
	ASSERT_EQ(ret, PMEMKV_STATUS_INVALID_ARGUMENT);

	custom_type *ptr = new custom_type;
	ptr->a = 10;
	ptr->b = 'a';
	ret = pmemkv_config_put_object(nullptr, "object_ptr", ptr, nullptr);
	ASSERT_EQ(ret, PMEMKV_STATUS_INVALID_ARGUMENT);

	ret = pmemkv_config_put_data(nullptr, "object", ptr, sizeof(*ptr));
	ASSERT_EQ(ret, PMEMKV_STATUS_INVALID_ARGUMENT);

	const char *value_string;
	ret = pmemkv_config_get_string(nullptr, "string", &value_string);
	ASSERT_EQ(ret, PMEMKV_STATUS_INVALID_ARGUMENT);

	int64_t value_int;
	ret = pmemkv_config_get_int64(nullptr, "int", &value_int);
	ASSERT_EQ(ret, PMEMKV_STATUS_INVALID_ARGUMENT);

	custom_type *value_custom_ptr;
	ret = pmemkv_config_get_object(nullptr, "object_ptr", (void **)&value_custom_ptr);
	ASSERT_EQ(ret, PMEMKV_STATUS_INVALID_ARGUMENT);

	custom_type *value_custom;
	size_t value_custom_size;
	ret = pmemkv_config_get_data(nullptr, "object", (const void **)&value_custom,
				     &value_custom_size);
	ASSERT_EQ(ret, PMEMKV_STATUS_INVALID_ARGUMENT);

	delete ptr;
}
