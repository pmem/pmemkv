// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include <algorithm>
#include <functional>
#include <memory>

#include "unittest.hpp"

using namespace pmem::kv;

class less_compare : public comparator {
public:
	less_compare()
	{
	}

	int compare(string_view k1, string_view k2) override
	{
		return k1.compare(k2);
	}

	std::string name() override
	{
		return "less_comapre";
	}
};

class greater_compare : public comparator {
public:
	greater_compare()
	{
	}

	int compare(string_view k1, string_view k2) override
	{
		return k2.compare(k1);
	}

	std::string name() override
	{
		return "less_comapre";
	}
};

using kv_pair = std::pair<std::string, std::string>;

template <typename Cmp>
class kv_comparator {
public:
	kv_comparator(const Cmp &cmp = Cmp{}) : cmp(cmp)
	{
	}

	bool operator()(const kv_pair &k1, const kv_pair &k2)
	{
		return cmp.compare(k1.first, k2.first) < 0;
	}

private:
	Cmp cmp;
};

template <typename Comparator>
static void TestOrdering(std::string engine, pmem::kv::config &&config)
{
	auto cmp = std::unique_ptr<comparator>(new Comparator());
	auto s = config.put_comparator(std::move(cmp));
	UT_ASSERTeq(s, status::OK);

	db kv;
	s = kv.open(engine, std::move(config));
	UT_ASSERTeq(s, status::OK);

	std::vector<kv_pair> expected_kv_pairs = {{"1", "1"}, {"2", "2"}, {"11", "11"}};

	for (auto &pair : expected_kv_pairs) {
		s = kv.put(pair.first, pair.second);
		UT_ASSERTeq(s, status::OK);
	}

	std::vector<kv_pair> actual_kv_pairs;

	s = kv.get_all([&](string_view k1, string_view k2) {
		actual_kv_pairs.push_back(kv_pair{std::string(k1.data(), k1.size()),
						  std::string(k2.data(), k2.size())});
		return 0;
	});

	std::sort(expected_kv_pairs.begin(), expected_kv_pairs.end(),
		  kv_comparator<Comparator>{});
	UT_ASSERT(actual_kv_pairs == expected_kv_pairs);

	kv.close();
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	auto engine = std::string(argv[1]);

	TestOrdering<less_compare>(engine, CONFIG_FROM_JSON(argv[2]));
	TestOrdering<greater_compare>(engine, CONFIG_FROM_JSON(argv[2]));
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
