// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2021, Intel Corporation */

/*
 * pmemkv_basic.cpp -- example of basic pmemkv usage.
 */

//! [basic]
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

	LOG("Opening pmemkv database with 'cmap' engine");
	db *kv = new db();
	ASSERT(kv != nullptr);
	s = kv->open("cmap", std::move(cfg));
	ASSERT(s == status::OK);

	LOG("Putting new key");
	s = kv->put("key1", "value1");
	ASSERT(s == status::OK);

	size_t cnt;
	s = kv->count_all(cnt);
	ASSERT(s == status::OK && cnt == 1);

	LOG("Reading key back");
	std::string value;
	s = kv->get("key1", &value);
	ASSERT(s == status::OK && value == "value1");

	LOG("Iterating existing keys");
	s = kv->put("key2", "value2");
	ASSERT(s == status::OK);
	s = kv->put("key3", "value3");
	ASSERT(s == status::OK);
	kv->get_all([](string_view k, string_view v) {
		LOG("  visited: " << k.data());
		return 0;
	});

	LOG("Defragmenting the database");
	s = kv->defrag(0, 100);
	ASSERT(s == status::OK);

	LOG("Removing existing key");
	s = kv->remove("key1");
	ASSERT(s == status::OK);
	s = kv->exists("key1");
	ASSERT(s == status::NOT_FOUND);

	/* Examples of using pmem:kv:status with std::ostream and operator<<,
	 * it's useful for debugging. */
	/* Print status */
	std::cout << s << std::endl;

	/* Write status to ostringstream */
	std::ostringstream oss;
	oss << s;
	assert(oss.str() == "NOT_FOUND (2)");

	LOG("Closing database");
	delete kv;

	return 0;
}
//! [basic]
