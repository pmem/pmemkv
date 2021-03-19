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

class std_allocator_factory {
public:
	template <typename T>
	using allocator_type = std::allocator<T>;

	template <typename T>
	static allocator_type<T> create(internal::config& cfg) {
		return allocator_type<T>();
	}
};
} /* namespace internal */

using dram_vcmap = basic_vcmap<internal::std_allocator_factory>;

} /* namespace kv */
} /* namespace pmem */
