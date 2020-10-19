// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#ifndef LIBPMEMKV_ITERATOR_H
#define LIBPMEMKV_ITERATOR_H

#include "libpmemkv.hpp"

namespace pmem
{
namespace kv
{
namespace internal
{

/* XXX: make class abstract */
class iterator_base {
public:
	// iterator_base(); // in ctor, we might take global locks if necessary (e.g. for
	// csmap)
	virtual ~iterator_base() = default;

	// iterator_base(const iterator_base&) = delete;
	// iterator_base(iterator_base&&);

	// iterator_base& operator=(iterator_base&&); // TODO: do we want move assignment
	// op? iterator_base& operator=(const iterator_base&) = delete;

	virtual status seek(string_view key);
	virtual status seek_lower(string_view key);
	virtual status seek_lower_eq(string_view key);
	virtual status seek_higher(string_view key);
	virtual status seek_higher_eq(string_view key);

	virtual status seek_to_first();
	virtual status seek_to_last();

	virtual status next();
	virtual status prev();

	// result<string_view, status> key();
};

template <bool IsConst>
class iterator : public iterator_base {
public:
	// result<string_view, status> value(); // only for IsConst

	// result<accessor, status> value();    // only for !IsConst, starts a transaction
	// TODO: should this function consume iterator?
};

} /* namespace internal */
} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_ITERATOR_H */
