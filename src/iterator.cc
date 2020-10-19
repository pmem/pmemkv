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

} /* namespace internal */
} /* namespace kv */
} /* namespace pmem */
