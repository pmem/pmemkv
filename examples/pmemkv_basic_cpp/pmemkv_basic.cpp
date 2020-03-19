// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

/*
 * pmemkv_basic.cpp -- example usage of pmemkv.
 */

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <libpmemkv.hpp>

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

	status s = cfg.put_string("path", argv[1]);
	assert(s == status::OK);
	s = cfg.put_uint64("size", SIZE);
	assert(s == status::OK);
	s = cfg.put_uint64("force_create", 1);
	assert(s == status::OK);

	LOG("Opening pmemkv database with 'cmap' engine");
	db *kv = new db();
	assert(kv != nullptr);
	s = kv->open("cmap", std::move(cfg));
	assert(s == status::OK);

	LOG("Putting new key");
	s = kv->put("key1", "value1");
	assert(s == status::OK);

	size_t cnt;
	s = kv->count_all(cnt);
	assert(s == status::OK && cnt == 1);

	LOG("Reading key back");
	std::string value;
	s = kv->get("key1", &value);
	assert(s == status::OK && value == "value1");

	LOG("Iterating existing keys");
	kv->put("key2", "value2");
	kv->put("key3", "value3");
	kv->get_all([](string_view k, string_view v) {
		LOG("  visited: " << k.data());
		return 0;
	});

	LOG("Defragmenting the database");
	s = kv->defrag(0, 100);
	assert(s == status::OK);

	LOG("Removing existing key");
	s = kv->remove("key1");
	assert(s == status::OK);
	s = kv->exists("key1");
	assert(s == status::NOT_FOUND);

	LOG("Closing database");
	delete kv;

	return 0;
}
