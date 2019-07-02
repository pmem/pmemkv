/*
 * Copyright 2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "../../src/libpmemkv.hpp"
#include "gtest/gtest.h"

using namespace pmem::kv;

class ConfigTest : public testing::Test {
public:
	pmemkv_config *config;

	ConfigTest()
	{
		config = pmemkv_config_new();
	}

	~ConfigTest()
	{
		if (config)
			pmemkv_config_delete(config);
	}
};

struct custom_type {
	int a;
	char b;
};

void deleter(custom_type *ct_ptr)
{
	ct_ptr->a = -1;
	ct_ptr->b = '0';
}

TEST_F(ConfigTest, SimpleTest)
{
	auto ret = pmemkv_config_put_string(config, "string", "abc");
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);

	ret = pmemkv_config_put_int64(config, "int", 123);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);

	ret = pmemkv_config_put_double(config, "double", 12.43);
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

	double value_double;
	ret = pmemkv_config_get_double(config, "double", &value_double);
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);
	ASSERT_EQ(value_double, 12.43);

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

	delete ptr;

	pmemkv_config_delete(config);
	config = nullptr;

	ASSERT_EQ(value_custom_ptr_deleter->a, -1);
	ASSERT_EQ(value_custom_ptr_deleter->b, '0');

	delete ptr_deleter;
}

TEST_F(ConfigTest, IntegralConversion)
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
