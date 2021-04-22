// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#ifndef LIBPMEMKV_ROBINHOOD_H
#define LIBPMEMKV_ROBINHOOD_H

#include <libpmemobj.h>
#include <stddef.h>
#include <stdint.h>

#include <mutex>
#include <shared_mutex>

#include <libpmemobj++/persistent_ptr.hpp>

#include "../comparator/pmemobj_comparator.h"
#include "../pmemobj_engine.h"

namespace pmem
{
namespace kv
{
namespace internal
{
namespace robinhood
{

#define SHARDS_DEFAULT 1024

#ifndef HASHMAP_RP_TYPE_OFFSET
#define HASHMAP_RP_TYPE_OFFSET 1008
#endif

/* Initial number of entries for hashamap_rp */
#define INIT_ENTRIES_NUM_RP 16
/* Load factor to indicate resize threshold */
#define HASHMAP_RP_LOAD_FACTOR 0.5f
/* Maximum number of swaps allowed during single insertion */
#define HASHMAP_RP_MAX_SWAPS 150
/* Size of an action array used during single insertion */
#define HASHMAP_RP_MAX_ACTIONS (4 * HASHMAP_RP_MAX_SWAPS + 5)
/* Size of a key or value (sizeof(uint64_t)) */
#define ENTRY_SIZE 8

#define TOMBSTONE_MASK (1ULL << 63)

/* layout definition */
struct hashmap_rp;
TOID_DECLARE(struct hashmap_rp, HASHMAP_RP_TYPE_OFFSET + 0);

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
};

struct hashmap_rp {
	/* number of values inserted */
	uint64_t count;

	/* container capacity */
	uint64_t capacity;

	/* resize threshold */
	uint64_t resize_threshold;

	/* load factor to indicate resize threshold */
	float load_factor;

	/* entries */
	TOID(struct entry) entries;
};

using map_type = hashmap_rp;

struct pmem_type {
	pmem_type() : map()
	{
		std::memset(reserved, 0, sizeof(reserved));
	}

	obj::persistent_ptr<TOID(struct hashmap_rp)[]> map;
	obj::p<size_t> shards_number;
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
	using mutex_type = std::shared_timed_mutex;
	using unique_lock_type = std::unique_lock<mutex_type>;
	using shared_lock_type = std::shared_lock<mutex_type>;

	void Recover();

	size_t shard_hash(uint64_t key);

	TOID(struct internal::robinhood::hashmap_rp) * container;

	std::vector<mutex_type> mtxs;

	size_t shards_number;
};

class robinhood_factory : public engine_base::factory_base {
public:
	std::unique_ptr<engine_base>
	create(std::unique_ptr<internal::config> cfg) override
	{
		check_config_null(get_name(), cfg);
		return std::unique_ptr<engine_base>(new robinhood(std::move(cfg)));
	};
	std::string get_name() override
	{
		return "robinhood";
	};
};

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_ROBINHOOD_H */
