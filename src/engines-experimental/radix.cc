// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#include "radix.h"
#include "../out.h"

namespace pmem
{
namespace kv
{
namespace internal
{
namespace radix
{
transaction::transaction(pmem::obj::pool_base &pop, map_type *container)
    : pop(pop), container(container)
{
}

status transaction::put(string_view key, string_view value)
{
	log.insert(key, value);
	return status::OK;
}

status transaction::remove(string_view key)
{
	log.remove(key);
	return status::OK;
}

status transaction::commit()
{
	auto insert_cb = [&](const dram_log::element_type &e) {
		auto result = container->try_emplace(e.first, e.second);

		if (result.second == false)
			result.first.assign_val(e.second);
	};

	auto remove_cb = [&](const dram_log::element_type &e) {
		container->erase(e.first);
	};

	pmem::obj::transaction::run(pop, [&] { log.foreach (insert_cb, remove_cb); });

	log.clear();

	return status::OK;
}

void transaction::abort()
{
	log.clear();
}
} /* namespace radix */
} /* namespace internal */

radix::radix(std::unique_ptr<internal::config> cfg)
    : pmemobj_engine_base(cfg, "pmemkv_radix"), config(std::move(cfg))
{
	Recover();
	LOG("Started ok");
}

radix::~radix()
{
	LOG("Stopped ok");
}

std::string radix::name()
{
	return "radix";
}

template <typename Iterator, typename Until>
static status iterate(Iterator first, get_kv_callback *callback, void *arg, Until &&until)
{
	for (auto it = first; until(it); ++it) {
		string_view key = it->key();
		string_view value = it->value();

		auto ret =
			callback(key.data(), key.size(), value.data(), value.size(), arg);

		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

status radix::count_all(std::size_t &cnt)
{
	LOG("count_all");
	check_outside_tx();
	cnt = container->size();

	return status::OK;
}

status radix::count_above(string_view key, std::size_t &cnt)
{
	LOG("count_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->upper_bound(key);
	auto last = container->end();

	cnt = internal::distance(first, last);

	return status::OK;
}

status radix::count_equal_above(string_view key, std::size_t &cnt)
{
	LOG("count_equal_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->lower_bound(key);
	auto last = container->end();

	cnt = internal::distance(first, last);

	return status::OK;
}

status radix::count_equal_below(string_view key, std::size_t &cnt)
{
	LOG("count_equal_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->begin();
	auto last = container->upper_bound(key);

	cnt = internal::distance(first, last);

	return status::OK;
}

status radix::count_below(string_view key, std::size_t &cnt)
{
	LOG("count_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->begin();
	auto last = container->lower_bound(key);

	cnt = internal::distance(first, last);

	return status::OK;
}

status radix::count_between(string_view key1, string_view key2, std::size_t &cnt)
{
	LOG("count_between for key1=" << key1.data() << ", key2=" << key2.data());
	check_outside_tx();

	if (key1.compare(key2) < 0) {
		auto first = container->upper_bound(key1);
		auto last = container->lower_bound(key2);

		cnt = internal::distance(first, last);
	} else {
		cnt = 0;
	}

	return status::OK;
}

status radix::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	check_outside_tx();

	auto first = container->begin();
	auto last = container->end();

	return iterate(first, callback, arg,
		       [&](const container_type::iterator &it) { return it != last; });
}

status radix::get_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->upper_bound(key);
	auto last = container->end();

	return iterate(first, callback, arg,
		       [&](const container_type::iterator &it) { return it != last; });
}

status radix::get_equal_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->lower_bound(key);
	auto last = container->end();

	return iterate(first, callback, arg,
		       [&](const container_type::iterator &it) { return it != last; });
}

status radix::get_equal_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->begin();
	auto last = container->upper_bound(key);

	return iterate(first, callback, arg,
		       [&](const container_type::iterator &it) { return it != last; });
}

status radix::get_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto first = container->begin();
	auto last = container->lower_bound(key);

	return iterate(first, callback, arg,
		       [&](const container_type::iterator &it) { return it != last; });
}

status radix::get_between(string_view key1, string_view key2, get_kv_callback *callback,
			  void *arg)
{
	LOG("get_between for key1=" << key1.data() << ", key2=" << key2.data());
	check_outside_tx();

	if (key1.compare(key2) < 0) {
		auto first = container->upper_bound(key1);
		auto last = container->lower_bound(key2);
		return iterate(
			first, callback, arg,
			[&](const container_type::iterator &it) { return it != last; });
	}

	return status::OK;
}

status radix::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	return container->find(key) != container->end() ? status::OK : status::NOT_FOUND;
}

status radix::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto it = container->find(key);
	if (it != container->end()) {
		auto value = string_view(it->value());
		callback(value.data(), value.size(), arg);
		return status::OK;
	}

