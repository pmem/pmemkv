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

struct key_type : public pmem::obj::string {
	key_type() = default;
	key_type(const key_type &) = default;
	key_type(key_type &&) = default;
	key_type(string_view str) : pmem::obj::string(str.data(), str.size())
	{
	}
};

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

} /* namespace kv */
} /* namespace pmem */
