// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include "vcmap.h"
#include "../out.h"

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

status vcmap::update(string_view key, size_t v_offset, size_t v_size,
		     update_v_callback *callback, void *arg)
{
	LOG("update key=" << std::string(key.data(), key.size()));

	map_t::accessor result;
	auto result_found = pmem_kv_container.find(
		result, pmem_string(key.data(), key.size(), ch_allocator));
	if (!result_found) {
		LOG("  key not found");
		return status::NOT_FOUND;
	}

	char *editable_element = const_cast<char *>(result->second.c_str()) + v_offset;
	auto editable_element_size = std::min(v_size, result->second.size());

	return snapshot(editable_element, editable_element_size, [&]() {
		return callback(editable_element, editable_element_size, arg);
	});
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

} // namespace kv
} // namespace pmem
