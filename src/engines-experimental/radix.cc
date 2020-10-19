// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "radix.h"
#include "../out.h"

namespace pmem
{
namespace kv
{

radix::radix(std::unique_ptr<internal::config> cfg)
    : pmemobj_engine_base(cfg, "pmemkv_radix"), config(std::move(cfg))
{
	Recover();
	LOG("Started ok");
}

radix::~radix()
{
	LOG("Stopped ok");
}

std::string radix::name()
{
	return "radix";
}

status radix::count_all(std::size_t &cnt)
{
	LOG("count_all");
	check_outside_tx();
	cnt = container->size();

	return status::OK;
}

template <typename It>
static std::size_t size(It first, It last)
{
	auto dist = std::distance(first, last);
	assert(dist >= 0);

	return static_cast<std::size_t>(dist);
}

status radix::count_above(string_view key, std::size_t &cnt)
{
	LOG("count_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->upper_bound(key);
	auto last = container->end();

	cnt = size(first, last);

	return status::OK;
}

status radix::count_equal_above(string_view key, std::size_t &cnt)
{
	LOG("count_equal_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->lower_bound(key);
	auto last = container->end();

	cnt = size(first, last);

	return status::OK;
}

status radix::count_equal_below(string_view key, std::size_t &cnt)
{
	LOG("count_equal_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->begin();
	auto last = container->upper_bound(key);

	cnt = size(first, last);

	return status::OK;
}

status radix::count_below(string_view key, std::size_t &cnt)
{
	LOG("count_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->begin();
	auto last = container->lower_bound(key);

	cnt = size(first, last);

	return status::OK;
}

status radix::count_between(string_view key1, string_view key2, std::size_t &cnt)
{
	LOG("count_between for key1=" << key1.data() << ", key2=" << key2.data());
	check_outside_tx();

	if (key1.compare(key2) < 0) {
		auto first = container->upper_bound(key1);
		auto last = container->lower_bound(key2);

		cnt = size(first, last);
	} else {
		cnt = 0;
	}

	return status::OK;
}

status radix::iterate(typename container_type::const_iterator first,
		      typename container_type::const_iterator last,
		      get_kv_callback *callback, void *arg)
{
	for (auto it = first; it != last; ++it) {
		string_view key = it->key();
		string_view value = it->value();

		auto ret =
			callback(key.data(), key.size(), value.data(), value.size(), arg);

		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

status radix::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	check_outside_tx();

	auto first = container->begin();
	auto last = container->end();

	return iterate(first, last, callback, arg);
}

status radix::get_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->upper_bound(key);
	auto last = container->end();

	return iterate(first, last, callback, arg);
}

status radix::get_equal_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->lower_bound(key);
	auto last = container->end();

	return iterate(first, last, callback, arg);
}

status radix::get_equal_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->begin();
	auto last = container->upper_bound(key);

	return iterate(first, last, callback, arg);
}

status radix::get_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->begin();
	auto last = container->lower_bound(key);

	return iterate(first, last, callback, arg);
}

status radix::get_between(string_view key1, string_view key2, get_kv_callback *callback,
			  void *arg)
{
	LOG("get_between for key1=" << key1.data() << ", key2=" << key2.data());
	check_outside_tx();

	if (key1.compare(key2) < 0) {
		auto first = container->upper_bound(key1);
		auto last = container->lower_bound(key2);
		return iterate(first, last, callback, arg);
	}

	return status::OK;
}

status radix::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	return container->find(key) != container->end() ? status::OK : status::NOT_FOUND;
}

status radix::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto it = container->find(key);
	if (it != container->end()) {
		auto value = string_view(it->value());
		callback(value.data(), value.size(), arg);
		return status::OK;
	}

	LOG("  key not found");
	return status::NOT_FOUND;
}

status radix::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));
	check_outside_tx();

	auto result = container->emplace(key, value);

	if (result.second == false) {
		pmem::obj::transaction::run(pmpool,
					    [&] { result.first.assign_val(value); });
	}

	return status::OK;
}

status radix::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto it = container->find(key);

	if (it == container->end())
		return status::NOT_FOUND;

	container->erase(it);

	return status::OK;
}

void radix::Recover()
{
	if (!OID_IS_NULL(*root_oid)) {
		auto pmem_ptr = static_cast<internal::radix::pmem_type *>(
			pmemobj_direct(*root_oid));

		container = &pmem_ptr->map;
	} else {
		pmem::obj::transaction::run(pmpool, [&] {
			pmem::obj::transaction::snapshot(root_oid);
			*root_oid =
				pmem::obj::make_persistent<internal::radix::pmem_type>()
					.raw();
			auto pmem_ptr = static_cast<internal::radix::pmem_type *>(
				pmemobj_direct(*root_oid));
			container = &pmem_ptr->map;
		});
	}
}

internal::iterator<false> *radix::new_iterator()
{
	return new radix_iterator<false>{container};
}

internal::iterator<true> *radix::new_const_iterator()
{
	return new radix_iterator<true>{container};
}

template <bool IsConst>
radix::radix_iterator<IsConst>::radix_iterator(container_type *c)
{
	container = c;
	_it = container->begin();
}

template <bool IsConst>
status radix::radix_iterator<IsConst>::seek(string_view key)
{
	_it = container->find(key);
	if (_it != container->end())
		return status::OK;

	return status::NOT_FOUND;
}

template <bool IsConst>
status radix::radix_iterator<IsConst>::seek_lower(string_view key)
{
	_it = container->lower_bound(key);
	if (_it == container->begin()) {
		_it = container->end();
		return status::NOT_FOUND;
	}

	--_it;

	return status::OK;
}

template <bool IsConst>
status radix::radix_iterator<IsConst>::seek_lower_eq(string_view key)
{
	_it = container->lower_bound(key);
	if (container->empty() ||
	    (_it == container->begin() &&
	     key.compare(string_view{_it->key().data(), _it->key().size()}) != 0)) {
		_it = container->end();
		return status::NOT_FOUND;
	}

	return status::OK;
}

template <bool IsConst>
status radix::radix_iterator<IsConst>::seek_higher(string_view key)
{
	_it = container->upper_bound(key);
	if (_it == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

template <bool IsConst>
status radix::radix_iterator<IsConst>::seek_higher_eq(string_view key)
{
	_it = container->upper_bound(key);
	if (container->empty() ||
	    (_it-- == container->end() &&
	     key.compare(string_view{_it->key().data(), _it->key().size()}) != 0)) {
		_it = container->end();
		return status::NOT_FOUND;
	}

	return status::OK;
}

template <bool IsConst>
status radix::radix_iterator<IsConst>::seek_to_first()
{
	if (container->empty())
		return status::NOT_FOUND;

	_it = container->begin();

	return status::OK;
}

template <bool IsConst>
status radix::radix_iterator<IsConst>::seek_to_last()
{
	if (container->empty())
		return status::NOT_FOUND;

	_it = container->end();
	--_it;

	return status::OK;
}

template <bool IsConst>
status radix::radix_iterator<IsConst>::next()
{
	if (_it == container->end() || ++_it == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

template <bool IsConst>
status radix::radix_iterator<IsConst>::prev()
{
	if (_it == container->begin())
		return status::NOT_FOUND;

	--_it;

	return status::OK;
}

} // namespace kv
} // namespace pmem
