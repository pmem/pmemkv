// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#include "robinhood.h"
#include "../fast_hash.h"
#include "../out.h"

#include <unistd.h>

namespace pmem
{
namespace kv
{

namespace internal
{
namespace robinhood
{

static std::function<std::string(void)> name;

/* Load factor to indicate resize threshold */
static float load_factor;

/*
 * entry_is_deleted -- checks 'tombstone' bit if hash is deleted
 */
static inline int entry_is_deleted(uint64_t hash)
{
	return (hash & TOMBSTONE_MASK) > 0;
}

/*
 * entry_is_empty -- checks if entry is empty
 */
static inline int entry_is_empty(uint64_t hash)
{
	return hash == 0 || entry_is_deleted(hash);
}

/*
 * increment_pos -- increment position index, skip 0
 */
static uint64_t increment_pos(const struct hashmap_rp *hashmap, uint64_t pos)
{
	pos = (pos + 1) & (hashmap->capacity - 1);
	return pos == 0 ? 1 : pos;
}

/*
 * probe_distance -- returns probe number, an indicator how far from
 * desired position given hash is stored in hashmap
 */
static uint64_t probe_distance(const struct hashmap_rp *hashmap, uint64_t hash_key,
			       uint64_t slot_index)
{
	uint64_t capacity = hashmap->capacity;

	return static_cast<uint64_t>(slot_index + capacity - hash_key) & (capacity - 1);
}

/*
 * hash -- hash function based on Austin Appleby MurmurHash3 64-bit finalizer.
 * Returned value is modified to work with special values for unused and
 * and deleted hashes.
 */
static uint64_t hash(const struct hashmap_rp *hashmap, uint64_t key)
{
	key ^= key >> 33;
	key *= 0xff51afd7ed558ccd;
	key ^= key >> 33;
	key *= 0xc4ceb9fe1a85ec53;
	key ^= key >> 33;
	key &= hashmap->capacity - 1;

	/* first, 'tombstone' bit is used to indicate deleted item */
	key &= ~TOMBSTONE_MASK;

	/*
	 * Ensure that we never return 0 as a hash, since we use 0 to
	 * indicate that element has never been used at all.
	 */
	return key == 0 ? 1 : key;
}

/*
 * hashmap_create -- hashmap initializer
 */
static void hashmap_create(PMEMobjpool *pop, TOID(struct hashmap_rp) * hashmap_p)
{
	struct pobj_action actv[4];
	size_t actv_cnt = 0;

	TOID(struct hashmap_rp)
	hashmap = POBJ_RESERVE_NEW(pop, struct hashmap_rp, &actv[actv_cnt]);

	if (TOID_IS_NULL(hashmap)) {
		LOG(std::string("hashmap alloc failed: ") +
		    std::string(pmemobj_errormsg()));
		pmemobj_cancel(pop, actv, actv_cnt);
		abort();
	}
	actv_cnt++;

	D_RW(hashmap)->count = 0;
	D_RW(hashmap)->capacity = INIT_ENTRIES_NUM_RP;
	D_RW(hashmap)->resize_threshold =
		static_cast<uint64_t>(INIT_ENTRIES_NUM_RP * load_factor);

	size_t sz = sizeof(struct entry) * D_RO(hashmap)->capacity;
	/* init entries with zero in order to track unused hashes */
	D_RW(hashmap)->entries = POBJ_XRESERVE_ALLOC(pop, struct entry, sz,
						     &actv[actv_cnt], POBJ_XALLOC_ZERO);
	if (TOID_IS_NULL(D_RO(hashmap)->entries)) {
		LOG(std::string("hashmap alloc failed: ") +
		    std::string(pmemobj_errormsg()));
		pmemobj_cancel(pop, actv, actv_cnt);
		abort();
	}
	actv_cnt++;

	pmemobj_persist(pop, D_RW(hashmap), sizeof(struct hashmap_rp));

	pmemobj_set_value(pop, &actv[actv_cnt++], &(hashmap_p->oid.pool_uuid_lo),
			  hashmap.oid.pool_uuid_lo);

	pmemobj_set_value(pop, &actv[actv_cnt++], &(hashmap_p->oid.off), hashmap.oid.off);

	pmemobj_publish(pop, actv, actv_cnt);

	return;
}

/*
 * entry_update -- updates entry in given hashmap with given arguments
 */
static void entry_update(PMEMobjpool *pop, struct hashmap_rp *hashmap,
			 struct add_entry *args, bool rebuild)
{
	struct entry *entry_p = D_RW(hashmap->entries);
	entry_p += args->pos;

