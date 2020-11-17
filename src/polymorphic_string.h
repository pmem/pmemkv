// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#pragma once

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/slice.hpp>
#include <string>

#include "libpmemkv.hpp"

namespace pmem
{
namespace kv
{
class polymorphic_string {
public:
	using pmem_string = pmem::obj::string;
	polymorphic_string()
	{
	}

	polymorphic_string(const char *data, std::size_t size) : pstr(data, size)
	{
	}

	polymorphic_string(const std::string &s) : pstr(s)
	{
	}

	polymorphic_string(const pmem_string &s) : pstr(s)
	{
	}

	polymorphic_string(string_view s) : pstr(s.data(), s.size())
	{
	}

	polymorphic_string(const polymorphic_string &s) : pstr(s.pstr)
	{
	}

	polymorphic_string &operator=(const std::string &s)
	{
		pstr = s;
		return *this;
	}

	polymorphic_string &operator=(const pmem_string &s)
	{
		check_pmem();
		pstr = s;

		return *this;
	}

	polymorphic_string &operator=(const polymorphic_string &s)
	{
		this->operator=(s.pstr);
		return *this;
	}

	polymorphic_string &operator=(string_view s)
	{
		pstr.assign(s.data(), s.size());
		return *this;
	}

	char &operator[](size_t n)
	{
		return pstr[n];
	}

	const char &operator[](size_t n) const
	{
		return pstr[n];
	}

	const char *c_str() const
	{
		return pstr.c_str();
	}

	size_t size() const
	{
		return pstr.size();
	}

	size_t length() const
	{
		return size();
	}

	bool empty() const
	{
		return size() == 0;
	}

	bool operator==(const polymorphic_string &rhs) const
	{
		return compare(0U, size(), rhs.c_str(), rhs.size()) == 0;
	}

	bool operator==(string_view rhs) const
	{
		return compare(0U, size(), rhs.data(), rhs.size()) == 0;
	}

	bool operator==(const std::string &rhs) const
	{
		return compare(0U, size(), rhs.c_str(), rhs.size()) == 0;
	}

	template <typename... Args>
	int compare(Args &&... args) const
	{
		return pstr.compare(std::forward<Args>(args)...);
	}

	pmem::obj::slice<char *> range(size_t p, size_t n)
	{
		return pstr.range(p, n);
	}

private:
	/* required for layout in compatibility purpose */
	pmem::obj::p<bool> unused = true;
	pmem_string pstr;

	void check_pmem() const
	{
		assert(pmemobj_pool_by_ptr(this) != nullptr);
	}
}; // class polymorphic_string

inline bool operator==(string_view lhs, const polymorphic_string &rhs)
{
	return rhs == lhs;
}

inline bool operator==(const std::string &lhs, const polymorphic_string &rhs)
{
	return rhs == lhs;
}

} /* namespace kv */
} /* namespace pmem */
