// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#ifndef LIBPMEMKV_STREE_H
#define LIBPMEMKV_STREE_H

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>

#include "../comparator/pmemobj_comparator.h"
#include "../iterator.h"
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

using string_t = pmem::obj::string;

using key_type = string_t;
using value_type = string_t;
using btree_type = b_tree<key_type, value_type, internal::pmemobj_compare, DEGREE>;

} /* namespace stree */
} /* namespace internal */

class stree : public pmemobj_engine_base<internal::stree::btree_type> {
private:
	using container_type = internal::stree::btree_type;
	using container_iterator = typename container_type::iterator;

	template <bool IsConst>
	class stree_iterator;

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

	internal::iterator_base *new_iterator() final;
	internal::iterator_base *new_const_iterator() final;

private:
	stree(const stree &);
	void operator=(const stree &);
	void Recover();

	internal::stree::btree_type *my_btree;
	std::unique_ptr<internal::config> config;
};

template <>
class stree::stree_iterator<true> : public internal::iterator_base {
	using container_type = stree::container_type;

public:
	stree_iterator(container_type *container);

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
class stree::stree_iterator<false> : public stree::stree_iterator<true> {
	using container_type = stree::container_type;

public:
	stree_iterator(container_type *container);

	result<pmem::obj::slice<char *>> write_range(size_t pos, size_t n) final;

	status commit() final;
	void abort() final;

private:
	std::vector<std::pair<std::string, size_t>> log;
};

class stree_factory : public engine_base::factory_base {
public:
	std::unique_ptr<engine_base>
	create(std::unique_ptr<internal::config> cfg) override
	{
		check_config_null(get_name(), cfg);
		return std::unique_ptr<engine_base>(new stree(std::move(cfg)));
	};
	std::string get_name() override
	{
		return "stree";
	};
};

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_STREE_H */
