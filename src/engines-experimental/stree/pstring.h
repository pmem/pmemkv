// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#ifndef PERSISTENT_PSTRING_H
#define PERSISTENT_PSTRING_H

#include <stdexcept>
#include <string.h>

template <size_t CAPACITY>
class pstring {
	static const size_t BUFFER_SIZE = CAPACITY + 1;

public:
	pstring(const std::string &s = "\0")
	{
		init(s.c_str(), s.size());
	}

	pstring(const char *data, size_t size)
	{
		init(data, size);
	}

	pstring(const pstring &other)
	{
		init(other.c_str(), other.size());
	}

	pstring &operator=(const pstring &other)
	{
		init(other.c_str(), other.size());
		return *this;
	}

	pstring &operator=(const std::string &s)
	{
		init(s.c_str(), s.size());
		return *this;
	}

	const char *c_str() const
	{
		return str;
	}

	size_t size() const
	{
		return _size;
	}

	char *begin()
	{
		return str;
	}

	char *end()
	{
		return str + _size;
	}

	const char *begin() const
	{
		return str;
	}

	const char *end() const
	{
		return str + _size;
	}

	int compare(const pstring &rhs) const
	{
		auto m_l = std::min(size(), rhs.size());

		auto r = memcmp(begin(), rhs.begin(), m_l);
		if (r == 0) {
			if (size() < rhs.size())
				return -1;
			else if (size() > rhs.size())
				return 1;
			else
				return 0;
		}

		return r;
	}

private:
	void init(const char *src, size_t size)
	{
		if (size > CAPACITY)
			throw std::length_error("size exceed pstring capacity");
		memcpy(str, src, size);
		str[size] = '\0';
		_size = size;
	}

	char str[BUFFER_SIZE];
	size_t _size;
};

template <size_t size>
inline bool operator<(const pstring<size> &lhs, const pstring<size> &rhs)
{
	return lhs.compare(rhs) < 0;
}

template <size_t size>
inline bool operator>(const pstring<size> &lhs, const pstring<size> &rhs)
{
	return lhs.compare(rhs) > 0;
}

template <size_t size>
inline bool operator==(const pstring<size> &lhs, const pstring<size> &rhs)
{
	return lhs.compare(rhs) == 0;
}

template <size_t size>
std::ostream &operator<<(std::ostream &os, const pstring<size> &obj)
{
	return os << obj.c_str();
}

#endif // PERSISTENT_PSTRING_H
