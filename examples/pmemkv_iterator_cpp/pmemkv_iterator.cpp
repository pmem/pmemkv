// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

/*
 * pmemkv_iterator.cpp -- example usage of pmemkv.
 */

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <libpmemkv.hpp>

#define ASSERT(expr)                                                                     \
	do {                                                                             \
		if (!(expr))                                                             \
			std::cout << pmemkv_errormsg() << std::endl;                     \
		assert(expr);                                                            \
	} while (0)
#define LOG(msg) std::cout << msg << std::endl

using namespace pmem::kv;

const uint64_t SIZE = 1024UL * 1024UL * 1024UL;

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " file\n";
		exit(1);
	}

	/* See libpmemkv_config(3) for more detailed example of config creation */
	LOG("Creating config");
	config cfg;

	status s = cfg.put_path(argv[1]);
	ASSERT(s == status::OK);
	s = cfg.put_size(SIZE);
	ASSERT(s == status::OK);
	s = cfg.put_force_create(true);
	ASSERT(s == status::OK);

	LOG("\nOpening pmemkv database with 'radix' engine");
	db *kv = new db();
	ASSERT(kv != nullptr);
	s = kv->open("radix", std::move(cfg));
	ASSERT(s == status::OK);

	LOG("\nPutting new keys");
	for (size_t i = 0; i < 10; i++) {
		s = kv->put(std::to_string(i), std::string(10 + i, 'a'));
		ASSERT(s == status::OK);
	}

	/* get new read iterator */
	auto it = kv->new_read_iterator();

	LOG("\nIterate from first to last element");
	s = it.seek_to_first();
	ASSERT(s == status::OK);
	size_t cnt = 0;
	do {
		/* read key */
		auto key_result = it.key();
		ASSERT(key_result.second == status::OK);
		auto key_string = std::string(key_result.first.data());
		ASSERT(key_string == std::to_string(cnt));
		LOG("Key = " + key_string);

		/* read value */
		/* to read whole value, pass max size_t as n */
		auto val_result = it.read_range(0, std::numeric_limits<size_t>::max());
		ASSERT(val_result.second == status::OK);
		auto val_string = std::string(val_result.first.begin());
		ASSERT(val_string == std::string(10 + cnt, 'a'));
		LOG("Value = " + val_string);

		++cnt;
	} while (it.next() == status::OK);

	--cnt;

	LOG("\nIterate from last to first element");
	s = it.seek_to_last();
	ASSERT(s == status::OK);
	do {
		/* read key */
		auto key_result = it.key();
		ASSERT(key_result.second == status::OK);
		auto key_string = std::string(key_result.first.data());
		ASSERT(key_string == std::to_string(cnt));
		LOG("Key = " + key_string);

		/* read value */
		/* to read whole value, pass max size_t as n */
		auto val_result = it.read_range(0, std::numeric_limits<size_t>::max());
		ASSERT(val_result.second == status::OK);
		auto val_string = std::string(val_result.first.begin());
		ASSERT(val_string == std::string(10 + cnt, 'a'));
		LOG("Value = " + val_string);

		--cnt;
	} while (it.prev() == status::OK);

	/* get new write iterator */
	auto w_it = kv->new_write_iterator();

	LOG("\nModify value of element lower than \"5\"");
	s = w_it.seek_lower("5");
	ASSERT(s == status::OK);

	std::string value_before_write =
		w_it.read_range(0, std::numeric_limits<size_t>::max()).first.begin();

	/* get write range, to obtain whole value's range, pass max size_t as n */
	auto res = w_it.write_range(0, std::numeric_limits<size_t>::max());
	ASSERT(res.second == status::OK);

	/* set all chars to 'x' */
	for (auto &c : res.first)
		c = 'x';

	/* note that changes aren't visible until commit */
	auto read_res = w_it.read_range(0, std::numeric_limits<size_t>::max());
	ASSERT(read_res.first.begin() == value_before_write);
	LOG("Value before commit = " + value_before_write);

	/* now modification will be visible */
	w_it.commit();

	read_res = w_it.read_range(0, std::numeric_limits<size_t>::max());
	auto current_value = std::string(read_res.first.begin());
	ASSERT(current_value != value_before_write);
	ASSERT(current_value == std::string(read_res.first.size(), 'x'));
	LOG("Value after commit = " + current_value);

	LOG("\nClosing database");
	delete kv;

	return 0;
}
