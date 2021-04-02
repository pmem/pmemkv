// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#ifndef LIBPMEMKV_TREE3_H
#define LIBPMEMKV_TREE3_H

#include "../pmemobj_engine.h"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/transaction.hpp>
#include <memory>
#include <vector>

using pmem::obj::delete_persistent;
using pmem::obj::make_persistent;
using pmem::obj::p;
using pmem::obj::persistent_ptr;
using pmem::obj::pool;
using pmem::obj::transaction;
using std::move;
using std::unique_ptr;
using std::vector;

namespace pmem
{
namespace kv
{
namespace internal
{
namespace tree3
{

#define INNER_KEYS 4				// maximum keys for inner nodes
#define INNER_KEYS_MIDPOINT (INNER_KEYS / 2)	// halfway point within the node
#define INNER_KEYS_UPPER ((INNER_KEYS / 2) + 1) // index where upper half of keys begins
#define LEAF_KEYS 48				// maximum keys in tree nodes
#define LEAF_KEYS_MIDPOINT (LEAF_KEYS / 2)	// halfway point within the node

class KVSlot {
public:
	uint8_t hash() const
	{
		return get_ph();
	}
	static uint8_t hash_direct(char *p)
	{
		return *((uint8_t *)(p + sizeof(uint32_t) + sizeof(uint32_t)));
	}
	const char *key() const
	{
		return ((char *)(kv.get()) + sizeof(uint8_t) + sizeof(uint32_t) +
			sizeof(uint32_t));
	}
	static const char *key_direct(char *p)
	{
		return (p + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t));
	}
	const uint32_t keysize() const
	{
		return get_ks();
	}
	static const uint32_t keysize_direct(char *p)
	{
		return *((uint32_t *)(p));
	}
	const char *val() const
	{
		return ((char *)(kv.get()) + sizeof(uint8_t) + sizeof(uint32_t) +
			sizeof(uint32_t) + get_ks() + 1);
	}
	static const char *val_direct(char *p)
	{
		return (p + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) +
			*((uint32_t *)(p)) + 1);
	}
	const uint32_t valsize() const
	{
		return get_vs();
	}
	static const uint32_t valsize_direct(char *p)
	{
		return *((uint32_t *)(p + sizeof(uint32_t)));
	}
	void clear();
	void set(const uint8_t hash, const std::string &key, const std::string &value);
	void set_ph(uint8_t v)
	{
		*((uint8_t *)((char *)(kv.get()) + sizeof(uint32_t) + sizeof(uint32_t))) =
			v;
	}
	static void set_ph_direct(char *p, uint8_t v)
	{
		*((uint8_t *)(p + sizeof(uint32_t) + sizeof(uint32_t))) = v;
	}
	void set_ks(uint32_t v)
	{
		*((uint32_t *)(kv.get())) = v;
	}
	static void set_ks_direct(char *p, uint32_t v)
	{
		*((uint32_t *)(p)) = v;
	}
	void set_vs(uint32_t v)
	{
		*((uint32_t *)((char *)(kv.get()) + sizeof(uint32_t))) = v;
	}
	static void set_vs_direct(char *p, uint32_t v)
	{
		*((uint32_t *)((char *)(p) + sizeof(uint32_t))) = v;
	}
	uint8_t get_ph() const
	{
		return *((uint8_t *)((char *)(kv.get()) + sizeof(uint32_t) +
				     sizeof(uint32_t)));
	}
	static uint8_t get_ph_direct(char *p)
	{
		return *((uint8_t *)((char *)(p) + sizeof(uint32_t) + sizeof(uint32_t)));
	}
	uint32_t get_ks() const
	{
		return *((uint32_t *)(kv.get()));
	}
	static uint32_t get_ks_direct(char *p)
	{
		return *((uint32_t *)(p));
	}
	uint32_t get_vs() const
	{
		return *((uint32_t *)((char *)(kv.get()) + sizeof(uint32_t)));
	}
	static uint32_t get_vs_direct(char *p)
	{
		return *((uint32_t *)((char *)(p) + sizeof(uint32_t)));
	}
	bool empty();

private:
	persistent_ptr<char[]> kv; // buffer for key & value
};

struct KVLeaf {
	p<KVSlot> slots[LEAF_KEYS];  // array of slot containers
	persistent_ptr<KVLeaf> next; // next leaf in unsorted list
};

struct KVInnerNode;

struct KVNode {		      // volatile nodes of the tree
	bool is_leaf = false; // indicate inner or leaf node
	KVInnerNode *parent;  // parent of this node (null if top)
	virtual ~KVNode() = default;
};

struct KVInnerNode final : KVNode {		     // volatile inner nodes of the tree
	uint8_t keycount;			     // count of keys in this node
	std::string keys[INNER_KEYS + 1];	     // child keys plus one overflow slot
	unique_ptr<KVNode> children[INNER_KEYS + 2]; // child nodes plus one overflow slot
	void assert_invariants();
};

struct KVLeafNode final : KVNode {   // volatile leaf nodes of the tree
	uint8_t hashes[LEAF_KEYS];   // Pearson hashes of keys
	std::string keys[LEAF_KEYS]; // keys stored in this leaf
	persistent_ptr<KVLeaf> leaf; // pointer to persistent leaf
};

struct KVRecoveredLeaf {		 // temporary wrapper used for recovery
	unique_ptr<KVLeafNode> leafnode; // leaf node being recovered
	std::string max_key;		 // highest sorting key present
};

} /* namespace tree3 */
} /* namespace internal */

class tree3
    : public pmemobj_engine_base<internal::tree3::KVLeaf> { // hybrid B+ tree engine
public:
	tree3(std::unique_ptr<internal::config> cfg);
	tree3(const tree3 &) = delete;
	tree3 &operator=(const tree3 &) = delete;
	~tree3();

	std::string name() final;

	status count_all(std::size_t &cnt) final;

	status get_all(get_kv_callback *callback, void *arg) final;

	status exists(string_view key) final;

	status get(string_view key, get_v_callback *callback, void *arg) final;

	status put(string_view key, string_view value) final;

	status remove(string_view key) final;

protected:
	internal::tree3::KVLeafNode *LeafSearch(const std::string &key);
	void LeafFillEmptySlot(internal::tree3::KVLeafNode *leafnode, uint8_t hash,
			       const std::string &key, const std::string &value);
	bool LeafFillSlotForKey(internal::tree3::KVLeafNode *leafnode, uint8_t hash,
				const std::string &key, const std::string &value);
	void LeafFillSpecificSlot(internal::tree3::KVLeafNode *leafnode, uint8_t hash,
				  const std::string &key, const std::string &value,
				  int slot);
	void LeafSplitFull(internal::tree3::KVLeafNode *leafnode, uint8_t hash,
			   const std::string &key, const std::string &value);
	void InnerUpdateAfterSplit(internal::tree3::KVNode *node,
				   unique_ptr<internal::tree3::KVNode> newnode,
				   std::string *split_key);
	uint8_t PearsonHash(const char *data, size_t size);
	void Recover();

private:
	vector<persistent_ptr<internal::tree3::KVLeaf>>
		leaves_prealloc;		      // persisted but unused leaves
	unique_ptr<internal::tree3::KVNode> tree_top; // pointer to uppermost inner node
};

class tree3_factory : public engine_base::factory_base {
public:
	unique_ptr<engine_base> create(unique_ptr<internal::config> cfg) override
	{
		check_config_null(get_name(), cfg);
		return unique_ptr<engine_base>(new tree3(move(cfg)));
	};
	std::string get_name() override
	{
		return "tree3";
	};
};

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_TREE3_H */
