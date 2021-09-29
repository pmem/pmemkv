// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#ifndef LIBPMEMKV_BASIC_VCMAP_H
#define LIBPMEMKV_BASIC_VCMAP_H

#include "../engine.h"
#include "../out.h"

#include <cassert>
#include <memory>
#include <scoped_allocator>
#include <string>
#include <tbb/concurrent_hash_map.h>

namespace pmem
{
namespace kv
{

template <typename AllocatorFactory>
class basic_vcmap : public engine_base {
	class basic_vcmap_iterator;
	class basic_vcmap_const_iterator;

public:
	basic_vcmap(std::unique_ptr<internal::config> cfg);
	~basic_vcmap();

	std::string name() final;

	status count_all(std::size_t &cnt) final;

	status get_all(get_kv_callback *callback, void *arg) final;

	status exists(string_view key) final;

	status get(string_view key, get_v_callback *callback, void *arg) final;

	status put(string_view key, string_view value) final;

	status remove(string_view key) final;

	internal::iterator_base *new_iterator() final;
	internal::iterator_base *new_const_iterator() final;

private:
	using ch_allocator_t = typename AllocatorFactory::template allocator_type<char>;
	using pmem_string =
		std::basic_string<char, std::char_traits<char>, ch_allocator_t>;
	using kv_allocator_t = typename AllocatorFactory::template allocator_type<
		std::pair<const pmem_string, pmem_string>>;

	using map_t =
		tbb::concurrent_hash_map<pmem_string, pmem_string,
					 tbb::tbb_hash_compare<pmem_string>,
					 std::scoped_allocator_adaptor<kv_allocator_t>>;

	/*  **config_** may hold instance of allocator used by memkind_allocator_wrapper,
	 * so it has to be stored during whole lifetime of vcmap instance. */
	std::unique_ptr<internal::config> config_;
	kv_allocator_t kv_allocator;
	ch_allocator_t ch_allocator;
	map_t pmem_kv_container;
};

template <typename AllocatorFactory>
basic_vcmap<AllocatorFactory>::basic_vcmap(std::unique_ptr<internal::config> cfg)
    : config_(std::move(cfg)),
      kv_allocator(AllocatorFactory::template create<kv_allocator_t>(*config_)),
      ch_allocator(kv_allocator),
      pmem_kv_container(std::scoped_allocator_adaptor<kv_allocator_t>(kv_allocator))
{
	LOG("Started ok");
}

template <typename AllocatorFactory>
basic_vcmap<AllocatorFactory>::~basic_vcmap()
{
	LOG("Stopped ok");
}

template <typename AllocatorFactory>
std::string basic_vcmap<AllocatorFactory>::name()
{
	return "basic_vcmap";
}

template <typename AllocatorFactory>
status basic_vcmap<AllocatorFactory>::count_all(std::size_t &cnt)
{
	LOG("count_all");
	cnt = pmem_kv_container.size();

	return status::OK;
}

template <typename AllocatorFactory>
status basic_vcmap<AllocatorFactory>::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	for (auto &iterator : pmem_kv_container) {
		auto ret = callback(iterator.first.c_str(), iterator.first.size(),
				    iterator.second.c_str(), iterator.second.size(), arg);

		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

template <typename AllocatorFactory>
status basic_vcmap<AllocatorFactory>::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	typename map_t::const_accessor result;
	// XXX - do not create temporary string
	const bool result_found = pmem_kv_container.find(
		result, pmem_string(key.data(), key.size(), ch_allocator));
	return (result_found ? status::OK : status::NOT_FOUND);
}

template <typename AllocatorFactory>
status basic_vcmap<AllocatorFactory>::get(string_view key, get_v_callback *callback,
					  void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));
	typename map_t::const_accessor result;
	// XXX - do not create temporary string
	const bool result_found = pmem_kv_container.find(
		result, pmem_string(key.data(), key.size(), ch_allocator));
	if (!result_found) {
		LOG("  key not found");
		return status::NOT_FOUND;
	}

	callback(result->second.c_str(), result->second.size(), arg);
	return status::OK;
}

template <typename AllocatorFactory>
status basic_vcmap<AllocatorFactory>::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));

	typename map_t::value_type kv_pair(
		std::piecewise_construct,
		std::forward_as_tuple(key.data(), key.size(), ch_allocator),
		std::forward_as_tuple(ch_allocator));

	typename map_t::accessor acc;
	pmem_kv_container.insert(acc, std::move(kv_pair));
	acc->second.assign(value.data(), value.size());

	return status::OK;
}

