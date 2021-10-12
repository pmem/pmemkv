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
/* Wrapper class, which holds only a pointer to memkind allocator instance, and
 * doesn't manage allocator's lifecycle. An actual allocator instance has to be
 * stored and managed elsewhere. This is needed as a workaround for poor performance of
 * memkind allocator copy ctor. */
template <typename T>
class memkind_allocator_wrapper {

public:
	using allocator_type = memkind_ns::allocator<T>;
	using value_type = typename allocator_type::value_type;
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

	memkind_allocator_wrapper() = delete;

	memkind_allocator_wrapper(allocator_type *allocator) : allocator_ptr(allocator)
	{
	}

	memkind_allocator_wrapper(const memkind_allocator_wrapper &other)
	{
		allocator_ptr = other.allocator_ptr;
	}

	template <typename U>
	memkind_allocator_wrapper(const memkind_allocator_wrapper<U> &other) noexcept
	{
		allocator_ptr = (allocator_type *)(other.allocator_ptr);
	}

	memkind_allocator_wrapper(memkind_allocator_wrapper &&other) = default;

	memkind_allocator_wrapper<T> &operator=(const memkind_allocator_wrapper &other)
	{
		if (this != &other) {
			allocator_ptr = other.allocator_ptr;
		}
		return *this;
	}

	memkind_allocator_wrapper<T> &
	operator=(memkind_allocator_wrapper &&other) = default;

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

private:
	allocator_type *allocator_ptr;
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
