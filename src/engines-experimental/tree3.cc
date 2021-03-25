// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#include "tree3.h"
#include "../out.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <list>
#include <unistd.h>

namespace pmem
{
namespace kv
{

tree3::tree3(std::unique_ptr<internal::config> cfg)
    : pmemobj_engine_base(cfg, "pmemkv_tree3")
{
	Recover();
	LOG("Started ok");
}

tree3::~tree3()
{
	LOG("Stopped ok");
}

std::string tree3::name()
{
	return "tree3";
}

// ===============================================================================================
// KEY/VALUE METHODS
// ===============================================================================================

status tree3::count_all(std::size_t &cnt)
{
	LOG("count_all");
	check_outside_tx();
	std::size_t result = 0;
	auto leaf = (pmem::kv::internal::tree3::KVLeaf *)pmemobj_direct(*root_oid);
	while (leaf) {
		for (int slot = LEAF_KEYS; slot--;) {
			auto kvslot = leaf->slots[slot].get_ro();
			if (kvslot.empty() || kvslot.hash() == 0)
				continue;
			result++;
		}
		leaf = leaf->next.get(); // advance to next linked leaf
	}

	cnt = result;

	return status::OK;
}

status tree3::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	check_outside_tx();
	auto leaf = (pmem::kv::internal::tree3::KVLeaf *)pmemobj_direct(*root_oid);
	while (leaf) {
		for (int slot = LEAF_KEYS; slot--;) {
			auto kvslot = leaf->slots[slot].get_ro();
			if (kvslot.empty() || kvslot.hash() == 0)
				continue;
			auto ret = callback(kvslot.key(), kvslot.get_ks(), kvslot.val(),
					    kvslot.get_vs(), arg);
			if (ret != 0)
				return status::STOPPED_BY_CB;
		}
		leaf = leaf->next.get(); // advance to next linked leaf
	}

	return status::OK;
}

status tree3::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	check_outside_tx();
	// XXX - do not create temporary string
	auto leafnode = LeafSearch(std::string(key.data(), key.size()));
	if (leafnode) {
		const uint8_t hash = PearsonHash(key.data(), key.size());
		for (int slot = LEAF_KEYS; slot--;) {
			if (leafnode->hashes[slot] == hash) {
				if (leafnode->keys[slot].compare(
					    std::string(key.data(), key.size())) == 0)
					return status::OK;
			}
		}
	}
	LOG("   could not find key");
	return status::NOT_FOUND;
}

status tree3::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get using callback for key=" << std::string(key.data(), key.size()));
	check_outside_tx();
	// XXX - do not create temporary string
	auto leafnode = LeafSearch(std::string(key.data(), key.size()));
	if (leafnode) {
		const uint8_t hash = PearsonHash(key.data(), key.size());
		for (int slot = LEAF_KEYS; slot--;) {
			if (leafnode->hashes[slot] == hash) {
				LOG("   found hash match, slot=" << slot);
				if (leafnode->keys[slot].compare(
					    std::string(key.data(), key.size())) == 0) {
					auto kv = leafnode->leaf->slots[slot].get_ro();
					LOG("   found value, slot="
					    << slot
					    << ", size=" << std::to_string(kv.valsize()));
					callback(kv.val(), kv.valsize(), arg);
					return status::OK;
				}
			}
		}
	}
	LOG("   could not find key");
	return status::NOT_FOUND;
}

status tree3::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));
	check_outside_tx();

	const auto hash = PearsonHash(key.data(), key.size());
	// XXX - do not create temporary string
	auto leafnode = LeafSearch(std::string(key.data(), key.size()));
	if (!leafnode) {
		LOG("   adding head leaf");
		unique_ptr<internal::tree3::KVLeafNode> new_node(
			new internal::tree3::KVLeafNode());
		new_node->is_leaf = true;
		transaction::run(pmpool, [&] {
			if (!leaves_prealloc.empty()) {
				new_node->leaf = leaves_prealloc.back();
				leaves_prealloc.pop_back();
			} else {
				auto old_head = persistent_ptr<internal::tree3::KVLeaf>(
					*root_oid);
				auto new_leaf =
					make_persistent<internal::tree3::KVLeaf>();
				transaction::snapshot(root_oid);
				*root_oid = new_leaf.raw();
				new_leaf->next = old_head;
				new_node->leaf = new_leaf;
			}
			LeafFillSpecificSlot(new_node.get(), hash,
					     std::string(key.data(), key.size()),
					     std::string(value.data(), value.size()), 0);
		});
		tree_top = move(new_node);
	} else if (LeafFillSlotForKey(leafnode, hash, std::string(key.data(), key.size()),
				      std::string(value.data(), value.size()))) {
		// nothing else to do
	} else {
		// XXX - do not create temporary string
		LeafSplitFull(leafnode, hash, std::string(key.data(), key.size()),
			      std::string(value.data(), value.size()));
	}
	return status::OK;
}

