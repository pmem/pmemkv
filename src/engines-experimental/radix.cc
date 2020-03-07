// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "radix.h"
#include "../out.h"
#include "../utils.h"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/transaction.hpp>

#include <libpmemobj/action_base.h>

#include <mutex>
#include <shared_mutex>
#include <vector>

namespace pmem
{
namespace kv
{

namespace internal
{
namespace radix
{

tree::leaf::leaf(uint64_t key, string_view value) : key(key), value_size(value.size())
{
	std::memcpy(this->data(), value.data(), value.size());
}

const char *tree::leaf::data() const noexcept
{
	return reinterpret_cast<const char *>(this + 1);
}

const char *tree::leaf::cdata() const noexcept
{
	return reinterpret_cast<const char *>(this + 1);
}

std::size_t tree::leaf::capacity()
{
	return pmemobj_alloc_usable_size(pmemobj_oid(this)) - sizeof(this);
}

char *tree::leaf::data()
{
	auto *ptr = reinterpret_cast<char *>(this + 1);

	return ptr;
}

tree::tree() : root(0), size_(0)
{
	pool_id = pmemobj_oid(this).pool_uuid_lo;
}

tree::~tree()
{
	if (root.load(std::memory_order_acquire))
		delete_node(root);
}

uint64_t tree::size()
{
	return this->size_;
}

struct actions {
	actions(obj::pool_base pop, uint64_t pool_id, std::size_t cap = 4)
	    : pop(pop), pool_id(pool_id)
	{
		acts.reserve(cap);
	}

	void set(uint64_t *what, uint64_t value)
	{
		acts.emplace_back();
		pmemobj_set_value(pop.handle(), &acts.back(), what, value);
	}

	void free(uint64_t off)
	{
		acts.emplace_back();
		pmemobj_defer_free(pop.handle(), PMEMoid{pool_id, off}, &acts.back());
	}

	template <typename T, typename... Args>
	obj::persistent_ptr<T> make(uint64_t size, Args &&... args)
	{
		acts.emplace_back();
		obj::persistent_ptr<T> ptr =
			pmemobj_reserve(pop.handle(), &acts.back(), size, 0);

		new (ptr.get()) T(std::forward<Args>(args)...);

		return ptr;
	}

	void publish()
	{
		/* XXX - this probably won't work if there is no operation on std::atomic
		 */
		std::atomic_thread_fence(std::memory_order_release);

		if (pmemobj_publish(pop.handle(), acts.data(), acts.size()))
			throw std::runtime_error("XXX");
	}

