/*
 * Copyright 2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <libpmemobj++/experimental/string.hpp>
#include <libpmemobj++/p.hpp>
#include <string>

#include "libpmemkv.hpp"

namespace pmem
{
namespace kv
{
class polymorphic_string {
public:
	using pmem_string = pmem::obj::experimental::string;
	polymorphic_string()
	{
		construct();
	}

	polymorphic_string(const char *data, std::size_t size)
	{
		construct(data, size);
	}

	polymorphic_string(const std::string &s)
	{
		construct(s);
	}

	polymorphic_string(const pmem_string &s)
	{
		construct(s.c_str(), s.size());
	}

	polymorphic_string(string_view s)
	{
		construct(s.data(), s.size());
	}

	polymorphic_string(const polymorphic_string &s)
	{
		construct(s.c_str(), s.size());
	}

	polymorphic_string &operator=(const std::string &s)
	{
		if (is_pmem.get_ro()) {
			pstr = s;
		} else {
			str = s;
		}

		return *this;
	}

	polymorphic_string &operator=(const pmem_string &s)
	{
		assert(is_pmem.get_ro());
		pstr = s;

		return *this;
	}

	polymorphic_string &operator=(const polymorphic_string &s)
	{
		if (s.is_pmem.get_ro()) {
			this->operator=(s.pstr);
		} else {
			this->operator=(s.str);
		}

		return *this;
	}

	polymorphic_string &operator=(string_view s)
	{
		if (is_pmem.get_ro()) {
			pstr.assign(s.data(), s.size());
		} else {
			str.assign(s.data(), s.size());
		}

		return *this;
	}

	~polymorphic_string()
	{
		if (is_pmem.get_ro()) {
			pstr.~basic_string();
		} else {
			str.~basic_string();
		}
	}

	char &operator[](size_t n)
	{
		return is_pmem.get_ro() ? pstr[n] : str[n];
	}

	const char &operator[](size_t n) const
	{
		return is_pmem.get_ro() ? pstr[n] : str[n];
	}

	const char *c_str() const
	{
		return is_pmem.get_ro() ? pstr.c_str() : str.c_str();
	}

	size_t size() const
	{
		return is_pmem.get_ro() ? pstr.size() : str.size();
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
		return is_pmem.get_ro() ? pstr.compare(std::forward<Args>(args)...)
					: str.compare(std::forward<Args>(args)...);
	}

private:
	pmem::obj::p<bool> is_pmem;
	union {
		std::string str;
		pmem_string pstr;
	};

	bool check_pmem() const
	{
		return pmemobj_pool_by_ptr(this) != nullptr;
	}

	template <typename... Args>
	void construct(Args &&... args)
	{
		is_pmem.get_rw() = check_pmem();
		if (is_pmem.get_ro()) {
			new (&pstr) pmem_string(std::forward<Args>(args)...);
		} else {
			new (&str) std::string(std::forward<Args>(args)...);
		}
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