	LOG("  key not found");
	return status::NOT_FOUND;
}

status radix::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));
	check_outside_tx();

	auto result = container->try_emplace(key, value);

	if (result.second == false) {
		pmem::obj::transaction::run(pmpool,
					    [&] { result.first.assign_val(value); });
	}

	return status::OK;
}

status radix::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	auto it = container->find(key);

	if (it == container->end())
		return status::NOT_FOUND;

	container->erase(it);

	return status::OK;
}

internal::transaction *radix::begin_tx()
{
	return new internal::radix::transaction(pmpool, container);
}

void radix::Recover()
{
	if (!OID_IS_NULL(*root_oid)) {
		auto pmem_ptr = static_cast<internal::radix::pmem_type *>(
			pmemobj_direct(*root_oid));

		container = &pmem_ptr->map;
	} else {
		pmem::obj::transaction::run(pmpool, [&] {
			pmem::obj::transaction::snapshot(root_oid);
			*root_oid =
				pmem::obj::make_persistent<internal::radix::pmem_type>()
					.raw();
			auto pmem_ptr = static_cast<internal::radix::pmem_type *>(
				pmemobj_direct(*root_oid));
			container = &pmem_ptr->map;
		});
	}
}

// HETEROGENOUS_RADIX

heterogenous_radix::merged_iterator::merged_iterator(
	internal::radix::ordered_cache<uvalue_type> &cache,
	internal::radix::map_type &pmem,
	typename internal::radix::ordered_cache<uvalue_type>::iterator dram_it,
	typename internal::radix::map_type::iterator pmem_it)
    : cache(cache), pmem(pmem), dram_it(dram_it), pmem_it(pmem_it)
{
	set_current_it();
}

heterogenous_radix::merged_iterator &heterogenous_radix::merged_iterator::operator++()
{
	assert(dereferenceable());

	if (curr_it == current_it::dram) {
		assert(dram_it != cache.end());
		assert(pmem_it == pmem.end() ||
		       dram_it->first.compare(pmem_it->key()) < 0);
	} else {
		assert(pmem_it != pmem.end());
		assert(dram_it == cache.end() ||
		       dram_it->first.compare(pmem_it->key()) > 0);
	}

	if (curr_it == current_it::dram)
		++dram_it;
	else
		++pmem_it;

	set_current_it();

	return *this;
}

heterogenous_radix::merged_iterator *heterogenous_radix::merged_iterator::operator->()
{
	return this;
}

string_view heterogenous_radix::merged_iterator::key() const
{
	assert(dereferenceable());

	if (curr_it == current_it::dram)
		return dram_it->first;
	else
		return pmem_it->key();
}

string_view heterogenous_radix::merged_iterator::value() const
{
	assert(dereferenceable());

	if (curr_it == current_it::dram)
		return *dram_it->second->second
				.load(); // XXX -optimistic conc control!!! + ebr?
	else
		return pmem_it->value();
}

bool heterogenous_radix::merged_iterator::dereferenceable() const
{
	return pmem_it != pmem.end() || dram_it != cache.end();
}

void heterogenous_radix::merged_iterator::set_current_it()
{
	while (dereferenceable()) {
		if (pmem_it != pmem.end() && dram_it != cache.end() &&
		    dram_it->first == string_view(pmem_it->key())) {
			/* If keys are the same, skip the one in pmem (the dram one is
			 * more recent) */
			++pmem_it;
		} else if (dram_it == cache.end() ||
			   (pmem_it != pmem.end() &&
			    dram_it->first.compare(pmem_it->key()) > 0)) {
			/* If there are no more dram elements or pmem element is smaller
			 */
			curr_it = current_it::pmem;
			return;
		} else {
			auto v = dram_it->second->second.load(std::memory_order_acquire);

			/* Skip removed entries */
			if (v == heterogenous_radix::tombstone_volatile() ||
			    v == heterogenous_radix::tombstone_persistent()) {
				++dram_it;
			} else {
				curr_it = current_it::dram;
				return;
			}
		}
	}
}

