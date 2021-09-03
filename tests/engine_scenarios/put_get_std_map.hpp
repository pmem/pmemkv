// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#include "unittest.hpp"

#include <map>
#include <string>

using namespace pmem::kv;

template <typename Inserter = pmem::kv::db>
std::map<std::string, std::string> PutToMapTest(size_t n_inserts, size_t key_length,
						size_t value_length, Inserter &kv)
{
	/**
	 * Test: Put data into db and get it back
	 */
	std::map<std::string, std::string> proto_dictionary;
	for (size_t i = 0; i < n_inserts; i++) {
		std::string key = std::to_string(i);
		if (key_length > key.length())
			key += std::string(key_length - key.length(), '0');

		std::string val = std::to_string(i);
		if (value_length > val.length())
			val += std::string(value_length - val.length(), '0');

		proto_dictionary[key] = val;
	}

	/* Put data into db */
	for (const auto &record : proto_dictionary) {
		const auto &key = record.first;
		const auto &val = record.second;

		auto s = kv.put(key, val);
		ASSERT_STATUS(s, status::OK);
	}

	return proto_dictionary;
}

void VerifyKv(const std::map<std::string, std::string> &prototype, pmem::kv::db &kv)
{
	ASSERT_SIZE(kv, prototype.size());

	/* Retrieve data from db and compare with prototype */
	for (const auto &record : prototype) {
		const auto &key = record.first;
		const auto &val = record.second;

		auto s = kv.get(key, [&](string_view value) {
			UT_ASSERTeq(value.compare(val), 0);
		});
		ASSERT_STATUS(s, status::OK);
	}
}
