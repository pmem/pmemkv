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

#include "vsmap.h"
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
    : kv_allocator(get_path(*cfg), get_size(*cfg)), pmem_kv_container(kv_allocator)
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
	if (key1.compare(key2) < 0) {
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
	if (key1.compare(key2) < 0) {
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
	// XXX - do not create temporary string
	pmem_kv_container[key_type(key.data(), key.size(), kv_allocator)] =
		mapped_type(value.data(), value.size(), kv_allocator);
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

} // namespace kv
} // namespace pmem
