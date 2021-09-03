// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2021, Intel Corporation */

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
#include <sstream>
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

#define ASSERT_UNREACHABLE                                                               \
	do {                                                                             \
		UT_FATAL("%s:%d in function %s should never be reached", __FILE__,       \
			 __LINE__, __func__);                                            \
	} while (0)

#define ASSERT_STATUS(status, expected_status)                                           \
	do {                                                                             \
		auto current_status = status;                                            \
		UT_ASSERTeq(current_status, expected_status);                            \
		std::string expected_string = #expected_status;                          \
		expected_string.erase(0, expected_string.rfind(":") + 1);                \
		expected_string +=                                                       \
			" (" + std::to_string(static_cast<int>(expected_status)) + ")";  \
		std::ostringstream oss;                                                  \
		oss << current_status;                                                   \
		if (oss.str() != expected_string)                                        \
			UT_FATAL("%s:%d %s - wrong status message (%s), should be: %s",  \
				 __FILE__, __LINE__, __func__, oss.str().c_str(),        \
				 expected_string.c_str());                               \
	} while (0)

#define ASSERT_SIZE(kv, expected_size)                                                   \
	do {                                                                             \
		size_t cnt;                                                              \
		ASSERT_STATUS(kv.count_all(cnt), pmem::kv::status::OK);                  \
		UT_ASSERTeq(cnt, expected_size);                                         \
	} while (0)

static std::string currently_tested;

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
	currently_tested = engine;

	pmem::kv::db kv;
	auto s = kv.open(engine, std::move(config));
	ASSERT_STATUS(s, pmem::kv::status::OK);

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
		ASSERT_STATUS(kv.remove(k), pmem::kv::status::OK);
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

/* It creates a string that contains 8 bytes from uint64_t. */
static inline std::string uint64_to_string(uint64_t &key)
{
	return std::string(reinterpret_cast<const char *>(&key), 8);
}

/* It aligns entry to a certain size. It is useful for testing engines with fixed-size
 * keys. */
static inline std::string align_to_size(size_t size, std::string str,
					char char_to_fill = 'x')
{
	if (str.size() > size) {
		UT_FATAL((str + " - too long entry for the engine: " + currently_tested)
				 .c_str());
	}

	return str + std::string(size - str.size(), char_to_fill);
}

/* It returns an entry filled/shrank to a certain size. It's necessary during testing
 * engines with fixed size keys/values */
static inline std::string entry_from_string(std::string str)
{
	if (currently_tested == "robinhood")
		return align_to_size(8, str);

	return str;
}

static inline std::string entry_from_number(size_t number, std::string prefix = "",
					    std::string postfix = "")
{
	std::string res = prefix + std::to_string(number) + postfix;

	return entry_from_string(res);
}

#endif /* PMEMKV_UNITTEST_HPP */
