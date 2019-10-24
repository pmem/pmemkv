/*
 * Copyright 2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
	assert(s == pmem::kv::status::OK);
	s = cfg.put_uint64("size", SIZE);
	assert(s == pmem::kv::status::OK);
	s = cfg.put_uint64("force_create", 1);
	assert(s == pmem::kv::status::OK);

	pmem::kv::db *kv = new pmem::kv::db;
	s = kv->open("cmap", std::move(cfg));
	assert(s == pmem::kv::status::OK);

	return kv;
}

pmem::kv::db *db_open(std::string path)
{
	pmem::kv::config cfg;

	pmem::kv::status s = cfg.put_string("path", path);
	assert(s == pmem::kv::status::OK);

	pmem::kv::db *kv = new pmem::kv::db;
	s = kv->open("cmap", std::move(cfg));
	assert(s == pmem::kv::status::OK);

	return kv;
}

void populate_db(pmem::kv::db &db, size_t num_elements)
{
	for (size_t i = 0; i < num_elements; i++) {
		auto s = db.put(std::to_string(i), std::to_string(i));
		assert(s == pmem::kv::status::OK);
	}
}

void verify_db(pmem::kv::db &db, size_t num_elements)
{
	std::size_t count;
	auto s = db.count_all(count);
	assert(s == pmem::kv::status::OK);
	assert(count == num_elements);

	for (size_t i = 0; i < num_elements; i++) {
		auto s = db.get(std::to_string(i), [&](pmem::kv::string_view value) {
			assert(std::to_string(i).compare(0, std::string::npos,
							 value.data()) == 0);
		});
		assert(s == pmem::kv::status::OK);
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
