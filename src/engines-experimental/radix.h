// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#pragma once

#include "../pmemobj_engine.h"
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <atomic>
#include <libpmemobj++/shared_mutex.hpp>

namespace pmem
{
namespace kv
{
namespace internal
{
namespace radix
{

/**
 * BASED ON: https://github.com/pmem/pmdk/blob/master/src/libpmemobj/critnib.h
 */

static constexpr std::size_t SLICE = 4;
static constexpr std::size_t NIB = ((1ULL << SLICE) - 1);
static constexpr std::size_t SLNODES = (1 << SLICE);

class tree {
public:
	tree();

	~tree();

	/*
	 * insert -- write a key:value pair to the critnib structure
	 */
	void insert(obj::pool_base &pop, uint64_t key, string_view value);

	/*
	 * get -- query for a key ("==" match)
	 */
	bool get(uint64_t key, pmemkv_get_v_callback *cb, void *arg);

	/*
	 * remove -- delete a key from the critnib structure, return its value
	 */
	bool remove(obj::pool_base &pop, uint64_t key);

	/*
	 * iterate -- iterate over all leafs
	 */
	void iterate(pmemkv_get_kv_callback *callback, void *arg);

	/*
	 * size -- return number of elements
	 */
	uint64_t size();

private:
	struct leaf;
	struct node;

	struct tagged_node_ptr {
		tagged_node_ptr();
		tagged_node_ptr(uint64_t off)
		{
			this->off = off;
		}
		tagged_node_ptr(const tagged_node_ptr &rhs);
		tagged_node_ptr(const obj::persistent_ptr<leaf> &ptr);
		tagged_node_ptr(const obj::persistent_ptr<node> &ptr);

		tagged_node_ptr(tagged_node_ptr &&rhs) = default;
		tagged_node_ptr &operator=(tagged_node_ptr &&rhs) = default;

		tagged_node_ptr &operator=(const tagged_node_ptr &rhs);

		tagged_node_ptr &operator=(std::nullptr_t);

		tagged_node_ptr &operator=(const obj::persistent_ptr<leaf> &rhs);
		tagged_node_ptr &operator=(const obj::persistent_ptr<node> &rhs);

		bool operator==(const tagged_node_ptr &rhs) const
		{
			return load_offset() == rhs.load_offset();
		}

		bool operator!=(const tagged_node_ptr &rhs) const
		{
			return !(*this == rhs);
		}

		bool is_leaf() const;

		tree::leaf *get_leaf(uint64_t) const;
		tree::node *get_node(uint64_t) const;

		tagged_node_ptr load() const
		{
			return off.load(std::memory_order_acquire);
		}

		void store(uint64_t val)
		{
			off.store(val, std::memory_order_release);
		}

		uint64_t load_offset() const
		{
			return off.load(std::memory_order_acquire);
		}

		explicit operator bool() const noexcept;

	private:
		std::atomic<uint64_t> off;
	};

	struct node {
		/*
		 * path is the part of a tree that's already traversed (be it through
		 * explicit nodes or collapsed links) -- ie, any subtree below has all
		 * those bits set to this value.
		 *
		 * nib is a 4-bit slice that's an index into the node's children.
		 *
		 * shift is the length (in bits) of the part of the key below this node.
		 *
		 *            nib
		 * |XXXXXXXXXX|?|*****|
		 *    path      ^
		 *              +-----+
		 *               shift
		 */
		obj::shared_mutex mtx;
		tagged_node_ptr child[SLNODES];
		obj::p<uint64_t> path;
		obj::p<uint8_t> shift;

		uint8_t padding[256 - sizeof(mtx) - sizeof(child) - sizeof(path) - sizeof(shift)];
	};

	static_assert(sizeof(node) == 256, "Wrong node size");

	struct leaf {
		leaf(uint64_t key, string_view value);

		const char *data() const noexcept;
		const char *cdata() const noexcept;

		std::size_t capacity();

		obj::p<uint64_t> key;
		obj::p<uint64_t> value_size;

	private:
		char *data();
	};

	obj::shared_mutex root_mtx;
	tagged_node_ptr root;
	obj::p<uint64_t> size_;
	uint64_t pool_id = 0;

	/*
	 * internal: path_mask -- return bit mask of a path above a subtree [shift]
	 * bits tall
	 */
	uint64_t path_mask(uint8_t shift);

	/*
	 * internal: slice_index -- return index of child at the given nib
	 */
	unsigned slice_index(uint64_t key, uint8_t shift);

	/*
	 * internal: delete_node -- recursively free (to malloc) a subtree
	 */
	void delete_node(tagged_node_ptr n);

	void iterate_rec(tagged_node_ptr n, pmemkv_get_kv_callback *callback, void *arg);
};

}
}

class radix : public pmemobj_engine_base<internal::radix::tree> {
public:
	radix(std::unique_ptr<internal::config> cfg);

	~radix();

	radix(const radix &) = delete;
	radix &operator=(const radix &) = delete;

	std::string name() final;

	status get_all(get_kv_callback *callback, void *arg) final;

	status count_all(std::size_t &cnt) final;

	status exists(string_view key) final;

	status get(string_view key, get_v_callback *callback, void *arg) final;

	status put(string_view key, string_view value) final;

	status remove(string_view key) final;

	// status defrag(double start_percent, double amount_percent) final;

private:
	uint64_t key_to_uint64(string_view v);

	internal::radix::tree *tree;
};

} /* namespace kv */
} /* namespace pmem */
