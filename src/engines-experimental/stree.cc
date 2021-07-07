// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

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
    : pmemobj_engine_base(cfg, "pmemkv_stree"), config(std::move(cfg))
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

status stree::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	check_outside_tx();

	auto first = my_btree->begin();
	auto last = my_btree->end();

	return internal::iterate_through_pairs(first, last, callback, arg);
}

/* (key, end), above key */
status stree::get_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_above start key>=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_btree->upper_bound(key);
	auto last = my_btree->end();

	return internal::iterate_through_pairs(first, last, callback, arg);
}

/* [key, end), above or equal to key */
status stree::get_equal_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_above start key>=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_btree->lower_bound(key);
	auto last = my_btree->end();

	return internal::iterate_through_pairs(first, last, callback, arg);
}

/* [start, key], below or equal to key */
status stree::get_equal_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_below start key>=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_btree->begin();
	auto last = my_btree->upper_bound(key);

	return internal::iterate_through_pairs(first, last, callback, arg);
}

/* [start, key), less than key, key exclusive */
status stree::get_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_below key<" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_btree->begin();
	auto last = my_btree->lower_bound(key);

	return internal::iterate_through_pairs(first, last, callback, arg);
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

		return internal::iterate_through_pairs(first, last, callback, arg);
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

internal::iterator_base *stree::new_iterator()
{
	return new stree_iterator<false>{my_btree};
}

internal::iterator_base *stree::new_const_iterator()
{
	return new stree_iterator<true>{my_btree};
}

stree::stree_iterator<true>::stree_iterator(container_type *c)
    : container(c), it_(nullptr), pop(pmem::obj::pool_by_vptr(c))
{
}

stree::stree_iterator<false>::stree_iterator(container_type *c)
    : stree::stree_iterator<true>(c)
{
}

status stree::stree_iterator<true>::seek(string_view key)
{
	init_seek();

	it_ = container->find(key);
	if (it_ != container->end())
		return status::OK;

	return status::NOT_FOUND;
}

status stree::stree_iterator<true>::seek_lower(string_view key)
{
	init_seek();

	it_ = container->lower_bound(key);
	if (it_ == container->begin()) {
		it_ = container->end();
		return status::NOT_FOUND;
	}

	--it_;

	return status::OK;
}

status stree::stree_iterator<true>::seek_lower_eq(string_view key)
{
	init_seek();

	it_ = container->upper_bound(key);
	if (it_ == container->begin()) {
		it_ = container->end();
		return status::NOT_FOUND;
	}

	--it_;

	return status::OK;
}

status stree::stree_iterator<true>::seek_higher(string_view key)
{
	init_seek();

	it_ = container->upper_bound(key);
	if (it_ == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status stree::stree_iterator<true>::seek_higher_eq(string_view key)
{
	init_seek();

	it_ = container->lower_bound(key);
	if (it_ == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status stree::stree_iterator<true>::seek_to_first()
{
	init_seek();

	if (container->size() == 0)
		return status::NOT_FOUND;

	it_ = container->begin();

	return status::OK;
}

status stree::stree_iterator<true>::seek_to_last()
{
	init_seek();

	if (container->size() == 0)
		return status::NOT_FOUND;

	it_ = container->end();
	--it_;

	return status::OK;
}

status stree::stree_iterator<true>::is_next()
{
	auto tmp = it_;
	if (tmp == container->end() || ++tmp == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status stree::stree_iterator<true>::next()
{
	init_seek();

	if (it_ == container->end() || ++it_ == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status stree::stree_iterator<true>::prev()
{
	init_seek();

	if (it_ == container->begin())
		return status::NOT_FOUND;

	--it_;

	return status::OK;
}

result<string_view> stree::stree_iterator<true>::key()
{
	assert(it_ != container->end());

	return string_view(it_->first.cdata(), it_->first.length());
}

result<pmem::obj::slice<const char *>> stree::stree_iterator<true>::read_range(size_t pos,
									       size_t n)
{
	assert(it_ != container->end());

	if (pos + n > it_->second.size() || pos + n < pos)
		n = it_->second.size() - pos;

	return {it_->second.crange(pos, n)};
}

result<pmem::obj::slice<char *>> stree::stree_iterator<false>::write_range(size_t pos,
									   size_t n)
{
	assert(it_ != container->end());

	if (pos + n > it_->second.size() || pos + n < pos)
		n = it_->second.size() - pos;

	log.push_back({{it_->second.cdata() + pos, n}, pos});
	auto &val = log.back().first;

	return {{&val[0], &val[0] + n}};
}

status stree::stree_iterator<false>::commit()
{
	pmem::obj::transaction::run(pop, [&] {
		for (auto &p : log) {
			auto dest = it_->second.range(p.second, p.first.size());
			std::copy(p.first.begin(), p.first.end(), dest.begin());
		}
	});
	log.clear();

	return status::OK;
}

void stree::stree_iterator<false>::abort()
{
	log.clear();
}

static factory_registerer
	register_stree(std::unique_ptr<engine_base::factory_base>(new stree_factory));

} // namespace kv
} // namespace pmem