status tree3::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));
	check_outside_tx();

	// XXX - do not create temporary string
	auto leafnode = LeafSearch(std::string(key.data(), key.size()));
	if (!leafnode) {
		LOG("   head not present");
		return status::NOT_FOUND;
	}

	const auto hash = PearsonHash(key.data(), key.size());
	for (int slot = LEAF_KEYS; slot--;) {
		if (leafnode->hashes[slot] == hash) {
			if (leafnode->keys[slot].compare(
				    std::string(key.data(), key.size())) == 0) {
				LOG("   freeing slot=" << slot);
				leafnode->hashes[slot] = 0;
				leafnode->keys[slot].clear();
				auto leaf = leafnode->leaf;
				transaction::run(pmpool, [&] {
					leaf->slots[slot].get_rw().clear();
				});
				return status::OK; // no duplicate keys allowed
			}
		}
	}
	return status::NOT_FOUND;
}

// ===============================================================================================
// PROTECTED LEAF METHODS
// ===============================================================================================

internal::tree3::KVLeafNode *tree3::LeafSearch(const std::string &key)
{
	internal::tree3::KVNode *node = tree_top.get();
	if (node == nullptr)
		return nullptr;
	bool matched;
	while (!node->is_leaf) {
		matched = false;
		auto inner = (internal::tree3::KVInnerNode *)node;
#ifndef NDEBUG
		inner->assert_invariants();
#endif
		const uint8_t keycount = inner->keycount;
		for (uint8_t idx = 0; idx < keycount; idx++) {
			node = inner->children[idx].get();
			if (key.compare(inner->keys[idx]) <= 0) {
				matched = true;
				break;
			}
		}
		if (!matched)
			node = inner->children[keycount].get();
	}
	return (internal::tree3::KVLeafNode *)node;
}

void tree3::LeafFillEmptySlot(internal::tree3::KVLeafNode *leafnode, const uint8_t hash,
			      const std::string &key, const std::string &value)
{
	for (int slot = LEAF_KEYS; slot--;) {
		if (leafnode->hashes[slot] == 0) {
			LeafFillSpecificSlot(leafnode, hash, key, value, slot);
			return;
		}
	}
}

bool tree3::LeafFillSlotForKey(internal::tree3::KVLeafNode *leafnode, const uint8_t hash,
			       const std::string &key, const std::string &value)
{
	// scan for empty/matching slots
	int last_empty_slot = -1;
	int key_match_slot = -1;
	for (int slot = LEAF_KEYS; slot--;) {
		auto slot_hash = leafnode->hashes[slot];
		if (slot_hash == 0) {
			last_empty_slot = slot;
		} else if (slot_hash == hash) {
			if (leafnode->keys[slot].compare(key) == 0) {
				key_match_slot = slot;
				break; // no duplicate keys allowed
			}
		}
	}

	// update suitable slot if found
	int slot = key_match_slot >= 0 ? key_match_slot : last_empty_slot;
	if (slot >= 0) {
		LOG("   filling slot=" << slot);
		transaction::run(pmpool, [&] {
			LeafFillSpecificSlot(leafnode, hash, key, value, slot);
		});
	}
	return slot >= 0;
}

void tree3::LeafFillSpecificSlot(internal::tree3::KVLeafNode *leafnode,
				 const uint8_t hash, const std::string &key,
				 const std::string &value, const int slot)
{
	leafnode->leaf->slots[slot].get_rw().set(hash, key, value);
	leafnode->hashes[slot] = hash;
	leafnode->keys[slot] = key;
}

