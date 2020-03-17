/*
 * Copyright 2020, Intel Corporation
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

#include "unittest.hpp"

#include <libpmemobj++/transaction.hpp>

using namespace pmem::kv;

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
		UT_ASSERT(kv.defrag() == status::TRANSACTION_SCOPE_ERROR);
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
