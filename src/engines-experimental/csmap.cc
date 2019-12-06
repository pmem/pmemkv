// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include "csmap.h"
#include "../out.h"

#include <unistd.h>

namespace pmem
{
namespace kv
{

csmap::csmap(std::unique_ptr<internal::config> cfg) : pmemobj_engine_base(cfg)
{
	LOG("Started ok");
	Recover();
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
	cnt = container->size();

	return status::OK;
}

status csmap::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	check_outside_tx();
	for (auto it = container->begin(); it != container->end(); ++it) {
		auto ret = callback(it->first.c_str(), it->first.size(),
				    it->second.val.c_str(), it->second.val.size(), arg);

		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

status csmap::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	/*
	 * We take read lock for thread-safe methods (like contains) to synchronize with
	 * unsafe_erase() which sis not thread-safe.
	 */
	std::shared_lock<mutex_type> lock(mtx);
	return container->contains(key) ? status::OK : status::NOT_FOUND;
}

status csmap::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	/*
	 * We take read lock for thread-safe methods (like find) to synchronize with
	 * unsafe_erase() which sis not thread-safe.
	 */
	std::shared_lock<mutex_type> lock(mtx);
	auto it = container->find(key);
	if (it != container->end()) {
		std::shared_lock<decltype(it->second.mtx)> lock(it->second.mtx);
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

	/*
	 * We take read lock for thread-safe methods (like emplace) to synchronize with
	 * unsafe_erase() which sis not thread-safe.
	 */
	std::shared_lock<mutex_type> lock(mtx);

	auto result = container->try_emplace(key, value);

	if (result.second == false) {
		auto &it = result.first;
		std::unique_lock<decltype(it->second.mtx)> lock(it->second.mtx);
		pmem::obj::transaction::manual tx(pmpool);
		it->second.val.assign(value.data(), value.size());
		pmem::obj::transaction::commit();
	}

	return status::OK;
}

status csmap::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));
	check_outside_tx();
	std::unique_lock<mutex_type> lock(mtx);
	return container->unsafe_erase(key) > 0 ? status::OK : status::NOT_FOUND;
}

status csmap::defrag(double start_percent, double amount_percent)
{
	LOG("defrag: start_percent = " << start_percent
				       << " amount_percent = " << amount_percent);
	check_outside_tx();

	return status::NOT_SUPPORTED;
}

void csmap::Recover()
{
	if (!OID_IS_NULL(*root_oid)) {
		container = (pmem::kv::internal::csmap::map_t *)pmemobj_direct(*root_oid);
		container->runtime_initialize();
	} else {
		{ // tx scope
			pmem::obj::transaction::manual tx(pmpool);
			pmem::obj::transaction::snapshot(root_oid);
			*root_oid = pmem::obj::make_persistent<internal::csmap::map_t>()
					    .raw();
			pmem::obj::transaction::commit();
		}
		container = (pmem::kv::internal::csmap::map_t *)pmemobj_direct(*root_oid);
		container->runtime_initialize();
	}
}

} // namespace kv
} // namespace pmem
