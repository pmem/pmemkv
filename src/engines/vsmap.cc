// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include "vsmap.h"
#include "../comparator/comparator.h"
#include "../comparator/volatile_comparator.h"
#include "../out.h"

#include <libpmemobj++/transaction.hpp>

#include <iostream>

namespace pmem
{
namespace kv
{

static std::string get_path(internal::config &cfg)
{
	const char *path;
	if (!cfg.get_string("path", &path))
		throw internal::invalid_argument(
			"Config does not contain item with key: \"path\"");

	return std::string(path);
}

static uint64_t get_size(internal::config &cfg)
{
	std::size_t size;
	if (!cfg.get_uint64("size", &size))
		throw internal::invalid_argument(
			"Config does not contain item with key: \"size\"");

	return size;
}

vsmap::vsmap(std::unique_ptr<internal::config> cfg)
    : kv_allocator(get_path(*cfg), get_size(*cfg)),
      pmem_kv_container(internal::volatile_compare(internal::extract_comparator(*cfg)),
			kv_allocator),
      config(std::move(cfg))
{
	LOG("Started ok");
}

vsmap::~vsmap()
{
	LOG("Stopped ok");
}

std::string vsmap::name()
{
	return "vsmap";
}

status vsmap::count_all(std::size_t &cnt)
{
	cnt = pmem_kv_container.size();

	return status::OK;
}

status vsmap::count_above(string_view key, std::size_t &cnt)
{
	LOG("count_above for key=" << std::string(key.data(), key.size()));
	std::size_t result = 0;
	// XXX - do not create temporary string
	auto it = pmem_kv_container.upper_bound(
		key_type(key.data(), key.size(), kv_allocator));
	auto end = pmem_kv_container.end();
	for (; it != end; it++)
		result++;

	cnt = result;

	return status::OK;
}

status vsmap::count_equal_above(string_view key, std::size_t &cnt)
{
	LOG("count_equal_above for key=" << std::string(key.data(), key.size()));
	std::size_t result = 0;
	// XXX - do not create temporary string
	auto it = pmem_kv_container.lower_bound(
		key_type(key.data(), key.size(), kv_allocator));
	auto end = pmem_kv_container.end();
	for (; it != end; it++)
		result++;

	cnt = result;

	return status::OK;
}

status vsmap::count_equal_below(string_view key, std::size_t &cnt)
{
	LOG("count_equal_below for key=" << std::string(key.data(), key.size()));
	std::size_t result = 0;
	auto it = pmem_kv_container.begin();
	// XXX - do not create temporary string
	auto end = pmem_kv_container.upper_bound(
		key_type(key.data(), key.size(), kv_allocator));
	for (; it != end; it++)
		result++;

	cnt = result;

	return status::OK;
}

status vsmap::count_below(string_view key, std::size_t &cnt)
{
	LOG("count_below for key=" << std::string(key.data(), key.size()));
	std::size_t result = 0;
	auto it = pmem_kv_container.begin();
	// XXX - do not create temporary string
	auto end = pmem_kv_container.lower_bound(
		key_type(key.data(), key.size(), kv_allocator));
	for (; it != end; it++)
		result++;

	cnt = result;

	return status::OK;
}

status vsmap::count_between(string_view key1, string_view key2, std::size_t &cnt)
{
	LOG("count_between for key1=" << key1.data() << ", key2=" << key2.data());
	std::size_t result = 0;
	if (pmem_kv_container.key_comp()(key1, key2)) {
		// XXX - do not create temporary string
		auto it = pmem_kv_container.upper_bound(
			key_type(key1.data(), key1.size(), kv_allocator));
		auto end = pmem_kv_container.lower_bound(
			key_type(key2.data(), key2.size(), kv_allocator));
		for (; it != end; it++)
			result++;
	}

	cnt = result;

	return status::OK;
}

status vsmap::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	for (auto &it : pmem_kv_container) {
		auto ret = callback(it.first.c_str(), it.first.size(), it.second.c_str(),
				    it.second.size(), arg);

		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

status vsmap::get_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_above for key=" << std::string(key.data(), key.size()));
	// XXX - do not create temporary string
	auto it = pmem_kv_container.upper_bound(
		key_type(key.data(), key.size(), kv_allocator));
	auto end = pmem_kv_container.end();
	for (; it != end; it++) {
		auto ret = callback(it->first.c_str(), it->first.size(),
				    it->second.c_str(), it->second.size(), arg);

		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

status vsmap::get_equal_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_above for key=" << std::string(key.data(), key.size()));
	// XXX - do not create temporary string
	auto it = pmem_kv_container.lower_bound(
		key_type(key.data(), key.size(), kv_allocator));
	auto end = pmem_kv_container.end();
	for (; it != end; it++) {
		auto ret = callback(it->first.c_str(), it->first.size(),
				    it->second.c_str(), it->second.size(), arg);

		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

status vsmap::get_equal_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_above for key=" << std::string(key.data(), key.size()));
	auto it = pmem_kv_container.begin();
	// XXX - do not create temporary string
	auto end = pmem_kv_container.upper_bound(
		key_type(key.data(), key.size(), kv_allocator));
	for (; it != end; it++) {
		auto ret = callback(it->first.c_str(), it->first.size(),
				    it->second.c_str(), it->second.size(), arg);

		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

status vsmap::get_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_below for key=" << std::string(key.data(), key.size()));
	auto it = pmem_kv_container.begin();
	// XXX - do not create temporary string
	auto end = pmem_kv_container.lower_bound(
		key_type(key.data(), key.size(), kv_allocator));
	for (; it != end; it++) {
		auto ret = callback(it->first.c_str(), it->first.size(),
				    it->second.c_str(), it->second.size(), arg);

		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

status vsmap::get_between(string_view key1, string_view key2, get_kv_callback *callback,
			  void *arg)
{
	LOG("get_between for key1=" << key1.data() << ", key2=" << key2.data());
	if (pmem_kv_container.key_comp()(key1, key2)) {
		// XXX - do not create temporary string
		auto it = pmem_kv_container.upper_bound(
			key_type(key1.data(), key1.size(), kv_allocator));
		auto end = pmem_kv_container.lower_bound(
			key_type(key2.data(), key2.size(), kv_allocator));
		for (; it != end; it++) {
			auto ret = callback(it->first.c_str(), it->first.size(),
					    it->second.c_str(), it->second.size(), arg);

			if (ret != 0)
				return status::STOPPED_BY_CB;
		}
	}

	return status::OK;
}

status vsmap::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	// XXX - do not create temporary string
	bool r = pmem_kv_container.find(key_type(key.data(), key.size(), kv_allocator)) !=
		pmem_kv_container.end();
	return (r ? status::OK : status::NOT_FOUND);
}

status vsmap::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));
	// XXX - do not create temporary string
	const auto pos =
		pmem_kv_container.find(key_type(key.data(), key.size(), kv_allocator));
	if (pos == pmem_kv_container.end()) {
		LOG("  key not found");
		return status::NOT_FOUND;
	}

	callback(pos->second.c_str(), pos->second.size(), arg);
	return status::OK;
}

status vsmap::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));
	// XXX - starting from C++17 std::map has try_emplace method which could be more
	// efficient
	auto res = pmem_kv_container.emplace(
		std::piecewise_construct, std::forward_as_tuple(key.data(), key.size()),
		std::forward_as_tuple(value.data(), value.size()));
	if (!res.second) {
		auto it = res.first;
		it->second.assign(value.data(), value.size());
	}
	return status::OK;
}

