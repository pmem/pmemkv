// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/transaction.hpp>

using namespace pmem::kv;

// XXX should this be per engine or implemented in some generic way, e.g.
// UT_ASSERT(kv.some_func() == NOT_SUPPORTED
//		  || kv.some_func() == TRANSACTION_SCOPE_ERROR); ?
static void TransactionTest(pmem::obj::pool_base &pmemobj_pool, pmem::kv::db &kv)
{

	std::string value;
	UT_ASSERT(kv.get("key1", &value) == status::NOT_FOUND);

	pmem::obj::transaction::run(pmemobj_pool, [&] {
		UT_ASSERT(kv.put("key1", "value1") == status::TRANSACTION_SCOPE_ERROR);
	});

	pmem::obj::transaction::run(pmemobj_pool, [&] {
		UT_ASSERT(kv.get("key1", &value) == status::TRANSACTION_SCOPE_ERROR);
	});

	pmem::obj::transaction::run(pmemobj_pool, [&] {
		UT_ASSERT(kv.remove("key1") == status::TRANSACTION_SCOPE_ERROR);
	});

	pmem::obj::transaction::run(pmemobj_pool, [&] {
		UT_ASSERT(kv.exists("key1") == status::TRANSACTION_SCOPE_ERROR);
	});

	pmem::obj::transaction::run(pmemobj_pool, [&] {
		UT_ASSERT(kv.get_all([&](pmem::kv::string_view key,
					 pmem::kv::string_view value) { return 0; }) ==
			  status::TRANSACTION_SCOPE_ERROR);
	});

	pmem::obj::transaction::run(pmemobj_pool, [&] {
		size_t cnt;
		UT_ASSERT(kv.count_all(cnt) == status::TRANSACTION_SCOPE_ERROR);
	});
}
