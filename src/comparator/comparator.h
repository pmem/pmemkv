// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#ifndef LIBPMEMKV_COMPARATOR_H
#define LIBPMEMKV_COMPARATOR_H

#include "../config.h"
#include "../exceptions.h"
#include "../libpmemkv.h"
#include "../libpmemkv.hpp"
#include "../out.h"

namespace pmem
{
namespace kv
{
namespace internal
{

class comparator {
public:
	comparator(pmemkv_compare_function *cmp, std::string name, void *arg)
	    : cmp(cmp), name_(name), arg(arg)
	{
	}

	int compare(string_view key1, string_view key2) const
	{
		return (*cmp)(key1.data(), key1.size(), key2.data(), key2.size(), arg);
	}

	std::string name() const
	{
		return name_;
	}

private:
	pmemkv_compare_function *cmp;
	std::string name_;
	void *arg;
};

static inline int binary_compare(const char *key1, size_t kb1, const char *key2,
				 size_t kb2, void *arg)
{
	(void)arg;
	return string_view(key1, kb1).compare(string_view(key2, kb2));
}

static inline const comparator &binary_comparator()
{
	static const comparator cmp(binary_compare, "__pmemkv_binary_comparator",
				    nullptr);
	return cmp;
}

template <typename T,
	  typename Enable = typename std::enable_if<std::is_same<
		  const char *, decltype(std::declval<T>().c_str())>::value>::type>
static inline string_view make_string_view(const T &str)
{
	return string_view(str.c_str(), str.size());
}

static inline string_view make_string_view(string_view v)
{
	return v;
}

static inline const comparator *extract_comparator(internal::config &cfg)
{
	comparator *cmp;

	if (!cfg.get_object("comparator", (void **)&cmp))
		return &internal::binary_comparator();
	else
		return cmp;
}

} /* namespace internal */
} /* namespace kv */
} /* namespace pmem */

#endif