void tree3::LeafSplitFull(internal::tree3::KVLeafNode *leafnode, const uint8_t hash,
			  const std::string &key, const std::string &value)
{
	std::string keys[LEAF_KEYS + 1];
	keys[LEAF_KEYS] = key;
	for (int slot = LEAF_KEYS; slot--;)
		keys[slot] = leafnode->keys[slot];
	std::sort(std::begin(keys), std::end(keys),
		  [](const std::string &lhs, const std::string &rhs) {
			  return lhs.compare(rhs) < 0;
		  });
	std::string split_key = keys[LEAF_KEYS_MIDPOINT];
	LOG("   splitting leaf at key=" << split_key);

	// split leaf into two leaves, moving slots that sort above split key to new leaf
	unique_ptr<internal::tree3::KVLeafNode> new_leafnode(
		new internal::tree3::KVLeafNode());
	new_leafnode->parent = leafnode->parent;
	new_leafnode->is_leaf = true;
	transaction::run(pmpool, [&] {
		persistent_ptr<internal::tree3::KVLeaf> new_leaf;
		if (!leaves_prealloc.empty()) {
			new_leaf = leaves_prealloc.back();
			new_leafnode->leaf = new_leaf;
			leaves_prealloc.pop_back();
		} else {
			auto old_head =
				persistent_ptr<internal::tree3::KVLeaf>(*root_oid);
			new_leaf = make_persistent<internal::tree3::KVLeaf>();
			transaction::snapshot(root_oid);
			*root_oid = new_leaf.raw();
			new_leaf->next = old_head;
			new_leafnode->leaf = new_leaf;
		}
		for (int slot = LEAF_KEYS; slot--;) {
			if (leafnode->keys[slot].compare(split_key) > 0) {
				new_leaf->slots[slot].swap(leafnode->leaf->slots[slot]);
				new_leafnode->hashes[slot] = leafnode->hashes[slot];
				new_leafnode->keys[slot] = leafnode->keys[slot];
				leafnode->hashes[slot] = 0;
				leafnode->keys[slot].clear();
			}
		}
		auto target = key.compare(split_key) > 0 ? new_leafnode.get() : leafnode;
		LeafFillEmptySlot(target, hash, key, value);
	});

	// recursively update volatile parents outside persistent transaction
	InnerUpdateAfterSplit(leafnode, move(new_leafnode), &split_key);
}

void tree3::InnerUpdateAfterSplit(internal::tree3::KVNode *node,
				  unique_ptr<internal::tree3::KVNode> new_node,
				  std::string *split_key)
{
	if (!node->parent) {
		assert(node == tree_top.get());
		LOG("   creating new top node for split_key=" << *split_key);
		unique_ptr<internal::tree3::KVInnerNode> top(
			new internal::tree3::KVInnerNode());
		top->keycount = 1;
		top->keys[0] = *split_key;
		node->parent = top.get();
		new_node->parent = top.get();
		top->children[0] = move(tree_top);
		top->children[1] = move(new_node);
#ifndef NDEBUG
		top->assert_invariants();
#endif
		tree_top = move(top); // assign new top node
		return;		      // end recursion
	}

	LOG("   updating parents for split_key=" << *split_key);
	internal::tree3::KVInnerNode *inner = node->parent;
	{ // insert split_key and new_node into inner node in sorted order
		const uint8_t keycount = inner->keycount;
		int idx = 0; // position where split_key should be inserted
		while (idx < keycount && inner->keys[idx].compare(*split_key) <= 0)
			idx++;
		for (int i = keycount - 1; i >= idx; i--)
			inner->keys[i + 1] = move(inner->keys[i]);
		for (int i = keycount; i > idx; i--)
			inner->children[i + 1] = move(inner->children[i]);
		inner->keys[idx] = *split_key;
		inner->children[idx + 1] = move(new_node);
		inner->keycount = (uint8_t)(keycount + 1);
	}
	const uint8_t keycount = inner->keycount;
	if (keycount <= INNER_KEYS) {
#ifndef NDEBUG
		inner->assert_invariants();
#endif
		return; // end recursion
	}

	// split inner node at the midpoint, update parents as needed
	unique_ptr<internal::tree3::KVInnerNode> ni(
		new internal::tree3::KVInnerNode());	    // create new inner node
	ni->parent = inner->parent;			    // set parent reference
	for (int i = INNER_KEYS_UPPER; i < keycount; i++) { // move all upper keys
		ni->keys[i - INNER_KEYS_UPPER] = move(inner->keys[i]); // move key string
	}
	for (int i = INNER_KEYS_UPPER; i < keycount + 1; i++) { // move all upper children
		ni->children[i - INNER_KEYS_UPPER] =
			move(inner->children[i]); // move child reference
		ni->children[i - INNER_KEYS_UPPER]->parent =
			ni.get(); // set parent reference
	}
	ni->keycount = INNER_KEYS_MIDPOINT; // always half the keys
	std::string new_split_key =
		inner->keys[INNER_KEYS_MIDPOINT]; // save for recursion
	inner->keycount = INNER_KEYS_MIDPOINT;	  // half of keys remain

	// perform deep check on modified inner nodes
#ifndef NDEBUG
	inner->assert_invariants(); // check node just split
	ni->assert_invariants();    // check new node
#endif

	InnerUpdateAfterSplit(inner, move(ni), &new_split_key); // recursive update
}

