// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#ifndef LIBPMEMKV_PMEMOBJ_HANDLE_H
#define LIBPMEMKV_PMEMOBJ_HANDLE_H

#include <cassert>
#include <iostream>
#include <unistd.h>

#include "config.h"
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmem
{

/* Helper method which throws an exception when called in a tx */
static inline void check_outside_tx()
{
	if (pmemobj_tx_stage() != TX_STAGE_NONE)
		throw transaction_scope_error(
			"Function called inside transaction scope.");
}

namespace kv
{

template <typename T>
struct layout {
	static const char *name()
	{
		return T::layout();
	}
};

template <typename EngineData>
class pmemobj_handle {
public:
	pmemobj_handle(std::unique_ptr<internal::config> &cfg)
	{
		const char *path = nullptr;
		std::size_t size;
		PMEMoid *oid;

		auto is_path = cfg->get_string("path", &path);
		auto is_oid = cfg->get_object("oid", (void **)&oid);

		if (is_path && is_oid) {
			throw internal::invalid_argument(
				"Config contains both: \"path\" and \"oid\"");
		} else if (!is_path && !is_oid) {
			throw internal::invalid_argument(
				"Config does not contain item with key: \"path\" or \"oid\"");
		} else if (is_path) {
			uint64_t force_create;
			cfg_by_path = true;

			if (!cfg->get_uint64("force_create", &force_create)) {
				force_create = 0;
			}

			pmem::obj::pool<Root> pop;
			if (force_create) {
				if (!cfg->get_uint64("size", &size))
					throw internal::invalid_argument(
						"Config does not contain item with key: \"size\"");

				pop = pmem::obj::pool<Root>::create(
					path, layout<EngineData>::name(), size, S_IRWXU);
			} else {
				pop = pmem::obj::pool<Root>::open(
					path, layout<EngineData>::name());
			}

			root_oid = pop.root()->ptr.raw_ptr();
			pmpool = pop;

		} else if (is_oid) {
			pmpool = pmem::obj::pool_base(pmemobj_pool_by_ptr(oid));
			root_oid = oid;
		}

		cached_ptr = static_cast<EngineData *>(pmemobj_direct(*root_oid));
	}

	~pmemobj_handle()
	{
		if (cfg_by_path)
			pmpool.close();
	}

	void initialize(pmem::obj::persistent_ptr<EngineData> ptr)
	{
		assert(pmemobj_tx_stage() == TX_STAGE_WORK);

		pmem::obj::transaction::snapshot(root_oid);
		*root_oid = ptr.raw();

		cached_ptr = static_cast<EngineData *>(pmemobj_direct(*root_oid));
	}

	pmem::obj::pool_base pool()
	{
		return pmpool;
	}

	EngineData *get()
	{
		return cached_ptr;
	}

	EngineData *operator->()
	{
		return get();
	}

private:
	struct Root {
		pmem::obj::persistent_ptr<EngineData>
			ptr; /* used when path is specified */
	};

	pmem::obj::pool_base pmpool;
	PMEMoid *root_oid;

	bool cfg_by_path = false;

	EngineData *cached_ptr = nullptr;
};

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_PMEMOBJ_HANDLE_H */
