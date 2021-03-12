// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#include "unittest.hpp"

#include <map>
#include <mutex>
#include <random>
#include <set>
#include <string>

using namespace pmem::kv;

static size_t value_prefix_size = 256;

static std::map<uint64_t, uint64_t> t_seed;
static std::mutex mtx;

static std::mt19937_64 main_generator;

std::mt19937_64 make_ts_generator(size_t thread_id, std::function<void(void)> syncthreads)
{
	std::random_device rd;
	auto seed = rd();

	{
		std::unique_lock<std::mutex> lock(mtx);
		t_seed[thread_id] = seed;
	}

	std::mt19937_64 g(seed);

	syncthreads();

	if (thread_id == 0) {
		for (auto i : t_seed)
			std::cout << "tid: " << i.first << " seed: " << i.second
				  << std::endl;
	}

	syncthreads();

	return g;
}

uint64_t unique_value(std::mt19937_64 &generator, std::set<uint64_t> &set)
{
	uint64_t v;
	do {
		v = generator();
	} while (set.count(v) == 1);

	return v;
}

void verify_init_elements(const std::set<uint64_t> &init, pmem::kv::db &kv)
{
	std::set<uint64_t> keys;
	auto s = kv.get_all([&](string_view k, string_view v) {
		uint64_t *uk = (uint64_t *)k.data();

		std::string value1 = entry_from_string(
			uint64_to_string(*uk) + std::string(value_prefix_size, '0'));
		std::string value2 = entry_from_string(
			uint64_to_string(*uk) + std::string(value_prefix_size, '1'));
		UT_ASSERT(v.compare(value1) == 0 || v.compare(value2) == 0);

		keys.insert(*uk);

		return 0;
	});

	ASSERT_STATUS(s, status::OK);
	UT_ASSERT(keys.size() >= init.size());

	for (const auto &v : init) {
		UT_ASSERTeq(keys.count(v), 1);
	}
}

static void ConcurrentIterationAndPutTest(const size_t threads_number,
					  const size_t thread_items, pmem::kv::db &kv)
{
	/**
	 * TEST: prepares thread_number * thread_items elements in pmemkv.
	 * Then, it concurrently inserts additional element and updates existing one.
	 * At the same time some threads are iterating over pmemkv making sure
	 * the initial data is still accessible.
	 */
	std::set<uint64_t> set;
	auto init_size = threads_number * thread_items;

	for (uint64_t i = 0; i < init_size; i++) {
		auto k = unique_value(main_generator, set);
		set.insert(k);

		std::string value = entry_from_string(
			uint64_to_string(k) + std::string(value_prefix_size, '0'));
		ASSERT_STATUS(kv.put(uint64_to_strv(k), value), status::OK);
	}

	parallel_xexec(
		threads_number,
		[&](size_t thread_id, std::function<void(void)> syncthreads) {
			auto g = make_ts_generator(thread_id, syncthreads);

			if (thread_id < threads_number / 4) {
				for (uint64_t i = 0; i < thread_items; i++) {
					auto k = unique_value(g, set);
					std::string value = entry_from_string(
						uint64_to_string(k) +
						std::string(value_prefix_size, '0'));
					ASSERT_STATUS(kv.put(uint64_to_strv(k), value),
						      status::OK);
				}
			} else if (thread_id < threads_number / 2) {
				for (uint64_t i = 0; i < thread_items; i++) {
					auto existing_e = *set.find(g() % set.size());

					std::string value = entry_from_string(
						uint64_to_string(existing_e) +
						std::string(value_prefix_size, '1'));
					ASSERT_STATUS(
						kv.put(uint64_to_strv(existing_e), value),
						status::OK);
				}
			} else {
				verify_init_elements(set, kv);
			}
		});

	verify_init_elements(set, kv);
}

static void ConcurrentIterationAndRemoveTest(const size_t threads_number,
					     const size_t thread_items, pmem::kv::db &kv)
{
	/**
	 * TEST: prepares 2 * thread_number * thread_items elements in pmemkv.
	 * Then, it concurrently removes half of them, while making sure the other
	 * half is still accessible.
	 */
	std::set<uint64_t> init_set;
	std::set<uint64_t> to_remove_set;
	auto init_size = threads_number * thread_items;

	for (uint64_t i = 0; i < init_size; i++) {
		auto k = unique_value(main_generator, init_set);
		init_set.insert(k);

		std::string value = entry_from_string(
			uint64_to_string(k) + std::string(value_prefix_size, '0'));
		ASSERT_STATUS(kv.put(uint64_to_strv(k), value), status::OK);
	}

	for (uint64_t i = 0; i < init_size; i++) {
		auto k = unique_value(main_generator, to_remove_set);
		to_remove_set.insert(k);

		std::string value = entry_from_string(
			uint64_to_string(k) + std::string(value_prefix_size, '0'));
		ASSERT_STATUS(kv.put(uint64_to_strv(k), value), status::OK);
	}

	parallel_xexec(threads_number,
		       [&](size_t thread_id, std::function<void(void)> syncthreads) {
			       auto g = make_ts_generator(thread_id, syncthreads);

			       if (thread_id < threads_number / 2) {
				       for (uint64_t i = 0; i < thread_items; i++) {
					       auto existing_e = *to_remove_set.find(
						       g() % to_remove_set.size());

					       auto s = kv.remove(
						       uint64_to_strv(existing_e));
					       UT_ASSERT(s == status::OK ||
							 s == status::NOT_FOUND);
				       }
			       } else {
				       verify_init_elements(init_set, kv);
			       }
		       });

	verify_init_elements(init_set, kv);
}

static void test(int argc, char *argv[])
{
	using namespace std::placeholders;

	if (argc < 5)
		UT_FATAL("usage: %s engine json_config threads items [value_prefix_size]",
			 argv[0]);
	if (argc > 5)
		value_prefix_size = std::stoul(argv[5]);

	std::random_device rd;
	auto seed = rd();

	std::cout << "main thread rand seed: " << seed << std::endl;

	main_generator = std::mt19937_64(seed);

	size_t threads_number = std::stoull(argv[3]);
	size_t thread_items = std::stoull(argv[4]);
	run_engine_tests(argv[1], argv[2],
			 {
				 std::bind(ConcurrentIterationAndPutTest, threads_number,
					   thread_items, _1),
				 std::bind(ConcurrentIterationAndRemoveTest,
					   threads_number, thread_items, _1),
			 });
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