	if (rebuild) {
		entry_p->key = args->data.key;
		entry_p->value = args->data.value;
		entry_p->hash = args->data.hash;
	} else {
		pmemobj_set_value(pop, args->actv + args->actv_cnt++, &entry_p->key,
				  args->data.key);
		pmemobj_set_value(pop, args->actv + args->actv_cnt++, &entry_p->value,
				  args->data.value);
		pmemobj_set_value(pop, args->actv + args->actv_cnt++, &entry_p->hash,
				  args->data.hash);
	}
}

/*
 * entry_add -- increments given hashmap's elements counter and calls
 * entry_update
 */
static void entry_add(PMEMobjpool *pop, struct hashmap_rp *hashmap,
		      struct add_entry *args, bool rebuild)
{
	if (rebuild)
		hashmap->count++;
	else {
		pmemobj_set_value(pop, args->actv + args->actv_cnt++, &hashmap->count,
				  hashmap->count + 1);
	}

	entry_update(pop, hashmap, args, rebuild);
}

/*
 * insert_helper -- inserts specified value into the hashmap
 * If function was called during rebuild process, no redo logs will be used.
 * returns:
 * - 0 if successful,
 * - 1 if value already existed
 * - -1 on error
 */
static int insert_helper(PMEMobjpool *pop, struct hashmap_rp *hashmap, uint64_t key,
			 uint64_t value, bool rebuild)
{
	struct pobj_action actv[HASHMAP_RP_MAX_ACTIONS];

	struct add_entry args;
	args.data.key = key;
	args.data.value = value;
	args.data.hash = hash(hashmap, key);
	args.pos = args.data.hash;
	if (!rebuild) {
		args.actv = actv;
		args.actv_cnt = 0;
	}

	uint64_t dist = 0;
	struct entry *entry_p = NULL;

	for (int n = 0; n < HASHMAP_RP_MAX_SWAPS; ++n) {
		entry_p = D_RW(hashmap->entries);
		entry_p += args.pos;

		/* Case 1: key already exists, override value */
		if (!entry_is_empty(entry_p->hash) && entry_p->key == args.data.key) {
			entry_update(pop, hashmap, &args, rebuild);
			if (!rebuild)
				pmemobj_publish(pop, args.actv, args.actv_cnt);

			return 0;
		}

		/* Case 2: slot is empty from the beginning */
		if (entry_p->hash == 0) {
			entry_add(pop, hashmap, &args, rebuild);
			if (!rebuild)
				pmemobj_publish(pop, args.actv, args.actv_cnt);

			return 0;
		}

		/*
		 * Case 3: existing element (or tombstone) has probed less than
		 * current element. Swap them (or put into tombstone slot) and
		 * keep going to find another slot for that element.
		 */
		uint64_t existing_dist = probe_distance(hashmap, entry_p->hash, args.pos);
		if (existing_dist < dist) {
			if (entry_is_deleted(entry_p->hash)) {
				entry_add(pop, hashmap, &args, rebuild);
				if (!rebuild)
					pmemobj_publish(pop, args.actv, args.actv_cnt);

				return 0;
			}

			struct entry temp = *entry_p;
			entry_update(pop, hashmap, &args, rebuild);
			args.data = temp;

			dist = existing_dist;
		}

		/*
		 * Case 4: increment slot number and probe counter, keep going
		 * to find free slot
		 */
		args.pos = increment_pos(hashmap, args.pos);
		dist += 1;
	}
	LOG("insertion requires too many swaps");
	if (!rebuild)
		pmemobj_cancel(pop, args.actv, args.actv_cnt);

	return -1;
}

/*
 * index_lookup -- checks if given key exists in hashmap.
 * Returns index number if key was found, 0 otherwise.
 */
static uint64_t index_lookup(const struct hashmap_rp *hashmap, uint64_t key)
{
	const uint64_t hash_lookup = hash(hashmap, key);
	uint64_t pos = hash_lookup;
	uint64_t dist = 0;

	const struct entry *entry_p = NULL;
	do {
		entry_p = D_RO(hashmap->entries);
		entry_p += pos;

		if (entry_p->hash == hash_lookup && entry_p->key == key)
			return pos;

		pos = increment_pos(hashmap, pos);

	} while (entry_p->hash != 0 &&
		 (dist++) <= probe_distance(hashmap, entry_p->hash, pos) - 1);

	return 0;
}

/*
 * entries_cache -- cache entries from second argument in entries from first
 * argument
 */
static int entries_cache(PMEMobjpool *pop, struct hashmap_rp *dest,
			 const struct hashmap_rp *src)
{
	const struct entry *e_begin = D_RO(src->entries);
	const struct entry *e_end = e_begin + src->capacity;

	for (const struct entry *e = e_begin; e != e_end; ++e) {
		if (entry_is_empty(e->hash))
			continue;

		if (insert_helper(pop, dest, e->key, e->value, true) == -1)
			return -1;
	}
	assert(src->count == dest->count);

	return 0;
}

/*
 * hm_rp_rebuild -- rebuilds the hashmap with a new capacity.
 * Returns 0 on success, -1 otherwise.
 */
static int hm_rp_rebuild(PMEMobjpool *pop, TOID(struct hashmap_rp) hashmap,
			 size_t capacity_new)
{
	/*
	 * We will need 6 actions:
	 * - 1 action to set new capacity
	 * - 1 action to set new resize threshold
	 * - 1 action to alloc memory for new entries
	 * - 1 action to free old entries
	 * - 2 actions to set new oid pointing to new entries
	 */
	struct pobj_action actv[6];
	size_t actv_cnt = 0;

	size_t sz_alloc = sizeof(struct entry) * capacity_new;
	uint64_t resize_threshold_new = static_cast<uint64_t>(capacity_new * load_factor);

	pmemobj_set_value(pop, &actv[actv_cnt++], &D_RW(hashmap)->capacity, capacity_new);

	pmemobj_set_value(pop, &actv[actv_cnt++], &D_RW(hashmap)->resize_threshold,
			  resize_threshold_new);

	struct hashmap_rp hashmap_rebuild;
	hashmap_rebuild.count = 0;
	hashmap_rebuild.capacity = capacity_new;
	hashmap_rebuild.resize_threshold = resize_threshold_new;
	hashmap_rebuild.entries = POBJ_XRESERVE_ALLOC(pop, struct entry, sz_alloc,
						      &actv[actv_cnt], POBJ_XALLOC_ZERO);

	if (TOID_IS_NULL(hashmap_rebuild.entries)) {
		LOG(std::string("hashmap alloc failed: ") +
		    std::string(pmemobj_errormsg()));
		goto rebuild_err;
	}
	actv_cnt++;

	if (entries_cache(pop, &hashmap_rebuild, D_RW(hashmap)) == -1)
		goto rebuild_err;

	pmemobj_persist(pop, D_RW(hashmap_rebuild.entries), sz_alloc);

	pmemobj_defer_free(pop, D_RW(hashmap)->entries.oid, &actv[actv_cnt++]);

	pmemobj_set_value(pop, &actv[actv_cnt++],
			  &D_RW(hashmap)->entries.oid.pool_uuid_lo,
			  hashmap_rebuild.entries.oid.pool_uuid_lo);
	pmemobj_set_value(pop, &actv[actv_cnt++], &D_RW(hashmap)->entries.oid.off,
			  hashmap_rebuild.entries.oid.off);

	assert(sizeof(actv) / sizeof(actv[0]) >= actv_cnt);
	pmemobj_publish(pop, actv, actv_cnt);

	return 0;

rebuild_err:
	pmemobj_cancel(pop, actv, actv_cnt);

	return -1;
}

/*
 * hm_rp_create --  initializes hashmap state, called after pmemobj_create
 */
int hm_rp_create(PMEMobjpool *pop, TOID(struct hashmap_rp) * map)
{
	hashmap_create(pop, map);

	return 0;
}

/*
 * hm_rp_insert -- rebuilds hashmap if necessary and wraps insert_helper.
 * returns:
 * - 0 if successful,
 * - 1 if value already existed
 * - -1 if something bad happened
 */
int hm_rp_insert(PMEMobjpool *pop, TOID(struct hashmap_rp) hashmap, uint64_t key,
		 uint64_t value)
{
	if (D_RO(hashmap)->count + 1 >= D_RO(hashmap)->resize_threshold) {
		uint64_t capacity_new = D_RO(hashmap)->capacity * 2;
		if (hm_rp_rebuild(pop, hashmap, capacity_new) != 0)
			return -1;
	}

	return insert_helper(pop, D_RW(hashmap), key, value, false);
}

/*
 * hm_rp_remove -- removes specified key from the hashmap,
 * returns:
 * - true if successful,
 * - false if value didn't exist or if something bad happened
 */
bool hm_rp_remove(PMEMobjpool *pop, TOID(struct hashmap_rp) hashmap, uint64_t key)
{
	const uint64_t pos = index_lookup(D_RO(hashmap), key);

	if (pos == 0)
		return false;

	struct entry *entry_p = D_RW(D_RW(hashmap)->entries);
	entry_p += pos;

	size_t actvcnt = 0;

	struct pobj_action actv[5];

	pmemobj_set_value(pop, &actv[actvcnt++], &entry_p->hash,
			  entry_p->hash | TOMBSTONE_MASK);
	pmemobj_set_value(pop, &actv[actvcnt++], &entry_p->value, 0);
	pmemobj_set_value(pop, &actv[actvcnt++], &entry_p->key, 0);
	pmemobj_set_value(pop, &actv[actvcnt++], &D_RW(hashmap)->count,
			  D_RW(hashmap)->count - 1);

	assert(sizeof(actv) / sizeof(actv[0]) >= actvcnt);
	pmemobj_publish(pop, actv, actvcnt);

	uint64_t reduced_threshold = static_cast<uint64_t>(
		(static_cast<uint64_t>(D_RO(hashmap)->capacity / 2)) * load_factor);

	if (reduced_threshold >= INIT_ENTRIES_NUM_RP &&
	    D_RW(hashmap)->count < reduced_threshold &&
	    hm_rp_rebuild(pop, hashmap, D_RO(hashmap)->capacity / 2))
		return false;

	return true;
}

/*
 * hm_rp_get -- checks whether specified key is in the hashmap.
 */
std::pair<uint64_t, bool> hm_rp_get(PMEMobjpool *pop, TOID(struct hashmap_rp) hashmap,
				    uint64_t key)
{
	struct entry *entry_p = reinterpret_cast<struct entry *>(
		pmemobj_direct(D_RW(hashmap)->entries.oid));

	uint64_t pos = index_lookup(D_RO(hashmap), key);
	return pos == 0 ? std::pair<uint64_t, bool>{0, false}
			: std::pair<uint64_t, bool>{(entry_p + pos)->value, true};
}

/*
 * hm_rp_lookup -- checks whether specified key is in the hashmap.
 * Returns 1 if key was found, 0 otherwise.
 */
int hm_rp_lookup(PMEMobjpool *pop, TOID(struct hashmap_rp) hashmap, uint64_t key)
{
	return index_lookup(D_RO(hashmap), key) != 0;
}

/*
 * hm_rp_foreach -- calls cb for all values from the hashmap
 */
int hm_rp_foreach(PMEMobjpool *pop, TOID(struct hashmap_rp) hashmap,
		  int (*cb)(const char *key, size_t key_size, const char *value,
			    size_t value_size, void *arg),
		  void *arg)
{
	struct entry *entry_p = reinterpret_cast<struct entry *>(
		pmemobj_direct(D_RO(hashmap)->entries.oid));

	int ret = 0;
	for (size_t i = 0; i < D_RO(hashmap)->capacity; ++i, ++entry_p) {
		uint64_t hash = entry_p->hash;
		if (entry_is_empty(hash))
			continue;

		ret = cb(reinterpret_cast<const char *>(&(entry_p->key)), 8,
			 reinterpret_cast<const char *>(&(entry_p->value)), 8, arg);

		if (ret)
			return ret;
	}

	return 0;
}

/*
 * hm_rp_count -- returns number of elements
 */
size_t hm_rp_count(PMEMobjpool *pop, TOID(struct hashmap_rp) hashmap)
{
	return D_RO(hashmap)->count;
}

} /* namespace robinhood */
} /* namespace internal */

size_t robinhood::shard_hash(uint64_t key)
{
	return static_cast<size_t>(fast_hash(8, reinterpret_cast<const char *>(&key)) &
				   (shards_number - 1));
}

robinhood::robinhood(std::unique_ptr<internal::config> cfg)
    : pmemobj_engine_base(cfg, "pmemkv_robinhood")
{
	internal::robinhood::name = [&] { return this->name(); };

	/* for now, load factor number can be changed from default by env variable */
	auto lf = std::getenv("ENGINE_ROBINHOOD_LOAD_FACTOR");
	if (lf)
		internal::robinhood::load_factor = std::stof(lf);
	else
		internal::robinhood::load_factor = HASHMAP_RP_LOAD_FACTOR;

	Recover();

	LOG("Started ok");
}

robinhood::~robinhood()
{
	LOG("Stopped ok");
}

std::string robinhood::name()
{
	return "robinhood";
}

status robinhood::count_all(std::size_t &cnt)
{
	LOG("count_all");
	check_outside_tx();

	size_t size = 0;
	for (size_t i = 0; i < shards_number; ++i) {
		shared_lock_type lock(mtxs[i]);
		size += D_RO(container[i])->count;
	}

	cnt = size;

	return status::OK;
}

status robinhood::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	check_outside_tx();