template <typename AllocatorFactory>
status basic_vcmap<AllocatorFactory>::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));

	// XXX - do not create temporary string
	bool erased = pmem_kv_container.erase(
		pmem_string(key.data(), key.size(), ch_allocator));
	return (erased ? status::OK : status::NOT_FOUND);
}

template <typename AllocatorFactory>
class basic_vcmap<AllocatorFactory>::basic_vcmap_const_iterator
    : virtual public internal::iterator_base {
	using container_type = basic_vcmap<AllocatorFactory>::map_t;
	using ch_allocator_t = basic_vcmap<AllocatorFactory>::ch_allocator_t;

public:
	basic_vcmap_const_iterator(container_type *container, ch_allocator_t *ca);

	status seek(string_view key) final;

	result<string_view> key() final;

	result<pmem::obj::slice<const char *>> read_range(size_t pos, size_t n) final;

protected:
	container_type *container;
	typename container_type::accessor acc_;
	ch_allocator_t *ch_allocator;
};

template <typename AllocatorFactory>
class basic_vcmap<AllocatorFactory>::basic_vcmap_iterator
    : public basic_vcmap<AllocatorFactory>::basic_vcmap_const_iterator {
	using container_type = basic_vcmap<AllocatorFactory>::map_t;
	using ch_allocator_t = basic_vcmap<AllocatorFactory>::ch_allocator_t;

public:
	basic_vcmap_iterator(container_type *container, ch_allocator_t *ca);

	result<pmem::obj::slice<char *>> write_range(size_t pos, size_t n) final;
	status commit() final;
	void abort() final;

private:
	std::vector<std::pair<std::string, size_t>> log;
};

template <typename AllocatorFactory>
internal::iterator_base *basic_vcmap<AllocatorFactory>::new_iterator()
{
	return new basic_vcmap_iterator{&pmem_kv_container, &ch_allocator};
}

template <typename AllocatorFactory>
internal::iterator_base *basic_vcmap<AllocatorFactory>::new_const_iterator()
{
	return new basic_vcmap_const_iterator{&pmem_kv_container, &ch_allocator};
}

template <typename AllocatorFactory>
basic_vcmap<AllocatorFactory>::basic_vcmap_const_iterator::basic_vcmap_const_iterator(
	container_type *c, ch_allocator_t *ca)
    : container(c), ch_allocator(ca)
{
}

template <typename AllocatorFactory>
basic_vcmap<AllocatorFactory>::basic_vcmap_iterator::basic_vcmap_iterator(
	container_type *c, ch_allocator_t *ca)
    : basic_vcmap<AllocatorFactory>::basic_vcmap_const_iterator(c, ca)
{
}

template <typename AllocatorFactory>
status basic_vcmap<AllocatorFactory>::basic_vcmap_const_iterator::seek(string_view key)
{
	init_seek();

	if (container->find(acc_, pmem_string(key.data(), key.size(), *ch_allocator)))
		return status::OK;

	return status::NOT_FOUND;
}

template <typename AllocatorFactory>
result<string_view> basic_vcmap<AllocatorFactory>::basic_vcmap_const_iterator::key()
{
	assert(!acc_.empty());

	return string_view(acc_->first.data(), acc_->first.length());
}

template <typename AllocatorFactory>
result<pmem::obj::slice<const char *>>
basic_vcmap<AllocatorFactory>::basic_vcmap_const_iterator::read_range(size_t pos,
								      size_t n)
{
	assert(!acc_.empty());

	if (pos + n > acc_->second.size() || pos + n < pos)
		n = acc_->second.size() - pos;

	return {{acc_->second.c_str() + pos, acc_->second.c_str() + pos + n}};
}

template <typename AllocatorFactory>
result<pmem::obj::slice<char *>>
basic_vcmap<AllocatorFactory>::basic_vcmap_iterator::write_range(size_t pos, size_t n)
{
	assert(!this->acc_.empty());

	if (pos + n > this->acc_->second.size() || pos + n < pos)
		n = this->acc_->second.size() - pos;

	log.push_back({std::string(this->acc_->second.c_str() + pos, n), pos});
	auto &val = log.back().first;

	return {{&val[0], &val[n]}};
}

template <typename AllocatorFactory>
status basic_vcmap<AllocatorFactory>::basic_vcmap_iterator::commit()
{
	for (auto &p : log) {
		auto dest = &(this->acc_->second[0]) + p.second;
		std::copy(p.first.begin(), p.first.end(), dest);
	}
	log.clear();

	return status::OK;
}

template <typename AllocatorFactory>
void basic_vcmap<AllocatorFactory>::basic_vcmap_iterator::abort()
{
	log.clear();
}

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_BASIC_VCMAP_H */
