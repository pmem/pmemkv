// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#pragma once

#include "../comparator/volatile_comparator.h"
#include "../engine.h"
#include "../iterator.h"

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
	template <bool IsConst>
	class vsmap_iterator;

	class vsmap_accessor;

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

	status remove(string_view key) final;

	internal::iterator<false> *new_iterator() final;
	internal::iterator<true> *new_const_iterator() final;

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

template <bool IsConst>
class vsmap::vsmap_iterator : public internal::iterator<IsConst> {
	using container_type = vsmap::map_type;
	using value_return_type =
		typename std::conditional<IsConst, string_view,
					  internal::accessor_base *>::type;

public:
	vsmap_iterator(container_type *container,
		       vsmap::map_allocator_type *kv_allocator);

	status seek(string_view key) final;
	status seek_lower(string_view key) final;
	status seek_lower_eq(string_view key) final;
	status seek_higher(string_view key) final;
	status seek_higher_eq(string_view key) final;

	status seek_to_first() final;
	status seek_to_last() final;

	status next() final;
	status prev() final;

	std::pair<string_view, status> key() final;
	std::pair<value_return_type, status> value() final;

private:
	container_type *container;
	vsmap::map_allocator_type *kv_allocator;
	container_type::iterator _it;
};

class vsmap::vsmap_accessor : public internal::accessor_base {
public:
	vsmap_accessor(map_type::iterator it);

	std::pair<pmem::obj::slice<const char *>, status> read_range(size_t pos,
								     size_t n) final;
	std::pair<pmem::obj::slice<char *>, status> write_range(size_t pos, size_t n);

private:
	map_type::iterator _it;
};

} /* namespace kv */
} /* namespace pmem */
