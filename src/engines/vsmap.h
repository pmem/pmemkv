// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#ifndef LIBPMEMKV_VSMAP_H
#define LIBPMEMKV_VSMAP_H

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

	internal::iterator_base *new_iterator() final;
	internal::iterator_base *new_const_iterator() final;

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

template <>
class vsmap::vsmap_iterator<true> : public internal::iterator_base {
	using container_type = vsmap::map_type;

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

	status is_next() final;
	status next() final;
	status prev() final;

	result<string_view> key() final;

	result<pmem::obj::slice<const char *>> read_range(size_t pos, size_t n) final;

protected:
	container_type *container;
	vsmap::map_allocator_type *kv_allocator;
	container_type::iterator it_;
};

template <>
class vsmap::vsmap_iterator<false> : public vsmap::vsmap_iterator<true> {
	using container_type = vsmap::map_type;

public:
	vsmap_iterator(container_type *container,
		       vsmap::map_allocator_type *kv_allocator);

	result<pmem::obj::slice<char *>> write_range(size_t pos, size_t n) final;

	status commit() final;
	void abort() final;

private:
	std::vector<std::pair<std::string, size_t>> log;
};

class vsmap_factory : public engine_base::factory_base {
public:
	virtual std::unique_ptr<engine_base> create(std::unique_ptr<internal::config> cfg)
	{
		check_config_null(get_name(), cfg);
		return std::unique_ptr<engine_base>(new vsmap(std::move(cfg)));
	};
	virtual std::string get_name()
	{
		return "vsmap";
	};
};

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_VSMAP_H */
