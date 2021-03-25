// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "dram_vcmap.h"

namespace pmem
{
namespace kv
{

template<>
std::string dram_vcmap::name() {
	return "dram_vcmap";
}

static factory_registerer register_dram_vcmap(
    std::unique_ptr<engine_base::factory_base>(new dram_vcmap_factory));

} // namespace kv
} // namespace pmem
