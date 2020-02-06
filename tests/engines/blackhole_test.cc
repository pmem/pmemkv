// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include "../../src/libpmemkv.hpp"
#include "gtest/gtest.h"

using namespace pmem::kv;

class BlackholeTest : public testing::Test {
public:
	db kv;

	BlackholeTest()
	{
		auto s = kv.open("blackhole");
		if (s != status::OK)
			throw std::runtime_error(errormsg());
	}

	~BlackholeTest()
	{
		kv.close();
	}
};

TEST_F(BlackholeTest, SimpleTest_TRACERS_MP)
{
	std::string value;
	std::size_t cnt = 1;

	ASSERT_TRUE(kv.count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(kv.get("key1", &value) == status::NOT_FOUND);
	ASSERT_TRUE(kv.put("key1", "value1") == status::OK);

	cnt = 1;

	ASSERT_TRUE(kv.count_all(cnt) == status::OK);
	ASSERT_TRUE(cnt == 0);
	ASSERT_TRUE(kv.get("key1", &value) == status::NOT_FOUND);
	ASSERT_TRUE(kv.remove("key1") == status::OK);
	ASSERT_TRUE(kv.get("key1", &value) == status::NOT_FOUND);
	ASSERT_TRUE(kv.defrag() == status::NOT_SUPPORTED);
}

TEST_F(BlackholeTest, GetRangeTest_TRACERS_MP)
{
	std::string result;
	std::size_t cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv.put("key1", "value1") == status::OK);
	ASSERT_TRUE(kv.put("key2", "value2") == status::OK);
	ASSERT_TRUE(kv.put("key3", "value3") == status::OK);

	ASSERT_TRUE(kv.count_above("key1", cnt) == status::OK);
	ASSERT_EQ(0, cnt);
	ASSERT_TRUE(kv.get_above(
			    "key1",
			    [](const char *k, size_t kb, const char *v, size_t vb,
			       void *arg) {
				    const auto c = ((std::string *)arg);
				    c->append(std::string(k, kb));
				    c->append(std::string(v, vb));
				    return 0;
			    },
			    &result) == status::NOT_FOUND);
	ASSERT_TRUE(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv.count_equal_above("key1", cnt) == status::OK);
	ASSERT_EQ(0, cnt);
	ASSERT_TRUE(kv.get_equal_above(
			    "key1",
			    [](const char *k, size_t kb, const char *v, size_t vb,
			       void *arg) {
				    const auto c = ((std::string *)arg);
				    c->append(std::string(k, kb));
				    c->append(std::string(v, vb));
				    return 0;
			    },
			    &result) == status::NOT_FOUND);
	ASSERT_TRUE(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv.count_below("key1", cnt) == status::OK);
	ASSERT_EQ(0, cnt);
	ASSERT_TRUE(kv.get_below(
			    "key1",
			    [](const char *k, size_t kb, const char *v, size_t vb,
			       void *arg) {
				    const auto c = ((std::string *)arg);
				    c->append(std::string(k, kb));
				    c->append(std::string(v, vb));
				    return 0;
			    },
			    &result) == status::NOT_FOUND);
	ASSERT_TRUE(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv.count_equal_below("key1", cnt) == status::OK);
	ASSERT_EQ(0, cnt);
	ASSERT_TRUE(kv.get_equal_below(
			    "key1",
			    [](const char *k, size_t kb, const char *v, size_t vb,
			       void *arg) {
				    const auto c = ((std::string *)arg);
				    c->append(std::string(k, kb));
				    c->append(std::string(v, vb));
				    return 0;
			    },
			    &result) == status::NOT_FOUND);
	ASSERT_TRUE(result.empty());

	cnt = std::numeric_limits<std::size_t>::max();
	ASSERT_TRUE(kv.count_between("", "key3", cnt) == status::OK);
	ASSERT_EQ(0, cnt);
	ASSERT_TRUE(kv.get_between(
			    "", "key3",
			    [](const char *k, size_t kb, const char *v, size_t vb,
			       void *arg) {
				    const auto c = ((std::string *)arg);
				    c->append(std::string(k, kb));
				    c->append(std::string(v, vb));
				    return 0;
			    },
			    &result) == status::NOT_FOUND);
	ASSERT_TRUE(result.empty());
}

/* XXX port it to other engines */
TEST_F(BlackholeTest, ErrormsgTest)
{
	auto s = kv.open("non-existing name");
	ASSERT_TRUE(s == pmem::kv::status::WRONG_ENGINE_NAME);

	auto err = pmem::kv::errormsg();
	ASSERT_TRUE(err.size() > 0);

	s = kv.open("non-existing name");
	ASSERT_TRUE(s == pmem::kv::status::WRONG_ENGINE_NAME);
	s = kv.open("non-existing name");
	ASSERT_TRUE(s == pmem::kv::status::WRONG_ENGINE_NAME);

	/* Test whether errormsg is cleared correctly after each error */
	ASSERT_TRUE(pmem::kv::errormsg() == err);
}
