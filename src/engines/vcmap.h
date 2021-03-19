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

class memkind_allocator_factory {
public:
	template <typename T>
	using allocator_type = memkind_ns::allocator<T>;

	template <typename T>
	static allocator_type<T> create(internal::config &cfg)
	{
		return allocator_type<T>(get_path(cfg), get_size(cfg));
	}

private:
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

using vcmap = basic_vcmap<internal::memkind_allocator_factory>;

} /* namespace kv */
} /* namespace pmem */
