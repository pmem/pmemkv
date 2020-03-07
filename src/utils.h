// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#ifndef PMEMKV_UTILS
#define PMEMKV_UTILS

#include <cstdint>

namespace pmem
{
namespace kv
{
namespace internal
{
namespace utils
{

static inline uint64_t mssb_index64(std::size_t value)
{
	return ((unsigned char)(63 - __builtin_clzll(value)));
}

}
}
}
}

#endif /* PMEMKV_UTILS */