	for (size_t i = 0; i < shards_number; ++i) {
		shared_lock_type lock(mtxs[i]);
		hm_rp_foreach(pmpool.handle(), container[i], callback, arg);
	}

	return status::OK;
}

status robinhood::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	if (key.size() != 8)
		return status::INVALID_ARGUMENT;

	auto k = *reinterpret_cast<const uint64_t *>(key.data());

	auto shard = shard_hash(k);
	shared_lock_type lock(mtxs[shard]);

	return hm_rp_lookup(pmpool.handle(), container[shard], k) == 0 ? status::NOT_FOUND
								       : status::OK;
}

status robinhood::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	if (key.size() != 8)
		return status::INVALID_ARGUMENT;

	auto k = *reinterpret_cast<const uint64_t *>(key.data());

	auto shard = shard_hash(k);
	shared_lock_type lock(mtxs[shard]);

	auto result = hm_rp_get(pmpool.handle(), container[shard], k);

	lock.unlock();

	if (!result.second) {
		LOG("  key not found");
		return status::NOT_FOUND;
	}

	callback(reinterpret_cast<const char *>(&result.first), 8, arg);

	return status::OK;
}

status robinhood::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));
	check_outside_tx();

	if (key.size() != 8 || value.size() != 8)
		return status::INVALID_ARGUMENT;

	auto k = *reinterpret_cast<const uint64_t *>(key.data());
	auto v = *reinterpret_cast<const uint64_t *>(value.data());

	auto shard = shard_hash(k);
	unique_lock_type lock(mtxs[shard]);

	if (hm_rp_insert(pmpool.handle(), container[shard], k, v) != 0) {
		// XXX: Extend the C error handling code to pass the actual reason of the
		// failure.
		return status::UNKNOWN_ERROR;
	}

	return status::OK;
}

