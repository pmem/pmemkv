// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#ifndef LIBPMEMKV_RADIX_H
#define LIBPMEMKV_RADIX_H

#include "../comparator/pmemobj_comparator.h"
#include "../iterator.h"
#include "../pmemobj_engine.h"

#include <libpmemobj++/experimental/inline_string.hpp>
#include <libpmemobj++/experimental/radix_tree.hpp>
#include <libpmemobj++/persistent_ptr.hpp>

#include <tbb/concurrent_queue.h>

#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <shared_mutex>

namespace pmem
{
namespace kv
{
namespace internal
{
namespace radix
{

using map_type =
	pmem::obj::experimental::radix_tree<pmem::obj::experimental::inline_string,
					    pmem::obj::experimental::inline_string>;

struct pmem_type {
	pmem_type() : map()
	{
		std::memset(reserved, 0, sizeof(reserved));
	}

	map_type map;
	pmem::obj::persistent_ptr<char[]> log;
	uint64_t reserved[6];
};

static_assert(sizeof(pmem_type) == sizeof(map_type) + 64, "");

class transaction : public ::pmem::kv::internal::transaction {
public:
	transaction(pmem::obj::pool_base &pop, map_type *container);
	status put(string_view key, string_view value) final;
	status remove(string_view key) final;
	status commit() final;
	void abort() final;

private:
	pmem::obj::pool_base &pop;
	dram_log log;
	map_type *container;
};

template <typename Value>
class ordered_cache {
private:
	using key_type = std::string;
	using dram_value_type = std::pair<key_type, std::atomic<Value *>>;
	using lru_list_type = std::list<dram_value_type>;
	using dram_map_type = std::map<string_view, typename lru_list_type::iterator>;

public:
	using iterator = typename dram_map_type::iterator;

	using value_type = std::atomic<Value *>;

	ordered_cache(size_t max_size) : max_size(max_size)
	{
	}

	ordered_cache(const ordered_cache &) = delete;
	ordered_cache(ordered_cache &&) = delete;

	ordered_cache &operator=(const ordered_cache &) = delete;
	ordered_cache &operator=(ordered_cache &&) = delete;

	template <typename F>
	value_type *put(string_view key, Value *v, F &&evict)
	{
		auto it = map.find(key);
		if (it == map.end()) {
			if (lru_list.size() < max_size) {
				lru_list.emplace_front(key, v);
				auto lit = lru_list.begin();

				auto ret = map.try_emplace(lit->first, lit);
				assert(ret.second);

				it = ret.first;
			} else {
				auto lit = evict(lru_list);

				if (lit == lru_list.end())
					return nullptr;

				auto cnt = map.erase(lit->first);

				assert(cnt == 1 && lit->first != key);

				lru_list.splice(lru_list.begin(), lru_list, lit);
				lru_list.begin()->first = key;
				lru_list.begin()->second.store(v,
							       std::memory_order_release);
				lit = lru_list.begin();

				auto ret = map.try_emplace(lit->first, lit);
				assert(ret.second);

				it = ret.first;
			}
		} else {
			lru_list.splice(lru_list.begin(), lru_list, it->second);
			lru_list.begin()->second.store(v, std::memory_order_release);
		}

		return &it->second->second;
	}

	Value *get(string_view key, bool promote)
	{
		auto it = map.find(key);
		if (it == map.end()) {
			return nullptr;
		} else {
			if (promote)
				lru_list.splice(lru_list.begin(), lru_list, it->second);

			return it->second->second.load(std::memory_order_acquire);
		}
	}

	iterator begin()
	{
		return map.begin();
	}

	iterator end()
	{
		return map.end();
	}

	iterator lower_bound(string_view key)
	{
		return map.lower_bound(key);
	}

	iterator upper_bound(string_view key)
	{
		return map.upper_bound(key);
	}

private:
	lru_list_type lru_list;
	dram_map_type map;
	const size_t max_size;
};

} /* namespace radix */
} /* namespace internal */

/**
 * Radix tree engine backed by:
 * https://github.com/pmem/libpmemobj-cpp/blob/master/include/libpmemobj%2B%2B/experimental/radix_tree.hpp
 *
 * It is a sorted, singlethreaded engine. Unlike other sorted engines it does not support
 * custom comparator (the order is defined by the keys' representation).
 *
 * The implementation is a variation of a PATRICIA trie - the internal
 * nodes do not store the path explicitly, but only a position at which
 * the keys differ. Keys are stored entirely in leafs.
 *
 * More info about radix tree: https://en.wikipedia.org/wiki/Radix_tree
 */
class radix : public pmemobj_engine_base<internal::radix::pmem_type> {
	template <bool IsConst>
	class radix_iterator;

public:
	radix(std::unique_ptr<internal::config> cfg);
	~radix();

	radix(const radix &) = delete;
	radix &operator=(const radix &) = delete;

	std::string name() final;

	status count_all(std::size_t &cnt) final;
	status count_above(string_view key, std::size_t &cnt) final;
	status count_equal_above(string_view key, std::size_t &cnt) final;
	status count_equal_below(string_view key, std::size_t &cnt) final;
	status count_below(string_view key, std::size_t &cnt) final;
	status count_between(string_view key1, string_view key2, std::size_t &cnt) final;

	status get_all(get_kv_callback *callback, void *arg) final;
	status get_above(string_view key, get_kv_callback *callback, void *arg) final;
	status get_equal_above(string_view key, get_kv_callback *callback,
			       void *arg) final;
	status get_equal_below(string_view key, get_kv_callback *callback,
			       void *arg) final;
	status get_below(string_view key, get_kv_callback *callback, void *arg) final;
	status get_between(string_view key1, string_view key2, get_kv_callback *callback,
			   void *arg) final;

