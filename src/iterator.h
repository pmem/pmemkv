// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#ifndef LIBPMEMKV_ITERATOR_H
#define LIBPMEMKV_ITERATOR_H

#include "libpmemkv.hpp"

#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/slice.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmem
{
namespace kv
{
namespace internal
{
class iterator_base {
public:
	virtual ~iterator_base() = default;

	virtual status seek(string_view key) = 0;
	virtual status seek_lower(string_view key);
	virtual status seek_lower_eq(string_view key);
	virtual status seek_higher(string_view key);
	virtual status seek_higher_eq(string_view key);

	virtual status seek_to_first();
	virtual status seek_to_last();

	virtual status is_next();
	virtual status next();
	virtual status prev();

	virtual result<string_view> key() = 0;
	virtual result<pmem::obj::slice<const char *>> read_range(size_t pos,
								  size_t n) = 0;

	virtual result<pmem::obj::slice<char *>> write_range(size_t pos, size_t n);

	virtual status commit();
	virtual void abort();

protected:
	virtual void init_seek();
};

template <typename It>
std::size_t distance(It first, It last)
{
	auto dist = std::distance(first, last);
	assert(dist >= 0);
	return static_cast<std::size_t>(dist);
}

/**
 * Helper function to iterate between specified range and execute
 * callback on every item.
 */
template <typename It>
status iterate_through_pairs(It first, It last, get_kv_callback *callback, void *arg)
{
	for (auto it = first; it != last; ++it) {
		auto ret = callback(it->first.c_str(), it->first.size(),
				    it->second.c_str(), it->second.size(), arg);
		if (ret != 0)
			return status::STOPPED_BY_CB;
	}
	return status::OK;
}

} /* namespace internal */
} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_ITERATOR_H */
