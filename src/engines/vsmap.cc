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
#include <libpmemobj++/transaction.hpp>

#include <iostream>

#define DO_LOG 0
#define LOG(msg)                                                                         \
	do {                                                                             \
		if (DO_LOG)                                                              \
			std::cout << "[blackhole] " << msg << "\n";                      \
	} while (0)

namespace pmem
{
namespace kv
{

vsmap::vsmap(void *context, const std::string &path, size_t size)
    : context(context), kv_allocator(path, size), pmem_kv_container(kv_allocator)
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

void *vsmap::engine_context()
{
	return context;
}

status vsmap::all(all_callback *callback, void *arg)
{
	LOG("All");
	for (auto &it : pmem_kv_container)
		(*callback)(it.first.c_str(), it.first.size(), arg);

	return status::OK;
}

status vsmap::all_above(const std::string &key, all_callback *callback, void *arg)
{
	LOG("AllAbove for key=" << key);
	auto it = pmem_kv_container.upper_bound(
		key_type(key.c_str(), key.size(), kv_allocator));
	auto end = pmem_kv_container.end();
	for (; it != end; it++)
		(*callback)(it->first.c_str(), it->first.size(), arg);

	return status::OK;
}

status vsmap::all_below(const std::string &key, all_callback *callback, void *arg)
{
	LOG("AllBelow for key=" << key);
	auto it = pmem_kv_container.begin();
	auto end = pmem_kv_container.lower_bound(
		key_type(key.c_str(), key.size(), kv_allocator));
	for (; it != end; it++)
		(*callback)(it->first.c_str(), it->first.size(), arg);

	return status::OK;
}

status vsmap::all_between(const std::string &key1, const std::string &key2,
			  all_callback *callback, void *arg)
{
	LOG("AllBetween for key1=" << key1 << ", key2=" << key2);
	if (key1 < key2) {
		auto it = pmem_kv_container.upper_bound(
			key_type(key1.c_str(), key1.size(), kv_allocator));
		auto end = pmem_kv_container.lower_bound(
			key_type(key2.c_str(), key2.size(), kv_allocator));
		for (; it != end; it++)
			(*callback)(it->first.c_str(), it->first.size(), arg);
	}

	return status::OK;
}

status vsmap::count(std::size_t &cnt)
{
	cnt = pmem_kv_container.size();

	return status::OK;
}

status vsmap::count_above(const std::string &key, std::size_t &cnt)
{
	LOG("CountAbove for key=" << key);
	std::size_t result = 0;
	auto it = pmem_kv_container.upper_bound(
		key_type(key.c_str(), key.size(), kv_allocator));
	auto end = pmem_kv_container.end();
	for (; it != end; it++)
		result++;

	cnt = result;

	return status::OK;
}

status vsmap::count_below(const std::string &key, std::size_t &cnt)
{
	LOG("CountBelow for key=" << key);
	std::size_t result = 0;
	auto it = pmem_kv_container.begin();
	auto end = pmem_kv_container.lower_bound(
		key_type(key.c_str(), key.size(), kv_allocator));
	for (; it != end; it++)
		result++;

	cnt = result;

	return status::OK;
}

status vsmap::count_between(const std::string &key1, const std::string &key2,
			    std::size_t &cnt)
{
	LOG("CountBetween for key1=" << key1 << ", key2=" << key2);
	std::size_t result = 0;
	if (key1 < key2) {
		auto it = pmem_kv_container.upper_bound(
			key_type(key1.c_str(), key1.size(), kv_allocator));
		auto end = pmem_kv_container.lower_bound(
			key_type(key2.c_str(), key2.size(), kv_allocator));
		for (; it != end; it++)
			result++;
	}

	cnt = result;

	return status::OK;
}

status vsmap::each(each_callback *callback, void *arg)
{
	LOG("Each");
	for (auto &it : pmem_kv_container)
		(*callback)(it.first.c_str(), it.first.size(), it.second.c_str(),
			    it.second.size(), arg);

	return status::OK;
}

status vsmap::each_above(const std::string &key, each_callback *callback, void *arg)
{
	LOG("EachAbove for key=" << key);
	auto it = pmem_kv_container.upper_bound(
		key_type(key.c_str(), key.size(), kv_allocator));
	auto end = pmem_kv_container.end();
	for (; it != end; it++)
		(*callback)(it->first.c_str(), it->first.size(), it->second.c_str(),
			    it->second.size(), arg);

	return status::OK;
}

status vsmap::each_below(const std::string &key, each_callback *callback, void *arg)
{
	LOG("EachBelow for key=" << key);
	auto it = pmem_kv_container.begin();
	auto end = pmem_kv_container.lower_bound(
		key_type(key.c_str(), key.size(), kv_allocator));
	for (; it != end; it++)
		(*callback)(it->first.c_str(), it->first.size(), it->second.c_str(),
			    it->second.size(), arg);

	return status::OK;
}

status vsmap::each_between(const std::string &key1, const std::string &key2,
			   each_callback *callback, void *arg)
{
	LOG("EachBetween for key1=" << key1 << ", key2=" << key2);
	if (key1 < key2) {
		auto it = pmem_kv_container.upper_bound(
			key_type(key1.c_str(), key1.size(), kv_allocator));
		auto end = pmem_kv_container.lower_bound(
			key_type(key2.c_str(), key2.size(), kv_allocator));
		for (; it != end; it++)
			(*callback)(it->first.c_str(), it->first.size(),
				    it->second.c_str(), it->second.size(), arg);
	}

	return status::OK;
}

status vsmap::exists(const std::string &key)
{
	LOG("Exists for key=" << key);
	bool r =
		pmem_kv_container.find(key_type(key.c_str(), key.size(), kv_allocator)) !=
		pmem_kv_container.end();
	return (r ? status::OK : status::NOT_FOUND);
}

status vsmap::get(const std::string &key, get_callback *callback, void *arg)
{
	LOG("Get key=" << key);
	const auto pos =
		pmem_kv_container.find(key_type(key.c_str(), key.size(), kv_allocator));
	if (pos == pmem_kv_container.end()) {
		LOG("  key not found");
		return status::NOT_FOUND;
	}

	(*callback)(pos->second.c_str(), pos->second.size(), arg);
	return status::OK;
}

status vsmap::put(const std::string &key, const std::string &value)
{
	LOG("Put key=" << key << ", value.size=" << std::to_string(value.size()));
	try {
		pmem_kv_container[key_type(key.c_str(), key.size(), kv_allocator)] =
			mapped_type(value.c_str(), value.size(), kv_allocator);
		return status::OK;
	} catch (std::bad_alloc e) {
		LOG("Put failed due to exception, " << e.what());
		return status::FAILED;
	} catch (pmem::transaction_alloc_error e) {
		LOG("Put failed due to pmem::transaction_alloc_error, " << e.what());
		return status::FAILED;
	} catch (pmem::transaction_error e) {
		LOG("Put failed due to pmem::transaction_error, " << e.what());
		return status::FAILED;
	}
}

status vsmap::remove(const std::string &key)
{
	LOG("Remove key=" << key);
	try {
		size_t erased = pmem_kv_container.erase(
			key_type(key.c_str(), key.size(), kv_allocator));
		return (erased == 1) ? status::OK : status::NOT_FOUND;
	} catch (std::bad_alloc e) {
		LOG("Put failed due to exception, " << e.what());
		return status::FAILED;
	} catch (pmem::transaction_alloc_error e) {
		LOG("Put failed due to pmem::transaction_alloc_error, " << e.what());
		return status::FAILED;
	} catch (pmem::transaction_error e) {
		LOG("Put failed due to pmem::transaction_error, " << e.what());
		return status::FAILED;
	}
}

} // namespace kv
} // namespace pmem
