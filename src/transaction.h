// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#ifndef LIBPMEMKV_TRANSACTION_H
#define LIBPMEMKV_TRANSACTION_H

#include "libpmemkv.hpp"

#include <cassert>

namespace pmem
{

namespace kv
{

namespace internal
{

class transaction {
public:
	transaction()
	{
	}

	virtual ~transaction()
	{
	}

	virtual status put(string_view key, string_view value) = 0;
	virtual status commit() = 0;
	virtual void abort() = 0;

	virtual status remove(string_view key)
	{
		return status::NOT_SUPPORTED;
	}
};

class dram_log {
public:
	using element_type = std::pair<std::string, std::string>;

	void insert(string_view key, string_view value)
	{
		op_type.emplace_back(operation::insert);
		log.emplace_back(std::piecewise_construct,
				 std::forward_as_tuple(key.data(), key.size()),
				 std::forward_as_tuple(value.data(), value.size()));
	}

	void remove(string_view key)
	{
		op_type.emplace_back(operation::remove);
		log.emplace_back(std::piecewise_construct,
				 std::forward_as_tuple(key.data(), key.size()),
				 std::forward_as_tuple());
	}

	template <typename F1, typename F2>
	void foreach (F1 &&insert_cb, F2 && remove_cb)
	{
		assert(op_type.size() == log.size());

		for (size_t i = 0; i < log.size(); i++) {
			switch (op_type[i]) {
				case operation::insert:
					insert_cb(log[i]);
					break;
				case operation::remove:
					remove_cb(log[i]);
					break;
				default:
					assert(false);
					break;
			}
		}
	}

	void clear()
	{
		op_type.clear();
		log.clear();
	}

private:
	enum class operation { insert, remove };

	std::vector<operation> op_type;
	std::vector<element_type> log;
};

} /* namespace internal */
} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_TRANSACTION_H */
