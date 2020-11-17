// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include "vcmap.h"
#include "../out.h"
#include <libpmemobj++/transaction.hpp>

#include <cassert>
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

vcmap::vcmap(std::unique_ptr<internal::config> cfg)
    : kv_allocator(get_path(*cfg), get_size(*cfg)),
      ch_allocator(kv_allocator),
      pmem_kv_container(std::scoped_allocator_adaptor<kv_allocator_t>(kv_allocator))
{
	LOG("Started ok");
}

vcmap::~vcmap()
{
	LOG("Stopped ok");
}

std::string vcmap::name()
{
	return "vcmap";
}

status vcmap::count_all(std::size_t &cnt)
{
	LOG("count_all");
	cnt = pmem_kv_container.size();

	return status::OK;
}

status vcmap::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	for (auto &iterator : pmem_kv_container) {
		auto ret = callback(iterator.first.c_str(), iterator.first.size(),
				    iterator.second.c_str(), iterator.second.size(), arg);

		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

status vcmap::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	map_t::const_accessor result;
	// XXX - do not create temporary string
	const bool result_found = pmem_kv_container.find(
		result, pmem_string(key.data(), key.size(), ch_allocator));
	return (result_found ? status::OK : status::NOT_FOUND);
}

status vcmap::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));
	map_t::const_accessor result;
	// XXX - do not create temporary string
	const bool result_found = pmem_kv_container.find(
		result, pmem_string(key.data(), key.size(), ch_allocator));
	if (!result_found) {
		LOG("  key not found");
		return status::NOT_FOUND;
	}

	callback(result->second.c_str(), result->second.size(), arg);
	return status::OK;
}

status vcmap::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));

	map_t::value_type kv_pair{// XXX - do not create temporary string
				  pmem_string(key.data(), key.size(), ch_allocator),
				  pmem_string(value.data(), value.size(), ch_allocator)};
	bool result = pmem_kv_container.insert(kv_pair);
	if (!result) {
		map_t::accessor result_found;
		pmem_kv_container.find(result_found, kv_pair.first);
		result_found->second = kv_pair.second;
	}
	return status::OK;
}

status vcmap::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));

	// XXX - do not create temporary string
	size_t erased = pmem_kv_container.erase(
		pmem_string(key.data(), key.size(), ch_allocator));
	return (erased == 1) ? status::OK : status::NOT_FOUND;
}

internal::iterator_base *vcmap::new_iterator()
{
	return new vcmap_iterator<false>{&pmem_kv_container, &ch_allocator};
}

internal::iterator_base *vcmap::new_const_iterator()
{
	return new vcmap_iterator<true>{&pmem_kv_container, &ch_allocator};
}

vcmap::vcmap_iterator<true>::vcmap_iterator(container_type *c, ch_allocator_t *ca)
    : container(c), _acc(), ch_allocator(ca)
{
}

vcmap::vcmap_iterator<false>::vcmap_iterator(container_type *c, ch_allocator_t *ca)
    : vcmap::vcmap_iterator<true>(c, ca)
{
}

status vcmap::vcmap_iterator<true>::seek(string_view key)
{
	init_seek();

	if (container->find(_acc, pmem_string(key.data(), key.size(), *ch_allocator)))
		return status::OK;

	return status::NOT_FOUND;
}

std::pair<string_view, status> vcmap::vcmap_iterator<true>::key()
{
	assert(!_acc.empty());

	return {{_acc->first.c_str()}, status::OK};
}

std::pair<pmem::obj::slice<const char *>, status>
vcmap::vcmap_iterator<true>::read_range(size_t pos, size_t n)
{
	assert(!_acc.empty());

	if (pos + n > _acc->second.size() || pos + n < pos)
		n = _acc->second.size() - pos;

	return {{_acc->second.c_str() + pos, _acc->second.c_str() + pos + n}, status::OK};
}

std::pair<pmem::obj::slice<char *>, status>
vcmap::vcmap_iterator<false>::write_range(size_t pos, size_t n)
{
	assert(!_acc.empty());

	if (pos + n > _acc->second.size() || pos + n < pos)
		n = _acc->second.size() - pos;

	log.push_back({std::string(_acc->second.c_str() + pos, n), pos});
	auto &val = log.back().first;

	return {{&val[0], &val[n]}, status::OK};
}

status vcmap::vcmap_iterator<false>::commit()
{
	for (auto &p : log) {
		auto dest = &(_acc->second[0]) + p.second;
		std::copy(p.first.begin(), p.first.end(), dest);
	}
	log.clear();

	return status::OK;
}

void vcmap::vcmap_iterator<false>::abort()
{
	log.clear();
}

} // namespace kv
} // namespace pmem