status vsmap::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));

	// XXX - do not create temporary string
	size_t erased =
		pmem_kv_container.erase(key_type(key.data(), key.size(), kv_allocator));
	return (erased == 1) ? status::OK : status::NOT_FOUND;
}

internal::iterator<false> *vsmap::new_iterator()
{
	return new vsmap_iterator<false>{&pmem_kv_container, &kv_allocator};
}

internal::iterator<true> *vsmap::new_const_iterator()
{
	return new vsmap_iterator<true>{&pmem_kv_container, &kv_allocator};
}

template <bool IsConst>
vsmap::vsmap_iterator<IsConst>::vsmap_iterator(container_type *c,
					       vsmap::map_allocator_type *alloc)
    : container(c), kv_allocator(alloc), _it(c->begin())
{
}

template <bool IsConst>
status vsmap::vsmap_iterator<IsConst>::seek(string_view key)
{
	_it = container->find(vsmap::key_type(key.data(), key.size(), *kv_allocator));
	if (_it != container->end())
		return status::OK;

	return status::NOT_FOUND;
}

template <bool IsConst>
status vsmap::vsmap_iterator<IsConst>::seek_lower(string_view key)
{
	_it = container->lower_bound(
		vsmap::key_type(key.data(), key.size(), *kv_allocator));
	if (_it == container->begin()) {
		_it = container->end();
		return status::NOT_FOUND;
	}

	--_it;

	return status::OK;
}