heterogenous_radix::uvalue_type *heterogenous_radix::tombstone_volatile()
{
	return reinterpret_cast<heterogenous_radix::uvalue_type *>(1ULL);
}

heterogenous_radix::uvalue_type *heterogenous_radix::tombstone_persistent()
{
	return reinterpret_cast<heterogenous_radix::uvalue_type *>(2ULL);
}

heterogenous_radix::queue_entry::queue_entry(string_view key_, string_view value_)
    : remove(false)
{
	auto key_size = pmem::obj::experimental::total_sizeof<uvalue_type>::value(key_);

	auto key_dst = reinterpret_cast<uvalue_type *>(this + 1);
	auto val_dst = reinterpret_cast<uvalue_type *>(reinterpret_cast<char *>(key_dst) +
						       key_size);

	new (key_dst) uvalue_type(key_);
	new (val_dst) uvalue_type(value_);
}

heterogenous_radix::queue_entry::queue_entry(string_view key_) : queue_entry(key_, "")
{
	remove = true;
}

const heterogenous_radix::uvalue_type &heterogenous_radix::queue_entry::key() const
{
	auto key = reinterpret_cast<const uvalue_type *>(this + 1);
	return *key;
}

heterogenous_radix::uvalue_type &heterogenous_radix::queue_entry::value()
{
	auto key_dst = reinterpret_cast<char *>(this + 1);
	auto val_dst = reinterpret_cast<uvalue_type *>(
		key_dst +
		pmem::obj::experimental::total_sizeof<uvalue_type>::value(key()));

	return *reinterpret_cast<uvalue_type *>(val_dst);
}

heterogenous_radix::heterogenous_radix(std::unique_ptr<internal::config> cfg)
    : pmemobj_engine_base(cfg, "pmemkv_radix"), config(std::move(cfg))
{
	// size_t log_size;
	// if (!cfg->get_uint64("log_size", &log_size))
	// 	throw internal::invalid_argument("XXX");
	// if (!cfg->get_uint64("dram_size", &dram_size))
	// 	throw internal::invalid_argument("XXX");

	size_t log_size = std::stoull(std::getenv("PMEMKV_LOG_SIZE"));
	dram_size = std::stoull(std::getenv("PMEMKV_DRAM_SIZE"));

	internal::radix::pmem_type *pmem_ptr;

	if (!OID_IS_NULL(*root_oid)) {
		pmem_ptr = static_cast<internal::radix::pmem_type *>(
			pmemobj_direct(*root_oid));
	} else {
		pmem::obj::transaction::run(pmpool, [&] {
			pmem::obj::transaction::snapshot(root_oid);
			*root_oid =
				pmem::obj::make_persistent<internal::radix::pmem_type>()
					.raw();
			pmem_ptr = static_cast<internal::radix::pmem_type *>(
				pmemobj_direct(*root_oid));
			pmem_ptr->log = pmem::obj::make_persistent<char[]>(log_size);
		});
	}

	// XXX - do recovery first
	// pmem_ptr->log->resize(log_size);

	container = &pmem_ptr->map;
	container->runtime_initialize_mt();

	cache = std::unique_ptr<cache_type>(new cache_type(dram_size));

	stopped.store(false);
	bg_thread = std::thread([&] { bg_work(); });

	pop = pmem::obj::pool_by_vptr(&pmem_ptr->log);
}

heterogenous_radix::~heterogenous_radix()
{
	stopped.store(true);
	bg_thread.join();
}

heterogenous_radix::cache_type::value_type *
heterogenous_radix::cache_put(string_view key, uvalue_type *value)
{
	auto evict_cb = [&](auto &list) {
		for (auto rit = list.rbegin(); rit != list.rend(); rit++) {
			auto t = rit->second.load(std::memory_order_relaxed);

			// XXX - check if within mpsc_queue
			if (pmemobj_pool_by_ptr(t) != nullptr ||
			    t == tombstone_persistent())
				return std::next(rit).base();
		}

		return list.end();
	};

	return cache->put(key, value, evict_cb);
}

