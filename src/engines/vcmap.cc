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

template <>
bool vcmap::is_registered = storage_engine_factory::Register(
	std::unique_ptr<engine_base::IFactory>(new vcmap_factory));

} // namespace kv
} // namespace pmem