status robinhood::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	if (key.size() != 8)
		return status::INVALID_ARGUMENT;

	auto k = *reinterpret_cast<const uint64_t *>(key.data());

	auto shard = shard_hash(k);
	unique_lock_type lock(mtxs[shard]);

	auto result = hm_rp_remove(pmpool.handle(), container[shard], k);

	if (!result)
		return status::NOT_FOUND;

	return status::OK;
}

void robinhood::Recover()
{
	if (!OID_IS_NULL(*root_oid)) {
		auto pmem_ptr = static_cast<internal::robinhood::pmem_type *>(
			pmemobj_direct(*root_oid));

		container = pmem_ptr->map.get();

		this->shards_number = pmem_ptr->shards_number;
	} else {
		/* for now, shards number can be changed from default by env variable */
		auto sn = std::getenv("ENGINE_ROBINHOOD_SHARDS_NUMBER");
		if (sn)
			sscanf(sn, "%zu", &shards_number);
		else
			shards_number = SHARDS;

		pmem::obj::transaction::run(pmpool, [&] {
			pmem::obj::transaction::snapshot(root_oid);

			*root_oid = pmem::obj::make_persistent<
					    internal::robinhood::pmem_type>()
					    .raw();
			auto pmem_ptr = static_cast<internal::robinhood::pmem_type *>(
				pmemobj_direct(*root_oid));
			pmem_ptr->map = obj::make_persistent<TOID(
				struct internal::robinhood::hashmap_rp)[]>(shards_number);
			container = pmem_ptr->map.get();

			pmem_ptr->shards_number = this->shards_number;
		});

		for (size_t i = 0; i < shards_number; ++i)
			internal::robinhood::hm_rp_create(pmpool.handle(), &container[i]);
	}

	mtxs = std::vector<mutex_type>(shards_number);
}

} // namespace kv
} // namespace pmem
