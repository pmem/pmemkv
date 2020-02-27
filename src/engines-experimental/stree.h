// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#pragma once

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>

#include "../comparator/pmemobj_comparator.h"
#include "../pmemobj_engine.h"
#include "stree/persistent_b_tree.h"

using pmem::obj::persistent_ptr;
using pmem::obj::pool;

namespace pmem
{
namespace kv
{
namespace internal
{
namespace stree
{

/**
 * Indicates the maximum number of descendants a single node can have.
 * DEGREE - 1 is the maximum number of entries a node can have.
 */
const size_t DEGREE = 32;

struct string_t : public pmem::obj::string {
	string_t() = default;
	string_t(const string_t &) = default;
	string_t(string_t &&) = default;
	string_t(string_view str) : pmem::obj::string(str.data(), str.size())
	{
	}
	pmem::obj::string &operator=(const string_view &other)
	{
		return pmem::obj::string::assign(other.data(), other.size());
	}
	pmem::obj::string &operator=(const string_t &other)
	{
		return pmem::obj::string::assign(other.data(), other.size());
	}
};

using key_type = string_t;
using value_type = string_t;
using btree_type = b_tree<key_type, value_type, internal::pmemobj_compare, DEGREE>;

} /* namespace stree */
} /* namespace internal */

class stree : public pmemobj_engine_base<internal::stree::btree_type> {
private:
	using container_type = internal::stree::btree_type;
	using iterator = typename container_type::iterator;

public:
	stree(std::unique_ptr<internal::config> cfg);
	~stree();

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
	stree(const stree &);
	void operator=(const stree &);
	status iterate(iterator first, iterator last, get_kv_callback *callback,
		       void *arg);
	void Recover();

	internal::stree::btree_type *my_btree;
	std::unique_ptr<internal::config> config;
};

} /* namespace kv */
} /* namespace pmem */
