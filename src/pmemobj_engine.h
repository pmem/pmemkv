// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2021, Intel Corporation */

#ifndef LIBPMEMKV_PMEMOBJ_ENGINE_H
#define LIBPMEMKV_PMEMOBJ_ENGINE_H

#include "engine.h"
#include "libpmemkv.h"
#include <libpmemobj++/pool.hpp>

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

template <typename EngineData>
class pmemobj_engine_base : public engine_base {
public:
	pmemobj_engine_base(const std::unique_ptr<internal::config> &cfg,
			    const std::string &layout)
	{
		const char *path = nullptr;
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
			uint64_t create_or_error_if_exists = 0;
			uint64_t create_if_missing = 0;
			cfg_by_path = true;

			cfg->get_uint64("create_if_missing", &create_if_missing);
			if (!cfg->get_uint64("create_or_error_if_exists",
					     &create_or_error_if_exists)) {
				/* 'force_create' is here for compatibility with bindings,
				 * which may still use this flag in their API */
				cfg->get_uint64("force_create",
						&create_or_error_if_exists);
			}

			if (create_if_missing && create_or_error_if_exists) {
				throw internal::invalid_argument(
					"Both flags set in config: \"create_if_missing\" and \"create_or_error_if_exists\"");
			}

			if (create_if_missing || create_or_error_if_exists) {
				bool failed_open = false;
				if (!create_or_error_if_exists)
					try {
						pmpool = pmem::obj::pool<Root>::open(
							path, layout);
					} catch (pmem::pool_invalid_argument &e) {
						failed_open = true;
					}

				if (failed_open || create_or_error_if_exists) {
					auto size = cfg->get_size();
					pmpool = create_or_fail(path, size, layout);
				}
			} else { /* no flags set, just open */
				try {
					pmpool =
						pmem::obj::pool<Root>::open(path, layout);
				} catch (pmem::pool_invalid_argument &e) {
					throw internal::invalid_argument(e.what());
				}
			}

			root_oid = static_cast<pmem::obj::pool<Root>>(pmpool)
					   .root()
					   ->ptr.raw_ptr();

		} else if (is_oid) {
			pmpool = pmem::obj::pool_base(pmemobj_pool_by_ptr(oid));
			root_oid = oid;
		}
	}

	~pmemobj_engine_base()
	{
		if (cfg_by_path) {
			try {
				pmpool.close();
			} catch (const std::logic_error &e) {
				std::terminate();
			}
		}
	}

protected:
	struct Root {
		/* field ptr used when path is specified */
		pmem::obj::persistent_ptr<EngineData> ptr;
	};

	pmem::obj::pool_base pmpool;
	PMEMoid *root_oid;
	bool cfg_by_path = false;

private:
	pmem::obj::pool<Root> create_or_fail(const char *path, const std::size_t size,
					     const std::string &layout)
	{
		try {
			return pmem::obj::pool<Root>::create(path, layout, size, S_IRWXU);
		} catch (pmem::pool_invalid_argument &e) {
			throw internal::invalid_argument(e.what());
		}
	}
};

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_PMEMOBJ_ENGINE_H */
