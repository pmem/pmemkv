// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation. */

#ifndef MEMKIND_ALLOCATOR_WRAPPER_H
#define MEMKIND_ALLOCATOR_WRAPPER_H

#include "pmem_allocator.h"

#ifdef USE_LIBMEMKIND_NAMESPACE
namespace memkind_ns = libmemkind::pmem;
#else
namespace memkind_ns = pmem;
#endif

namespace pmem
{
namespace kv
{
namespace internal
{
template <typename T>
class memkind_allocator_wrapper {
	using allocator_type = memkind_ns::allocator<T>;
	allocator_type *allocator_ptr;
	bool is_copy = false;

public:
	using value_type = T;
	using pointer = value_type *;
	using const_pointer = const value_type *;
	using reference = value_type &;
	using const_reference = const value_type &;
	using size_type = size_t;
	using difference_type = ptrdiff_t;

	template <class U>
	struct rebind {
		using other = memkind_allocator_wrapper<U>;
	};

	template <typename U>
	friend class memkind_allocator_wrapper;

	memkind_allocator_wrapper(internal::config &cfg)
	    : memkind_allocator_wrapper(cfg.get_path(), cfg.get_size())
	{
	}

	explicit memkind_allocator_wrapper(const char *dir, size_t max_size)
	{
		allocator_ptr = new allocator_type(dir, max_size);
	}

	explicit memkind_allocator_wrapper(const std::string &dir, size_t max_size)
	    : memkind_allocator_wrapper(dir.c_str(), max_size)
	{
	}

	explicit memkind_allocator_wrapper(const char *dir, size_t max_size,
					   libmemkind::allocation_policy alloc_policy)
	{
		allocator_ptr = new allocator_type(dir, max_size, alloc_policy);
	}

	explicit memkind_allocator_wrapper(const std::string &dir, size_t max_size,
					   libmemkind::allocation_policy alloc_policy)
	    : memkind_allocator_wrapper(dir.c_str(), max_size, alloc_policy)
	{
	}

	memkind_allocator_wrapper(const memkind_allocator_wrapper &other)
	{
		allocator_ptr = other.allocator_ptr;
		is_copy = true;
	}

	template <typename U>
	memkind_allocator_wrapper(const memkind_allocator_wrapper<U> &other) noexcept
	{
		allocator_ptr = (allocator_type *)(other.allocator_ptr);
		is_copy = true;
	}

	memkind_allocator_wrapper(memkind_allocator_wrapper &&other) = default;

	template <typename U>
	memkind_allocator_wrapper(memkind_allocator_wrapper<U> &&other) noexcept
	{
		allocator_ptr = (allocator_type *)other.allocator_ptr;
		is_copy = other.is_copy;
		other.is_copy = true;
		other.allocator_ptr = nullptr;
	}

	memkind_allocator_wrapper<T> &operator=(const memkind_allocator_wrapper &other)
	{
		if (this != &other) {
			allocator_ptr = other.allocator_ptr;
			is_copy = true;
		}
		return *this;
	}

	template <typename U>
	memkind_allocator_wrapper<T> &
	operator=(const memkind_allocator_wrapper<U> &other) noexcept
	{
		if (this != &other) {
			allocator_ptr = other.allocator_ptr;
			is_copy = true;
		}
		return *this;
	}

	memkind_allocator_wrapper<T> &
	operator=(memkind_allocator_wrapper &&other) = default;

	template <typename U>
	memkind_allocator_wrapper<T> &
	operator=(memkind_allocator_wrapper<U> &&other) noexcept
	{
		allocator_ptr = (allocator_type *)other.allocator_ptr;
		is_copy = false;
		other.is_copy = true;
		other.allocator_ptr = nullptr;
		return *this;
	}

	~memkind_allocator_wrapper()
	{
		if (!is_copy)
			delete allocator_ptr;
	}

	pointer allocate(size_type n) const
	{
		return allocator_ptr->allocate(n);
	}

	void deallocate(pointer p, size_type n) const
	{
		allocator_ptr->deallocate(p, n);
	}

	template <class U, class... Args>
	void construct(U *p, Args &&... args) const
	{
		allocator_ptr->construct(p, std::forward<Args>(args)...);
	}

	void destroy(pointer p) const
	{
		allocator_ptr->destroy(p);
	}

	template <typename U, typename V>
	friend bool operator==(const memkind_allocator_wrapper<U> &lhs,
			       const memkind_allocator_wrapper<V> &rhs);

	template <typename U, typename V>
	friend bool operator!=(const memkind_allocator_wrapper<U> &lhs,
			       const memkind_allocator_wrapper<V> &rhs);
};

template <typename U, typename V>
bool operator==(const memkind_allocator_wrapper<U> &lhs,
		const memkind_allocator_wrapper<V> &rhs)
{
	return *lhs.allocator_ptr == *rhs.allocator_ptr;
}

template <typename U, typename V>
bool operator!=(const memkind_allocator_wrapper<U> &lhs,
		const memkind_allocator_wrapper<V> &rhs)
{
	return !(lhs == rhs);
}

} /* namespace internal */
} /* namespace kv */
} /* namespace pmem */

#endif /* MEMKIND_ALLOCATOR_WRAPPER_H */
