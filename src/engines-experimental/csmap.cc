// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "csmap.h"
#include "../out.h"

namespace pmem
{
namespace kv
{

csmap::csmap(std::unique_ptr<internal::config> cfg) : container(cfg)
{
	recover();
	LOG("Started ok");
}

csmap::~csmap()
{
	LOG("Stopped ok");
}

std::string csmap::name()
{
	return "csmap";
}

status csmap::count_all(std::size_t &cnt)
{
	LOG("count_all");
	check_outside_tx();
	cnt = container->map.size();

	return status::OK;
}

template <typename It>
static std::size_t size(It first, It last)
{
	auto dist = std::distance(first, last);
	assert(dist >= 0);

	return static_cast<std::size_t>(dist);
}

status csmap::count_above(string_view key, std::size_t &cnt)
{
	LOG("count_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->map.upper_bound(key);
	auto last = container->map.end();

	cnt = size(first, last);

	return status::OK;
}

status csmap::count_equal_above(string_view key, std::size_t &cnt)
{
	LOG("count_equal_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->map.lower_bound(key);
	auto last = container->map.end();

	cnt = size(first, last);

	return status::OK;
}

status csmap::count_equal_below(string_view key, std::size_t &cnt)
{
	LOG("count_equal_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->map.begin();
	auto last = container->map.upper_bound(key);

	cnt = size(first, last);

	return status::OK;
}

status csmap::count_below(string_view key, std::size_t &cnt)
{
	LOG("count_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->map.begin();
	auto last = container->map.lower_bound(key);

	cnt = size(first, last);

	return status::OK;
}

status csmap::count_between(string_view key1, string_view key2, std::size_t &cnt)
{
	LOG("count_between for key1=" << key1.data() << ", key2=" << key2.data());

	if (container->map.key_comp()(key1, key2)) {
		shared_global_lock_type lock(mtx);

		auto first = container->map.upper_bound(key1);
		auto last = container->map.lower_bound(key2);

		cnt = size(first, last);
	} else {
		cnt = 0;
	}

	return status::OK;
}

status csmap::iterate(typename container_type::iterator first,
		      typename container_type::iterator last, get_kv_callback *callback,
		      void *arg)
{
	for (auto it = first; it != last; ++it) {
		shared_node_lock_type lock(it->second.mtx);

		auto ret = callback(it->first.c_str(), it->first.size(),
				    it->second.val.c_str(), it->second.val.size(), arg);

		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

status csmap::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->map.begin();
	auto last = container->map.end();

	return iterate(first, last, callback, arg);
}

status csmap::get_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->map.upper_bound(key);
	auto last = container->map.end();

	return iterate(first, last, callback, arg);
}

status csmap::get_equal_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->map.lower_bound(key);
	auto last = container->map.end();

	return iterate(first, last, callback, arg);
}

status csmap::get_equal_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->map.begin();
	auto last = container->map.upper_bound(key);

	return iterate(first, last, callback, arg);
}

status csmap::get_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->map.begin();
	auto last = container->map.lower_bound(key);

	return iterate(first, last, callback, arg);
}

status csmap::get_between(string_view key1, string_view key2, get_kv_callback *callback,
			  void *arg)
{
	LOG("get_between for key1=" << key1.data() << ", key2=" << key2.data());
	check_outside_tx();

	if (container->map.key_comp()(key1, key2)) {
		shared_global_lock_type lock(mtx);

		auto first = container->map.upper_bound(key1);
		auto last = container->map.lower_bound(key2);
		return iterate(first, last, callback, arg);
	}

	return status::OK;
}

status csmap::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);
	return container->map.contains(key) ? status::OK : status::NOT_FOUND;
}

status csmap::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);
	auto it = container->map.find(key);
	if (it != container->map.end()) {
		shared_node_lock_type lock(it->second.mtx);
		callback(it->second.val.c_str(), it->second.val.size(), arg);
		return status::OK;
	}

	LOG("  key not found");
	return status::NOT_FOUND;
}

status csmap::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto result = container->map.try_emplace(key, value);

	if (result.second == false) {
		auto pop = container.pool();

		auto &it = result.first;
		unique_node_lock_type lock(it->second.mtx);
		pmem::obj::transaction::run(
			pop, [&] { it->second.val.assign(value.data(), value.size()); });
	}

	return status::OK;
}

status csmap::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));
	check_outside_tx();
	unique_global_lock_type lock(mtx);
	return container->map.unsafe_erase(key) > 0 ? status::OK : status::NOT_FOUND;
}

void csmap::recover()
{
	auto pop = container.pool();

	if (!container.get()) {
		obj::transaction::run(pop, [&] {
			container.initialize(
				obj::make_persistent<internal::csmap::pmem_type>());
		});
	}

	container->map.runtime_initialize();
}

} // namespace kv
} // namespace pmem
