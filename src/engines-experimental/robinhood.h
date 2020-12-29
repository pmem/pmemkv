// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#pragma once

#include "../comparator/pmemobj_comparator.h"
#include "../pmemobj_engine.h"

#include <libpmemobj++/persistent_ptr.hpp>

#include <libpmemobj.h>
#include <mutex>
#include <shared_mutex>
#include <stddef.h>
#include <stdint.h>

namespace pmem
{
namespace kv
{
namespace internal
{
namespace robinhood
{

#define SHARDS 1024

#ifndef HASHMAP_RP_TYPE_OFFSET
#define HASHMAP_RP_TYPE_OFFSET 1008
#endif

/* Flags to indicate if insertion is being made during rebuild process */
#define HASHMAP_RP_REBUILD 1
#define HASHMAP_RP_NO_REBUILD 0
/* Initial number of entries for hashamap_rp */
#define INIT_ENTRIES_NUM_RP 16
/* Load factor to indicate resize threshold */
#define HASHMAP_RP_LOAD_FACTOR 0.5f
/* Maximum number of swaps allowed during single insertion */
#define HASHMAP_RP_MAX_SWAPS 150
/* Size of an action array used during single insertion */
#define HASHMAP_RP_MAX_ACTIONS (4 * HASHMAP_RP_MAX_SWAPS + 5)

struct hashmap_rp;
TOID_DECLARE(struct hashmap_rp, HASHMAP_RP_TYPE_OFFSET + 0);

struct hashmap_args {
	uint32_t seed;
};

enum hashmap_cmd {
	HASHMAP_CMD_REBUILD,
	HASHMAP_CMD_DEBUG,
};

#define TOMBSTONE_MASK (1ULL << 63)

#ifdef DEBUG
#define HM_ASSERT(cnd) assert(cnd)
#else
#define HM_ASSERT(cnd)
#endif

/* layout definition */
TOID_DECLARE(struct entry, HASHMAP_RP_TYPE_OFFSET + 1);

struct entry {
	uint64_t key;
	uint64_t value;

	uint64_t hash;
};

struct add_entry {
	struct entry data;

	/* position index in hashmap, where data should be inserted/updated */
	size_t pos;

	/* Action array to perform addition in set of actions */
	struct pobj_action *actv;
	/* Action array index counter */
	size_t actv_cnt;

#ifdef DEBUG
	/* Swaps counter for current insertion. Enabled in debug mode */
	int swaps;
#endif
};

struct hashmap_rp {
	/* number of values inserted */
	uint64_t count;

	/* container capacity */
	uint64_t capacity;

	/* resize threshold */
	uint64_t resize_threshold;

	/* entries */
	TOID(struct entry) entries;
};

static int *swaps_array = NULL;

using map_type = hashmap_rp;

struct pmem_type {
	pmem_type() : map()
	{
		std::memset(reserved, 0, sizeof(reserved));
	}

	obj::persistent_ptr<TOID(struct hashmap_rp)[SHARDS]> map;
	uint64_t reserved[8];
};

} /* namespace robinhood */
} /* namespace internal */

class robinhood : public pmemobj_engine_base<internal::robinhood::pmem_type> {
public:
	robinhood(std::unique_ptr<internal::config> cfg);
	~robinhood();

	robinhood(const robinhood &) = delete;
	robinhood &operator=(const robinhood &) = delete;

	std::string name() final;

	status count_all(std::size_t &cnt) final;

	status get_all(get_kv_callback *callback, void *arg) final;

	status exists(string_view key) final;

	status get(string_view key, get_v_callback *callback, void *arg) final;

	status put(string_view key, string_view value) final;

	status remove(string_view key) final;

private:
	using container_type = internal::robinhood::map_type;

	void Recover();

	TOID(struct internal::robinhood::hashmap_rp) * container;

	using mutex_type = std::shared_mutex;
	using unique_lock_type = std::unique_lock<mutex_type>;
	using shared_lock_type = std::shared_lock<mutex_type>;

	std::vector<mutex_type> mtxs;

	mutex_type global_mtx;
};

} /* namespace kv */
} /* namespace pmem */
