// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#include "vsmap.h"

#include "../comparator/comparator.h"
#include "../comparator/volatile_comparator.h"
#include "../out.h"

#include <libpmemobj++/transaction.hpp>

#include <cassert>

namespace pmem
{
namespace kv
{

vsmap::vsmap(std::unique_ptr<internal::config> cfg)
    : kv_allocator(cfg->get_path(), cfg->get_size()),
      pmem_kv_container(internal::volatile_compare(internal::extract_comparator(*cfg)),
			kv_allocator),
      config(std::move(cfg))
{
	LOG("Started ok");
}

vsmap::~vsmap()
{
	LOG("Stopped ok");
}

std::string vsmap::name()
{
	return "vsmap";
}

status vsmap::count_all(std::size_t &cnt)
{
	cnt = pmem_kv_container.size();
	return status::OK;
}

status vsmap::count_above(string_view key, std::size_t &cnt)
{
	LOG("count_above for key=" << std::string(key.data(), key.size()));
	// XXX - do not create temporary string
	auto it = pmem_kv_container.upper_bound(
		key_type(key.data(), key.size(), kv_allocator));
	auto end = pmem_kv_container.end();

	cnt = internal::distance(it, end);
	return status::OK;
}

status vsmap::count_equal_above(string_view key, std::size_t &cnt)
{
	LOG("count_equal_above for key=" << std::string(key.data(), key.size()));
	// XXX - do not create temporary string
	auto it = pmem_kv_container.lower_bound(
		key_type(key.data(), key.size(), kv_allocator));
	auto end = pmem_kv_container.end();

	cnt = internal::distance(it, end);
	return status::OK;
}

status vsmap::count_equal_below(string_view key, std::size_t &cnt)
{
	LOG("count_equal_below for key=" << std::string(key.data(), key.size()));
	auto it = pmem_kv_container.begin();
	// XXX - do not create temporary string
	auto end = pmem_kv_container.upper_bound(
		key_type(key.data(), key.size(), kv_allocator));

	cnt = internal::distance(it, end);
	return status::OK;
}

status vsmap::count_below(string_view key, std::size_t &cnt)
{
	LOG("count_below for key=" << std::string(key.data(), key.size()));
	auto it = pmem_kv_container.begin();
	// XXX - do not create temporary string
	auto end = pmem_kv_container.lower_bound(
		key_type(key.data(), key.size(), kv_allocator));

	cnt = internal::distance(it, end);
	return status::OK;
}

status vsmap::count_between(string_view key1, string_view key2, std::size_t &cnt)
{
	LOG("count_between for key1=" << key1.data() << ", key2=" << key2.data());
	std::size_t result = 0;
	if (pmem_kv_container.key_comp()(key1, key2)) {
		// XXX - do not create temporary string
		auto it = pmem_kv_container.upper_bound(
			key_type(key1.data(), key1.size(), kv_allocator));
		auto end = pmem_kv_container.lower_bound(
			key_type(key2.data(), key2.size(), kv_allocator));
		result = internal::distance(it, end);
	}

	cnt = result;
	return status::OK;
}

status vsmap::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	return internal::iterate_through_pairs(pmem_kv_container.begin(),
					       pmem_kv_container.end(), callback, arg);
}

status vsmap::get_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_above for key=" << std::string(key.data(), key.size()));
	// XXX - do not create temporary string
	auto it = pmem_kv_container.upper_bound(
		key_type(key.data(), key.size(), kv_allocator));
	auto end = pmem_kv_container.end();
	return internal::iterate_through_pairs(it, end, callback, arg);
}

status vsmap::get_equal_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_above for key=" << std::string(key.data(), key.size()));
	// XXX - do not create temporary string
	auto it = pmem_kv_container.lower_bound(
		key_type(key.data(), key.size(), kv_allocator));
	auto end = pmem_kv_container.end();
	return internal::iterate_through_pairs(it, end, callback, arg);
}

status vsmap::get_equal_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_above for key=" << std::string(key.data(), key.size()));
	auto it = pmem_kv_container.begin();
	// XXX - do not create temporary string
	auto end = pmem_kv_container.upper_bound(
		key_type(key.data(), key.size(), kv_allocator));
	return internal::iterate_through_pairs(it, end, callback, arg);
}

status vsmap::get_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_below for key=" << std::string(key.data(), key.size()));
	auto it = pmem_kv_container.begin();
	// XXX - do not create temporary string
	auto end = pmem_kv_container.lower_bound(
		key_type(key.data(), key.size(), kv_allocator));
	return internal::iterate_through_pairs(it, end, callback, arg);
}

status vsmap::get_between(string_view key1, string_view key2, get_kv_callback *callback,
			  void *arg)
{
	LOG("get_between for key1=" << key1.data() << ", key2=" << key2.data());
	if (pmem_kv_container.key_comp()(key1, key2)) {
		// XXX - do not create temporary string
		auto it = pmem_kv_container.upper_bound(
			key_type(key1.data(), key1.size(), kv_allocator));
		auto end = pmem_kv_container.lower_bound(
			key_type(key2.data(), key2.size(), kv_allocator));
		return internal::iterate_through_pairs(it, end, callback, arg);
	}

	return status::OK;
}

status vsmap::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	// XXX - do not create temporary string
	bool r = pmem_kv_container.find(key_type(key.data(), key.size(), kv_allocator)) !=
		pmem_kv_container.end();
	return (r ? status::OK : status::NOT_FOUND);
}

