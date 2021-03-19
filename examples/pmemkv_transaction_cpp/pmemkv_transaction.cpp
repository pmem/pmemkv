// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

/*
 * pmemkv_transaction.cpp -- example usage of pmemkv transactions.
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

/*
 * This example expects a path to already created database pool.
 *
 * To create a pool use one of the following commands.
 *
 * For regular pools use:
 * pmempool create -l -s 1G "pmemkv_radix" obj path_to_a_pool
 *
 * For poolsets use:
 * pmempool create -l "pmemkv_radix" obj ../examples/example.poolset
 */
//! [transaction]
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

	LOG("Opening pmemkv database with 'radix' engine");
	db kv;
	s = kv.open("radix", std::move(cfg));
	ASSERT(s == status::OK);

	LOG("Putting new key");
	s = kv.put("key1", "value1");
	ASSERT(s == status::OK);

	auto result_tx = kv.tx_begin();
	ASSERT(result_tx.is_ok());

	/* This function is guaranteed to not throw if is_ok is true */
	auto &tx = result_tx.get_value();
	s = tx.remove("key1");
	s = tx.put("key2", "value2");
	s = tx.put("key3", "value3");

	/* Until transaction is committed, changes are not visible */
	ASSERT(kv.exists("key1") == status::OK);
	ASSERT(kv.exists("key2") == status::NOT_FOUND);
	ASSERT(kv.exists("key3") == status::NOT_FOUND);

	s = tx.commit();
	ASSERT(s == status::OK);

	ASSERT(kv.exists("key1") == status::NOT_FOUND);
	ASSERT(kv.exists("key2") == status::OK);
	ASSERT(kv.exists("key3") == status::OK);

	{
		/* Alternative method of obtaining tx object. This line can throw if
		 * tx_begin() fails. */
		auto tx = kv.tx_begin().get_value();

		s = tx.put("key4", "value4");
		s = tx.put("key5", "value5");
	}

	/* The second tx was not committed, so the changes are not visible. */
	ASSERT(kv.exists("key4") == status::NOT_FOUND);
	ASSERT(kv.exists("key5") == status::NOT_FOUND);

	return 0;
}
//! [transaction]
