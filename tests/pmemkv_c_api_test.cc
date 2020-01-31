/*
 * Copyright 2019-2020, Intel Corporation
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

#include "../src/libpmemkv.h"
#include "gtest/gtest.h"
#include <cstdio>
#include <map>
#include <string>
#include <sys/stat.h>

// Tests and params' list
#include "basic_tests.h"

class PmemkvCApiTest : public ::testing::TestWithParam<Basic> {
public:
	std::map<std::string, int> init_status;
	pmemkv_db *db = NULL;
	Basic params = GetParam();
	std::string path;

	void SetUp()
	{
		path = params.get_path();
		std::remove(path.c_str());

		if (!params.use_file)
			mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

		pmemkv_config *cfg = pmemkv_config_new();
		ASSERT_NE(cfg, nullptr) << pmemkv_errormsg();
		ASSERT_EQ(pmemkv_config_put_string(cfg, "path", path.c_str()),
			  PMEMKV_STATUS_OK)
			<< pmemkv_errormsg();
		ASSERT_EQ(pmemkv_config_put_uint64(cfg, "size", params.size),
			  PMEMKV_STATUS_OK)
			<< pmemkv_errormsg();
		ASSERT_EQ(pmemkv_config_put_uint64(cfg, "force_create",
						   params.force_create),
			  PMEMKV_STATUS_OK)
			<< pmemkv_errormsg();
		ASSERT_EQ(pmemkv_open(params.engine, cfg, &db), PMEMKV_STATUS_OK)
			<< pmemkv_errormsg();
	}

	void TearDown()
	{
		pmemkv_close(db);
		std::remove(path.c_str());
	}
};

struct GetTestName {
	template <typename ParamType>
	std::string operator()(const testing::TestParamInfo<ParamType> &info) const
	{
		auto test = static_cast<ParamType>(info.param);
		auto name = test.name;
		if (!info.param.tracers.empty()) {
			name += "_TRACERS_" + info.param.tracers;
		}
		return name;
	}
};

TEST_P(PmemkvCApiTest, PutAndGet)
{
	/**
	 * Test: Put data into db and get it back
	 */
	std::map<std::string, std::string> proto_dictionary;
	/// Create test dictionary
	for (size_t i = 0; i < params.test_value_length; i++) {
		std::string key = std::to_string(i);
		key.insert(key.begin(), params.key_length - key.length(), '0');
		std::string val = std::to_string(i);
		val.insert(val.begin(), params.value_length - val.length(), '0');
		proto_dictionary[key] = val;
	}

	/// Put data into db
	for (const auto &record : proto_dictionary) {
		const char *key = record.first.c_str();
		const char *val = record.second.c_str();
		int s = pmemkv_put(db, key, strlen(key), val, strlen(val));
		ASSERT_EQ(PMEMKV_STATUS_OK, s)
			<< "Cannot put key: " << key << " with value: " << val;
	}
	/// Retrieve data from db and compare with prototype
	for (const auto &record : proto_dictionary) {
		char *buffer = new char[params.value_length + 1];
		const char *key = record.first.c_str();
		int s = pmemkv_get_copy(db, key, strlen(key), buffer,
					params.value_length + 1, NULL);
		ASSERT_EQ(PMEMKV_STATUS_OK, s) << "Cannot get value for key: " << key
					       << ". " << pmemkv_errormsg();
		ASSERT_STREQ(record.second.c_str(), buffer)
			<< "Retrieved value is different than original";
		delete[] buffer;
	}
}

/* Test if null can be passed as db to pmemkv_* functions */
TEST_P(PmemkvCApiTest, CheckNullDB)
{
	size_t cnt;
	const char *key1 = "key1";
	const char *value1 = "value1";
	const char *key2 = "key2";
	char val[10];

	int s = pmemkv_count_all(NULL, &cnt);
	(void)s;
	ASSERT_TRUE(s == PMEMKV_STATUS_INVALID_ARGUMENT) << pmemkv_errormsg();

	s = pmemkv_count_above(NULL, key1, strlen(key1), &cnt);
	ASSERT_TRUE(s == PMEMKV_STATUS_INVALID_ARGUMENT) << pmemkv_errormsg();

	s = pmemkv_count_below(NULL, key1, strlen(key1), &cnt);
	ASSERT_TRUE(s == PMEMKV_STATUS_INVALID_ARGUMENT) << pmemkv_errormsg();

	s = pmemkv_count_between(NULL, key1, strlen(key1), key2, strlen(key2), &cnt);
	ASSERT_TRUE(s == PMEMKV_STATUS_INVALID_ARGUMENT) << pmemkv_errormsg();

	s = pmemkv_get_all(NULL, NULL, NULL);
	ASSERT_TRUE(s == PMEMKV_STATUS_INVALID_ARGUMENT) << pmemkv_errormsg();

	s = pmemkv_get_above(NULL, key1, strlen(key1), NULL, NULL);
	ASSERT_TRUE(s == PMEMKV_STATUS_INVALID_ARGUMENT) << pmemkv_errormsg();

	s = pmemkv_get_below(NULL, key1, strlen(key1), NULL, NULL);
	ASSERT_TRUE(s == PMEMKV_STATUS_INVALID_ARGUMENT) << pmemkv_errormsg();

	s = pmemkv_get_between(NULL, key1, strlen(key1), key2, strlen(key2), NULL, NULL);
	ASSERT_TRUE(s == PMEMKV_STATUS_INVALID_ARGUMENT) << pmemkv_errormsg();

	s = pmemkv_exists(NULL, key1, strlen(key1));
	ASSERT_TRUE(s == PMEMKV_STATUS_INVALID_ARGUMENT) << pmemkv_errormsg();

	s = pmemkv_get(NULL, key1, strlen(key1), NULL, NULL);
	ASSERT_TRUE(s == PMEMKV_STATUS_INVALID_ARGUMENT) << pmemkv_errormsg();

	s = pmemkv_get_copy(NULL, key1, strlen(key1), val, 10, &cnt);
	ASSERT_TRUE(s == PMEMKV_STATUS_INVALID_ARGUMENT) << pmemkv_errormsg();

	s = pmemkv_put(NULL, key1, strlen(key1), value1, strlen(value1));
	ASSERT_TRUE(s == PMEMKV_STATUS_INVALID_ARGUMENT) << pmemkv_errormsg();

	s = pmemkv_remove(NULL, key1, strlen(key1));
	ASSERT_TRUE(s == PMEMKV_STATUS_INVALID_ARGUMENT) << pmemkv_errormsg();

	s = pmemkv_defrag(NULL, 0, 100);
	ASSERT_TRUE(s == PMEMKV_STATUS_INVALID_ARGUMENT) << pmemkv_errormsg();
}

TEST_P(PmemkvCApiTest, NullConfig)
{
	/* XXX solve it generically, for all tests */
	if (params.engine == std::string("blackhole"))
		return;
	pmemkv_config *empty_cfg = NULL;
	pmemkv_db *db = NULL;
	int s = pmemkv_open(params.engine, empty_cfg, &db);
	ASSERT_TRUE(s == PMEMKV_STATUS_INVALID_ARGUMENT) << pmemkv_errormsg();
}

INSTANTIATE_TEST_CASE_P(basic_tests, PmemkvCApiTest, ::testing::ValuesIn(basic_tests),
			GetTestName());
