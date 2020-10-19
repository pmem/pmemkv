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

	auto result = container->try_emplace(key, value);

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

internal::iterator_base *radix::new_iterator()
{
	return new radix_iterator<false>{container};
}

internal::iterator_base *radix::new_const_iterator()
{
	return new radix_iterator<true>{container};
}

radix::radix_iterator<true>::radix_iterator(container_type *c)
    : container(c), _it(c->begin()), pop(pmem::obj::pool_by_vptr(c))
{
}

radix::radix_iterator<false>::radix_iterator(container_type *c)
    : radix::radix_iterator<true>(c)
{
}

status radix::radix_iterator<true>::seek(string_view key)
{
	init_seek();

	_it = container->find(key);
	if (_it != container->end())
		return status::OK;

	return status::NOT_FOUND;
}

status radix::radix_iterator<true>::seek_lower(string_view key)
{
	init_seek();

	_it = container->lower_bound(key);
	if (_it == container->begin()) {
		_it = container->end();
		return status::NOT_FOUND;
	}

	--_it;

	return status::OK;
}

status radix::radix_iterator<true>::seek_lower_eq(string_view key)
{
	init_seek();

	_it = container->upper_bound(key);
	if (_it == container->begin()) {
		_it = container->end();
		return status::NOT_FOUND;
	}

	--_it;

	return status::OK;
}

status radix::radix_iterator<true>::seek_higher(string_view key)
{
	init_seek();

	_it = container->upper_bound(key);
	if (_it == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status radix::radix_iterator<true>::seek_higher_eq(string_view key)
{
	init_seek();

	_it = container->lower_bound(key);
	if (_it == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status radix::radix_iterator<true>::seek_to_first()
{
	init_seek();

	if (container->empty())
		return status::NOT_FOUND;

	_it = container->begin();

	return status::OK;
}

status radix::radix_iterator<true>::seek_to_last()
{
	init_seek();

	if (container->empty())
		return status::NOT_FOUND;

	_it = container->end();
	--_it;

	return status::OK;
}

status radix::radix_iterator<true>::is_next()
{
	auto tmp = _it;
	if (tmp == container->end() || ++tmp == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status radix::radix_iterator<true>::next()
{
	init_seek();

	if (_it == container->end() || ++_it == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status radix::radix_iterator<true>::prev()
{
	init_seek();

	if (_it == container->begin())
		return status::NOT_FOUND;

	--_it;

	return status::OK;
}

std::pair<string_view, status> radix::radix_iterator<true>::key()
{
	assert(_it != container->end());

	return {_it->key().cdata(), status::OK};
}

std::pair<pmem::obj::slice<const char *>, status>
radix::radix_iterator<true>::read_range(size_t pos, size_t n)
{
	assert(_it != container->end());

	if (pos + n > _it->value().size() || pos + n < pos)
		n = _it->value().size() - pos;

	return {{_it->value().cdata() + pos, _it->value().cdata() + pos + n}, status::OK};
}

std::pair<pmem::obj::slice<char *>, status>
radix::radix_iterator<false>::write_range(size_t pos, size_t n)
{
	assert(_it != container->end());

	if (pos + n > _it->value().size() || pos + n < pos)
		n = _it->value().size() - pos;

	log.push_back({std::string(_it->value().cdata() + pos, n), pos});
	auto &val = log.back().first;

	return {{&val[0], &val[n]}, status::OK};
}

status radix::radix_iterator<false>::commit()
{
	pmem::obj::transaction::run(pop, [&] {
		for (auto &p : log) {
			auto dest = _it->value().range(p.second, p.first.size());
			std::copy(p.first.begin(), p.first.end(), dest.begin());
		}
	});
	log.clear();

	return status::OK;
}

void radix::radix_iterator<false>::abort()
{
	log.clear();
}

} // namespace kv
} // namespace pmem
