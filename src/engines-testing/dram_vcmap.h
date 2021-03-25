// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#pragma once

#include "../engines/basic_vcmap.h"

namespace pmem
{
namespace kv
{
namespace internal
{
template <typename AllocatorT>
class std_allocator_wrapper : public std::allocator<AllocatorT> {
public:
	using std::allocator<AllocatorT>::allocator;
	std_allocator_wrapper(internal::config &cfg)
	    : std::allocator<AllocatorT>()
	{
	}
};
} /* namespace internal */

using dram_vcmap = basic_vcmap<internal::std_allocator_wrapper>;

class dram_vcmap_factory : public engine_base::IFactory {
public:
    std::unique_ptr<engine_base> create(std::unique_ptr<internal::config> cfg) override
    {
        check_config_null(get_name(), cfg);
		return std::unique_ptr<engine_base>(new dram_vcmap(std::move(cfg)));
	};
    std::string get_name() override
    {
		return "dram_vcmap";
	};
};

} /* namespace kv */
} /* namespace pmem */
