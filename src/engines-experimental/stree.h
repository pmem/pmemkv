// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#pragma once

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>

#include "../pmemobj_engine.h"
#include "stree/persistent_b_tree.h"
#include "stree/pstring.h"

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

const size_t DEGREE = 64;
// const size_t MAX_KEY_SIZE = 20;
// const size_t MAX_VALUE_SIZE = 200;

struct string_t : public pmem::obj::string {
	string_t() = default;
	string_t(const string_t &) = default;
	string_t(string_t &&) = default;
	string_t(string_view &str) : pmem::obj::string(str.data(), str.size())
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
	void assign(const char *s, size_t count)
	{
		pmem::obj::string::assign(s, count);
	}
};

inline bool operator<(const string_t &lhs, const string_view &rhs)
{
	return lhs.compare(0, lhs.size(), rhs.data(), rhs.size()) < 0;
}

inline bool operator<(const string_view &lhs, const string_t &rhs)
{
	return rhs.compare(0, rhs.size(), lhs.data(), lhs.size()) > 0;
}

inline bool operator>(const string_t &lhs, const string_view &rhs)
{
	return lhs.compare(0, lhs.size(), rhs.data(), rhs.size()) > 0;
}

inline bool operator==(const string_t &lhs, const string_view &rhs)
{
	return lhs.compare(0, lhs.size(), rhs.data(), rhs.size()) == 0;
}

inline bool operator==(const string_view &lhs, const string_t &rhs)
{
	return rhs.compare(0, rhs.size(), lhs.data(), lhs.size()) == 0;
}

using key_type = string_t;
using value_type = string_t;
struct hetero_less {
	using is_transparent = void;
	template <typename T1, typename T2>
	bool operator()(const T1 &lhs, const T2 &rhs) const
	{
		return lhs < rhs;
	}
};
using btree_type = persistent::b_tree<key_type, value_type, hetero_less, DEGREE>;

} /* namespace stree */
} /* namespace internal */

class stree : public pmemobj_engine_base<internal::stree::btree_type> {
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
	void Recover();

	internal::stree::btree_type *my_btree;
};

} /* namespace kv */
} /* namespace pmem */
