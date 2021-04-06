// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, 4Paradigm Inc. */

#include <iostream>
#include <unistd.h>

#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/transaction.hpp>

#include "../out.h"
#include "pskiplist.h"

using pmem::detail::conditional_add_to_tx;
using pmem::obj::make_persistent_atomic;
using pmem::obj::transaction;

namespace pmem
{
namespace kv
{

pskiplist::pskiplist(std::unique_ptr<internal::config> cfg)
    : pmemobj_engine_base(cfg, "pmemkv_pskiplist"), config(std::move(cfg))
{
	Recover();
	LOG("Started ok");
}

pskiplist::~pskiplist()
{
	LOG("Stopped ok");
}

std::string pskiplist::name()
{
	return "pskiplist";
}

status pskiplist::count_all(std::size_t &cnt)
{
	LOG("count_all");
	check_outside_tx();

	cnt = my_skiplist->size();

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
status pskiplist::count_above(string_view key, std::size_t &cnt)
{
	LOG("count_above key>=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_skiplist->upper_bound(key);
	auto last = my_skiplist->end();

	cnt = size(first, last);

	return status::OK;
}

/* above or equal to key, key inclusive */
status pskiplist::count_equal_above(string_view key, std::size_t &cnt)
{
	LOG("count_equal_above key>=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_skiplist->lower_bound(key);
	auto last = my_skiplist->end();

	cnt = size(first, last);

	return status::OK;
}

/* below key, key exclusive */
status pskiplist::count_below(string_view key, std::size_t &cnt)
{
	LOG("count_below key<" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_skiplist->begin();
	auto last = my_skiplist->lower_bound(key);

	cnt = size(first, last);

	return status::OK;
}

/* below or equal to key, key inclusive */
status pskiplist::count_equal_below(string_view key, std::size_t &cnt)
{
	LOG("count_equal_below key>=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_skiplist->begin();
	auto last = my_skiplist->upper_bound(key);

	cnt = size(first, last);

	return status::OK;
}

status pskiplist::count_between(string_view key1, string_view key2, std::size_t &cnt)
{
	LOG("count_between key range=[" << std::string(key1.data(), key1.size()) << ","
					<< std::string(key2.data(), key2.size()) << ")");
	check_outside_tx();

	if (my_skiplist->key_comp()(key1, key2)) {
		auto first = my_skiplist->upper_bound(key1);
		auto last = my_skiplist->lower_bound(key2);

		cnt = size(first, last);
	} else {
		cnt = 0;
	}

	return status::OK;
}

status pskiplist::iterate(container_iterator first, container_iterator last,
		      get_kv_callback *callback, void *arg)
{
	for (auto it = first; it != last; ++it) {
		auto ret = callback(it->first.c_str(), it->first.size(),
				    it->second.c_str(), it->second.size(), arg);

		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

status pskiplist::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	check_outside_tx();

	auto first = my_skiplist->begin();
	auto last = my_skiplist->end();

	return iterate(first, last, callback, arg);
}

/* (key, end), above key */
status pskiplist::get_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_above start key>=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_skiplist->upper_bound(key);
	auto last = my_skiplist->end();

	return iterate(first, last, callback, arg);
}

/* [key, end), above or equal to key */
status pskiplist::get_equal_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_above start key>=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_skiplist->lower_bound(key);
	auto last = my_skiplist->end();

	return iterate(first, last, callback, arg);
}

/* [start, key], below or equal to key */
status pskiplist::get_equal_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_below start key>=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_skiplist->begin();
	auto last = my_skiplist->upper_bound(key);

	return iterate(first, last, callback, arg);
}

/* [start, key), less than key, key exclusive */
status pskiplist::get_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_below key<" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = my_skiplist->begin();
	auto last = my_skiplist->lower_bound(key);

	return iterate(first, last, callback, arg);
}

/* get between (key1, key2), key1 exclusive, key2 exclusive */
status pskiplist::get_between(string_view key1, string_view key2, get_kv_callback *callback,
			  void *arg)
{
	LOG("get_between key range=[" << std::string(key1.data(), key1.size()) << ","
				      << std::string(key2.data(), key2.size()) << ")");
	check_outside_tx();

	if (my_skiplist->key_comp()(key1, key2)) {
		auto first = my_skiplist->upper_bound(key1);
		auto last = my_skiplist->lower_bound(key2);

		return iterate(first, last, callback, arg);
	}

	return status::OK;
}

status pskiplist::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	internal::pskiplist::skiplist_type::iterator it = my_skiplist->find(key);
	if (it == my_skiplist->end()) {
		LOG("  key not found");
		return status::NOT_FOUND;
	}
	return status::OK;
}

status pskiplist::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get using callback for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	internal::pskiplist::skiplist_type::iterator it = my_skiplist->find(key);
	if (it == my_skiplist->end()) {
		LOG("  key not found");
		return status::NOT_FOUND;
	}

	callback(it->second.c_str(), it->second.size(), arg);
	return status::OK;
}

status pskiplist::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));
	check_outside_tx();

	auto result = my_skiplist->try_emplace(key, value);
	if (!result.second) { // key already exists, so update
		typename internal::pskiplist::skiplist_type::value_type &entry = *result.first;
		transaction::manual tx(pmpool);
		entry.second = value;
		transaction::commit();
	}
	return status::OK;
}

