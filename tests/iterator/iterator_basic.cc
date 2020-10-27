// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include <vector>

#include "../common/unittest.hpp"

using pair = std::pair<std::string, std::string>;

static std::vector<pair> keys{{"aaa", "1"}, {"bbb", "2"}, {"ccc", "3"},	 {"rrr", "4"},
			      {"sss", "5"}, {"ttt", "6"}, {"yyy", "记!"}};

static void insert_keys(pmem::kv::db &kv)
{
	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(kv.put(p.first, p.second), pmem::kv::status::OK);
	});
}

template <bool IsConst>
static void seek_test(std::string engine, pmem::kv::config &&config)
{
	auto kv = INITIALIZE_KV(engine, std::move(config));

	auto it = kv.new_iterator<IsConst>();

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek(p.first), pmem::kv::status::NOT_FOUND);
	});

	insert_keys(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek(p.first), pmem::kv::status::OK);
		// XXX: ASSERT(it.value() == p.second and it.key() == p.first)
	});

	CLEAR_KV(kv);
}

template <bool IsConst>
static void seek_lower_test(std::string engine, pmem::kv::config &&config)
{
	auto kv = INITIALIZE_KV(engine, std::move(config));

	auto it = kv.new_iterator<IsConst>();

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek_lower(p.first), pmem::kv::status::NOT_FOUND);
	});

	insert_keys(kv);

	ASSERT_STATUS(it.seek_lower(keys[0].first), pmem::kv::status::NOT_FOUND);

	std::for_each(keys.begin() + 1, keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek_lower(p.first), pmem::kv::status::OK);
		// XXX: ASSERT(it.value() and it.key())
	});

	CLEAR_KV(kv);
}

template <bool IsConst>
static void seek_lower_eq_test(std::string engine, pmem::kv::config &&config)
{
	auto kv = INITIALIZE_KV(engine, std::move(config));

	auto it = kv.new_iterator<IsConst>();

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek_lower_eq(p.first), pmem::kv::status::NOT_FOUND);
	});

	insert_keys(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek_lower_eq(p.first), pmem::kv::status::OK);
		// XXX: ASSERT(it.value() and it.key())
	});

	CLEAR_KV(kv);
}

template <bool IsConst>
static void seek_higher_test(std::string engine, pmem::kv::config &&config)
{
	auto kv = INITIALIZE_KV(engine, std::move(config));

	auto it = kv.new_iterator<IsConst>();

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek_higher(p.first), pmem::kv::status::NOT_FOUND);
	});

	insert_keys(kv);

	std::for_each(keys.begin(), keys.end() - 1, [&](pair p) {
		ASSERT_STATUS(it.seek_higher(p.first), pmem::kv::status::OK);
		// XXX: ASSERT(it.value() and it.key())
	});

	ASSERT_STATUS(it.seek_higher(keys[keys.size() - 1].first),
		      pmem::kv::status::NOT_FOUND);

	CLEAR_KV(kv);
}

template <bool IsConst>
static void seek_higher_eq_test(std::string engine, pmem::kv::config &&config)
{
	auto kv = INITIALIZE_KV(engine, std::move(config));

	auto it = kv.new_iterator<IsConst>();

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek_higher_eq(p.first), pmem::kv::status::NOT_FOUND);
	});

	insert_keys(kv);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek_higher_eq(p.first), pmem::kv::status::OK);
		// XXX: ASSERT(it.value() and it.key())
	});

	CLEAR_KV(kv);
}

template <bool IsConst>
static void seek_to_first_test(std::string engine, pmem::kv::config &&config)
{
	auto kv = INITIALIZE_KV(engine, std::move(config));

	auto it = kv.new_iterator<IsConst>();

	insert_keys(kv);

	ASSERT_STATUS(it.seek_to_first(), pmem::kv::status::OK);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek(p.first), pmem::kv::status::OK);
		ASSERT_STATUS(it.seek_to_first(), pmem::kv::status::OK);
		// XXX: ASSERT(it.value() and it.key())
	});

	CLEAR_KV(kv);
}

