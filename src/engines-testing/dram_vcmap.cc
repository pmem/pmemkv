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

template<>
bool dram_vcmap::is_registered =
    storage_engine_factory::Register(std::unique_ptr<engine_base::IFactory>(new dram_vcmap_factory));

} // namespace kv
} // namespace pmem
