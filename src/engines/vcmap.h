// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#ifndef LIBPMEMKV_VCMAP_H
#define LIBPMEMKV_VCMAP_H

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

class vcmap_factory : public engine_base::factory_base {
public:
	virtual std::unique_ptr<engine_base> create(std::unique_ptr<internal::config> cfg)
	{
		check_config_null(get_name(), cfg);
		return std::unique_ptr<engine_base>(new vcmap(std::move(cfg)));
	};
	virtual std::string get_name()
	{
		return "vcmap";
	};
};

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_VCMAP_H */
