// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#pragma once

#include "../engine.h"
#include "pmem_allocator.h"
#include <scoped_allocator>
#include <string>
#include <tbb/concurrent_hash_map.h>

#ifdef USE_LIBMEMKIND_NAMESPACE
namespace memkind_ns = libmemkind::pmem;
#else
namespace memkind_ns = pmem;
#endif

namespace pmem
{
namespace kv
{

class vcmap : public engine_base {
public:
	vcmap(std::unique_ptr<internal::config> cfg);
	~vcmap();

	std::string name() final;

	status count_all(std::size_t &cnt) final;

	status get_all(get_kv_callback *callback, void *arg) final;

	status exists(string_view key) final;

	status get(string_view key, get_v_callback *callback, void *arg) final;

	status put(string_view key, string_view value) final;

	status update(string_view key, size_t v_offset, size_t v_size,
		      update_v_callback *callback, void *arg) final;

	status remove(string_view key) final;

private:
	typedef memkind_ns::allocator<char> ch_allocator_t;
	typedef std::basic_string<char, std::char_traits<char>, ch_allocator_t>
		pmem_string;
	typedef memkind_ns::allocator<std::pair<pmem_string, pmem_string>> kv_allocator_t;
	typedef tbb::concurrent_hash_map<pmem_string, pmem_string,
					 tbb::tbb_hash_compare<pmem_string>,
					 std::scoped_allocator_adaptor<kv_allocator_t>>
		map_t;
	kv_allocator_t kv_allocator;
	ch_allocator_t ch_allocator;
	map_t pmem_kv_container;
};

} /* namespace kv */
} /* namespace pmem */
