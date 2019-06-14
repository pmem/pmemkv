/*
 * Copyright 2017-2019, Intel Corporation
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
		pmemkv_config_delete(config);
	}
};

struct custom_type {
	int a;
	char b;
};

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
	ret = pmemkv_config_put_object(config, "object_ptr", &ptr, sizeof(ptr));
	ASSERT_EQ(ret, PMEMKV_STATUS_OK);

	ret = pmemkv_config_put_object(config, "object", ptr, sizeof(*ptr));
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

	custom_type **value_custom_ptr;
	size_t value_custom_ptr_size;
	ret = pmemkv_config_get_object(config, "object_ptr",
				       (const void **)&value_custom_ptr,
				       &value_custom_ptr_size);

	ASSERT_EQ(ret, PMEMKV_STATUS_OK);
	ASSERT_EQ(value_custom_ptr_size, sizeof(*value_custom_ptr));
	ASSERT_EQ((*value_custom_ptr)->a, 10);
	ASSERT_EQ((*value_custom_ptr)->b, 'a');

	custom_type *value_custom;
	size_t value_custom_size;
	ret = pmemkv_config_get_object(config, "object", (const void **)&value_custom,
				       &value_custom_size);

	ASSERT_EQ(ret, PMEMKV_STATUS_OK);
	ASSERT_EQ(value_custom_size, sizeof(value_custom));
	ASSERT_EQ(value_custom->a, 10);
	ASSERT_EQ(value_custom->b, 'a');

	delete ptr;
}
