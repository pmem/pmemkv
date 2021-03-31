// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#include "vcmap.h"

namespace pmem
{
namespace kv
{

template <>
std::string vcmap::name()
{
	return "vcmap";
}

static factory_registerer
	register_vcmap(std::unique_ptr<engine_base::factory_base>(new vcmap_factory));

} // namespace kv
} // namespace pmem