template <bool IsConst>
status vsmap::vsmap_iterator<IsConst>::seek_lower_eq(string_view key)
{
	_it = container->upper_bound(
		vsmap::key_type(key.data(), key.size(), *kv_allocator));
	if (_it == container->begin()) {
		_it = container->end();
		return status::NOT_FOUND;
	}

	--_it;

	return status::OK;
}

template <bool IsConst>
status vsmap::vsmap_iterator<IsConst>::seek_higher(string_view key)
{
	_it = container->upper_bound(
		vsmap::key_type(key.data(), key.size(), *kv_allocator));
	if (_it == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

template <bool IsConst>
status vsmap::vsmap_iterator<IsConst>::seek_higher_eq(string_view key)
{
	_it = container->lower_bound(
		vsmap::key_type(key.data(), key.size(), *kv_allocator));
	if (_it == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

template <bool IsConst>
status vsmap::vsmap_iterator<IsConst>::seek_to_first()
{
	if (container->empty())
		return status::NOT_FOUND;

	_it = container->begin();

	return status::OK;
}

template <bool IsConst>
status vsmap::vsmap_iterator<IsConst>::seek_to_last()
{
	if (container->empty())
		return status::NOT_FOUND;

	_it = container->end();
	--_it;

	return status::OK;
}

template <bool IsConst>
status vsmap::vsmap_iterator<IsConst>::next()
{
	if (_it == container->end() || ++_it == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

template <bool IsConst>
status vsmap::vsmap_iterator<IsConst>::prev()
{
	if (_it == container->begin())
		return status::NOT_FOUND;

	--_it;

	return status::OK;
}

template <bool IsConst>
std::pair<string_view, status> vsmap::vsmap_iterator<IsConst>::key()
{
	if (_it == container->end())
		return {{}, status::NOT_FOUND};

	return {_it->first.data(), status::OK};
}

template <>
std::pair<string_view, status> vsmap::vsmap_iterator<true>::value()
{
	if (_it == container->end())
		return {{}, status::NOT_FOUND};

	return {{_it->second.data()}, status::OK};
}

template <>
std::pair<internal::accessor_base *, status> vsmap::vsmap_iterator<false>::value()
{
	if (_it == container->end())
		return {nullptr, status::NOT_FOUND};

	return {new vsmap_accessor{_it}, status::OK};
}

vsmap::vsmap_accessor::vsmap_accessor(map_type::iterator it) : _it(it)
{
}

std::pair<pmem::obj::slice<const char *>, status>
vsmap::vsmap_accessor::read_range(size_t pos, size_t n)
{
	if (pos + n > _it->second.size())
		n = _it->second.size() - pos;

	return {{_it->second.data() + pos, _it->second.data() + pos + n}, status::OK};
}

std::pair<pmem::obj::slice<char *>, status> vsmap::vsmap_accessor::write_range(size_t pos,
									       size_t n)
{
	if (pos + n > _it->second.size())
		n = _it->second.size() - pos;

	return {{&(_it->second[0]) + pos, &(_it->second[0]) + pos + n}, status::OK};
}

} // namespace kv
} // namespace pmem