// ===============================================================================================
// PROTECTED LIFECYCLE METHODS
// ===============================================================================================

void tree3::Recover()
{
	LOG("Recovering");

	// traverse persistent leaves to build list of leaves to recover
	std::list<internal::tree3::KVRecoveredLeaf> leaves;

	auto root_leaf = persistent_ptr<internal::tree3::KVLeaf>(*root_oid);

	while (root_leaf) {
		unique_ptr<internal::tree3::KVLeafNode> leafnode(
			new internal::tree3::KVLeafNode());
		leafnode->leaf = root_leaf;
		leafnode->is_leaf = true;

		// find highest sorting key in leaf, while recovering all hashes
		bool empty_leaf = true;
		std::string max_key;
		for (int slot = LEAF_KEYS; slot--;) {
			auto kvslot = root_leaf->slots[slot].get_ro();
			if (kvslot.empty())
				continue;
			leafnode->hashes[slot] = kvslot.hash();
			if (leafnode->hashes[slot] == 0)
				continue;
			const char *key = kvslot.key();
			if (empty_leaf) {
				max_key = std::string(kvslot.key(), kvslot.get_ks());
				empty_leaf = false;
			} else if (max_key.compare(0, std::string::npos, kvslot.key(),
						   kvslot.get_ks()) < 0) {
				max_key = std::string(kvslot.key(), kvslot.get_ks());
			}
			leafnode->keys[slot] = std::string(key, kvslot.get_ks());
		}

		// use highest sorting key to decide how to recover the leaf
		if (empty_leaf) {
			leaves_prealloc.push_back(root_leaf);
		} else {
			leaves.push_back({move(leafnode), max_key});
		}

		root_leaf = root_leaf->next.get(); // advance to next linked leaf
	}

	// sort recovered leaves in ascending key order
	leaves.sort([](const internal::tree3::KVRecoveredLeaf &lhs,
		       const internal::tree3::KVRecoveredLeaf &rhs) {
		return (lhs.max_key.compare(rhs.max_key) < 0);
	});

	// reconstruct top/inner nodes using adjacent pairs of recovered leaves
	tree_top.reset(nullptr);

	if (!leaves.empty()) {
		tree_top = move(leaves.front().leafnode);
		auto max_key = leaves.front().max_key;
		leaves.pop_front();

		auto prevnode = tree_top.get();
		while (!leaves.empty()) {
			std::string split_key = std::string(max_key);
			auto nextnode = leaves.front().leafnode.get();
			nextnode->parent = prevnode->parent;
			InnerUpdateAfterSplit(prevnode, move(leaves.front().leafnode),
					      &split_key);
			max_key = leaves.front().max_key;
			leaves.pop_front();
			prevnode = nextnode;
		}
	}

	LOG("Recovered ok");
}

// ===============================================================================================
// PEARSON HASH METHODS
// ===============================================================================================

