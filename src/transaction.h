// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#ifndef LIBPMEMKV_TRANSACTION_H
#define LIBPMEMKV_TRANSACTION_H

#include "libpmemkv.hpp"

namespace pmem
{

namespace kv
{

namespace internal
{

class transaction {
public:
	transaction()
	{
	}

	virtual ~transaction()
	{
	}

	virtual status put(string_view key, string_view value) = 0;
	virtual status commit() = 0;
	virtual void abort() = 0;
};

} /* namespace internal */
} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_TRANSACTION_H */