status pskiplist::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto result = my_skiplist->erase(key);
	return (result == 1) ? status::OK : status::NOT_FOUND;
}

void pskiplist::Recover()
{
	if (!OID_IS_NULL(*root_oid)) {
		my_skiplist = (internal::pskiplist::skiplist_type *)pmemobj_direct(*root_oid);
		my_skiplist->key_comp().runtime_initialize(
			internal::extract_comparator(*config));
	} else {
		pmem::obj::transaction::run(pmpool, [&] {
			pmem::obj::transaction::snapshot(root_oid);
			*root_oid =
				pmem::obj::make_persistent<internal::pskiplist::skiplist_type>()
					.raw();
			my_skiplist =
				(internal::pskiplist::skiplist_type *)pmemobj_direct(*root_oid);
			my_skiplist->key_comp().initialize(
				internal::extract_comparator(*config));
		});
	}
}

internal::iterator_base *pskiplist::new_iterator()
{
	return new pskiplist_iterator<false>{my_skiplist};
}

internal::iterator_base *pskiplist::new_const_iterator()
{
	return new pskiplist_iterator<true>{my_skiplist};
}

pskiplist::pskiplist_iterator<true>::pskiplist_iterator(container_type *c)
    : container(c), it_(nullptr), pop(pmem::obj::pool_by_vptr(c))
{
}

pskiplist::pskiplist_iterator<false>::pskiplist_iterator(container_type *c)
    : pskiplist::pskiplist_iterator<true>(c)
{
}

status pskiplist::pskiplist_iterator<true>::seek(string_view key)
{
	init_seek();

	it_ = container->find(key);
	if (it_ != container->end())
		return status::OK;

	return status::NOT_FOUND;
}

status pskiplist::pskiplist_iterator<true>::seek_lower(string_view key)
{
	init_seek();

	it_ = container->lower_bound(key);
	if (it_ == container->begin()) {
		it_ = container->end();
		return status::NOT_FOUND;
	}

	// --it_;

	return status::OK;
}

status pskiplist::pskiplist_iterator<true>::seek_lower_eq(string_view key)
{
	init_seek();

	it_ = container->upper_bound(key);
	if (it_ == container->begin()) {
		it_ = container->end();
		return status::NOT_FOUND;
	}

	// --it_;

	return status::OK;
}

status pskiplist::pskiplist_iterator<true>::seek_higher(string_view key)
{
	init_seek();

	it_ = container->upper_bound(key);
	if (it_ == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status pskiplist::pskiplist_iterator<true>::seek_higher_eq(string_view key)
{
	init_seek();

	it_ = container->lower_bound(key);
	if (it_ == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status pskiplist::pskiplist_iterator<true>::seek_to_first()
{
	init_seek();

	if (container->size() == 0)
		return status::NOT_FOUND;

	it_ = container->begin();

	return status::OK;
}

status pskiplist::pskiplist_iterator<true>::seek_to_last()
{
	// init_seek();

	// if (container->size() == 0)
	// 	return status::NOT_FOUND;

	// it_ = container->end();
	// --it_;

	// return status::OK;

	return status::NOT_SUPPORTED;
}

status pskiplist::pskiplist_iterator<true>::is_next()
{
	auto tmp = it_;
	if (tmp == container->end() || ++tmp == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status pskiplist::pskiplist_iterator<true>::next()
{
	init_seek();

	if (it_ == container->end() || ++it_ == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status pskiplist::pskiplist_iterator<true>::prev()
{
	// init_seek();

	// if (it_ == container->begin())
	// 	return status::NOT_FOUND;

	// --it_;

	// return status::OK;
	
	return status::NOT_SUPPORTED;
}

result<string_view> pskiplist::pskiplist_iterator<true>::key()
{
	assert(it_ != container->end());

	return {it_->first.cdata()};
}

result<pmem::obj::slice<const char *>> pskiplist::pskiplist_iterator<true>::read_range(size_t pos,
									       size_t n)
{
	assert(it_ != container->end());

	if (pos + n > it_->second.size() || pos + n < pos)
		n = it_->second.size() - pos;

	return {it_->second.crange(pos, n)};
}

result<pmem::obj::slice<char *>> pskiplist::pskiplist_iterator<false>::write_range(size_t pos,
									   size_t n)
{
	assert(it_ != container->end());

	if (pos + n > it_->second.size() || pos + n < pos)
		n = it_->second.size() - pos;

	log.push_back({{it_->second.cdata() + pos, n}, pos});
	auto &val = log.back().first;

	return {{&val[0], &val[0] + n}};
}

status pskiplist::pskiplist_iterator<false>::commit()
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

void pskiplist::pskiplist_iterator<false>::abort()
{
	log.clear();
}

static factory_registerer
	register_pskiplist(std::unique_ptr<engine_base::factory_base>(new pskiplist_factory));

} // namespace kv
} // namespace pmem
