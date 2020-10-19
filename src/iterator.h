// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

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

class accessor_base;

class iterator_base {
public:
	virtual ~iterator_base() = default;

	virtual status seek(string_view key) = 0;
	virtual status seek_lower(string_view key);
	virtual status seek_lower_eq(string_view key);
	virtual status seek_higher(string_view key);
	virtual status seek_higher_eq(string_view key);

	virtual status seek_to_first() = 0;
	virtual status seek_to_last() = 0;

	virtual status next();
	virtual status prev();

	virtual std::pair<string_view, status> key() = 0;
};

template <bool IsConst>
class iterator : public iterator_base {
	using value_return_type =
		typename std::conditional<IsConst, string_view,
					  internal::accessor_base *>::type;

public:
	virtual std::pair<value_return_type, status> value() = 0;
};

class accessor_base {
public:
	virtual ~accessor_base() = default;

	virtual std::pair<pmem::obj::slice<const char *>, status>
	read_range(size_t pos, size_t n) = 0;
	virtual std::pair<pmem::obj::slice<char *>, status> write_range(size_t pos,
									size_t n) = 0;

	virtual status commit();
	virtual void abort();
};

class non_volatile_accessor : public accessor_base {
public:
	non_volatile_accessor(pmem::obj::pool_base &pop);

	status commit() final;
	void abort() final;

private:
	pmem::obj::transaction::manual _tx;
};

} /* namespace internal */
} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_ITERATOR_H */
