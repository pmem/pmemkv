// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

/**
 * Blackhole engine is specific, it mostly does nothing,
 * but we can use it to test C++ API.
 */

static void BlackholeSimpleTest()
{
	/**
	 * TEST: Basic test for blackhole methods.
	 */
	db kv;
	auto s = kv.open("blackhole");
	ASSERT_STATUS(s, status::OK);

	std::string value;
	std::size_t cnt = 1;
	std::string result;
	auto key = "key1";
	(void)value;
	(void)result;
	(void)key;

	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERTeq(cnt, 0);
	ASSERT_STATUS(kv.get(key, &value), status::NOT_FOUND);
	ASSERT_STATUS(kv.put(key, "value1"), status::OK);
	ASSERT_STATUS(kv.exists(key), status::NOT_FOUND);

	cnt = 1;
	ASSERT_STATUS(kv.count_all(cnt), status::OK);
	UT_ASSERTeq(cnt, 0);
	ASSERT_STATUS(kv.get_all(
			  [](const char *k, size_t kb, const char *v, size_t vb,
			     void *arg) {
				  const auto c = ((std::string *)arg);
				  c->append(std::string(k, kb));
				  c->append(std::string(v, vb));
				  return 0;
			  },
			  &result), status::NOT_FOUND);
	UT_ASSERT(result.empty());
	ASSERT_STATUS(kv.get(key, &value), status::NOT_FOUND);
	ASSERT_STATUS(kv.remove(key), status::OK);
	ASSERT_STATUS(kv.get(key, &value), status::NOT_FOUND);
	ASSERT_STATUS(kv.get(key, nullptr, nullptr), status::NOT_FOUND);

	ASSERT_STATUS(kv.defrag(), status::NOT_SUPPORTED);

	kv.close();
}

static void BlackholeRangeTest()
{
	/**
	 * TEST: Testf for all range methods (designed for sorted engines).
	 */
	db kv;
	auto s = kv.open("blackhole");
	ASSERT_STATUS(s, status::OK);

	std::string result;
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.put("key1", "value1"), status::OK);
	ASSERT_STATUS(kv.put("key2", "value2"), status::OK);
	ASSERT_STATUS(kv.put("key3", "value3"), status::OK);

	ASSERT_STATUS(kv.count_above("key1", cnt), status::OK);
	UT_ASSERTeq(0, cnt);
	ASSERT_STATUS(kv.get_above(
			  "key1",
			  [](const char *k, size_t kb, const char *v, size_t vb,
			     void *arg) {
				  const auto c = ((std::string *)arg);
				  c->append(std::string(k, kb));
				  c->append(std::string(v, vb));
				  return 0;
			  },
			  &result), status::NOT_FOUND);
	UT_ASSERT(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_equal_above("key1", cnt), status::OK);
	UT_ASSERTeq(0, cnt);
	ASSERT_STATUS(kv.get_equal_above(
			  "key1",
			  [](const char *k, size_t kb, const char *v, size_t vb,
			     void *arg) {
				  const auto c = ((std::string *)arg);
				  c->append(std::string(k, kb));
				  c->append(std::string(v, vb));
				  return 0;
			  },
			  &result), status::NOT_FOUND);
	UT_ASSERT(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_below("key1", cnt), status::OK);
	UT_ASSERTeq(0, cnt);
	ASSERT_STATUS(kv.get_below(
			  "key1",
			  [](const char *k, size_t kb, const char *v, size_t vb,
			     void *arg) {
				  const auto c = ((std::string *)arg);
				  c->append(std::string(k, kb));
				  c->append(std::string(v, vb));
				  return 0;
			  },
			  &result), status::NOT_FOUND);
	UT_ASSERT(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_equal_below("key1", cnt), status::OK);
	UT_ASSERTeq(0, cnt);
	ASSERT_STATUS(kv.get_equal_below(
			  "key1",
			  [](const char *k, size_t kb, const char *v, size_t vb,
			     void *arg) {
				  const auto c = ((std::string *)arg);
				  c->append(std::string(k, kb));
				  c->append(std::string(v, vb));
				  return 0;
			  },
			  &result), status::NOT_FOUND);
	UT_ASSERT(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_STATUS(kv.count_between("", "key3", cnt), status::OK);
	UT_ASSERTeq(0, cnt);
	ASSERT_STATUS(kv.get_between(
			  "", "key3",
			  [](const char *k, size_t kb, const char *v, size_t vb,
			     void *arg) {
				  const auto c = ((std::string *)arg);
				  c->append(std::string(k, kb));
				  c->append(std::string(v, vb));
				  return 0;
			  },
			  &result), status::NOT_FOUND);
	UT_ASSERT(result.empty());

	kv.close();
}

int main(int argc, char *argv[])
{
	return run_test([&] {
		BlackholeSimpleTest();
		BlackholeRangeTest();
	});
}