status heterogenous_radix::put(string_view key, string_view value)
{
	auto req_size = pmem::obj::experimental::total_sizeof<uvalue_type>::value(key) +
		pmem::obj::experimental::total_sizeof<uvalue_type>::value(value) +
		sizeof(queue_entry);

	auto qe = new char[req_size];
	new (qe) queue_entry(key, value);

	auto entry = reinterpret_cast<queue_entry *>(qe);

	while (true) {
		std::exception_ptr *exc;
		if ((exc = bg_exception_ptr.load(std::memory_order_acquire)) !=
		    nullptr) { // XXX: && !queue.try_produce()
			std::rethrow_exception(*exc);
		}

		auto ret = cache_put(key, &entry->value());
		if (ret) {
			entry->dram_entry = ret;
			break;
		}
	}

	// XXX - if try_produce == false, we can just allocate new radix node to
	// TLS and the publish pointer to this node
	// NEED TX support for produce():
	// tx {queue.produce([&] (data) { data = make_persistent(); }) }

	queue.emplace(entry);

	return status::OK;
}

status heterogenous_radix::remove(string_view k)
{
	bool found = false;
	auto ret = cache->get(k, false);
	if (ret) {
		found = (ret != tombstone_persistent() && ret != tombstone_volatile());
	} else {
		found = container->find(k) != container->end();
	}

	// XXX - remove duplication
	auto req_size = pmem::obj::experimental::total_sizeof<uvalue_type>::value(k) +
		pmem::obj::experimental::total_sizeof<uvalue_type>::value("") +
		sizeof(queue_entry);

	auto qe = new char[req_size];
	new (qe) queue_entry(k);

	auto entry = reinterpret_cast<queue_entry *>(qe);

	while (true) {
		std::exception_ptr *exc;
		if ((exc = bg_exception_ptr.load(std::memory_order_acquire)) !=
		    nullptr) { // XXX: && !queue.try_produce()
			try {
				std::rethrow_exception(*exc);
			} catch (pmem::transaction_out_of_memory &e) {
				cache_put(k, tombstone_persistent());

				/* Try to free the element directly, bypassing the queue.
				 */
				std::unique_lock<std::mutex> lock(erase_mtx);
				auto cnt = container->erase(
					k); // XXX - force freeing (instead of appending
					    // to garbage)

				delete exc;

				/* Notify bg thread that the exception was consumed */
				bg_exception_ptr.store(nullptr,
						       std::memory_order_release);
				bg_exception_cv.notify_one();

				return cnt ? status::OK : status::NOT_FOUND;
			} catch (...) {
				throw;
			}
		} else {
			auto v = cache_put(k, tombstone_volatile());
			if (v) {
				entry->dram_entry = v;
				break;
			}
		}
	}

	queue.emplace(entry);

	return found ? status::OK : status::NOT_FOUND;
}

status heterogenous_radix::get(string_view key, get_v_callback *callback, void *arg)
{
	auto v = cache->get(key, true);
	if (v) {
		if (v == tombstone_volatile() || v == tombstone_persistent())
			return status::NOT_FOUND;

		callback(v->data(), v->size(), arg);

		return status::OK;
	} else {
		auto it = container->find(key);
		if (it != container->end()) {
			auto value = string_view(it->value());
			callback(value.data(), value.size(), arg);

			cache_put(key, &it->value());

			return status::OK;
		} else
			return status::NOT_FOUND;
	}
}

std::string heterogenous_radix::name()
{
	return "radix";
}

heterogenous_radix::merged_iterator heterogenous_radix::merged_begin()
{
	return merged_iterator(*cache, *container, cache->begin(), container->begin());
}

heterogenous_radix::merged_iterator heterogenous_radix::merged_end()
{
	return merged_iterator(*cache, *container, cache->end(), container->end());
}

heterogenous_radix::merged_iterator
heterogenous_radix::merged_lower_bound(string_view key)
{
	auto dram_lo = cache->lower_bound(key);
	auto pmem_lo = container->lower_bound(key);

	assert(dram_lo == cache->end() || dram_lo->first.compare(key) >= 0);
	assert(pmem_lo == container->end() || pmem_lo->key().compare(key) >= 0);

	return merged_iterator(*cache, *container, dram_lo, pmem_lo);
}

heterogenous_radix::merged_iterator
heterogenous_radix::merged_upper_bound(string_view key)
{
	auto dram_up = cache->upper_bound(key);
	auto pmem_up = container->upper_bound(key);

	assert(dram_up == cache->end() || dram_up->first.compare(key) > 0);
	assert(pmem_up == container->end() || pmem_up->key().compare(key) > 0);

	return merged_iterator(*cache, *container, dram_up, pmem_up);
}

status heterogenous_radix::get_all(get_kv_callback *callback, void *arg)
{
	check_outside_tx();

	auto first = merged_begin();

	return iterate(first, callback, arg,
		       [&](const merged_iterator &it) { return it.dereferenceable(); });
}

