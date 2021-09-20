// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/*
 * pmemkv_fill.cpp -- example which calculates how many elements fit
 * into pmemkv. It inserts elements with specified key and value size
 * to the database until OUT_OF_MEMORY status is returned. It then prints
 * number of elements inserted. It may be used to observe the memory overhead
 * of a certain engine with specific key/value sizes.
 */

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <libpmemkv.hpp>
#include <sstream>

#define ASSERT(expr)                                                                     \
	do {                                                                             \
		if (!(expr))                                                             \
			std::cout << pmemkv_errormsg() << std::endl;                     \
		assert(expr);                                                            \
	} while (0)
#define LOG(msg) std::cout << msg << std::endl

using namespace pmem::kv;

size_t insert_till_oom(db *kv, size_t key_size, size_t value_size)
{
	size_t num = 0;
	std::string v(value_size, 'x');
	status s;

	do {
		if (num % 100000 == 0)
			std::cout << "Inserting " << num << "th key..." << std::endl;

		std::string k((char *)&num, sizeof(uint64_t));
		if (k.size() < key_size)
			k += std::string(key_size - k.size(), 'x');

		s = kv->put(k, v);
		num++;
	} while (s == status::OK);

	ASSERT(s == status::OUT_OF_MEMORY);

	return num;
}

int main(int argc, char *argv[])
{
	if (argc < 6) {
		std::cerr << "Usage: " << argv[0]
			  << " file size engine key_size value_size" << std::endl;
		exit(1);
	}

	std::string path = argv[1];
	size_t size = std::stoull(std::string(argv[2]));
	std::string engine = argv[3];
	size_t key_size = std::stoull(std::string(argv[4]));
	size_t value_size = std::stoull(std::string(argv[5]));

	if (key_size < 8) {
		std::cerr << "Key size must be at least 8 bytes" << std::endl;
		exit(1);
	}

	/* See libpmemkv_config(3) for more detailed example of config creation */
	config cfg;

	status s = cfg.put_path(path);
	ASSERT(s == status::OK);
	s = cfg.put_size(size);
	ASSERT(s == status::OK);
	s = cfg.put_create_if_missing(true);
	ASSERT(s == status::OK);

	/* Alternatively create_or_error_if_exists flag can be set, to fail if file exists
	 * For differences between the two flags, see e.g. libpmemkv(7) manpage. */
	/* s = cfg.put_create_or_error_if_exists(true); */

	db kv;
	s = kv.open(engine, std::move(cfg));
	ASSERT(s == status::OK);

	auto elements = insert_till_oom(&kv, key_size, value_size);

	std::cout << "Number of elements: " << elements << std::endl;

	LOG("Closing database");
	return 0;
}
