/*
 * Copyright 2019-2020, Intel Corporation
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

/*
 * pmemkv_pmemobj_basic.cpp -- example usage of pmemkv supporting multiple
 * engines.
 */

#include "../../src/pmemobj_engine.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <libpmemkv.hpp>
#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/transaction.hpp>

#undef LOG
#define LOG(msg) std::cout << msg << std::endl

using namespace pmem::kv;
using pmemoid_vector = pmem::obj::vector<PMEMoid>;

const uint64_t SIZE = 1024UL * 1024UL * 1024UL;

struct Root {
	pmem::obj::persistent_ptr<pmemoid_vector> oids;
	pmem::obj::persistent_ptr<pmem::obj::string> str;
};

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " file\n";
		exit(1);
	}

	const char *path = argv[1];

	pmem::obj::pool<Root> pop;

	try {
		pop = pmem::obj::pool<Root>::create(path, "pmemkv", SIZE, S_IRWXU);

		pmem::obj::transaction::run(pop, [&] {
			pop.root()->oids = pmem::obj::make_persistent<pmemoid_vector>();
			pop.root()->str = pmem::obj::make_persistent<pmem::obj::string>();

			pop.root()->oids->emplace_back(OID_NULL);
			pop.root()->oids->emplace_back(OID_NULL);
		});

		LOG("Creating configs");
		config cfg_1;
		config cfg_2;

		status ret = cfg_1.put_object("oid", &(pop.root()->oids->at(0)), nullptr);
		assert(ret == status::OK);
		ret = cfg_2.put_object("oid", &(pop.root()->oids->at(1)), nullptr);
		assert(ret == status::OK);

		LOG("Starting first cmap engine");
		db *kv_1 = new db();
		assert(kv_1 != nullptr);
		status s = kv_1->open("cmap", std::move(cfg_1));
		assert(s == status::OK);

		*(pop.root()->str) = "some string";

		LOG("Starting second cmap engine");
		db *kv_2 = new db();
		assert(kv_2 != nullptr);
		s = kv_2->open("cmap", std::move(cfg_2));
		assert(s == status::OK);

		LOG("Putting new key into first cmap");
		s = kv_1->put("key_1", "value_1");
		assert(s == status::OK);

		LOG("Putting new key into second cmap");
		s = kv_2->put("key_2", "value_2");
		assert(s == status::OK);

		LOG("Reading key back from first cmap");
		std::string value;
		s = kv_1->get("key_1", &value);
		assert(s == status::OK && value == "value_1");

		LOG("Reading key back from second cmap");
		value.clear();
		s = kv_2->get("key_2", &value);
		assert(s == status::OK && value == "value_2");

		LOG("Defragmenting the first cmap");
		s = kv_1->defrag(0, 100);
		assert(s == status::OK);

		LOG("Defragmenting the second cmap");
		s = kv_2->defrag(0, 100);
		assert(s == status::OK);

		LOG("Stopping first cmap engine");
		delete kv_1;

		LOG("Stopping second cmap engine");
		delete kv_2;

	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}

	pop.close();

	return 0;
}
