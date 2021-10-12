// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#ifndef LIBPMEMKV_VCMAP_H
#define LIBPMEMKV_VCMAP_H

#include "basic_vcmap.h"
#include "memkind_allocator_wrapper.h"

namespace pmem
{
namespace kv
{
namespace internal
{

class memkind_allocator_factory {
public:
	template <typename T>
	using allocator_type = internal::memkind_allocator_wrapper<T>;

	template <typename T>
	using wrapped_type = typename allocator_type<T>::allocator_type;

	template <typename T>
	static allocator_type<T> create(internal::config &cfg)
	{
		auto *allocator_ptr = new wrapped_type<T>(cfg.get_path(), cfg.get_size());

		cfg.put_object("allocator", allocator_ptr, [](void *arg) {
			delete reinterpret_cast<wrapped_type<T> *>(arg);
		});

		return allocator_type<T>(allocator_ptr);
	}
};
}

using vcmap = basic_vcmap<internal::memkind_allocator_factory>;

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
