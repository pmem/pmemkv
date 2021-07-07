// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#include "csmap.h"

#include "../iterator.h"
#include "../out.h"

namespace pmem
{
namespace kv
{

csmap::csmap(std::unique_ptr<internal::config> cfg)
    : pmemobj_engine_base(cfg, "pmemkv_csmap"), config(std::move(cfg))
{
	Recover();
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
	cnt = container->size();

	return status::OK;
}

status csmap::count_above(string_view key, std::size_t &cnt)
{
	LOG("count_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->upper_bound(key);
	auto last = container->end();

	cnt = internal::distance(first, last);

	return status::OK;
}

status csmap::count_equal_above(string_view key, std::size_t &cnt)
{
	LOG("count_equal_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->lower_bound(key);
	auto last = container->end();

	cnt = internal::distance(first, last);

	return status::OK;
}

status csmap::count_equal_below(string_view key, std::size_t &cnt)
{
	LOG("count_equal_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->begin();
	auto last = container->upper_bound(key);

	cnt = internal::distance(first, last);

	return status::OK;
}

status csmap::count_below(string_view key, std::size_t &cnt)
{
	LOG("count_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->begin();
	auto last = container->lower_bound(key);

	cnt = internal::distance(first, last);

	return status::OK;
}

status csmap::count_between(string_view key1, string_view key2, std::size_t &cnt)
{
	LOG("count_between for key1=" << key1.data() << ", key2=" << key2.data());
	check_outside_tx();

	if (container->key_comp()(key1, key2)) {
		shared_global_lock_type lock(mtx);

		auto first = container->upper_bound(key1);
		auto last = container->lower_bound(key2);

		cnt = internal::distance(first, last);
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

	auto first = container->begin();
	auto last = container->end();

	return iterate(first, last, callback, arg);
}

status csmap::get_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->upper_bound(key);
	auto last = container->end();

	return iterate(first, last, callback, arg);
}

status csmap::get_equal_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_above for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->lower_bound(key);
	auto last = container->end();

	return iterate(first, last, callback, arg);
}

status csmap::get_equal_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->begin();
	auto last = container->upper_bound(key);

	return iterate(first, last, callback, arg);
}

status csmap::get_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_below for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);

	auto first = container->begin();
	auto last = container->lower_bound(key);

	return iterate(first, last, callback, arg);
}

status csmap::get_between(string_view key1, string_view key2, get_kv_callback *callback,
			  void *arg)
{
	LOG("get_between for key1=" << key1.data() << ", key2=" << key2.data());
	check_outside_tx();

	if (container->key_comp()(key1, key2)) {
		shared_global_lock_type lock(mtx);

		auto first = container->upper_bound(key1);
		auto last = container->lower_bound(key2);
		return iterate(first, last, callback, arg);
	}

	return status::OK;
}

status csmap::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);
	return container->contains(key) ? status::OK : status::NOT_FOUND;
}

status csmap::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	shared_global_lock_type lock(mtx);
	auto it = container->find(key);
	if (it != container->end()) {
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

	auto result = container->try_emplace(key, value);

	if (result.second == false) {
		auto &it = result.first;
		unique_node_lock_type lock(it->second.mtx);
		pmem::obj::transaction::run(pmpool, [&] {
			it->second.val.assign(value.data(), value.size());
		});
	}

	return status::OK;
}

status csmap::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));
	check_outside_tx();
	unique_global_lock_type lock(mtx);
	return container->unsafe_erase(key) > 0 ? status::OK : status::NOT_FOUND;
}

void csmap::Recover()
{
	if (!OID_IS_NULL(*root_oid)) {
		auto pmem_ptr = static_cast<internal::csmap::pmem_type *>(
			pmemobj_direct(*root_oid));

		container = &pmem_ptr->map;
		container->runtime_initialize();
		container->key_comp().runtime_initialize(
			internal::extract_comparator(*config));
	} else {
		pmem::obj::transaction::run(pmpool, [&] {
			pmem::obj::transaction::snapshot(root_oid);
			*root_oid =
				pmem::obj::make_persistent<internal::csmap::pmem_type>()
					.raw();
			auto pmem_ptr = static_cast<internal::csmap::pmem_type *>(
				pmemobj_direct(*root_oid));
			container = &pmem_ptr->map;
			container->runtime_initialize();
			container->key_comp().initialize(
				internal::extract_comparator(*config));
		});
	}
}

internal::iterator_base *csmap::new_iterator()
{
	return new csmap_iterator<false>{container, mtx};
}

