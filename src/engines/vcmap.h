// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#pragma once

#include "basic_vcmap.h"
#include "pmem_allocator.h"

#ifdef USE_LIBMEMKIND_NAMESPACE
namespace memkind_ns = libmemkind::pmem;
#else
namespace memkind_ns = pmem;
#endif

namespace pmem
{
namespace kv
{
namespace internal
{
template <typename AllocatorT>
class memkind_allocator_wrapper : public memkind_ns::allocator<AllocatorT> {
public:
	using memkind_ns::allocator<AllocatorT>::allocator;
	memkind_allocator_wrapper(internal::config &cfg)
	    : memkind_ns::allocator<AllocatorT>(get_path(cfg), get_size(cfg))
	{
	}

	static std::string get_path(internal::config &cfg)
	{
		const char *path;
		if (!cfg.get_string("path", &path))
			throw internal::invalid_argument(
				"Config does not contain item with key: \"path\"");

		return std::string(path);
	}

	static uint64_t get_size(internal::config &cfg)
	{
		std::size_t size;
		if (!cfg.get_uint64("size", &size))
			throw internal::invalid_argument(
				"Config does not contain item with key: \"size\"");

		return size;
	}
};
}

using vcmap = basic_vcmap<internal::memkind_allocator_wrapper>;

} /* namespace kv */
} /* namespace pmem */
