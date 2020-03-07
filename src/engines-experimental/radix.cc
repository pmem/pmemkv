// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "radix.h"
#include "../out.h"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/transaction.hpp>

#include <libpmemobj/action_base.h>

#include <mutex>
#include <vector>
#include <shared_mutex>

namespace pmem
{
namespace kv
{

namespace internal
{
namespace radix
{

// XXX - move to utils?
static inline uint64_t util_mssb_index64(std::size_t value)
{
	return ((unsigned char)(63 - __builtin_clzll(value)));
}

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

tree::tree()
{
	size_ = 0;
	pool_id = pmemobj_oid(this).pool_uuid_lo;
}

tree::~tree()
{
	if (root)
		delete_node(root);
}

uint64_t tree::size()
{
	return this->size_;
}

struct actions
{
	actions(obj::pool_base pop, uint64_t pool_id) : pop(pop), pool_id(pool_id)
	{
		acts.reserve(4);
	}

	void set(uint64_t *what, uint64_t how)
	{
		acts.emplace_back();
		pmemobj_set_value(pop.handle(), &acts.back(), what, how);
	}

	void free(uint64_t off)
	{
		acts.emplace_back();
		pmemobj_defer_free(pop.handle(), PMEMoid{pool_id, off}, &acts.back());
	}

	PMEMoid alloc(uint64_t size)
	{
		acts.emplace_back();
		return pmemobj_reserve(pop.handle(), &acts.back(), size, 0);
	}