	status exists(string_view key) final;

	status get(string_view key, get_v_callback *callback, void *arg) final;

	status put(string_view key, string_view value) final;

	status remove(string_view key) final;

	internal::transaction *begin_tx() final;

	internal::iterator_base *new_iterator() final;
	internal::iterator_base *new_const_iterator() final;

private:
	using container_type = internal::radix::map_type;

	void Recover();

	container_type *container;
	std::unique_ptr<internal::config> config;
};

class heterogenous_radix : public pmemobj_engine_base<internal::radix::pmem_type> {
public:
	heterogenous_radix(std::unique_ptr<internal::config> cfg);
	~heterogenous_radix();

	heterogenous_radix(const heterogenous_radix &) = delete;
	heterogenous_radix &operator=(const heterogenous_radix &) = delete;

	std::string name() final;

	status count_all(std::size_t &cnt) final;
	status count_above(string_view key, std::size_t &cnt) final;
	status count_equal_above(string_view key, std::size_t &cnt) final;
	status count_equal_below(string_view key, std::size_t &cnt) final;
	status count_below(string_view key, std::size_t &cnt) final;
	status count_between(string_view key1, string_view key2, std::size_t &cnt) final;

	status get_all(get_kv_callback *callback, void *arg) final;
	status get_above(string_view key, get_kv_callback *callback, void *arg) final;
	status get_equal_above(string_view key, get_kv_callback *callback,
			       void *arg) final;
	status get_equal_below(string_view key, get_kv_callback *callback,
			       void *arg) final;
	status get_below(string_view key, get_kv_callback *callback, void *arg) final;
	status get_between(string_view key1, string_view key2, get_kv_callback *callback,
			   void *arg) final;

	status exists(string_view key) final;

	status put(string_view key, string_view value) final;

	status remove(string_view k) final;

	status get(string_view key, get_v_callback *callback, void *arg) final;

private:
	using uvalue_type = pmem::obj::experimental::inline_string;
	using container_type = internal::radix::map_type;
	using cache_type = internal::radix::ordered_cache<uvalue_type>;

	using dram_iterator = typename cache_type::iterator;
	using pmem_iterator = typename container_type::iterator;

	struct queue_entry {
		queue_entry(string_view key_, string_view value_);
		queue_entry(string_view key_);

		const uvalue_type &key() const;
		uvalue_type &value();

		cache_type::value_type *dram_entry;
		bool remove;
	};

	using pmem_queue_type = tbb::concurrent_bounded_queue<queue_entry *>;

	struct merged_iterator {
		merged_iterator(cache_type &cache, container_type &pmem,
				dram_iterator dram_it, pmem_iterator pmem_it);
		merged_iterator(const merged_iterator &) = default;

		merged_iterator &operator++();

		bool dereferenceable() const;

		merged_iterator *operator->();

		string_view key() const;
		string_view value() const;

	private:
		cache_type &cache;
		container_type &pmem;

		dram_iterator dram_it;
		pmem_iterator pmem_it;

		enum class current_it { dram, pmem } curr_it;

		void set_current_it();
	};

	merged_iterator merged_begin();
	merged_iterator merged_end();
	merged_iterator merged_lower_bound(string_view key);
	merged_iterator merged_upper_bound(string_view key);

	void bg_work();
	cache_type::value_type *cache_put(string_view key, uvalue_type *value,
					  bool persisted);

	/* Element was logically removed but there might an older version on pmem. */
	static uvalue_type *tombstone_volatile();

	/* Element removed logically removed from cache and from pmem. */
	static uvalue_type *tombstone_persistent();

	std::unique_ptr<cache_type> cache;
	size_t dram_size;

	std::atomic<bool> stopped;
	std::thread bg_thread;

	pmem::obj::pool_base pop;

	container_type *container;
	std::unique_ptr<internal::config> config;

	std::mutex eviction_lock;
	std::condition_variable eviction_cv;

	pmem_queue_type queue;
};

template <>
class radix::radix_iterator<true> : public internal::iterator_base {
	using container_type = radix::container_type;

public:
	radix_iterator(container_type *container);

	status seek(string_view key) final;
	status seek_lower(string_view key) final;
	status seek_lower_eq(string_view key) final;
	status seek_higher(string_view key) final;
	status seek_higher_eq(string_view key) final;

	status seek_to_first() final;
	status seek_to_last() final;

	status is_next() final;
	status next() final;
	status prev() final;

	result<string_view> key() final;

	result<pmem::obj::slice<const char *>> read_range(size_t pos, size_t n) final;

protected:
	container_type *container;
	container_type::iterator it_;
	pmem::obj::pool_base pop;
};

template <>
class radix::radix_iterator<false> : public radix::radix_iterator<true> {
	using container_type = radix::container_type;

public:
	radix_iterator(container_type *container);

	result<pmem::obj::slice<char *>> write_range(size_t pos, size_t n) final;

	status commit() final;
	void abort() final;

private:
	std::vector<std::pair<std::string, size_t>> log;
};

class radix_factory : public engine_base::factory_base {
public:
	std::unique_ptr<engine_base>
	create(std::unique_ptr<internal::config> cfg) override
	{
		check_config_null(get_name(), cfg);
		return std::unique_ptr<engine_base>(
			new heterogenous_radix(std::move(cfg)));
		// uint64_t dram_caching;
		// if (cfg->get_uint64("dram_caching", &dram_caching) && dram_caching) {
		// 	return std::unique_ptr<engine_base>(new
		// heterogenous_radix(std::move(cfg))); } else { 	return
		// std::unique_ptr<engine_base>(new radix(std::move(cfg)));
		// }
	};

	std::string get_name() override
	{
		return "radix";
	};
};

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_RADIX_H */
