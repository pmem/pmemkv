// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2021, Intel Corporation */

/*
 * pmemkv_open.cpp -- example usage of pmemkv with already existing pools.
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

//! [open]
/*
 * This example expects a path to already created database pool.
 *
 * If you want to create pool, use one of the following commands.
 *
 * For regular pools use:
 * pmempool create -l -s 1G "pmemkv" obj path_to_a_pool
 *
 * For poolsets use:
 * pmempool create -l "pmemkv" obj ../examples/example.poolset
 */
int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " pool\n";
		exit(1);
	}

	/* See libpmemkv_config(3) for more detailed example of creating a config */
	LOG("Creating config");
	config cfg;

	status s = cfg.put_path(argv[1]);
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

	LOG("Removing existing key");
	s = kv->remove("key1");
	ASSERT(s == status::OK);
	s = kv->exists("key1");
	ASSERT(s == status::NOT_FOUND);

	LOG("Closing database");
	delete kv;

	return 0;
}
//! [open]
