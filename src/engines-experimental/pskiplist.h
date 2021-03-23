// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, 4Paradigm Inc. */

#pragma once

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>

#include "../comparator/pmemobj_comparator.h"
#include "../iterator.h"
#include "../pmemobj_engine.h"
#include "pskiplist/persistent_skiplist.h"

using pmem::obj::persistent_ptr;
using pmem::obj::pool;

using namespace pmem::obj;
using namespace pmem::obj::experimental;

#define DEFAULT_HEIGHT 8
#define DEFAULT_BRANCH 4

namespace pmem
{
namespace kv
{
namespace internal
{
namespace pskiplist
{

/**
 * Indicates the maximum number of descendants a single node can have.
 * DEGREE - 1 is the maximum number of entries a node can have.
 */
const size_t HEIGHT = 8;
const size_t BRANCH = 4;

using string_t = pmem::obj::string;

using key_type = string_t;
using value_type = string_t;
using skiplist_type = persistent_skiplist<key_type, value_type, internal::pmemobj_compare, HEIGHT, BRANCH>;

} /* namespace pskiplist */
} /* namespace internal */

class pskiplist : public pmemobj_engine_base<internal::pskiplist::skiplist_type> {
private:
	using container_type = internal::pskiplist::skiplist_type;
	using container_iterator = typename container_type::iterator;

	template <bool IsConst>
	class pskiplist_iterator;

public:
	pskiplist(std::unique_ptr<internal::config> cfg);
	~pskiplist();

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
	pskiplist(const pskiplist &);
	void operator=(const pskiplist &);
	status iterate(container_iterator first, container_iterator last,
		       get_kv_callback *callback, void *arg);
	void Recover();

	internal::pskiplist::skiplist_type *my_skiplist;
	std::unique_ptr<internal::config> config;
};

template <>
class pskiplist::pskiplist_iterator<true> : virtual public internal::iterator_base {
	using container_type = pskiplist::container_type;

public:
	pskiplist_iterator(container_type *container);

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
	container_type::iterator it_;
	pmem::obj::pool_base pop;
};

template <>
class pskiplist::pskiplist_iterator<false> : public pskiplist::pskiplist_iterator<true> {
	using container_type = pskiplist::container_type;

public:
	pskiplist_iterator(container_type *container);

	result<pmem::obj::slice<char *>> write_range(size_t pos, size_t n) final;

	status commit() final;
	void abort() final;

private:
	std::vector<std::pair<std::string, size_t>> log;
};

} /* namespace kv */
} /* namespace pmem */
