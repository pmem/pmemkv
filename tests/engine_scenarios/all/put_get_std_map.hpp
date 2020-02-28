/*
 * Copyright 2020, Intel Corporation
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

#include "unittest.hpp"

#include <map>
#include <string>

using namespace pmem::kv;

std::map<std::string, std::string> PutToMapTest(size_t n_inserts, size_t key_length, size_t value_length,
				pmem::kv::db &kv)
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
		UT_ASSERTeq(status::OK, s);
	}

	return proto_dictionary;
}

void VerifyKv(const std::map<std::string, std::string> &prototype, pmem::kv::db &kv)
{
	/* Retrieve data from db and compare with prototype */
	for (const auto &record : prototype) {
		const auto &key = record.first;
		const auto &val = record.second;

		auto s = kv.get(key, [&](string_view value) {
			UT_ASSERT(value.compare(val) == 0);
		});
		UT_ASSERTeq(status::OK, s);
	}
}
