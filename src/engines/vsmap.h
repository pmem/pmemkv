// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#pragma once

#include "../comparator/volatile_comparator.h"
#include "../engine.h"

#include "pmem_allocator.h"
#include <map>
#include <scoped_allocator>
#include <string>

#ifdef USE_LIBMEMKIND_NAMESPACE
namespace memkind_ns = libmemkind::pmem;
#else
namespace memkind_ns = pmem;
#endif

namespace pmem
{
namespace kv
{

class vsmap : public engine_base {
public:
	vsmap(std::unique_ptr<internal::config> cfg);
	~vsmap();

	std::string name() final;

	status count_all(std::size_t &cnt) final;
	status count_above(string_view key, std::size_t &cnt) final;
	status count_equal_above(string_view key, std::size_t &cnt) final;
	status count_equal_below(string_view key, std::size_t &cnt) final;
	status count_below(string_view key, std::size_t &cnt) final;
	status count_between(string_view key1, string_view key2, std::size_t &cnt) final;

	status get_all(get_kv_callback *callback, void *arg) final;
	status get_above(string_view key, get_kv_callback *callback, void *arg) final;
	status get_equal_above(string_view key, get_kv_callback *callback,
			       void *arg) final;
	status get_equal_below(string_view key, get_kv_callback *callback,
			       void *arg) final;
	status get_below(string_view key, get_kv_callback *callback, void *arg) final;
	status get_between(string_view key1, string_view key2, get_kv_callback *callback,
			   void *arg) final;

	status exists(string_view key) final;

	status get(string_view key, get_v_callback *callback, void *arg) final;

	status put(string_view key, string_view value) final;

	status update(string_view key, size_t v_offset, size_t v_size,
		      update_v_callback *callback, void *arg) final;

	status remove(string_view key) final;

private:
	using storage_type = std::basic_string<char, std::char_traits<char>,
					       memkind_ns::allocator<char>>;

	using key_type = storage_type;
	using mapped_type = storage_type;
	using map_allocator_type =
		memkind_ns::allocator<std::pair<const key_type, mapped_type>>;
	using map_type = std::map<key_type, mapped_type, internal::volatile_compare,
				  std::scoped_allocator_adaptor<map_allocator_type>>;

	map_allocator_type kv_allocator;
	map_type pmem_kv_container;
	std::unique_ptr<internal::config> config;
};

} /* namespace kv */
} /* namespace pmem */
