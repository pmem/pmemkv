// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

/*
 * pmemkv_iterator.cpp -- example of pmemkv's iterators.
 *		Two usages, to show: single-threaded and concurrent approach.
 */

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <libpmemkv.hpp>
#include <thread>
#include <vector>

#define ASSERT(expr)                                                                     \
	do {                                                                             \
		if (!(expr))                                                             \
			std::cout << pmemkv_errormsg() << std::endl;                     \
		assert(expr);                                                            \
	} while (0)
#define LOG(msg) std::cout << msg << std::endl

using namespace pmem::kv;

const uint64_t SIZE = 1024UL * 1024UL * 1024UL;

static std::unique_ptr<db> init_kv(const std::string &engine, const std::string &path,
				   size_t n_elements)
{
	/* See libpmemkv_config(3) for more detailed example of config creation */
	LOG("Creating a new config");
	config cfg;

	status s = cfg.put_path(path);
	ASSERT(s == status::OK);
	s = cfg.put_size(SIZE);
	ASSERT(s == status::OK);
	s = cfg.put_create_if_missing(true);
	ASSERT(s == status::OK);

	LOG("Opening pmemkv database with " + engine + " engine");
	auto kv = std::unique_ptr<db>(new db());
	ASSERT(kv != nullptr);
	s = kv->open(engine, std::move(cfg));
	ASSERT(s == status::OK);

	LOG("Putting new keys");
	for (size_t i = 0; i < n_elements; i++) {
		s = kv->put(std::to_string(i), std::string(10 + i, 'a'));
		ASSERT(s == status::OK);
	}

	return kv;
}

//! [single-threaded]
static std::string read_key(pmem::kv::db::read_iterator &it)
{
	/* key_result's type is a pmem::kv::result<string_view>, for more information
	 * check pmem::kv::result documentation */
	pmem::kv::result<string_view> key_result = it.key();
	/* check if the result is ok, you can also do:
	 * key_result == pmem::kv::status::OK
	 * or
	 * key_result.get_status() == pmem::kv::status::OK */
	ASSERT(key_result.is_ok());

	return key_result.get_value().data();
}

static std::string read_value(pmem::kv::db::read_iterator &it)
{
	/* val_result's type is a pmem::kv::result<string_view>, for more information
	 * check pmem::kv::result documentation */
	pmem::kv::result<string_view> val_result = it.read_range();
	/* check if the result is ok, you can also do:
	 * val_result == pmem::kv::status::OK
	 * or
	 * val_result.get_status() == pmem::kv::status::OK */
	ASSERT(val_result.is_ok());

	return val_result.get_value().data();
}

static void single_threaded_engine_example(const std::string &path)
{
	const size_t n_elements = 10;
	/* init radix engine */
	auto kv = init_kv("radix", path + "_radix", n_elements);

	/* We shouldn't hold simultaneously in the same thread more than one iterator. So
	 * every iterator in this example will be in a separate scope */
	{
		/* get a new read iterator */
		auto res_it = kv->new_read_iterator();
		/* you should check if the result is ok before getting a value */
		ASSERT(res_it.is_ok());
		/* you need to take reference to the iterator from the result, because
		 * iterator isn't copyable */
		auto &it = res_it.get_value();

		LOG("Iterate from first to last element");
		auto s = it.seek_to_first();
		size_t cnt = 0;
		ASSERT(s == status::OK);
		do {
			/* read a key */
			auto key = read_key(it);
			ASSERT(key == std::to_string(cnt));
			LOG("Key = " + key);

			/* read a value */
			auto value = read_value(it);
			ASSERT(value == std::string(10 + cnt, 'a'));
			LOG("Value = " + value);

			++cnt;
		} while (it.next() == status::OK);

		LOG("Iterate from last to first element");
		s = it.seek_to_last();
		cnt = n_elements - 1;
		ASSERT(s == status::OK);
		do {
			/* read a key */
			auto key = read_key(it);
			ASSERT(key == std::to_string(cnt));
			LOG("Key = " + key);

			/* read a value */
			auto value = read_value(it);
			ASSERT(value == std::string(10 + cnt, 'a'));
			LOG("Value = " + value);

			--cnt;
		} while (it.prev() == status::OK);

		/* read iterator is being destroyed now */
	}

	/* scope for a write iterator */
	{
		/* get a new write iterator */
		auto res_w_it = kv->new_write_iterator();
		/* you should check if the result is ok before getting a value */
		ASSERT(res_w_it.is_ok());
		/* you need to take reference to the iterator from the result, because
		 * iterator isn't copyable */
		auto &w_it = res_w_it.get_value();

		LOG("Modify value of the elements lower than \"5\"");
		/* seek to the element lower than "5" */
		status s = w_it.seek_lower("5");
		ASSERT(s == status::OK);
		do {
			/* read a value before writing */
			std::string value_before_write =
				w_it.read_range().get_value().data();

			/* Get a write range. By default it is a whole value (pos = 0, n =
			 * std::numeric_limits<size_t>::max()). */
			auto res = w_it.write_range();
			ASSERT(res.is_ok());

			/* set all chars to 'x' */
			for (auto &c : res.get_value()) {
				/* note that you can't read elements from write_range, so
				 * e.g. char x = c isn't possible. If you want to read a
				 * value, you need to use read_range method instead of
				 * write_range */
				c = 'x';
			}

			/* commit modifications */
			w_it.commit();

			pmem::kv::result<string_view> key_result = w_it.key();
			ASSERT(key_result.is_ok());
			std::string current_key = key_result.get_value().data();
			LOG("Key = " + current_key);

			std::string current_value = w_it.read_range().get_value().data();
			/* check if the value has changed */
			ASSERT(current_value.compare(value_before_write) != 0);
			ASSERT(current_value.compare(
				       std::string(current_value.size(), 'x')) == 0);
			LOG("Value after commit = " + current_value);
		} while (w_it.prev() == status::OK);
		/* write iterator is being destroyed now */
	}
}
//! [single-threaded]

//! [concurrent]
static void concurrent_engine_example(const std::string &path)
{
	const size_t n_elements = 20;
	/* init csmap engine */
	auto kv = init_kv("csmap", path + "_csmap", n_elements);

	/* create 2 threads, in first thread iterate from the beginning to the element
	 * with key equal "5", in second from element with key equal "5" to the end */
	std::vector<std::thread> threads;

	/* thread1 */
	threads.emplace_back([&]() {
		auto res_it = kv->new_read_iterator();
		ASSERT(res_it.is_ok());
		auto &it = res_it.get_value();
		it.seek_to_first();
		do {
			/* read a key */
			auto key = read_key(it);
			LOG("Key (from thread1) = " + key);
		} while (it.next() == pmem::kv::status::OK && read_key(it).compare("5"));
	});

	/* thread2 */
	threads.emplace_back([&]() {
		auto res_it = kv->new_read_iterator();
		ASSERT(res_it.is_ok());
		auto &it = res_it.get_value();
		it.seek("5");
		do {
			/* read a key */
			auto key = read_key(it);
			LOG("Key (from thread2) = " + key);
		} while (it.next() == pmem::kv::status::OK);
	});

	for (auto &th : threads)
		th.join();
}
//! [concurrent]

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " file" << std::endl;
		exit(1);
	}

	single_threaded_engine_example(argv[1]);
	concurrent_engine_example(argv[1]);

	return 0;
}
