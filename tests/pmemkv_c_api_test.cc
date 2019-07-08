#include "../src/libpmemkv.h"
#include "gtest/gtest.h"
#include <map>
#include <string>
#include <vector>
#include "test_suits.h"

//Tests list
#include "basic_tests.h"

class PmemkvCApiTest : public ::testing::TestWithParam<Basic> {
public:

	std::map<std::string, int> init_status;

	pmemkv_config *cfg = pmemkv_config_new();
	pmemkv_db *db = NULL;
	Basic params = GetParam();
	PmemkvCApiTest()
	{
		init_status["path"] = pmemkv_config_put_string(cfg, "path", params.path);
		init_status["size"] = pmemkv_config_put_uint64(cfg, "size", params.size);
		init_status["force_create"] = pmemkv_config_put_uint64(
			cfg, "force_create", params.force_create);

		init_status["start_engine"] = pmemkv_open(params.engine, cfg, &db);
		// init_status["fail"] = PMEMKV_STATUS_FAILED;
	}
};

TEST_P(PmemkvCApiTest, ConfigCreated)
{
	/**
	 *  Test: creation of config and starting engine
	 */
	std::cout << "bar" << std::endl;
	ASSERT_NE(cfg, nullptr) << "Config not created";
	for (const auto &s : init_status) {
		ASSERT_EQ(s.second, PMEMKV_STATUS_OK)
			<< "Status error: " << s.first << " with: " << s.second;
	}
}

TEST_P(PmemkvCApiTest, PutAndGet)
{
	/**
	 * Test: Put data into db and get it back
	 */
	std::map<std::string, std::string> proto_dictionary;
	/// Create test dictionary
	for (auto i = 0; i < params.test_data_size;  i++) {
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
	/// Retrive data from db and compare with prototype
	for(const auto& record : proto_dictionary) {
		char *buffer = new char[params.value_length];
		const char *key = record.first.c_str();
		int s = pmemkv_get_copy(db, key, strlen(key), buffer, params.value_length,
				    NULL);
		ASSERT_EQ(PMEMKV_STATUS_OK, s)
			<< "Cannot get value for key: " << key << " " << pmemkv_errormsg();
		ASSERT_STREQ(record.second.c_str(), buffer)
			<< "Retrived value is different than original";
		delete buffer;
	}
}

struct GetTestName
{
	template <typename ParamType>
	std::string operator()(const testing::TestParamInfo<ParamType>& info)
	const {
		auto test = static_cast<ParamType>(info.param);
		return test.name;
	}
};

INSTANTIATE_TEST_CASE_P(basic_tests, PmemkvCApiTest, ::testing::ValuesIn(basic_tests),  GetTestName());

