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

status iterator_base::seek_to_first()
{
	return status::NOT_SUPPORTED;
}

status iterator_base::seek_to_last()
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

void write_iterator_base::abort()
{
	log.clear();
}

} /* namespace internal */
} /* namespace kv */
} /* namespace pmem */
