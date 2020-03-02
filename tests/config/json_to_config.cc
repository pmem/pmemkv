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

#include <libpmemkv.hpp>
#include <libpmemkv_json_config.h>

#include "unittest.hpp"

void simple_test()
{
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

void double_test()
{
	auto config = pmemkv_config_new();
	UT_ASSERT(config != nullptr);

	auto ret = pmemkv_config_from_json(config, "{\"double\": 12.34}");
	UT_ASSERTeq(ret, PMEMKV_STATUS_CONFIG_PARSING_ERROR);
	UT_ASSERT(
		std::string(pmemkv_config_from_json_errormsg()) ==
		"[pmemkv_config_from_json] Unsupported data type in JSON string: Number");

	pmemkv_config_delete(config);
}

void malformed_input_test()
{
	auto config = pmemkv_config_new();
	UT_ASSERT(config != nullptr);

	auto ret = pmemkv_config_from_json(config, "{\"int\": 12");
	UT_ASSERTeq(ret, PMEMKV_STATUS_CONFIG_PARSING_ERROR);
	UT_ASSERT(std::string(pmemkv_config_from_json_errormsg()) ==
		  "[pmemkv_config_from_json] Config parsing failed");

	pmemkv_config_delete(config);
}

void test(int argc, char *argv[])
{
	simple_test();
	double_test();
	malformed_input_test();
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