status vsmap::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));
	// XXX - do not create temporary string
	const auto pos =
		pmem_kv_container.find(key_type(key.data(), key.size(), kv_allocator));
	if (pos == pmem_kv_container.end()) {
		LOG("  key not found");
		return status::NOT_FOUND;
	}

	callback(pos->second.c_str(), pos->second.size(), arg);
	return status::OK;
}

status vsmap::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));
	// XXX - starting from C++17 std::map has try_emplace method which could be more
	// efficient
	auto res = pmem_kv_container.emplace(
		std::piecewise_construct, std::forward_as_tuple(key.data(), key.size()),
		std::forward_as_tuple(value.data(), value.size()));
	if (!res.second) {
		auto it = res.first;
		it->second.assign(value.data(), value.size());
	}
	return status::OK;
}

status vsmap::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));

	// XXX - do not create temporary string
	bool erased =
		pmem_kv_container.erase(key_type(key.data(), key.size(), kv_allocator));
	return (erased ? status::OK : status::NOT_FOUND);
}

internal::iterator_base *vsmap::new_iterator()
{
	return new vsmap_iterator<false>{&pmem_kv_container, &kv_allocator};
}

internal::iterator_base *vsmap::new_const_iterator()
{
	return new vsmap_iterator<true>{&pmem_kv_container, &kv_allocator};
}

vsmap::vsmap_iterator<true>::vsmap_iterator(container_type *c,
					    vsmap::map_allocator_type *alloc)
    : container(c), kv_allocator(alloc)
{
}

vsmap::vsmap_iterator<false>::vsmap_iterator(container_type *c,
					     vsmap::map_allocator_type *alloc)
    : vsmap::vsmap_iterator<true>(c, alloc)
{
}

status vsmap::vsmap_iterator<true>::seek(string_view key)
{
	init_seek();

	it_ = container->find(vsmap::key_type(key.data(), key.size(), *kv_allocator));
	if (it_ != container->end())
		return status::OK;

	return status::NOT_FOUND;
}

status vsmap::vsmap_iterator<true>::seek_lower(string_view key)
{
	init_seek();

	it_ = container->lower_bound(
		vsmap::key_type(key.data(), key.size(), *kv_allocator));
	if (it_ == container->begin()) {
		it_ = container->end();
		return status::NOT_FOUND;
	}

	--it_;

	return status::OK;
}

status vsmap::vsmap_iterator<true>::seek_lower_eq(string_view key)
{
	init_seek();

	it_ = container->upper_bound(
		vsmap::key_type(key.data(), key.size(), *kv_allocator));
	if (it_ == container->begin()) {
		it_ = container->end();
		return status::NOT_FOUND;
	}

	--it_;

	return status::OK;
}

status vsmap::vsmap_iterator<true>::seek_higher(string_view key)
{
	init_seek();

	it_ = container->upper_bound(
		vsmap::key_type(key.data(), key.size(), *kv_allocator));
	if (it_ == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status vsmap::vsmap_iterator<true>::seek_higher_eq(string_view key)
{
	init_seek();

	it_ = container->lower_bound(
		vsmap::key_type(key.data(), key.size(), *kv_allocator));
	if (it_ == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status vsmap::vsmap_iterator<true>::seek_to_first()
{
	init_seek();

	if (container->empty())
		return status::NOT_FOUND;

	it_ = container->begin();

	return status::OK;
}

status vsmap::vsmap_iterator<true>::seek_to_last()
{
	init_seek();

	if (container->empty())
		return status::NOT_FOUND;

	it_ = container->end();
	--it_;

	return status::OK;
}

status vsmap::vsmap_iterator<true>::is_next()
{
	auto tmp = it_;
	if (tmp == container->end() || ++tmp == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status vsmap::vsmap_iterator<true>::next()
{
	init_seek();

	if (it_ == container->end() || ++it_ == container->end())
		return status::NOT_FOUND;

	return status::OK;
}

status vsmap::vsmap_iterator<true>::prev()
{
	init_seek();

	if (it_ == container->begin())
		return status::NOT_FOUND;

	--it_;

	return status::OK;
}

result<string_view> vsmap::vsmap_iterator<true>::key()
{
	assert(it_ != container->end());

	return string_view(it_->first.data(), it_->first.length());
}

result<pmem::obj::slice<const char *>> vsmap::vsmap_iterator<true>::read_range(size_t pos,
									       size_t n)
{
	assert(it_ != container->end());

	if (pos + n > it_->second.size() || pos + n < pos)
		n = it_->second.size() - pos;

	return {{it_->second.data() + pos, it_->second.data() + pos + n}};
}

result<pmem::obj::slice<char *>> vsmap::vsmap_iterator<false>::write_range(size_t pos,
									   size_t n)
{
	assert(it_ != container->end());

	if (pos + n > it_->second.size() || pos + n < pos)
		n = it_->second.size() - pos;

	log.push_back({std::string(&(it_->second[pos]), n), pos});
	auto &val = log.back().first;

	return {{&val[0], &val[0] + n}};
}

status vsmap::vsmap_iterator<false>::commit()
{
	for (auto &p : log) {
		auto dest = &(it_->second[0]) + p.second;
		std::copy(p.first.begin(), p.first.end(), dest);
	}
	log.clear();

	return status::OK;
}

void vsmap::vsmap_iterator<false>::abort()
{
	log.clear();
}

static factory_registerer
	register_vsmap(std::unique_ptr<engine_base::factory_base>(new vsmap_factory));

} // namespace kv
} // namespace pmem
