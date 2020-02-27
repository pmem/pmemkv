// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include <iostream>
#include <unistd.h>

#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/transaction.hpp>

#include "../out.h"
#include "stree.h"

using pmem::detail::conditional_add_to_tx;
using pmem::obj::make_persistent_atomic;
using pmem::obj::transaction;

namespace pmem
{
namespace kv
{

stree::stree(std::unique_ptr<internal::config> cfg)
    : pmemobj_engine_base(cfg), config(std::move(cfg))
{
	Recover();
	LOG("Started ok");
}

stree::~stree()
{
	LOG("Stopped ok");
}

std::string stree::name()
{
	return "stree";
}

status stree::count_all(std::size_t &cnt)
{
	LOG("count_all");
	check_outside_tx();

	cnt = my_btree->size();
	assert(cnt >= 0);

	return status::OK;
}

template <typename It>
static std::size_t size(It first, It last)
{
	auto dist = std::distance(first, last);
	assert(dist >= 0);

	return static_cast<std::size_t>(dist);
}

/* above key, key exclusive */
status stree::count_above(string_view key, std::size_t &cnt)
{
	LOG("count_above key>=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_btree->upper_bound(key);
	auto last = my_btree->end();

	cnt = size(first, last);

	return status::OK;
}

/* above or equal to key, key inclusive */
status stree::count_equal_above(string_view key, std::size_t &cnt)
{
	LOG("count_equal_above key>=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_btree->lower_bound(key);
	auto last = my_btree->end();

	cnt = size(first, last);

	return status::OK;
}

/* below key, key exclusive */
status stree::count_below(string_view key, std::size_t &cnt)
{
	LOG("count_below key<" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_btree->begin();
	auto last = my_btree->lower_bound(key);

	cnt = size(first, last);

	return status::OK;
}

/* below or equal to key, key inclusive */
status stree::count_equal_below(string_view key, std::size_t &cnt)
{
	LOG("count_equal_below key>=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_btree->begin();
	auto last = my_btree->upper_bound(key);

	cnt = size(first, last);

	return status::OK;
}

status stree::count_between(string_view key1, string_view key2, std::size_t &cnt)
{
	LOG("count_between key range=[" << std::string(key1.data(), key1.size()) << ","
					<< std::string(key2.data(), key2.size()) << ")");
	check_outside_tx();

	if (my_btree->key_comp()(key1, key2)) {
		auto first = my_btree->upper_bound(key1);
		auto last = my_btree->lower_bound(key2);

		cnt = size(first, last);
	} else {
		cnt = 0;
	}

	return status::OK;
}

status stree::iterate(iterator first, iterator last, get_kv_callback *callback, void *arg)
{
	for (auto it = first; it != last; ++it) {
		auto ret = callback(it->first.c_str(), it->first.size(),
				    it->second.c_str(), it->second.size(), arg);

		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

status stree::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	check_outside_tx();

	auto first = my_btree->begin();
	auto last = my_btree->end();

	return iterate(first, last, callback, arg);
}

/* (key, end), above key */
status stree::get_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_above start key>=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_btree->upper_bound(key);
	auto last = my_btree->end();

	return iterate(first, last, callback, arg);
}

/* [key, end), above or equal to key */
status stree::get_equal_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_above start key>=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_btree->lower_bound(key);
	auto last = my_btree->end();

	return iterate(first, last, callback, arg);
}

/* [start, key], below or equal to key */
status stree::get_equal_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_below start key>=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_btree->begin();
	auto last = my_btree->upper_bound(key);

	return iterate(first, last, callback, arg);
}

/* [start, key), less than key, key exclusive */
status stree::get_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_below key<" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_btree->begin();
	auto last = my_btree->lower_bound(key);

	return iterate(first, last, callback, arg);
}

/* get between (key1, key2), key1 exclusive, key2 exclusive */
status stree::get_between(string_view key1, string_view key2, get_kv_callback *callback,
			  void *arg)
{
	LOG("get_between key range=[" << std::string(key1.data(), key1.size()) << ","
				      << std::string(key2.data(), key2.size()) << ")");
	check_outside_tx();

	if (my_btree->key_comp()(key1, key2)) {
		auto first = my_btree->upper_bound(key1);
		auto last = my_btree->lower_bound(key2);

		return iterate(first, last, callback, arg);
	}

	return status::OK;
}

status stree::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	internal::stree::btree_type::iterator it = my_btree->find(key);
	if (it == my_btree->end()) {
		LOG("  key not found");
		return status::NOT_FOUND;
	}
	return status::OK;
}

status stree::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get using callback for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	internal::stree::btree_type::iterator it = my_btree->find(key);
	if (it == my_btree->end()) {
		LOG("  key not found");
		return status::NOT_FOUND;
	}

	callback(it->second.c_str(), it->second.size(), arg);
	return status::OK;
}

status stree::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));
	check_outside_tx();

	auto result = my_btree->try_emplace(key, value);
	if (!result.second) { // key already exists, so update
		typename internal::stree::btree_type::value_type &entry = *result.first;
		transaction::manual tx(pmpool);
		entry.second = value;
		transaction::commit();
	}
	return status::OK;
}

status stree::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto result = my_btree->erase(key);
	return (result == 1) ? status::OK : status::NOT_FOUND;
}

void stree::Recover()
{
	if (!OID_IS_NULL(*root_oid)) {
		my_btree = (internal::stree::btree_type *)pmemobj_direct(*root_oid);
		my_btree->key_comp().runtime_initialize(
			internal::extract_comparator(*config));
	} else {
		pmem::obj::transaction::run(pmpool, [&] {
			pmem::obj::transaction::snapshot(root_oid);
			*root_oid =
				pmem::obj::make_persistent<internal::stree::btree_type>()
					.raw();
			my_btree =
				(internal::stree::btree_type *)pmemobj_direct(*root_oid);
			my_btree->key_comp().initialize(
				internal::extract_comparator(*config));
		});
	}
}

} // namespace kv
} // namespace pmem
