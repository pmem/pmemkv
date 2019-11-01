/*
 * Copyright 2017-2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

stree::stree(std::unique_ptr<internal::config> cfg) : pmemobj_engine_base(cfg)
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

	auto result = std::distance(my_btree->begin(), my_btree->end());
	assert(result >= 0);

	cnt = static_cast<std::size_t>(result);

	return status::OK;
}

// above key, key exclusive
status stree::count_above(string_view key, std::size_t &cnt)
{
	LOG("count_above key>=" << std::string(key.data(), key.size()));
	check_outside_tx();

	internal::stree::btree_type::iterator it = my_btree->upper_bound(
		pstring<internal::stree::MAX_KEY_SIZE>(key.data(), key.size()));
	auto result = std::distance(it, my_btree->end());
	assert(result >= 0);

	cnt = static_cast<std::size_t>(result);

	return status::OK;
}

// above or equal to key, key inclusive
status stree::count_equal_above(string_view key, std::size_t &cnt)
{
	LOG("count_above key>=" << std::string(key.data(), key.size()));
	check_outside_tx();

	internal::stree::btree_type::iterator it = my_btree->lower_bound(
		pstring<internal::stree::MAX_KEY_SIZE>(key.data(), key.size()));
	auto result = std::distance(it, my_btree->end());
	assert(result >= 0);

	cnt = static_cast<std::size_t>(result);

	return status::OK;
}

// below key, key exclusive
status stree::count_below(string_view key, std::size_t &cnt)
{
	LOG("count_below key<" << std::string(key.data(), key.size()));
	check_outside_tx();

	internal::stree::btree_type::iterator it = my_btree->lower_bound(
		pstring<internal::stree::MAX_KEY_SIZE>(key.data(), key.size()));
	auto result = std::distance(my_btree->begin(), it);
	assert(result >= 0);

	cnt = static_cast<std::size_t>(result);

	return status::OK;
}

// below or equal to key, key inclusive
status stree::count_equal_below(string_view key, std::size_t &cnt)
{
	LOG("count_above key>=" << std::string(key.data(), key.size()));
	check_outside_tx();

	internal::stree::btree_type::iterator it = my_btree->upper_bound(
		pstring<internal::stree::MAX_KEY_SIZE>(key.data(), key.size()));
	auto result = std::distance(my_btree->begin(), it);
	assert(result >= 0);

	cnt = static_cast<std::size_t>(result);

	return status::OK;
}

status stree::count_between(string_view key1, string_view key2, std::size_t &cnt)
{
	LOG("count_between key range=[" << std::string(key1.data(), key1.size()) << ","
					<< std::string(key2.data(), key2.size()) << ")");
	check_outside_tx();

	internal::stree::btree_type::iterator it1 = my_btree->upper_bound(
		pstring<internal::stree::MAX_KEY_SIZE>(key1.data(), key1.size()));
	internal::stree::btree_type::iterator it2 = my_btree->lower_bound(
		pstring<internal::stree::MAX_KEY_SIZE>(key2.data(), key2.size()));
	auto result = std::distance(it1, it2);
	assert(result >= 0);

	cnt = static_cast<std::size_t>(result);

	return status::OK;
}

status stree::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	check_outside_tx();
	for (auto &iterator : *my_btree) {
		auto ret = callback(iterator.first.c_str(), iterator.first.size(),
				    iterator.second.c_str(), iterator.second.size(), arg);
		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

// (key, end), above key
status stree::get_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_above start key>=" << std::string(key.data(), key.size()));
	check_outside_tx();
	internal::stree::btree_type::iterator it = my_btree->upper_bound(
		pstring<internal::stree::MAX_KEY_SIZE>(key.data(), key.size()));
	while (it != my_btree->end()) {
		auto ret = callback((*it).first.c_str(), (*it).first.size(),
				    (*it).second.c_str(), (*it).second.size(), arg);
		if (ret != 0)
			return status::STOPPED_BY_CB;
		it++;
	}

	return status::OK;
}

// [key, end), above or equal to key
status stree::get_equal_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_above start key>=" << std::string(key.data(), key.size()));
	check_outside_tx();
	internal::stree::btree_type::iterator it = my_btree->lower_bound(
		pstring<internal::stree::MAX_KEY_SIZE>(key.data(), key.size()));
	while (it != my_btree->end()) {
		auto ret = callback((*it).first.c_str(), (*it).first.size(),
				    (*it).second.c_str(), (*it).second.size(), arg);
		if (ret != 0)
			return status::STOPPED_BY_CB;
		it++;
	}

	return status::OK;
}

// [start, key], below or equal to key
status stree::get_equal_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_above start key>=" << std::string(key.data(), key.size()));
	check_outside_tx();
	internal::stree::btree_type::iterator it = my_btree->begin();
	auto pskey = pstring<internal::stree::MAX_KEY_SIZE>(key.data(), key.size());
	while (it != my_btree->end() && !((*it).first > pskey)) {
		auto ret = callback((*it).first.c_str(), (*it).first.size(),
				    (*it).second.c_str(), (*it).second.size(), arg);
		if (ret != 0)
			return status::STOPPED_BY_CB;
		it++;
	}

	return status::OK;
}

// [start, key), less than key, key exclusive
status stree::get_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_below key<" << std::string(key.data(), key.size()));
	check_outside_tx();
	auto pskey = pstring<internal::stree::MAX_KEY_SIZE>(key.data(), key.size());
	internal::stree::btree_type::iterator it = my_btree->begin();
	while (it != my_btree->end() && (*it).first < pskey) {
		auto ret = callback((*it).first.c_str(), (*it).first.size(),
				    (*it).second.c_str(), (*it).second.size(), arg);
		if (ret != 0)
			return status::STOPPED_BY_CB;
		it++;
	}

	return status::OK;
}

// get between (key1, key2), key1 exclusive, key2 exclusive
status stree::get_between(string_view key1, string_view key2, get_kv_callback *callback,
			  void *arg)
{
	LOG("get_between key range=[" << std::string(key1.data(), key1.size()) << ","
				      << std::string(key2.data(), key2.size()) << ")");
	check_outside_tx();
	auto pskey1 = pstring<internal::stree::MAX_KEY_SIZE>(key1.data(), key1.size());
	auto pskey2 = pstring<internal::stree::MAX_KEY_SIZE>(key2.data(), key2.size());
	internal::stree::btree_type::iterator it = my_btree->upper_bound(pskey1);
	while (it != my_btree->end() && (*it).first < pskey2) {
		auto ret = callback((*it).first.c_str(), (*it).first.size(),
				    (*it).second.c_str(), (*it).second.size(), arg);
		if (ret != 0)
			return status::STOPPED_BY_CB;
		it++;
	}

	return status::OK;
}

status stree::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	check_outside_tx();
	internal::stree::btree_type::iterator it = my_btree->find(
		pstring<internal::stree::MAX_KEY_SIZE>(key.data(), key.size()));
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
	internal::stree::btree_type::iterator it = my_btree->find(
		pstring<internal::stree::MAX_KEY_SIZE>(key.data(), key.size()));
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

	auto result = my_btree->insert(std::make_pair(
		pstring<internal::stree::MAX_KEY_SIZE>(key.data(), key.size()),
		pstring<internal::stree::MAX_VALUE_SIZE>(value.data(), value.size())));
	if (!result.second) { // key already exists, so update
		typename internal::stree::btree_type::value_type &entry = *result.first;
		transaction::manual tx(pmpool);
		conditional_add_to_tx(&(entry.second));
		entry.second = std::string(value.data(), value.size());
		transaction::commit();
	}
	return status::OK;
}

status stree::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto result = my_btree->erase(std::string(key.data(), key.size()));
	return (result == 1) ? status::OK : status::NOT_FOUND;
}

void stree::Recover()
{
	if (!OID_IS_NULL(*root_oid)) {
		my_btree = (internal::stree::btree_type *)pmemobj_direct(*root_oid);
		my_btree->garbage_collection();
	} else {
		pmem::obj::transaction::manual tx(pmpool);
		pmem::obj::transaction::snapshot(root_oid);
		*root_oid =
			pmem::obj::make_persistent<internal::stree::btree_type>().raw();
		pmem::obj::transaction::commit();
		my_btree = (internal::stree::btree_type *)pmemobj_direct(*root_oid);
	}
}

} // namespace kv
} // namespace pmem
