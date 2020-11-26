// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * pmemkv_iterator.cpp -- example usage of pmemkv.
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

static db *init_kv(const std::string &engine, const std::string &path, size_t n_elements)
{
	/* See libpmemkv_config(3) for more detailed example of config creation */
	LOG("Creating config");
	config cfg;

	status s = cfg.put_path(path);
	ASSERT(s == status::OK);
	s = cfg.put_size(SIZE);
	ASSERT(s == status::OK);
	s = cfg.put_force_create(true);
	ASSERT(s == status::OK);

	LOG("Opening pmemkv database with " + engine + " engine");
	db *kv = new db();
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

static std::string read_key(pmem::kv::db::read_iterator &it)
{
	/* key_result's type is a pmem::kv::result<string_view>, for more informations
	 * check pmem::kv::result's documentation */
	auto key_result = it.key();
	/* check if the result is ok, alternatively you can do:
	 * key_result == pmem::kv::status::OK
	 * or
	 * key_result.get_status() == pmem::kv::status::OK */
	ASSERT(key_result.is_ok());

	return std::string(key_result.get_value().data());
}

static std::string read_value(pmem::kv::db::read_iterator &it)
{
	/* val_result's type is a pmem::kv::result<string_view>, for more informations
	 * check pmem::kv::result's documentation */
	auto val_result = it.read_range();
	/* check if the result is ok, alternatively you can do:
	 * val_result == pmem::kv::status::OK
	 * or
	 * val_result.get_status() == pmem::kv::status::OK */
	ASSERT(val_result.is_ok());

	return std::string(val_result.get_value().data());
}

static void single_threaded_engine_example(const std::string &path)
{
	const size_t n_elements = 10;
	/* init radix engine */
	auto kv = init_kv("radix", path + "_radix", n_elements);

	/* get a new read iterator */
	auto res_it = kv->new_read_iterator();
	/* you need to take reference to the iterator from the result, because iterator
	 * isn't copyable */
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

	/* get a new write iterator */
	auto res_w_it = kv->new_write_iterator();
	/* you need to take reference to the iterator from the result, because iterator
	 * isn't copyable */
	auto &w_it = res_w_it.get_value();

	LOG("Modify value of the element lower than \"5\"");
	/* seek to the element lower than "5" */
	s = w_it.seek_lower("5");
	ASSERT(s == status::OK);

	/* read a value before writting */
	std::string value_before_write = w_it.read_range().get_value().data();

	/* Get a write range. Default it is a whole value (pos = 0, n =
	 * std::numeric_limits<size_t>::max()). */
	auto res = w_it.write_range();
	ASSERT(res.is_ok());

	/* set all chars to 'x' */
	for (auto &c : res.get_value()) {
		/* note that you can't read elements from write_range, so f.e. char x = c
		 * isn't possible, if you want to read a value, you need to use read_range
		 * method instead of write_range */
		c = 'x';
	}

	/* note that changes aren't visible until commit */
	auto value_during_write = w_it.read_range().get_value().data();
	ASSERT(value_before_write.compare(value_during_write) == 0);
	LOG("Value before commit = " + value_before_write);

	/* now modification will be visible */
	w_it.commit();

	std::string current_value = w_it.read_range().get_value().data();
	/* check if a value has changed */
	ASSERT(current_value.compare(value_before_write) != 0);
	ASSERT(current_value.compare(std::string(current_value.size(), 'x')) == 0);
	LOG("Value after commit = " + current_value);

	LOG("Closing database");
	delete kv;
}

static void concurrent_engine_example(const std::string &path)
{
	const size_t n_elements = 20;
	/* init csmap engine */
	auto kv = init_kv("csmap", path + "_csmap", n_elements);

	/* create 2 threads, in first thread iterate from begin to the element with key
	 * equal "5", in second from element with key equal "5" to the end */
	std::vector<std::thread> threads;

	/* thread1 */
	threads.emplace_back([&]() {
		auto res_it = kv->new_read_iterator();
		auto &it = res_it.get_value();
		it.seek_to_first();
		/* normal order */
		do {
			/* read a key */
			auto key = read_key(it);
			LOG("Key (from thread1) = " + key);
		} while (it.next() == pmem::kv::status::OK &&
			 std::string("5").compare(read_key(it)) != 0);
	});

	/* thread2 */
	threads.emplace_back([&]() {
		auto res_it = kv->new_read_iterator();
		auto &it = res_it.get_value();
		it.seek("5");
		/* reversed order */
		do {
			/* read a key */
			auto key = read_key(it);
			LOG("Key (from thread2) = " + key);
		} while (it.next() == pmem::kv::status::OK);
	});

	for (auto &th : threads)
		th.join();

	LOG("Closing database");
	delete kv;
}

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
