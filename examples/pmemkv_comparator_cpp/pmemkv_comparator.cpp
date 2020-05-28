// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * pmemkv_comparator.cpp -- example usage of pmemkv with comparator.
 */

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <libpmemkv.hpp>

#define LOG(msg) std::cout << msg << std::endl

using namespace pmem::kv;

const uint64_t SIZE = 1024UL * 1024UL * 1024UL;

//! [Custom comparator]
class lexicographical_comparator {
public:
	int compare(string_view k1, string_view k2)
	{
		if (k1.compare(k2) == 0)
			return 0;
		else if (std::lexicographical_compare(k1.data(), k1.data() + k1.size(),
						      k2.data(), k2.data() + k2.size()))
			return -1;
		else
			return 1;
	}

	std::string name()
	{
		return "lexicographical_comparator";
	}
};
//! [Custom comparator]

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
	s = cfg.put_comparator(lexicographical_comparator{});
	assert(s == status::OK);

	LOG("Opening pmemkv database with 'csmap' engine");
	db *kv = new db();
	assert(kv != nullptr);
	s = kv->open("csmap", std::move(cfg));
	assert(s == status::OK);

	LOG("Putting new keys");
	s = kv->put("key1", "value1");
	assert(s == status::OK);
	s = kv->put("key2", "value2");
	assert(s == status::OK);
	s = kv->put("key3", "value3");
	assert(s == status::OK);

	LOG("Iterating over existing keys in order specified by the comparator");
	kv->get_all([](string_view k, string_view v) {
		LOG("  visited: " << k.data());
		return 0;
	});

	LOG("Closing database");
	delete kv;

	return 0;
}