template <bool IsConst>
static void seek_to_last_test(std::string engine, pmem::kv::config &&config)
{
	auto kv = INITIALIZE_KV(engine, std::move(config));

	auto it = kv.new_iterator<IsConst>();

	insert_keys(kv);

	ASSERT_STATUS(it.seek_to_last(), pmem::kv::status::OK);

	std::for_each(keys.begin(), keys.end(), [&](pair p) {
		ASSERT_STATUS(it.seek(p.first), pmem::kv::status::OK);
		ASSERT_STATUS(it.seek_to_last(), pmem::kv::status::OK);
		// XXX: ASSERT(it.value() and it.key())
	});

	CLEAR_KV(kv);
}

template <bool IsConst>
static void next_test(std::string engine, pmem::kv::config &&config)
{
	auto kv = INITIALIZE_KV(engine, std::move(config));

	auto it = kv.new_iterator<IsConst>();

	insert_keys(kv);

	ASSERT_STATUS(it.seek_to_first(), pmem::kv::status::OK);

	std::for_each(keys.begin() + 1, keys.end(), [&](pair p) {
		ASSERT_STATUS(it.next(), pmem::kv::status::OK);
		// XXX: ASSERT(it.value() and it.key())
	});

	ASSERT_STATUS(it.next(), pmem::kv::status::NOT_FOUND);

	CLEAR_KV(kv);
}

template <bool IsConst>
static void prev_test(std::string engine, pmem::kv::config &&config)
{
	auto kv = INITIALIZE_KV(engine, std::move(config));

	auto it = kv.new_iterator<IsConst>();

	insert_keys(kv);

	ASSERT_STATUS(it.seek_to_last(), pmem::kv::status::OK);

	std::for_each(keys.rbegin() + 1, keys.rend(), [&](pair p) {
		ASSERT_STATUS(it.prev(), pmem::kv::status::OK);
		// XXX: ASSERT(it.value() and it.key())
	});

	ASSERT_STATUS(it.prev(), pmem::kv::status::NOT_FOUND);

	CLEAR_KV(kv);
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	auto engine = std::string(argv[1]);

	seek_test<true>(engine, CONFIG_FROM_JSON(argv[2]));
	seek_test<false>(engine, CONFIG_FROM_JSON(argv[2]));

	seek_lower_test<true>(engine, CONFIG_FROM_JSON(argv[2]));
	seek_lower_test<false>(engine, CONFIG_FROM_JSON(argv[2]));

	seek_lower_eq_test<true>(engine, CONFIG_FROM_JSON(argv[2]));
	seek_lower_eq_test<false>(engine, CONFIG_FROM_JSON(argv[2]));

	seek_higher_test<true>(engine, CONFIG_FROM_JSON(argv[2]));
	seek_higher_test<false>(engine, CONFIG_FROM_JSON(argv[2]));

	seek_higher_eq_test<true>(engine, CONFIG_FROM_JSON(argv[2]));
	seek_higher_eq_test<false>(engine, CONFIG_FROM_JSON(argv[2]));

	seek_to_first_test<true>(engine, CONFIG_FROM_JSON(argv[2]));
	seek_to_first_test<false>(engine, CONFIG_FROM_JSON(argv[2]));

	seek_to_last_test<true>(engine, CONFIG_FROM_JSON(argv[2]));
	seek_to_last_test<false>(engine, CONFIG_FROM_JSON(argv[2]));

	next_test<true>(engine, CONFIG_FROM_JSON(argv[2]));
	next_test<false>(engine, CONFIG_FROM_JSON(argv[2]));

	prev_test<true>(engine, CONFIG_FROM_JSON(argv[2]));
	prev_test<false>(engine, CONFIG_FROM_JSON(argv[2]));
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