	void publish()
	{
		/* XXX - this probably won't work if there is no operation on std::atomic */
		std::atomic_thread_fence(std::memory_order_release);

		if(pmemobj_publish(pop.handle(), acts.data(), acts.size()))
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
 * is always done under the parent lock. Going through inner nodes in lock-free manner is
 * always safe because they are not freed immediately - see remove description
 */
void tree::insert(obj::pool_base &pop, uint64_t key, string_view value)
{
restart : {
	actions acts(pop, pool_id);

	auto n = root.load();
	if (!n) {
		std::unique_lock<obj::shared_mutex> lock(root_mtx);

		if (n != root.load())
			goto restart;

		auto oid = acts.alloc(sizeof(tree::leaf) + value.size());
		new (pmemobj_direct(oid)) tree::leaf(key, value);

		// XXX - move size to TLS
		size_++;

		acts.set((uint64_t*)&root, oid.off | 1);
		acts.publish();

		return;
	}

	auto prev = root.load();
	auto *parent = &root;

	while (n && !n.is_leaf() &&
	       (key & path_mask(n.get_node(pool_id)->shift)) ==
		       n.get_node(pool_id)->path) {
		prev = n;
		parent = &n.get_node(pool_id)
				  ->child[slice_index(key, n.get_node(pool_id)->shift)];
		n = parent->load();
	}

	auto *mtx = parent == &root ? &root_mtx : &prev.get_node(pool_id)->mtx;
	std::unique_lock<obj::shared_mutex> lock(*mtx);

	/* this means that there was some concurrent insert (new node inserted at parent)
	 * or erase (in that case all child pointers are null) */
	if (n != parent->load())
		goto restart;

	if (!n) {
		auto oid = acts.alloc(sizeof(tree::leaf) + value.size());
		new (pmemobj_direct(oid)) tree::leaf(key, value);
		size_++;
		
		acts.set((uint64_t*)parent, oid.off | 1);
		acts.publish();

		return;
	}

	uint64_t path =
		n.is_leaf() ? n.get_leaf(pool_id)->key : n.get_node(pool_id)->path;
	/* Find where the path differs from our key. */
	uint64_t at = path ^ key;
	if (!at) {
		assert(n.is_leaf());

		auto oid = acts.alloc(sizeof(tree::leaf) + value.size());
		new (pmemobj_direct(oid)) tree::leaf(key, value);
		acts.set((uint64_t*)parent, oid.off | 1);
		acts.free(n.load_offset());

		acts.publish();

		return;
	}

	/* and convert that to an index. */
	uint8_t sh = util_mssb_index64(at) & (uint8_t) ~(SLICE - 1);

	auto node_oid = acts.alloc(sizeof(tree::node));
	auto node_ptr = (tree::node*) pmemobj_direct(node_oid);
	new(node_ptr) tree::node();

	auto leaf_oid = acts.alloc(sizeof(tree::leaf) + value.size());
	new (pmemobj_direct(leaf_oid)) tree::leaf(key, value);

	// if we would like to get rid of redo logs we could
	// just allocate both leaf and node (just ahead of time) in tls
	// at the beggining of this method an set all fields here
	node_ptr->child[slice_index(key, sh)] = leaf_oid.off | 1;
	node_ptr->child[slice_index(path, sh)] = n;
	node_ptr->shift = sh;
	node_ptr->path = key & path_mask(sh);
		
	acts.set((uint64_t*)parent, node_oid.off);

	// XXX - move size to TLS
	size_++;

	acts.publish();
}
}

bool tree::get(uint64_t key, pmemkv_get_v_callback *cb, void *arg)
{
restart: {
	auto n = root;
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
		n = parent->load();
	}

	if (!n)
		return false;

	auto &mtx = parent == &root ? root_mtx : prev.get_node(pool_id)->mtx;
	std::shared_lock<obj::shared_mutex> lock(mtx);

	if (n != parent->load())
		goto restart;

	if (n.get_leaf(pool_id)->key != key)
		return false;

	cb(n.get_leaf(pool_id)->cdata(), n.get_leaf(pool_id)->value_size, arg);

	return true;
}
}

bool tree::remove(obj::pool_base &pop, uint64_t key)
{
restart: {
	actions acts(pop, pool_id);

	auto n = root.load();
	if (!n)
		return false;

	if (n.is_leaf()) {
		std::unique_lock<obj::shared_mutex> lock(root_mtx);

		if (n != root.load())
			goto restart;

		if (n.get_leaf(pool_id)->key == key) {
				acts.free(n.load_offset());
				acts.set((uint64_t*)&root, 0);

				// XXX - move size to TLS
				size_--;

				acts.publish();

			return true;
		}

		return false;
	}

	/* prev, n and k are a parent:node:leaf at the end of this loop */
	auto *k_parent = &root;
	auto *n_parent = &root;
	auto kn = n;
	auto prev = kn;

	while (!kn.is_leaf()) {
		prev = n;

		n_parent = k_parent;
		n = kn;

		k_parent =
			&kn.get_node(pool_id)
				 ->child[slice_index(key, kn.get_node(pool_id)->shift)];
		kn = k_parent->load();

		if (!kn)
			return false;
	}

	auto mtx_parent = n_parent == &root ? &root_mtx : &prev.get_node(pool_id)->mtx;
	auto mtx_node = k_parent == &root ? &root_mtx : &n.get_node(pool_id)->mtx;

	std::shared_lock<obj::shared_mutex> lock_parent(*mtx_parent);
	std::shared_lock<obj::shared_mutex> lock_node;
	if (mtx_parent != mtx_node)
		lock_node = std::shared_lock<obj::shared_mutex>(*mtx_node);

	if (n != n_parent->load() || kn != k_parent->load())
		goto restart;

	if (kn.get_leaf(pool_id)->key != key)
		return false;

	acts.free(kn.load_offset());

	/* The child pointer is set to null even if parent node is being delted.
	 * Any threads which are waiting on that parent node will see this null
	 * and restart */
	acts.set((uint64_t*)k_parent, 0);

	auto only_child = [&] {
		int ochild = -1;
		for (int i = 0; i < (int)SLNODES; i++) {
			if (n.get_node(pool_id)->child[i] && &n.get_node(pool_id)->child[i] != k_parent) {
				if (ochild != -1)
					return -1;
				ochild = i;
			}
		}
		return ochild;
	}();

	/* Remove the node if there's only one remaining child. */
	if (only_child != -1) {
		acts.set((uint64_t*)n_parent, n.get_node(pool_id)->child[only_child].load_offset());
		// acts.free(n.load_offset()); // XXX - add to memory pool
	}

	// XXX - move size to TLS
	size_--;

	acts.publish();

	return true;
}
}

void tree::iterate_rec(tree::tagged_node_ptr n, pmemkv_get_kv_callback *callback,
		       void *arg)
{
	if (!n.is_leaf()) {
		for (int i = 0; i < (int)SLNODES; i++) {
			if (n.get_node(pool_id)->child[i])
				iterate_rec(n.get_node(pool_id)->child[i], callback,
					    arg); // XXX - get rid of recursion
		}
	} else {
		auto leaf = n.get_leaf(pool_id);
		callback((const char *)&leaf->key, sizeof(leaf->key), leaf->cdata(),
			 leaf->value_size, arg);
	}
}

void tree::iterate(pmemkv_get_kv_callback *callback, void *arg)
{
	if (root)
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
			if (n.get_node(pool_id)->child[i])
				delete_node(
					n.get_node(pool_id)
						->child[i]); // XXX - get rid of recursion
		}
		obj::delete_persistent<tree::node>(n.get_node(pool_id));
	} else {
		size_--;
		obj::delete_persistent<tree::leaf>(n.get_leaf(pool_id));
	}
}

tree::tagged_node_ptr::tagged_node_ptr()
{
	store(0);
}

tree::tagged_node_ptr::tagged_node_ptr(const obj::persistent_ptr<leaf> &ptr)
{
	store(ptr.raw().off | 1);
}

tree::tagged_node_ptr::tagged_node_ptr(const obj::persistent_ptr<node> &ptr)
{
	store(ptr.raw().off);
}

tree::tagged_node_ptr::tagged_node_ptr(const tree::tagged_node_ptr &rhs)
{
	store(rhs.load_offset());
}

tree::tagged_node_ptr &tree::tagged_node_ptr::operator=(const tree::tagged_node_ptr &rhs)
{
	store(rhs.load_offset());
	return *this;
}

tree::tagged_node_ptr &tree::tagged_node_ptr::operator=(std::nullptr_t)
{
	store(0);
	return *this;
}

tree::tagged_node_ptr &
tree::tagged_node_ptr::operator=(const obj::persistent_ptr<leaf> &rhs)
{
	store(rhs.raw().off | 1);
	return *this;
}

tree::tagged_node_ptr &
tree::tagged_node_ptr::operator=(const obj::persistent_ptr<node> &rhs)
{
	store(rhs.raw().off);
	return *this;
}

bool tree::tagged_node_ptr::is_leaf() const
{
	return load_offset() & 1;
}

tree::leaf *tree::tagged_node_ptr::get_leaf(uint64_t pool_id) const
{
	assert(is_leaf());
	return (tree::leaf *)pmemobj_direct(
		{pool_id, load_offset() & ~1ULL});
}

tree::node *tree::tagged_node_ptr::get_node(uint64_t pool_id) const
{
	assert(!is_leaf());
	return (tree::node *)pmemobj_direct(
		{pool_id, load_offset()});
}

tree::tagged_node_ptr::operator bool() const noexcept
{
	return load() != 0;
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
	auto found = tree->get(key_to_uint64(key), [](const char* v, size_t size, void *arg){}, nullptr);

	return found ? status::OK : status::NOT_FOUND;
}

status radix::get(string_view key, get_v_callback *callback, void *arg)
{
	auto found = tree->get(key_to_uint64(key), callback, arg);

	return found ? status::OK : status::NOT_FOUND;
}

status radix::put(string_view key, string_view value)
{
	tree->insert(pmpool, key_to_uint64(key), value);

	return status::OK;
}

status radix::remove(string_view key)
{
	return tree->remove(pmpool, key_to_uint64(key)) ? status::OK : status::NOT_FOUND;
}

// status defrag(double start_percent, double amount_percent) value_size;

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
