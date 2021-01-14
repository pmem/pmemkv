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

/* fixed size only for robinhood */
static const size_t min_len = 8;
static const size_t max_len = 8;

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

#define ASSERT_VALUE(result, expected)                                                   \
	do {                                                                             \
		auto expected_str = std::string(expected);                               \
                                                                                         \
		if (expected_str.size() < min_len)                                       \
			expected_str +=                                                  \
				std::string(min_len - expected_str.size(), '\0');        \
		else if (expected_str.size() > max_len)                                  \
			expected_str = expected_str.substr(0, max_len);                  \
                                                                                         \
		UT_ASSERT(result.compare(expected_str) == 0);                            \
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

class kv_to_test : public pmem::kv::db {
public:
	kv_to_test(std::string engine, pmem::kv::config &&config)
	{
		ASSERT_STATUS(pmem::kv::db::open(engine, std::move(config)),
			      pmem::kv::status::OK);
	}

	pmem::kv::status count_above(pmem::kv::string_view key, std::size_t &cnt) noexcept
	{
		return pmem::kv::db::count_above(prepare_string(key), cnt);
	}

	pmem::kv::status count_equal_above(pmem::kv::string_view key,
					   std::size_t &cnt) noexcept
	{
		return pmem::kv::db::count_equal_above(prepare_string(key), cnt);
	}

	pmem::kv::status count_equal_below(pmem::kv::string_view key,
					   std::size_t &cnt) noexcept
	{
		return pmem::kv::db::count_equal_below(prepare_string(key), cnt);
	}

	pmem::kv::status count_below(pmem::kv::string_view key, std::size_t &cnt) noexcept
	{
		return pmem::kv::db::count_below(prepare_string(key), cnt);
	}

	pmem::kv::status count_between(pmem::kv::string_view key1,
				       pmem::kv::string_view key2,
				       std::size_t &cnt) noexcept
	{
		return pmem::kv::db::count_between(prepare_string(key1),
						   prepare_string(key2), cnt);
	}

	pmem::kv::status get_above(pmem::kv::string_view key,
				   pmem::kv::get_kv_callback *callback,
				   void *arg) noexcept
	{
		return pmem::kv::db::get_above(prepare_string(key), callback, arg);
	}

	pmem::kv::status get_above(pmem::kv::string_view key,
				   std::function<pmem::kv::get_kv_function> f) noexcept
	{
		return pmem::kv::db::get_above(prepare_string(key), f);
	}

	pmem::kv::status get_equal_above(pmem::kv::string_view key,
					 pmem::kv::get_kv_callback *callback,
					 void *arg) noexcept
	{
		return pmem::kv::db::get_equal_above(prepare_string(key), callback, arg);
	}

	pmem::kv::status
	get_equal_above(pmem::kv::string_view key,
			std::function<pmem::kv::get_kv_function> f) noexcept
	{
		return pmem::kv::db::get_equal_above(prepare_string(key), f);
	}

	pmem::kv::status get_equal_below(pmem::kv::string_view key,
					 pmem::kv::get_kv_callback *callback,
					 void *arg) noexcept
	{
		return pmem::kv::db::get_equal_below(prepare_string(key), callback, arg);
	}

	pmem::kv::status
	get_equal_below(pmem::kv::string_view key,
			std::function<pmem::kv::get_kv_function> f) noexcept
	{
		return pmem::kv::db::get_equal_below(prepare_string(key), f);
	}

	pmem::kv::status get_below(pmem::kv::string_view key,
				   pmem::kv::get_kv_callback *callback,
				   void *arg) noexcept
	{
		return pmem::kv::db::get_below(prepare_string(key), callback, arg);
	}

	pmem::kv::status get_below(pmem::kv::string_view key,
				   std::function<pmem::kv::get_kv_function> f) noexcept
	{
		return pmem::kv::db::get_below(prepare_string(key), f);
	}

	pmem::kv::status get_between(pmem::kv::string_view key1,
				     pmem::kv::string_view key2,
				     pmem::kv::get_kv_callback *callback,
				     void *arg) noexcept
	{
		return pmem::kv::db::get_between(prepare_string(key1),
						 prepare_string(key2), callback, arg);
	}

	pmem::kv::status get_between(pmem::kv::string_view key1,
				     pmem::kv::string_view key2,
				     std::function<pmem::kv::get_kv_function> f) noexcept
	{
		return pmem::kv::db::get_between(prepare_string(key1),
						 prepare_string(key2), f);
	}

	pmem::kv::status exists(pmem::kv::string_view key) noexcept
	{
		return pmem::kv::db::exists(prepare_string(key));
	}

	pmem::kv::status get(pmem::kv::string_view key,
			     pmem::kv::get_v_callback *callback, void *arg) noexcept
	{
		return pmem::kv::db::get(prepare_string(key), callback, arg);
	}

	pmem::kv::status get(pmem::kv::string_view key,
			     std::function<pmem::kv::get_v_function> f) noexcept
	{
		return pmem::kv::db::get(prepare_string(key), f);
	}

	pmem::kv::status get(pmem::kv::string_view key, std::string *value) noexcept
	{
		return pmem::kv::db::get(prepare_string(key), value);
	}

	pmem::kv::status put(pmem::kv::string_view key,
			     pmem::kv::string_view value) noexcept
	{
		return pmem::kv::db::put(prepare_string(key), prepare_string(value));
	}

	pmem::kv::status remove(pmem::kv::string_view key) noexcept
	{
		return pmem::kv::db::remove(prepare_string(key));
	}

private:
	std::string prepare_string(pmem::kv::string_view &str)
	{
		auto ret = std::string{str.data(), str.size()};

		if (min_len == std::numeric_limits<size_t>::max() ||
		    max_len == std::numeric_limits<size_t>::max())
			return ret;

		if (ret.size() < min_len)
			ret += std::string(min_len - ret.size(), '\0');
		else if (ret.size() > max_len)
			ret = ret.substr(0, max_len);

		return ret;
	}
};

pmem::kv::db INITIALIZE_KV(std::string engine, pmem::kv::config &&config)
{
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

void CLEAR_KV(kv_to_test &kv)
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
				   std::vector<std::function<void(kv_to_test &)>> tests)
{
	test_register_sighandlers();

	try {
		auto kv = kv_to_test{engine, CONFIG_FROM_JSON(json)};

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