	void cancel()
	{
		pmemobj_cancel(pop.handle(), acts.data(), acts.size());
	}

private:
	std::vector<pobj_action> acts;
	obj::pool_base pop;
	uint64_t pool_id;
};

/**
 * We first find a position for a new leaf without any locks, then lock the
 * potential parent and insert/replace the leaf. Reading key from leafs
 * is always done under the parent lock.
 */
void tree::insert(obj::pool_base &pop, uint64_t key, string_view value)
{
restart : {
	actions acts(pop, pool_id);

	auto n = root.load(std::memory_order_acquire);
	if (!n) {
		std::unique_lock<obj::shared_mutex> lock(root_mtx);

		if (n != root.load(std::memory_order_relaxed))
			goto restart;

		tagged_node_ptr leaf = acts.make<tree::leaf>(
			sizeof(tree::leaf) + value.size(), key, value);

		// XXX - move size to TLS
		size_++;

		acts.set((uint64_t *)&root, leaf.offset());
		acts.publish();

		return;
	}

	auto prev = root.load(std::memory_order_acquire);
	auto *parent = &root;

	while (n && !n.is_leaf() &&
	       (key & path_mask(n.get_node(pool_id)->shift)) ==
		       n.get_node(pool_id)->path) {
		prev = n;
		parent = &n.get_node(pool_id)
				  ->child[slice_index(key, n.get_node(pool_id)->shift)];
		n = parent->load(std::memory_order_acquire);
	}

	auto &mtx = parent == &root ? root_mtx : prev.get_node(pool_id)->mtx;
	std::unique_lock<obj::shared_mutex> lock(mtx);

	/* this means that there was some concurrent insert */
	if (n != parent->load(std::memory_order_relaxed))
		goto restart;

	if (!n) {
		tagged_node_ptr leaf = acts.make<tree::leaf>(
			sizeof(tree::leaf) + value.size(), key, value);
		size_++;

		acts.set((uint64_t *)parent, leaf.offset());
		acts.publish();

		return;
	}

	uint64_t path =
		n.is_leaf() ? n.get_leaf(pool_id)->key : n.get_node(pool_id)->path;

	/* Find where the path differs from our key. */
	uint64_t at = path ^ key;
	if (!at) {
		assert(n.is_leaf());

		tagged_node_ptr leaf = acts.make<tree::leaf>(
			sizeof(tree::leaf) + value.size(), key, value);
		acts.set((uint64_t *)parent, leaf.offset());
		acts.free(n.offset());

		acts.publish();

		return;
	}

	/* and convert that to an index. */
	uint8_t sh = utils::mssb_index64(at) & (uint8_t) ~(SLICE - 1);

	tagged_node_ptr node = acts.make<tree::node>(sizeof(tree::node));
	tagged_node_ptr leaf =
		acts.make<tree::leaf>(sizeof(tree::leaf) + value.size(), key, value);

	auto node_ptr = node.get_node(pool_id);

	node_ptr->child[slice_index(key, sh)].store(leaf, std::memory_order_relaxed);
	node_ptr->child[slice_index(path, sh)].store(n, std::memory_order_relaxed);
	node_ptr->shift = sh;
	node_ptr->path = key & path_mask(sh);

	acts.set((uint64_t *)parent, node.offset());

	// XXX - move size to TLS
	size_++;

	acts.publish();
}
}

bool tree::get(uint64_t key, pmemkv_get_v_callback *cb, void *arg)
{
restart : {
	auto n = root.load(std::memory_order_acquire);
	auto prev = n;
	auto parent = &root;

	/*
	 * critbit algorithm: dive into the tree, looking at nothing but
	 * each node's critical bit^H^H^Hnibble.  This means we risk
	 * going wrong way if our path is missing, but that's ok...
	 */
	while (n && !n.is_leaf()) {
		prev = n;
		parent = &n.get_node(pool_id)
				  ->child[slice_index(key, n.get_node(pool_id)->shift)];
		n = parent->load(std::memory_order_acquire);
	}

	auto &mtx = parent == &root ? root_mtx : prev.get_node(pool_id)->mtx;
	std::shared_lock<obj::shared_mutex> lock(mtx);

	/* Concurrent erase could have inserted extra node at leaf position */
	if (n != parent->load(std::memory_order_relaxed))
		goto restart;

	if (!n || n.get_leaf(pool_id)->key != key)
		return false;

	cb(n.get_leaf(pool_id)->cdata(), n.get_leaf(pool_id)->value_size, arg);

	return true;
}
}

bool tree::remove(obj::pool_base &pop, uint64_t key)
{
	actions acts(pop, pool_id);

	auto p_node = root.load(std::memory_order_acquire);
	if (!p_node)
		return false;

	if (p_node.is_leaf()) {
		if (p_node.get_leaf(pool_id)->key == key) {
			acts.free(p_node.offset());
			acts.set((uint64_t *)&root, 0);

			// XXX - move size to TLS
			size_--;

			acts.publish();

			return true;
		}

		return false;
	}

	auto *leaf_parent = &root;
	auto *node_parent = &root;
	auto leaf = p_node;

	while (leaf && !leaf.is_leaf()) {
		node_parent = leaf_parent;
		p_node = leaf;

		leaf_parent =
			&leaf.get_node(pool_id)
				 ->child[slice_index(key, leaf.get_node(pool_id)->shift)];
		leaf = leaf_parent->load(std::memory_order_acquire);
	}

	if (!leaf || leaf.get_leaf(pool_id)->key != key)
		return false;

	acts.free(leaf.offset());
	acts.set((uint64_t *)leaf_parent, 0);

	auto only_child = [&] {
		int ochild = -1;
		for (int i = 0; i < (int)SLNODES; i++) {
			auto &child = p_node.get_node(pool_id)->child[i];

			if (child.load(std::memory_order_relaxed) &&
			    &child != leaf_parent) {
				if (ochild != -1)
					return -1;
				ochild = i;
			}
		}
		return ochild;
	}();

	/* Remove the node if there's only one remaining child. */
	if (only_child != -1) {
		acts.set((uint64_t *)node_parent,
			 p_node.get_node(pool_id)
				 ->child[only_child]
				 .load(std::memory_order_relaxed)
				 .offset());
		acts.free(p_node.offset());
	}

	// XXX - move size to TLS
	size_--;

	acts.publish();

	return true;
}

void tree::iterate_rec(tree::tagged_node_ptr n, pmemkv_get_kv_callback *callback,
		       void *arg)
{
	if (!n.is_leaf()) {
		auto &mtx = n == root.load(std::memory_order_acquire)
			? root_mtx
			: n.get_node(pool_id)->mtx;

		/*
		 * Keep locks on every level from root to leaf's parent. This simplifies
		 * synchronization with concurrent inserts.
		 */
		std::shared_lock<obj::shared_mutex> lock(mtx);

		for (int i = 0; i < (int)SLNODES; i++) {
			auto child = n.get_node(pool_id)->child[i].load(
				std::memory_order_relaxed);
			if (child)
				iterate_rec(child, callback, arg);
		}
	} else {
		auto leaf = n.get_leaf(pool_id);
		callback((const char *)&leaf->key, sizeof(leaf->key), leaf->cdata(),
			 leaf->value_size, arg);
	}
}

void tree::iterate(pmemkv_get_kv_callback *callback, void *arg)
{
	if (root.load(std::memory_order_acquire))
		iterate_rec(root, callback, arg);
}

uint64_t tree::path_mask(uint8_t shift)
{
	return ~NIB << shift;
}

unsigned tree::slice_index(uint64_t key, uint8_t shift)
{
	return (unsigned)((key >> shift) & NIB);
}

void tree::delete_node(tree::tagged_node_ptr n)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	if (!n.is_leaf()) {
		for (int i = 0; i < (int)SLNODES; i++) {
			if (n.get_node(pool_id)->child[i].load(std::memory_order_relaxed))
				delete_node(n.get_node(pool_id)->child[i]);
		}
		obj::delete_persistent<tree::node>(n.get_node(pool_id));
	} else {
		size_--;
		obj::delete_persistent<tree::leaf>(n.get_leaf(pool_id));
	}
}

tree::tagged_node_ptr::tagged_node_ptr(uint64_t off)
{
	this->off = off;
}

tree::tagged_node_ptr::tagged_node_ptr(const obj::persistent_ptr<leaf> &ptr)
{
	off = (ptr.raw().off | 1);
}

tree::tagged_node_ptr::tagged_node_ptr(const obj::persistent_ptr<node> &ptr)
{
	off = ptr.raw().off;
}

tree::tagged_node_ptr &tree::tagged_node_ptr::operator=(std::nullptr_t)
{
	off = 0;
	return *this;
}

tree::tagged_node_ptr &
tree::tagged_node_ptr::operator=(const obj::persistent_ptr<leaf> &rhs)
{
	off = (rhs.raw().off | 1);
	return *this;
}

tree::tagged_node_ptr &
tree::tagged_node_ptr::operator=(const obj::persistent_ptr<node> &rhs)
{
	off = rhs.raw().off;
	return *this;
}

bool tree::tagged_node_ptr::operator==(const tree::tagged_node_ptr &rhs) const
{
	return off == rhs.off;
}

bool tree::tagged_node_ptr::operator!=(const tree::tagged_node_ptr &rhs) const
{
	return !(*this == rhs);
}

bool tree::tagged_node_ptr::is_leaf() const
{
	return off & 1;
}

uint64_t tree::tagged_node_ptr::offset() const
{
	return off;
}

tree::leaf *tree::tagged_node_ptr::get_leaf(uint64_t pool_id) const
{
	assert(is_leaf());
	return (tree::leaf *)pmemobj_direct({pool_id, off & ~1ULL});
}

tree::node *tree::tagged_node_ptr::get_node(uint64_t pool_id) const
{
	assert(!is_leaf());
	return (tree::node *)pmemobj_direct({pool_id, off});
}

tree::tagged_node_ptr::operator bool() const noexcept
{
	return off != 0;
}

} // namespace radix
} // namespace internal

radix::radix(std::unique_ptr<internal::config> cfg) : pmemobj_engine_base(cfg)
{
	if (!OID_IS_NULL(*root_oid)) {
		tree = (pmem::kv::internal::radix::tree *)pmemobj_direct(*root_oid);
	} else {
		pmem::obj::transaction::run(pmpool, [&] {
			pmem::obj::transaction::snapshot(root_oid);
			*root_oid =
				pmem::obj::make_persistent<internal::radix::tree>().raw();
			tree = (pmem::kv::internal::radix::tree *)pmemobj_direct(
				*root_oid);
		});
	}
}

radix::~radix()
{
}

std::string radix::name()
{
	return "radix";
}

status radix::get_all(get_kv_callback *callback, void *arg)
{
	std::shared_lock<lock_type> lock(mtx);

	tree->iterate(callback, arg);

	return status::OK;
}

status radix::count_all(std::size_t &cnt)
{
	cnt = tree->size();

	return status::OK;
}

status radix::exists(string_view key)
{
	std::shared_lock<lock_type> lock(mtx);

	auto found = tree->get(
		key_to_uint64(key), [](const char *v, size_t size, void *arg) {},
		nullptr);

	return found ? status::OK : status::NOT_FOUND;
}

status radix::get(string_view key, get_v_callback *callback, void *arg)
{
	std::shared_lock<lock_type> lock(mtx);

	auto found = tree->get(key_to_uint64(key), callback, arg);

	return found ? status::OK : status::NOT_FOUND;
}

status radix::put(string_view key, string_view value)
{
	std::shared_lock<lock_type> lock(mtx);

	tree->insert(pmpool, key_to_uint64(key), value);

	return status::OK;
}

status radix::remove(string_view key)
{
	std::unique_lock<lock_type> lock(mtx);

	return tree->remove(pmpool, key_to_uint64(key)) ? status::OK : status::NOT_FOUND;
}

uint64_t radix::key_to_uint64(string_view v)
{
	if (v.size() > sizeof(uint64_t))
		throw internal::invalid_argument("Key length must be <= 8");

	uint64_t val = 0;
	memcpy(&val, v.data(), v.size());

	return val;
}

} // namespace kv
} // namespace pmem
