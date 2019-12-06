// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#pragma once

#include "../pmemobj_engine.h"

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/experimental/concurrent_map.hpp>
#include <libpmemobj++/experimental/v.hpp>
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

inline bool operator<(const pmem::obj::string &lhs, string_view rhs)
{
	return lhs.compare(0, lhs.size(), rhs.data(), rhs.size()) < 0;
}

inline bool operator<(string_view lhs, const pmem::obj::string &rhs)
{
	return rhs.compare(0, rhs.size(), lhs.data(), lhs.size()) > 0;
}

class hetero_less {
public:
	using is_transparent = void;

	template <typename M, typename U>
	bool operator()(const M &lhs, const U &rhs) const
	{
		return lhs < rhs;
	}
};

struct key_type : public pmem::obj::string {
	key_type() = default;
	key_type(const key_type &) = default;
	key_type(key_type &&) = default;
	key_type(string_view str) : pmem::obj::string(str.data(), str.size())
	{
	}
};

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

using map_t = pmem::obj::experimental::concurrent_map<key_type, mapped_type, hetero_less>;

} /* namespace csmap */
} /* namespace internal */

class csmap : public pmemobj_engine_base<internal::csmap::map_t> {
public:
	csmap(std::unique_ptr<internal::config> cfg);
	~csmap();

	csmap(const csmap &) = delete;
	csmap &operator=(const csmap &) = delete;

	std::string name() final;

	status count_all(std::size_t &cnt) final;

	status get_all(get_kv_callback *callback, void *arg) final;

	status exists(string_view key) final;

	status get(string_view key, get_v_callback *callback, void *arg) final;

	status put(string_view key, string_view value) final;

	status remove(string_view key) final;

	status defrag(double start_percent, double amount_percent) final;

private:
	using node_mutex_type = pmem::obj::shared_mutex;
	using mutex_type = std::shared_timed_mutex;
	using container_type = internal::csmap::map_t;

	void Recover();

	mutex_type mtx;
	container_type *container;
};

} /* namespace kv */
} /* namespace pmem */
