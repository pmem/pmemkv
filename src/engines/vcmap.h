// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#ifndef LIBPMEMKV_VCMAP_H
#define LIBPMEMKV_VCMAP_H

#include "basic_vcmap.h"
#include "memkind_allocator_wrapper.h"

#include <memory>

namespace pmem
{
namespace kv
{

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
