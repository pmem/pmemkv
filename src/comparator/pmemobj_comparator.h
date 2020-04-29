// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#ifndef LIBPMEMKV_PMEMOBJ_COMPARATOR_H
#define LIBPMEMKV_PMEMOBJ_COMPARATOR_H

#include "../exceptions.h"
#include "comparator.h"

#include "../valgrind/pmemcheck.h"

#include <libpmemobj++/container/string.hpp>

namespace pmem
{
namespace kv
{
namespace internal
{

class pmemobj_compare {
public:
	using is_transparent = void;

	pmemobj_compare() : name()
	{
	}

	void initialize(const comparator *cmp)
	{
		/* initialize should be called only once */
		assert(this->name.size() == 0);

		if (cmp->name().size() == 0)
			throw internal::invalid_argument(
				"Comparator does not have a valid name");

		this->cmp = cmp;

		auto pop = pmem::obj::pool_base(pmemobj_pool_by_ptr(this));
		pop.persist(&this->cmp, sizeof(this->cmp));

		this->name = cmp->name();
	}

	void runtime_initialize(const comparator *cmp)
	{
		if (this->name != cmp->name())
			throw internal::comparator_mismatch(
				"Comparator with name: \"" +
				std::string(this->name.c_str()) + "\" expected");

		this->cmp = cmp;

		auto pop = pmem::obj::pool_base(pmemobj_pool_by_ptr(this));
		pop.persist(&this->cmp, sizeof(this->cmp));
	}

	template <typename T, typename U>
	bool operator()(const T &lhs, const U &rhs) const
	{
		auto key1 = make_string_view(lhs);
		auto key2 = make_string_view(rhs);

		return (cmp->compare(key1, key2) < 0);
	}

private:
	pmem::obj::string name;
	const comparator *cmp = nullptr;
};

static_assert(sizeof(pmemobj_compare) == 40, "wrong pmemobj_compare size");

} /* namespace internal */
} /* namespace kv */
} /* namespace pmem */

#endif
