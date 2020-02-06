// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static void BlackholeSimpleTest()
{
	db kv;
	auto s = kv.open("blackhole");
	UT_ASSERTeq(status::OK, s);

	std::string value;
	std::size_t cnt = 1;

	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(kv.get("key1", &value) == status::NOT_FOUND);
	UT_ASSERT(kv.put("key1", "value1") == status::OK);

	cnt = 1;

	UT_ASSERT(kv.count_all(cnt) == status::OK);
	UT_ASSERT(cnt == 0);
	UT_ASSERT(kv.get("key1", &value) == status::NOT_FOUND);
	UT_ASSERT(kv.remove("key1") == status::OK);
	UT_ASSERT(kv.get("key1", &value) == status::NOT_FOUND);
	UT_ASSERT(kv.defrag() == status::NOT_SUPPORTED);

	kv.close();
}

static void BlackholeRangeTest()
{
	db kv;
	auto s = kv.open("blackhole");
	UT_ASSERTeq(status::OK, s);

	std::string result;
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.put("key1", "value1") == status::OK);
	UT_ASSERT(kv.put("key2", "value2") == status::OK);
	UT_ASSERT(kv.put("key3", "value3") == status::OK);

	UT_ASSERT(kv.count_above("key1", cnt) == status::OK);
	UT_ASSERTeq(0, cnt);
	UT_ASSERT(kv.get_above(
			  "key1",
			  [](const char *k, size_t kb, const char *v, size_t vb,
			     void *arg) {
				  const auto c = ((std::string *)arg);
				  c->append(std::string(k, kb));
				  c->append(std::string(v, vb));
				  return 0;
			  },
			  &result) == status::NOT_FOUND);
	UT_ASSERT(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_above("key1", cnt) == status::OK);
	UT_ASSERTeq(0, cnt);
	UT_ASSERT(kv.get_equal_above(
			  "key1",
			  [](const char *k, size_t kb, const char *v, size_t vb,
			     void *arg) {
				  const auto c = ((std::string *)arg);
				  c->append(std::string(k, kb));
				  c->append(std::string(v, vb));
				  return 0;
			  },
			  &result) == status::NOT_FOUND);
	UT_ASSERT(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_below("key1", cnt) == status::OK);
	UT_ASSERTeq(0, cnt);
	UT_ASSERT(kv.get_below(
			  "key1",
			  [](const char *k, size_t kb, const char *v, size_t vb,
			     void *arg) {
				  const auto c = ((std::string *)arg);
				  c->append(std::string(k, kb));
				  c->append(std::string(v, vb));
				  return 0;
			  },
			  &result) == status::NOT_FOUND);
	UT_ASSERT(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_equal_below("key1", cnt) == status::OK);
	UT_ASSERTeq(0, cnt);
	UT_ASSERT(kv.get_equal_below(
			  "key1",
			  [](const char *k, size_t kb, const char *v, size_t vb,
			     void *arg) {
				  const auto c = ((std::string *)arg);
				  c->append(std::string(k, kb));
				  c->append(std::string(v, vb));
				  return 0;
			  },
			  &result) == status::NOT_FOUND);
	UT_ASSERT(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_between("", "key3", cnt) == status::OK);
	UT_ASSERTeq(0, cnt);
	UT_ASSERT(kv.get_between(
			  "", "key3",
			  [](const char *k, size_t kb, const char *v, size_t vb,
			     void *arg) {
				  const auto c = ((std::string *)arg);
				  c->append(std::string(k, kb));
				  c->append(std::string(v, vb));
				  return 0;
			  },
			  &result) == status::NOT_FOUND);
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