// Pearson hashing lookup table from RFC 3074
const uint8_t PEARSON_LOOKUP_TABLE[256] = {
	251, 175, 119, 215, 81,	 14,  79,  191, 103, 49,  181, 143, 186, 157, 0,   232,
	31,  32,  55,  60,  152, 58,  17,  237, 174, 70,  160, 144, 220, 90,  57,  223,
	59,  3,	  18,  140, 111, 166, 203, 196, 134, 243, 124, 95,  222, 179, 197, 65,
	180, 48,  36,  15,  107, 46,  233, 130, 165, 30,  123, 161, 209, 23,  97,  16,
	40,  91,  219, 61,  100, 10,  210, 109, 250, 127, 22,  138, 29,	 108, 244, 67,
	207, 9,	  178, 204, 74,	 98,  126, 249, 167, 116, 34,  77,  193, 200, 121, 5,
	20,  113, 71,  35,  128, 13,  182, 94,	25,  226, 227, 199, 75,	 27,  41,  245,
	230, 224, 43,  225, 177, 26,  155, 150, 212, 142, 218, 115, 241, 73,  88,  105,
	39,  114, 62,  255, 192, 201, 145, 214, 168, 158, 221, 148, 154, 122, 12,  84,
	82,  163, 44,  139, 228, 236, 205, 242, 217, 11,  187, 146, 159, 64,  86,  239,
	195, 42,  106, 198, 118, 112, 184, 172, 87,  2,	  173, 117, 176, 229, 247, 253,
	137, 185, 99,  164, 102, 147, 45,  66,	231, 52,  141, 211, 194, 206, 246, 238,
	56,  110, 78,  248, 63,	 240, 189, 93,	92,  51,  53,  183, 19,	 171, 72,  50,
	33,  104, 101, 69,  8,	 252, 83,  120, 76,  135, 85,  54,  202, 125, 188, 213,
	96,  235, 136, 208, 162, 129, 190, 132, 156, 38,  47,  1,   7,	 254, 24,  4,
	216, 131, 89,  21,  28,	 133, 37,  153, 149, 80,  170, 68,  6,	 169, 234, 151};

// Modified Pearson hashing algorithm from RFC 3074
uint8_t tree3::PearsonHash(const char *data, const size_t size)
{
	auto hash = (uint8_t)size;
	for (size_t i = size; i > 0;) {
		hash = PEARSON_LOOKUP_TABLE[hash ^ data[--i]];
	}
	// MODIFICATION START
	return (hash == 0) ? (uint8_t)1 : hash; // 0 reserved for "null"
						// MODIFICATION END
}

// ===============================================================================================
// SLOT CLASS METHODS
// ===============================================================================================

bool internal::tree3::KVSlot::empty()
{
	if (kv)
		return false;
	else
		return true;
}

void internal::tree3::KVSlot::clear()
{
	if (kv) {
		char *p = kv.get();
		set_ph_direct(p, 0);
		set_ks_direct(p, 0);
		set_vs_direct(p, 0);
		delete_persistent<char[]>(kv,
					  sizeof(uint8_t) + sizeof(uint32_t) +
						  sizeof(uint32_t) + get_ks_direct(p) +
						  get_vs_direct(p) + 2);
		kv = nullptr;
	}
}

void internal::tree3::KVSlot::set(const uint8_t hash, const std::string &key,
				  const std::string &value)
{
	if (kv) {
		char *p = kv.get();
		delete_persistent<char[]>(kv,
					  sizeof(uint8_t) + sizeof(uint32_t) +
						  sizeof(uint32_t) + get_ks_direct(p) +
						  get_vs_direct(p) + 2);
	}
	size_t ksize;
	size_t vsize;
	ksize = key.size();
	vsize = value.size();
	size_t size =
		ksize + vsize + 2 + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t);
	kv = make_persistent<char[]>(size);
	char *p = kv.get();
	set_ph_direct(p, hash);
	set_ks_direct(p, (uint32_t)ksize);
	set_vs_direct(p, (uint32_t)vsize);
	char *kvptr = p + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t);
	memcpy(kvptr, key.data(), ksize);   // copy key into buffer
	kvptr += ksize + 1;		    // advance ptr past key
	memcpy(kvptr, value.data(), vsize); // copy value into buffer
}

// ===============================================================================================
// Node invariants
// ===============================================================================================

void internal::tree3::KVInnerNode::assert_invariants()
{
	assert(keycount <= INNER_KEYS);
	for (auto i = 0; i < keycount; ++i) {
		assert(keys[i].size() > 0);
		assert(children[i] != nullptr);
	}
	assert(children[keycount] != nullptr);
	for (auto i = keycount + 1; i < INNER_KEYS + 1; ++i)
		assert(children[i] == nullptr);
}

static factory_registerer
	register_tree3(std::unique_ptr<engine_base::factory_base>(new tree3_factory));

} // namespace kv
} // namespace pmem
