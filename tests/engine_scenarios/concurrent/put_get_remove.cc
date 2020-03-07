// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"
#include <numeric>

#include <unistd.h>

using namespace pmem::kv;

static void SimpleMultithreadedTest(pmem::kv::db &kv)
{

	size_t threads_number = 8;
	size_t thread_items = 50;
	parallel_exec(threads_number, [&](size_t thread_id) {
		size_t begin = thread_id * thread_items;
		size_t end = begin + thread_items;
		for (auto i = begin; i < end; i++) {
			std::string istr = std::to_string(i);
			UT_ASSERT(kv.put(istr, (istr + "!")) == status::OK);
			std::string value;
			UT_ASSERT(kv.get(istr, &value) == status::OK &&
				  value == (istr + "!"));
		}
		for (auto i = begin; i < end; i++) {
			std::string istr = std::to_string(i);
			std::string value;
			UT_ASSERT(kv.get(istr, &value) == status::OK &&
				  value == (istr + "!"));
		}
	});
	// std::size_t cnt = std::numeric_limits<std::size_t>::max();
	// UT_ASSERT(kv.count_all(cnt) == status::OK);
	// UT_ASSERTeq(cnt, threads_number * thread_items);
}

static void MultithreadedTestRemoveDataAside(pmem::kv::db &kv)
{
	static constexpr uint64_t size = 1000;
	size_t threads_number = size;

	uint64_t keys[size] = {};
	std::iota(keys, keys + size, 0);

	auto uint64_to_key = [](uint64_t &key) {
		return string_view((char *)&key, sizeof(uint64_t));
	};

	for (auto &k : keys)
		UT_ASSERT(kv.put(uint64_to_key(k), uint64_to_key(k)) == status::OK);

	/* test adding and removing data */
	parallel_exec(threads_number, [&](size_t thread_id) {
		if (thread_id % 2 == 1) {
			auto s = kv.get(uint64_to_key(keys[thread_id]),
					[&](string_view value) {
						UT_ASSERT(value.compare(uint64_to_key(
								  keys[thread_id])) == 0);
					});
			UT_ASSERTeq(s, status::OK);

			s = kv.get(uint64_to_key(keys[thread_id - 1]),
				   [&](string_view value) {
					   UT_ASSERT(value.compare(uint64_to_key(
							     keys[thread_id - 1])) == 0);
				   });
			UT_ASSERT(s == status::OK || s == status::NOT_FOUND);

			if (thread_id == threads_number - 1)
				return;

			s = kv.get(uint64_to_key(keys[thread_id + 1]),
				   [&](string_view value) {
					   UT_ASSERT(value.compare(uint64_to_key(
							     keys[thread_id + 1])) == 0);
				   });
			UT_ASSERT(s == status::OK || s == status::NOT_FOUND);
		} else {
			UT_ASSERTeq(kv.remove(uint64_to_key(keys[thread_id])),
				    status::OK);
		}
	});
}

static void MultithreadedTestRemoveDataAside2(pmem::kv::db &kv)
{
	static constexpr uint64_t size = 1000;
	size_t threads_number = size;

	uint64_t keys[size] = {};
	std::iota(keys, keys + size, 0);

	auto uint64_to_key = [](uint64_t &key) {
		return string_view((char *)&key, sizeof(uint64_t));
	};

	for (int i = 0; i < size; i += 2)
		UT_ASSERT(kv.put(uint64_to_key(keys[i]), uint64_to_key(keys[i])) ==
			  status::OK);

	/* test adding and removing data */
	parallel_exec(threads_number, [&](size_t thread_id) {
		if (thread_id % 2 == 0) {
			UT_ASSERTeq(kv.remove(uint64_to_key(keys[thread_id])),
				    status::OK);
		} else {
			UT_ASSERT(kv.put(uint64_to_key(keys[thread_id]),
					 uint64_to_key(keys[thread_id])) == status::OK);
		}
	});
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	run_engine_tests(argv[1], argv[2],
			 {SimpleMultithreadedTest, MultithreadedTestRemoveDataAside,
			  MultithreadedTestRemoveDataAside2});
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