status heterogenous_radix::get_above(string_view key, get_kv_callback *callback,
				     void *arg)
{
	check_outside_tx();

	auto first = merged_upper_bound(key);

	return iterate(first, callback, arg,
		       [&](const merged_iterator &it) { return it.dereferenceable(); });
}

status heterogenous_radix::get_equal_above(string_view key, get_kv_callback *callback,
					   void *arg)
{
	check_outside_tx();

	auto first = merged_lower_bound(key);

	return iterate(first, callback, arg,
		       [&](const merged_iterator &it) { return it.dereferenceable(); });
}

status heterogenous_radix::get_equal_below(string_view key, get_kv_callback *callback,
					   void *arg)
{
	check_outside_tx();

	auto first = merged_begin();

	/* We cannot rely on iterator comparisons because of concurrent inserts/erases.
	 * There are two problems:
	 * 1. Iterator can be erased. In that case `while(it != last) it++;` would
	 *    spin forever.
	 * 2. `auto last = merged_upper_bound(key);
	 *     while (it != last) it++;`
	 *
	 *     if merged_upper_bound(key) returns end(), this means there are no
	 *     elements bigger than key - we can then return all elements
	 *     from the container, right?
	 *
	 *     Not really - while we iterate someone might insert element bigger than key
	 *     we will process such element (since we are processing all elements).
	 */
	return iterate(first, callback, arg, [&](const merged_iterator &it) {
		return it.dereferenceable() && it.key().compare(key) <= 0;
	});
}

status heterogenous_radix::get_below(string_view key, get_kv_callback *callback,
				     void *arg)
{
	check_outside_tx();

	auto first = merged_begin();

	return iterate(first, callback, arg, [&](const merged_iterator &it) {
		return it.dereferenceable() && it.key().compare(key) < 0;
	});
}

status heterogenous_radix::get_between(string_view key1, string_view key2,
				       get_kv_callback *callback, void *arg)
{
	check_outside_tx();

	if (key1.compare(key2) < 0) {
		auto first = merged_upper_bound(key1);

		return iterate(first, callback, arg, [&](const merged_iterator &it) {
			return it.dereferenceable() && it.key().compare(key2) < 0;
		});
	}

	return status::OK;
}

static int count_elements(const char *, size_t, const char *, size_t, void *arg)
{
	auto *cnt = static_cast<size_t *>(arg);
	++(*cnt);

	return 0;
}

status heterogenous_radix::count_all(std::size_t &cnt)
{
	check_outside_tx();

	cnt = 0;
	return get_all(count_elements, (void *)&cnt);
}

status heterogenous_radix::count_above(string_view key, std::size_t &cnt)
{
	check_outside_tx();

	cnt = 0;
	return get_above(key, count_elements, (void *)&cnt);
}

status heterogenous_radix::count_equal_above(string_view key, std::size_t &cnt)
{
	check_outside_tx();

	cnt = 0;
	return get_equal_above(key, count_elements, (void *)&cnt);
}

status heterogenous_radix::count_equal_below(string_view key, std::size_t &cnt)
{
	check_outside_tx();

	cnt = 0;
	return get_equal_below(key, count_elements, (void *)&cnt);
}

status heterogenous_radix::count_below(string_view key, std::size_t &cnt)
{
	check_outside_tx();

	cnt = 0;
	return get_below(key, count_elements, (void *)&cnt);
}

status heterogenous_radix::count_between(string_view key1, string_view key2,
					 std::size_t &cnt)
{
	check_outside_tx();

	cnt = 0;

	if (key1.compare(key2) < 0) {
		auto first = merged_upper_bound(key1);
		auto last = merged_lower_bound(key2);

		return get_between(key1, key2, count_elements, (void *)&cnt);
	}

	return status::OK;
}

status heterogenous_radix::exists(string_view key)
{
	return get(
		key, [](const char *, size_t, void *) {}, nullptr);
}

