// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#pragma once

#include "../comparator/pmemobj_comparator.h"
#include "../pmemobj_engine.h"

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/experimental/concurrent_map.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/shared_mutex.hpp>

#include <mutex>
#include <shared_mutex>

namespace pmem
{
namespace kv
{
namespace internal
{
namespace csmap
{

using key_type = pmem::obj::string;

static_assert(sizeof(key_type) == 32, "");

struct mapped_type {
	mapped_type() = default;

	mapped_type(const mapped_type &other) : val(other.val)
	{
	}

	mapped_type(mapped_type &&other) : val(std::move(other.val))
	{
	}

	mapped_type(const std::string &str) : val(str)
	{
	}

	mapped_type(string_view str) : val(str.data(), str.size())
	{
	}

	pmem::obj::shared_mutex mtx;
	pmem::obj::string val;
};

static_assert(sizeof(mapped_type) == 96, "");

using map_type = pmem::obj::experimental::concurrent_map<key_type, mapped_type,
							 internal::pmemobj_compare>;

struct pmem_type {
	pmem_type() : map()
	{
		std::memset(reserved, 0, sizeof(reserved));
	}

	map_type map;
	uint64_t reserved[8];
};

static_assert(sizeof(pmem_type) == sizeof(map_type) + 64, "");

} /* namespace csmap */
} /* namespace internal */

class csmap : public pmemobj_engine_base<internal::csmap::pmem_type> {
	template <bool IsConst>
	class csmap_iterator;

	class csmap_accessor;

public:
	csmap(std::unique_ptr<internal::config> cfg);
	~csmap();

	csmap(const csmap &) = delete;
	csmap &operator=(const csmap &) = delete;

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
	using node_mutex_type = pmem::obj::shared_mutex;
	using global_mutex_type = std::shared_timed_mutex;
	using shared_global_lock_type = std::shared_lock<global_mutex_type>;
	using unique_global_lock_type = std::unique_lock<global_mutex_type>;
	using shared_node_lock_type = std::shared_lock<node_mutex_type>;
	using unique_node_lock_type = std::unique_lock<node_mutex_type>;
	using container_type = internal::csmap::map_type;

	void Recover();
	status iterate(typename container_type::iterator first,
		       typename container_type::iterator last, get_kv_callback *callback,
		       void *arg);

	/*
	 * We take read lock for thread-safe methods (like get/insert/get_all) to
	 * synchronize with unsafe_erase() which is not thread-safe.
	 */
	global_mutex_type mtx;
	container_type *container;
	std::unique_ptr<internal::config> config;
};

template <bool IsConst>
class csmap::csmap_iterator : public internal::iterator<IsConst> {
	using container_type = csmap::container_type;
	using value_return_type =
		typename std::conditional<IsConst, string_view,
					  internal::accessor_base *>::type;

public:
	csmap_iterator(container_type *container, global_mutex_type &mtx);

	status seek(string_view key) final;
	status seek_lower(string_view key) final;
	status seek_lower_eq(string_view key) final;
	status seek_higher(string_view key) final;
	status seek_higher_eq(string_view key) final;

	status seek_to_first() final;
	status seek_to_last() final;

	status next() final;

	std::pair<string_view, status> key() final;
	std::pair<value_return_type, status> value() final;

private:
	container_type *container;
	container_type::iterator _it;
	csmap::shared_global_lock_type lock;
	pmem::obj::pool_base pop;
};

class csmap::csmap_accessor : public internal::non_volatile_accessor {
public:
	csmap_accessor(container_type::iterator it,
		       pmem::obj::pool_base &pop); // , global_mutex_type &mtx);

	std::pair<pmem::obj::slice<const char *>, status> read_range(size_t pos,
								     size_t n) final;
	std::pair<pmem::obj::slice<char *>, status> write_range(size_t pos, size_t n);

private:
	container_type::iterator _it;
	// csmap::unique_node_lock_type lock;
};

} /* namespace kv */
} /* namespace pmem */
