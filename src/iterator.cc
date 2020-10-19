// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "iterator.h"

namespace pmem
{
namespace kv
{
namespace internal
{

status iterator_base::seek_lower(string_view key)
{
	return status::NOT_SUPPORTED;
}

status iterator_base::seek_lower_eq(string_view key)
{
	return status::NOT_SUPPORTED;
}

status iterator_base::seek_higher(string_view key)
{
	return status::NOT_SUPPORTED;
}

status iterator_base::seek_higher_eq(string_view key)
{
	return status::NOT_SUPPORTED;
}

status iterator_base::next()
{
	return status::NOT_SUPPORTED;
}

status iterator_base::prev()
{
	return status::NOT_SUPPORTED;
}

/* not supported for volatile engines */
status accessor_base::commit()
{
	return status::NOT_SUPPORTED;
}

void accessor_base::abort()
{
	throw status::NOT_SUPPORTED;
}

/* for non volatile engines */
non_volatile_accessor::non_volatile_accessor(pmem::obj::pool_base &pop) : _tx(pop)
{
}

status non_volatile_accessor::commit()
{
	pmem::obj::transaction::commit();
	return status::OK;
}

void non_volatile_accessor::abort()
{
	pmem::obj::transaction::abort(EINVAL);
}

} /* namespace internal */
} /* namespace kv */
} /* namespace pmem */
