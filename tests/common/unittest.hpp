// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

#ifndef PMEMKV_UNITTEST_HPP
#define PMEMKV_UNITTEST_HPP

#include "unittest.h"

#include <libpmemkv.h>
#include <libpmemkv.hpp>
#ifdef JSON_TESTS_SUPPORT
#include <libpmemkv_json_config.h>
#endif

#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

static inline void UT_EXCEPTION(std::exception &e)
{
	std::cerr << e.what() << std::endl;
}

/* assertion with exception related string printed */
#define UT_FATALexc(exception)                                                           \
	((void)(UT_EXCEPTION(exception),                                                 \
		(UT_FATAL("%s:%d %s - assertion failure", __FILE__, __LINE__, __func__), \
		 0)))

#ifdef _WIN32
#define __PRETTY_FUNCTION__ __func__
#endif
#define PRINT_TEST_PARAMS                                                                \
	do {                                                                             \
		std::cout << "TEST: " << __PRETTY_FUNCTION__ << std::endl;               \
	} while (0)

#define STAT(path, st) ut_stat(__FILE__, __LINE__, __func__, path, st)

#define UT_ASSERT_NOEXCEPT(...)                                                          \
	static_assert(noexcept(__VA_ARGS__), "Operation must be noexcept")

#define STR(x) #x

#define ASSERT_ALIGNED_BEGIN(type, ref)                                                  \
	do {                                                                             \
		size_t off = 0;                                                          \
		const char *last = "(none)";

#define ASSERT_ALIGNED_FIELD(type, ref, field)                                               \
	do {                                                                                 \
		if (offsetof(type, field) != off)                                            \
			UT_FATAL(                                                            \
				"%s: padding, missing field or fields not in order between " \
				"'%s' and '%s' -- offset %lu, real offset %lu",              \
				STR(type), last, STR(field), off,                            \
				offsetof(type, field));                                      \
		off += sizeof((ref).field);                                                  \
		last = STR(field);                                                           \
	} while (0)

#define ASSERT_FIELD_SIZE(field, ref, size)                                              \
	do {                                                                             \
		UT_COMPILE_ERROR_ON(size != sizeof((ref).field));                        \
	} while (0)

#define ASSERT_ALIGNED_CHECK(type)                                                       \
	if (off != sizeof(type))                                                         \
		UT_FATAL("%s: missing field or padding after '%s': "                     \
			 "sizeof(%s) = %lu, fields size = %lu",                          \
			 STR(type), last, STR(type), sizeof(type), off);                 \
	}                                                                                \
	while (0)

#define ASSERT_OFFSET_CHECKPOINT(type, checkpoint)                                       \
	do {                                                                             \
		if (off != checkpoint)                                                   \
			UT_FATAL("%s: violated offset checkpoint -- "                    \
				 "checkpoint %lu, real offset %lu",                      \
				 STR(type), checkpoint, off);                            \
	} while (0)

static inline int run_test(std::function<void()> test)
{
	test_register_sighandlers();

	try {
		test();
	} catch (std::exception &e) {
		UT_FATALexc(e);
	} catch (...) {
		UT_FATAL("catch(...){}");
	}

	return 0;
}

template <typename Function>
void parallel_exec(size_t threads_number, Function f)
{
	std::vector<std::thread> threads;
	threads.reserve(threads_number);

	for (size_t i = 0; i < threads_number; ++i) {
		threads.emplace_back(f, i);
	}

	for (auto &t : threads) {
		t.join();
	}
}

/*
 * This function executes 'concurrency' threads and provides
 * 'syncthreads' method (synchronization barrier) for f()
 */
template <typename Function>
void parallel_xexec(size_t concurrency, Function f)
{
	std::condition_variable cv;
	std::mutex m;
	size_t counter = 0;

	auto syncthreads = [&] {
		std::unique_lock<std::mutex> lock(m);
		counter++;
		if (counter < concurrency)
			cv.wait(lock);
		else
			/*
			 * notify_call could be called outside of a lock
			 * (it would perform better) but drd complains
			 * in that case
			 */
			cv.notify_all();
	};

	parallel_exec(concurrency, [&](size_t tid) { f(tid, syncthreads); });
}

#ifdef JSON_TESTS_SUPPORT
pmem::kv::config CONFIG_FROM_JSON(std::string json)
{
	return pmem::kv::config(C_CONFIG_FROM_JSON(json.c_str()));
}
#endif /* JSON_TESTS_SUPPORT */

pmem::kv::db INITIALIZE_KV(std::string engine, pmem::kv::config &&config)
{
	pmem::kv::db kv;
	auto s = kv.open(engine, std::move(config));
	UT_ASSERTeq(s, pmem::kv::status::OK);

	return kv;
}

/* XXX - replace with call to actual clear() method once implemented */
void CLEAR_KV(pmem::kv::db &kv)
{
	std::vector<std::string> keys;
	kv.get_all([&](pmem::kv::string_view key, pmem::kv::string_view value) {
		keys.emplace_back(key.data(), key.size());
		return 0;
	});

	for (auto &k : keys)
		kv.remove(k);
}

#ifdef JSON_TESTS_SUPPORT
static inline int run_engine_tests(std::string engine, std::string json,
				   std::vector<std::function<void(pmem::kv::db &)>> tests)
{
	test_register_sighandlers();

	try {
		auto kv = INITIALIZE_KV(engine, CONFIG_FROM_JSON(json));

		for (auto &test : tests) {
			test(kv);
			CLEAR_KV(kv);
		}
	} catch (std::exception &e) {
		UT_FATALexc(e);
	} catch (...) {
		UT_FATAL("catch(...){}");
	}

	return 0;
}
#endif /* JSON_TESTS_SUPPORT */

static inline pmem::kv::string_view uint64_to_strv(uint64_t &key)
{
	return pmem::kv::string_view((char *)&key, sizeof(uint64_t));
}

#endif /* PMEMKV_UNITTEST_HPP */