internal::iterator_base *csmap::new_const_iterator()
{
	return new csmap_iterator<true>{container, mtx};
}

csmap::csmap_iterator<true>::csmap_iterator(container_type *c, global_mutex_type &mtx)
    : container(c), lock(mtx), pop(pmem::obj::pool_by_vptr(c))
{
}

csmap::csmap_iterator<false>::csmap_iterator(container_type *c, global_mutex_type &mtx)
    : csmap::csmap_iterator<true>(c, mtx)
{
}

status csmap::csmap_iterator<true>::seek(string_view key)
{
	init_seek();

	it_ = container->find(key);
	if (it_ == container->end()) {
		return status::NOT_FOUND;
	}

	node_lock = csmap::unique_node_lock_type(it_->second.mtx);

	return status::OK;
}

status csmap::csmap_iterator<true>::seek_lower(string_view key)
{
	init_seek();

	it_ = container->find_lower(key);
	if (it_ == container->end())
		return status::NOT_FOUND;

	node_lock = csmap::unique_node_lock_type(it_->second.mtx);

	return status::OK;
}

status csmap::csmap_iterator<true>::seek_lower_eq(string_view key)
{
	init_seek();

	it_ = container->find_lower_eq(key);
	if (it_ == container->end())
		return status::NOT_FOUND;

	node_lock = csmap::unique_node_lock_type(it_->second.mtx);

	return status::OK;
}

status csmap::csmap_iterator<true>::seek_higher(string_view key)
{
	init_seek();

	it_ = container->find_higher(key);
	if (it_ == container->end())
		return status::NOT_FOUND;

	node_lock = csmap::unique_node_lock_type(it_->second.mtx);

	return status::OK;
}

status csmap::csmap_iterator<true>::seek_higher_eq(string_view key)
{
	init_seek();

	it_ = container->find_higher_eq(key);
	if (it_ == container->end())
		return status::NOT_FOUND;

	node_lock = csmap::unique_node_lock_type(it_->second.mtx);

	return status::OK;
}

status csmap::csmap_iterator<true>::seek_to_first()
{
	init_seek();

	if (container->empty())
		return status::NOT_FOUND;

	it_ = container->begin();

	node_lock = csmap::unique_node_lock_type(it_->second.mtx);

	return status::OK;
}

status csmap::csmap_iterator<true>::is_next()
{
	auto tmp = it_;
	if (tmp == container->end() || ++tmp == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status csmap::csmap_iterator<true>::next()
{
	init_seek();

	if (it_ == container->end() || ++it_ == container->end())
		return status::NOT_FOUND;

	node_lock = csmap::unique_node_lock_type(it_->second.mtx);

	return status::OK;
}

result<string_view> csmap::csmap_iterator<true>::key()
{
	assert(it_ != container->end());

	return string_view(it_->first.data(), it_->first.length());
}

result<pmem::obj::slice<const char *>> csmap::csmap_iterator<true>::read_range(size_t pos,
									       size_t n)
{
	assert(it_ != container->end());

	if (pos + n > it_->second.val.size() || pos + n < pos)
		n = it_->second.val.size() - pos;

	return {it_->second.val.crange(pos, n)};
}

result<pmem::obj::slice<char *>> csmap::csmap_iterator<false>::write_range(size_t pos,
									   size_t n)
{
	assert(it_ != container->end());

	if (pos + n > it_->second.val.size() || pos + n < pos)
		n = it_->second.val.size() - pos;

	log.push_back({{it_->second.val.cdata() + pos, n}, pos});
	auto &val = log.back().first;

	return {{&val[0], &val[n]}};
}

status csmap::csmap_iterator<false>::commit()
{
	pmem::obj::transaction::run(pop, [&] {
		for (auto &p : log) {
			auto dest = it_->second.val.range(p.second, p.first.size());
			std::copy(p.first.begin(), p.first.end(), dest.begin());
		}
	});
	log.clear();

	return status::OK;
}

void csmap::csmap_iterator<false>::abort()
{
	log.clear();
}

void csmap::csmap_iterator<true>::init_seek()
{
	if (it_ != container->end())
		node_lock.unlock();
}

void csmap::csmap_iterator<false>::init_seek()
{
	csmap::csmap_iterator<true>::init_seek();

	log.clear();
}

static factory_registerer
	register_csmap(std::unique_ptr<engine_base::factory_base>(new csmap_factory));

} // namespace kv
} // namespace pmem
