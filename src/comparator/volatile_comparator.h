// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#ifndef LIBPMEMKV_VOLATILE_COMPARATOR_H
#define LIBPMEMKV_VOLATILE_COMPARATOR_H

#include "../exceptions.h"
#include "comparator.h"

namespace pmem
{
namespace kv
{
namespace internal
{

class volatile_compare {
public:
	using is_transparent = void;

	volatile_compare(const comparator *cmp) : cmp(cmp)
	{
	}

	template <typename T, typename U>
	bool operator()(const T &lhs, const U &rhs) const
	{
		auto key1 = make_string_view(lhs);
		auto key2 = make_string_view(rhs);

		return (cmp->compare(key1, key2) < 0);
	}

private:
	const comparator *cmp;
};

} /* namespace internal */
} /* namespace kv */
} /* namespace pmem */

#endif
