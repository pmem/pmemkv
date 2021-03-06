// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2021, Intel Corporation */

/*
 * pmemkv_pmemobj.cpp -- example usage of pmemkv
 *		supporting multiple engines.
 */

//! [multiple-engines]
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
#define ASSERT(expr)                                                                     \
	do {                                                                             \
		if (!(expr))                                                             \
			std::cout << pmemkv_errormsg() << std::endl;                     \
		assert(expr);                                                            \
	} while (0)
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

		status ret = cfg_1.put_oid(&(pop.root()->oids->at(0)));
		ASSERT(ret == status::OK);
		ret = cfg_2.put_oid(&(pop.root()->oids->at(1)));
		ASSERT(ret == status::OK);

		LOG("Starting first cmap engine");
		db *kv_1 = new db();
		ASSERT(kv_1 != nullptr);
		status s = kv_1->open("cmap", std::move(cfg_1));
		ASSERT(s == status::OK);

		*(pop.root()->str) = "some string";

		LOG("Starting second cmap engine");
		db *kv_2 = new db();
		ASSERT(kv_2 != nullptr);
		s = kv_2->open("cmap", std::move(cfg_2));
		ASSERT(s == status::OK);

		LOG("Putting new key into first cmap");
		s = kv_1->put("key_1", "value_1");
		ASSERT(s == status::OK);

		LOG("Putting new key into second cmap");
		s = kv_2->put("key_2", "value_2");
		ASSERT(s == status::OK);

		LOG("Reading key back from first cmap");
		std::string value;
		s = kv_1->get("key_1", &value);
		ASSERT(s == status::OK && value == "value_1");

		LOG("Reading key back from second cmap");
		value.clear();
		s = kv_2->get("key_2", &value);
		ASSERT(s == status::OK && value == "value_2");

		LOG("Defragmenting the first cmap");
		s = kv_1->defrag(0, 100);
		ASSERT(s == status::OK);

		LOG("Defragmenting the second cmap");
		s = kv_2->defrag(0, 100);
		ASSERT(s == status::OK);

		LOG("Stopping first cmap engine");
		delete kv_1;

		LOG("Stopping second cmap engine");
		delete kv_2;

	} catch (std::exception &e) {
		std::cerr << "Exception occurred: " << e.what() << std::endl;
	}

	try {
		pop.close();
	} catch (const std::logic_error &e) {
		std::cerr << "Exception occurred: " << e.what() << std::endl;
	}

	return 0;
}
//! [multiple-engines]
