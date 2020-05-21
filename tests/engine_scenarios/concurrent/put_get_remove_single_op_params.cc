// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

#include <numeric>

using namespace pmem::kv;

static void MultithreadedGetAndRemove(const size_t threads_number, pmem::kv::db &kv)
{
	std::vector<uint64_t> keys(threads_number, 0);
	std::iota(keys.begin(), keys.end(), 0);

	for (auto &k : keys)
		UT_ASSERT(kv.put(uint64_to_strv(k), uint64_to_strv(k)) == status::OK);

	/* test reading and removing data */
	parallel_exec(threads_number, [&](size_t thread_id) {
		if (thread_id % 2 == 1) {
			auto s = kv.get(uint64_to_strv(keys[thread_id]),
					[&](string_view value) {
						UT_ASSERTeq(value.compare(uint64_to_strv(
								    keys[thread_id])),
							    0);
					});
			UT_ASSERTeq(s, status::OK);

			s = kv.get(uint64_to_strv(keys[thread_id - 1]),
				   [&](string_view value) {
					   UT_ASSERTeq(value.compare(uint64_to_strv(
							       keys[thread_id - 1])),
						       0);
				   });
			UT_ASSERT(s == status::OK || s == status::NOT_FOUND);

			if (thread_id == threads_number - 1)
				return;

			s = kv.get(uint64_to_strv(keys[thread_id + 1]),
				   [&](string_view value) {
					   UT_ASSERTeq(value.compare(uint64_to_strv(
							       keys[thread_id + 1])),
						       0);
				   });
			UT_ASSERT(s == status::OK || s == status::NOT_FOUND);
		} else {
			UT_ASSERTeq(kv.remove(uint64_to_strv(keys[thread_id])),
				    status::OK);
		}
	});
}

static void MultithreadedPutAndRemove(const size_t threads_number, pmem::kv::db &kv)
{
	std::vector<uint64_t> keys(threads_number, 0);
	std::iota(keys.begin(), keys.end(), 0);

	for (size_t i = 0; i < threads_number; i += 2)
		UT_ASSERTeq(kv.put(uint64_to_strv(keys[i]), uint64_to_strv(keys[i])),
			    status::OK);

	/* test adding and removing data */
	parallel_exec(threads_number, [&](size_t thread_id) {
		if (thread_id % 2 == 0) {
			UT_ASSERTeq(kv.remove(uint64_to_strv(keys[thread_id])),
				    status::OK);
		} else {
			UT_ASSERTeq(kv.put(uint64_to_strv(keys[thread_id]),
					   uint64_to_strv(keys[thread_id])),
				    status::OK);
		}
	});
}

static void test(int argc, char *argv[])
{
	using namespace std::placeholders;

	if (argc < 4)
		UT_FATAL("usage: %s engine json_config threads", argv[0]);

	size_t threads_number = std::stoull(argv[3]);
	run_engine_tests(argv[1], argv[2],
			 {
				 std::bind(MultithreadedGetAndRemove, threads_number, _1),
				 std::bind(MultithreadedPutAndRemove, threads_number, _1),
			 });
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
