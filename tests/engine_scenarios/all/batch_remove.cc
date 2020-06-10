// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void RemoveExistingValuesTest(pmem::kv::db &kv)
{
	UT_ASSERT(kv.put("key1", "value1") == status::OK);
	UT_ASSERT(kv.put("key2", "value2") == status::OK);
	UT_ASSERT(kv.put("key3", "value3") == status::OK);
	UT_ASSERT(kv.put("key4", "value4") == status::OK);
	UT_ASSERT(kv.put("key5", "value5") == status::OK);

	auto ret = kv.batch_remove({"key2", "key3", "key4"});
	UT_ASSERTeq(ret, status::OK);

	UT_ASSERTeq(kv.exists("key1"), status::OK);
	UT_ASSERTeq(kv.exists("key2"), status::NOT_FOUND);
	UT_ASSERTeq(kv.exists("key3"), status::NOT_FOUND);
	UT_ASSERTeq(kv.exists("key4"), status::NOT_FOUND);
	UT_ASSERTeq(kv.exists("key5"), status::OK);
}

static void RemoveNonExistingValuesTest(pmem::kv::db &kv)
{
	UT_ASSERT(kv.put("key1", "value1") == status::OK);
	UT_ASSERT(kv.put("key2", "value2") == status::OK);
	UT_ASSERT(kv.put("key3", "value3") == status::OK);
	UT_ASSERT(kv.put("key4", "value4") == status::OK);
	UT_ASSERT(kv.put("key5", "value5") == status::OK);

	auto ret = kv.batch_remove({"key2", "key3", "non-existing-key"});
	UT_ASSERTeq(ret, status::NOT_FOUND);

	UT_ASSERTeq(kv.exists("key1"), status::OK);
	UT_ASSERTeq(kv.exists("key2"), status::OK);
	UT_ASSERTeq(kv.exists("key3"), status::OK);
	UT_ASSERTeq(kv.exists("key4"), status::OK);
	UT_ASSERTeq(kv.exists("key5"), status::OK);
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2],
			 {RemoveExistingValuesTest, RemoveNonExistingValuesTest});
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
