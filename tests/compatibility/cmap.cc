// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include <cassert>
#include <iostream>
#include <libpmemkv.hpp>

#include <cstdlib>

const uint64_t SIZE = 1024UL * 1024UL * 1024UL;
const uint64_t NUM_ELEMENTS = 1024UL * 1024UL;

pmem::kv::db *db_create(std::string path)
{
	pmem::kv::config cfg;

	pmem::kv::status s = cfg.put_string("path", path);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = cfg.put_uint64("size", SIZE);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	s = cfg.put_uint64("force_create", 1);
	ASSERT_STATUS(s, pmem::kv::status::OK);

	pmem::kv::db *kv = new pmem::kv::db;
	s = kv->open("cmap", std::move(cfg));
	ASSERT_STATUS(s, pmem::kv::status::OK);

	return kv;
}

pmem::kv::db *db_open(std::string path)
{
	pmem::kv::config cfg;

	pmem::kv::status s = cfg.put_string("path", path);
	ASSERT_STATUS(s, pmem::kv::status::OK);

	pmem::kv::db *kv = new pmem::kv::db;
	s = kv->open("cmap", std::move(cfg));
	ASSERT_STATUS(s, pmem::kv::status::OK);

	return kv;
}

void populate_db(pmem::kv::db &db, size_t num_elements)
{
	for (size_t i = 0; i < num_elements; i++) {
		auto s = db.put(std::to_string(i), std::to_string(i));
		ASSERT_STATUS(s, pmem::kv::status::OK);
	}
}

void verify_db(pmem::kv::db &db, size_t num_elements)
{
	std::size_t count;
	auto s = db.count_all(count);
	ASSERT_STATUS(s, pmem::kv::status::OK);
	assert(count == num_elements);

	for (size_t i = 0; i < num_elements; i++) {
		auto s = db.get(std::to_string(i), [&](pmem::kv::string_view value) {
			assert(std::to_string(i).compare(0, std::string::npos,
							 value.data()) == 0);
		});
		ASSERT_STATUS(s, pmem::kv::status::OK);
	}
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		std::cerr << "Usage: " << argv[0]
			  << " file [create|create_ungraceful|open]\n";
		exit(1);
	}

	pmem::kv::db *db;
	if (std::string(argv[2]) == "create") {
		db = db_create(argv[1]);

		populate_db(*db, NUM_ELEMENTS);

		delete db;
	} else if (std::string(argv[2]) == "create_ungraceful") {
		db = db_create(argv[1]);

		populate_db(*db, NUM_ELEMENTS);

		/* Do not close db */
	} else if (std::string(argv[2]) == "open") {
		db = db_open(argv[1]);

		verify_db(*db, NUM_ELEMENTS);

		delete db;
	} else {
		std::cerr << "Wrong mode\n";
		exit(1);
	}

	return 0;
}