void heterogenous_radix::bg_work()
{
	while (!stopped.load()) {
		queue_entry *e;
		while (queue.try_pop(e)) {
			/* Loop until processing `e` is successfull */
			while (true) {
				if (stopped.load())
					return;

				try {
					// XXX - make sure tx does not abort
					if (e->remove) {
						container->erase(e->key());

						auto expected = tombstone_volatile();
						e->dram_entry->compare_exchange_strong(
							expected, tombstone_persistent());
					} else {
						auto ret = container->insert_or_assign(
							e->key(), e->value());

						auto expected = &e->value();
						e->dram_entry->compare_exchange_strong(
							expected, &ret.first->value());
					}

					break;
					//	delete e; // XXX - not needed on pmem!!!
				} catch (...) {
					// XXX - if there is any garbage try freeing it??

					bg_exception_ptr.store(new std::exception_ptr(
						std::current_exception()));

					std::unique_lock<std::mutex> lock(
						bg_exception_lock);

					/* Wait until exception is handled. */
					bg_exception_cv.wait(lock, [&] {
						return bg_exception_ptr.load() == nullptr;
					});
				}
			}
		}
	}
}

internal::iterator_base *radix::new_iterator()
{
	return new radix_iterator<false>{container};
}

internal::iterator_base *radix::new_const_iterator()
{
	return new radix_iterator<true>{container};
}

radix::radix_iterator<true>::radix_iterator(container_type *c)
    : container(c), pop(pmem::obj::pool_by_vptr(c))
{
}

radix::radix_iterator<false>::radix_iterator(container_type *c)
    : radix::radix_iterator<true>(c)
{
}

status radix::radix_iterator<true>::seek(string_view key)
{
	init_seek();

	it_ = container->find(key);
	if (it_ != container->end())
		return status::OK;

	return status::NOT_FOUND;
}

status radix::radix_iterator<true>::seek_lower(string_view key)
{
	init_seek();

	it_ = container->lower_bound(key);
	if (it_ == container->begin()) {
		it_ = container->end();
		return status::NOT_FOUND;
	}

	--it_;

	return status::OK;
}

status radix::radix_iterator<true>::seek_lower_eq(string_view key)
{
	init_seek();

	it_ = container->upper_bound(key);
	if (it_ == container->begin()) {
		it_ = container->end();
		return status::NOT_FOUND;
	}

	--it_;

	return status::OK;
}

status radix::radix_iterator<true>::seek_higher(string_view key)
{
	init_seek();

	it_ = container->upper_bound(key);
	if (it_ == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status radix::radix_iterator<true>::seek_higher_eq(string_view key)
{
	init_seek();

	it_ = container->lower_bound(key);
	if (it_ == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status radix::radix_iterator<true>::seek_to_first()
{
	init_seek();

	if (container->empty())
		return status::NOT_FOUND;

	it_ = container->begin();

	return status::OK;
}

status radix::radix_iterator<true>::seek_to_last()
{
	init_seek();

	if (container->empty())
		return status::NOT_FOUND;

	it_ = container->end();
	--it_;

	return status::OK;
}

status radix::radix_iterator<true>::is_next()
{
	auto tmp = it_;
	if (tmp == container->end() || ++tmp == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status radix::radix_iterator<true>::next()
{
	init_seek();

	if (it_ == container->end() || ++it_ == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status radix::radix_iterator<true>::prev()
{
	init_seek();

	if (it_ == container->begin())
		return status::NOT_FOUND;

	--it_;

	return status::OK;
}

result<string_view> radix::radix_iterator<true>::key()
{
	assert(it_ != container->end());

	return {it_->key().cdata()};
}

result<pmem::obj::slice<const char *>> radix::radix_iterator<true>::read_range(size_t pos,
									       size_t n)
{
	assert(it_ != container->end());

	if (pos + n > it_->value().size() || pos + n < pos)
		n = it_->value().size() - pos;

	return {{it_->value().cdata() + pos, it_->value().cdata() + pos + n}};
}

result<pmem::obj::slice<char *>> radix::radix_iterator<false>::write_range(size_t pos,
									   size_t n)
{
	assert(it_ != container->end());

	if (pos + n > it_->value().size() || pos + n < pos)
		n = it_->value().size() - pos;

	log.push_back({std::string(it_->value().cdata() + pos, n), pos});
	auto &val = log.back().first;

	return {{&val[0], &val[n]}};
}

status radix::radix_iterator<false>::commit()
{
	pmem::obj::transaction::run(pop, [&] {
		for (auto &p : log) {
			auto dest = it_->value().range(p.second, p.first.size());
			std::copy(p.first.begin(), p.first.end(), dest.begin());
		}
	});
	log.clear();

	return status::OK;
}

void radix::radix_iterator<false>::abort()
{
	log.clear();
}

static factory_registerer
	register_radix(std::unique_ptr<engine_base::factory_base>(new radix_factory));

} // namespace kv
} // namespace pmem
