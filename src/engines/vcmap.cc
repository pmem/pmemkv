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

} // namespace kv
} // namespace pmem
