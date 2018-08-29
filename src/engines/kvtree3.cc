/*
 * Copyright 2017-2018, Intel Corporation
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

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <list>
#include <regex>
#include <unistd.h>
#include "kvtree3.h"

#define DO_LOG 0
#define LOG(msg) if (DO_LOG) std::cout << "[kvtree3] " << msg << "\n"

namespace pmemkv {
namespace kvtree3 {

KVTree::KVTree(const string& path, const size_t size) : pmpath(path) {
    if ((access(path.c_str(), F_OK) != 0) && (size > 0)) {
        LOG("Creating filesystem pool, path=" << path << ", size=" << to_string(size));
        pmpool = pool<KVRoot>::create(path.c_str(), LAYOUT, size, S_IRWXU);
    } else {
        LOG("Opening pool, path=" << path);
        pmpool = pool<KVRoot>::open(path.c_str(), LAYOUT);
    }
    Recover();
    LOG("Opened ok");
}

KVTree::~KVTree() {
    LOG("Closing");
    pmpool.close();
    LOG("Closed ok");
}

// ===============================================================================================
// KEY/VALUE METHODS
// ===============================================================================================

int64_t KVTree::Count() {
    int64_t result = 0;
    auto leaf = pmpool.get_root()->head;
    while (leaf) {
        for (int slot = LEAF_KEYS; slot--;) {
            auto kvslot = leaf->slots[slot].get_ro();
            if (kvslot.empty() || kvslot.hash() == 0) continue;
            result++;
        }
        leaf = leaf->next;  // advance to next linked leaf
    }
    return result;
}

int64_t KVTree::CountLike(const string& pattern) {
    LOG("Count like pattern=" << pattern);
    try {
        std::regex p(pattern);
        int64_t result = 0;
        auto leaf = pmpool.get_root()->head;
        while (leaf) {
            for (int slot = LEAF_KEYS; slot--;) {
                auto kvslot = leaf->slots[slot].get_ro();
                if (kvslot.empty() || kvslot.hash() == 0) continue;
                auto key = string(kvslot.key(), kvslot.get_ks());
                if (std::regex_match(key, p)) result++;
            }
            leaf = leaf->next;  // advance to next linked leaf
        }
        return result;
    } catch (std::regex_error) {
        LOG("Invalid pattern: " << pattern);
        return 0;
    }
}

void KVTree::Each(void* context, KVEachCallback* callback) {
    LOG("Each");
    auto leaf = pmpool.get_root()->head;
    while (leaf) {
        for (int slot = LEAF_KEYS; slot--;) {
            auto kvslot = leaf->slots[slot].get_ro();
            if (kvslot.empty() || kvslot.hash() == 0) continue;
            (*callback)(context, kvslot.get_ks(), kvslot.get_vs(), kvslot.key(), kvslot.val());
        }
        leaf = leaf->next;  // advance to next linked leaf
    }
}

void KVTree::EachLike(const string& pattern, void* context, KVEachCallback* callback) {
    LOG("Each like pattern=" << pattern);
    try {
        std::regex p(pattern);
        auto leaf = pmpool.get_root()->head;
        while (leaf) {
            for (int slot = LEAF_KEYS; slot--;) {
                auto kvslot = leaf->slots[slot].get_ro();
                if (kvslot.empty() || kvslot.hash() == 0) continue;
                auto key = string(kvslot.key(), kvslot.get_ks());
                if (std::regex_match(key, p))
                    (*callback)(context, kvslot.get_ks(), kvslot.get_vs(), kvslot.key(), kvslot.val());
            }
            leaf = leaf->next;  // advance to next linked leaf
        }
    } catch (std::regex_error) {
        LOG("Invalid pattern: " << pattern);
    }
}

KVStatus KVTree::Exists(const string& key) {
    LOG("Exists for key=" << key);
    auto leafnode = LeafSearch(key);
    if (leafnode) {
        const uint8_t hash = PearsonHash(key.c_str(), key.size());
        for (int slot = LEAF_KEYS; slot--;) {
            if (leafnode->hashes[slot] == hash) {
                if (leafnode->keys[slot].compare(key) == 0)
                    return OK;
            }
        }
    }
    LOG("   could not find key");
    return NOT_FOUND;
}

void KVTree::Get(void* context, const string& key, KVGetCallback* callback) {
    LOG("Get using callback for key=" << key);
    auto leafnode = LeafSearch(key);
    if (leafnode) {
        const uint8_t hash = PearsonHash(key.c_str(), key.size());
        for (int slot = LEAF_KEYS; slot--;) {
            if (leafnode->hashes[slot] == hash) {
                if (leafnode->keys[slot].compare(key) == 0) {
                    auto kv = leafnode->leaf->slots[slot].get_ro();
                    LOG("   found value, slot=" << slot << ", size=" << to_string(kv.valsize()));
                    (*callback)(context, kv.valsize(), kv.val());
                    return;
                }
            }
        }
    }
    LOG("   could not find key");
}

KVStatus KVTree::Put(const string& key, const string& value) {
    LOG("Put key=" << key.c_str() << ", value.size=" << to_string(value.size()));
    try {
        const uint8_t hash = PearsonHash(key.c_str(), key.size());
        auto leafnode = LeafSearch(key);
        if (!leafnode) {
            LOG("   adding head leaf");
            unique_ptr<KVLeafNode> new_node(new KVLeafNode());
            new_node->is_leaf = true;
            transaction::exec_tx(pmpool, [&] {
                if (!leaves_prealloc.empty()) {
                    new_node->leaf = leaves_prealloc.back();
                    leaves_prealloc.pop_back();
                } else {
                    auto root = pmpool.get_root();
                    auto old_head = root->head;
                    auto new_leaf = make_persistent<KVLeaf>();
                    root->head = new_leaf;
                    new_leaf->next = old_head;
                    new_node->leaf = new_leaf;
                }
                LeafFillSpecificSlot(new_node.get(), hash, key, value, 0);
            });
            tree_top = move(new_node);
        } else if (LeafFillSlotForKey(leafnode, hash, key, value)) {
            // nothing else to do
        } else {
            LeafSplitFull(leafnode, hash, key, value);
        }
        return OK;
    } catch (pmem::transaction_alloc_error) {
        return FAILED;
    } catch (pmem::transaction_error) {
        return FAILED;
    }
}

KVStatus KVTree::Remove(const string& key) {
    LOG("Remove key=" << key.c_str());
    auto leafnode = LeafSearch(key);
    if (!leafnode) {
        LOG("   head not present");
        return OK;
    }
    const uint8_t hash = PearsonHash(key.c_str(), key.size());
    for (int slot = LEAF_KEYS; slot--;) {
        if (leafnode->hashes[slot] == hash) {
            if (leafnode->keys[slot].compare(key) == 0) {
                LOG("   freeing slot=" << slot);
                leafnode->hashes[slot] = 0;
                leafnode->keys[slot].clear();
                auto leaf = leafnode->leaf;
                transaction::exec_tx(pmpool, [&] {
                    leaf->slots[slot].get_rw().clear();
                });
                break;  // no duplicate keys allowed
            }
        }
    }
    return OK;
}

// ===============================================================================================
// PROTECTED LEAF METHODS
// ===============================================================================================

KVLeafNode* KVTree::LeafSearch(const string& key) {
    KVNode* node = tree_top.get();
    if (node == nullptr) return nullptr;
    bool matched;
    while (!node->is_leaf) {
        matched = false;
        auto inner = (KVInnerNode*) node;
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
        if (!matched) node = inner->children[keycount].get();
    }
    return (KVLeafNode*) node;
}

void KVTree::LeafFillEmptySlot(KVLeafNode* leafnode, const uint8_t hash,
                               const string& key, const string& value) {
    for (int slot = LEAF_KEYS; slot--;) {
        if (leafnode->hashes[slot] == 0) {
            LeafFillSpecificSlot(leafnode, hash, key, value, slot);
            return;
        }
    }
}

bool KVTree::LeafFillSlotForKey(KVLeafNode* leafnode, const uint8_t hash,
                                const string& key, const string& value) {
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
                break;  // no duplicate keys allowed
            }
        }
    }

    // update suitable slot if found
    int slot = key_match_slot >= 0 ? key_match_slot : last_empty_slot;
    if (slot >= 0) {
        LOG("   filling slot=" << slot);
        transaction::exec_tx(pmpool, [&] {
            LeafFillSpecificSlot(leafnode, hash, key, value, slot);
        });
    }
    return slot >= 0;
}

void KVTree::LeafFillSpecificSlot(KVLeafNode* leafnode, const uint8_t hash,
                                  const string& key, const string& value, const int slot) {
    if (leafnode->hashes[slot] == 0) {
        leafnode->hashes[slot] = hash;
        leafnode->keys[slot] = key;
    }
    leafnode->leaf->slots[slot].get_rw().set(hash, key, value);
}

void KVTree::LeafSplitFull(KVLeafNode* leafnode, const uint8_t hash,
                           const string& key, const string& value) {
    string keys[LEAF_KEYS + 1];
    keys[LEAF_KEYS] = key;
    for (int slot = LEAF_KEYS; slot--;) keys[slot] = leafnode->keys[slot];
    std::sort(std::begin(keys), std::end(keys), [](const string& lhs, const string& rhs) {
        return lhs.compare(rhs) < 0;
    });
    string split_key = keys[LEAF_KEYS_MIDPOINT];
    LOG("   splitting leaf at key=" << split_key);

    // split leaf into two leaves, moving slots that sort above split key to new leaf
    unique_ptr<KVLeafNode> new_leafnode(new KVLeafNode());
    new_leafnode->parent = leafnode->parent;
    new_leafnode->is_leaf = true;
    transaction::exec_tx(pmpool, [&] {
        persistent_ptr<KVLeaf> new_leaf;
        if (!leaves_prealloc.empty()) {
            new_leaf = leaves_prealloc.back();
            new_leafnode->leaf = new_leaf;
            leaves_prealloc.pop_back();
        } else {
            auto root = pmpool.get_root();
            auto old_head = root->head;
            new_leaf = make_persistent<KVLeaf>();
            root->head = new_leaf;
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

void KVTree::InnerUpdateAfterSplit(KVNode* node, unique_ptr<KVNode> new_node, string* split_key) {
    if (!node->parent) {
        assert(node == tree_top.get());
        LOG("   creating new top node for split_key=" << *split_key);
        unique_ptr<KVInnerNode> top(new KVInnerNode());
        top->keycount = 1;
        top->keys[0] = *split_key;
        node->parent = top.get();
        new_node->parent = top.get();
        top->children[0] = move(tree_top);
        top->children[1] = move(new_node);
#ifndef NDEBUG
        top->assert_invariants();
#endif
        tree_top = move(top);                                            // assign new top node
        return;                                                          // end recursion
    }

    LOG("   updating parents for split_key=" << *split_key);
    KVInnerNode* inner = node->parent;
    { // insert split_key and new_node into inner node in sorted order
        const uint8_t keycount = inner->keycount;
        int idx = 0;  // position where split_key should be inserted
        while (idx < keycount && inner->keys[idx].compare(*split_key) <= 0) idx++;
        for (int i = keycount - 1; i >= idx; i--) inner->keys[i + 1] = move(inner->keys[i]);
        for (int i = keycount; i > idx; i--) inner->children[i + 1] = move(inner->children[i]);
        inner->keys[idx] = *split_key;
        inner->children[idx + 1] = move(new_node);
        inner->keycount = (uint8_t) (keycount + 1);
    }
    const uint8_t keycount = inner->keycount;
    if (keycount <= INNER_KEYS) {
#ifndef NDEBUG
        inner->assert_invariants();
#endif
        return;                                                          // end recursion
    }

    // split inner node at the midpoint, update parents as needed
    unique_ptr<KVInnerNode> ni(new KVInnerNode());                       // create new inner node
    ni->parent = inner->parent;                                          // set parent reference
    for (int i = INNER_KEYS_UPPER; i < keycount; i++) {                  // move all upper keys
        ni->keys[i - INNER_KEYS_UPPER] = move(inner->keys[i]);           // move key string
    }
    for (int i = INNER_KEYS_UPPER; i < keycount + 1; i++) {              // move all upper children
        ni->children[i - INNER_KEYS_UPPER] = move(inner->children[i]);   // move child reference
        ni->children[i - INNER_KEYS_UPPER]->parent = ni.get();           // set parent reference
    }
    ni->keycount = INNER_KEYS_MIDPOINT;                                  // always half the keys
    string new_split_key = inner->keys[INNER_KEYS_MIDPOINT];             // save for recursion
    inner->keycount = INNER_KEYS_MIDPOINT;                               // half of keys remain

    // perform deep check on modified inner nodes
#ifndef NDEBUG
    inner->assert_invariants();                                          // check node just split
    ni->assert_invariants();                                             // check new node
#endif

    InnerUpdateAfterSplit(inner, move(ni), &new_split_key);              // recursive update
}

// ===============================================================================================
// PROTECTED LIFECYCLE METHODS
// ===============================================================================================

void KVTree::Recover() {
    LOG("Recovering");

    // traverse persistent leaves to build list of leaves to recover
    std::list<KVRecoveredLeaf> leaves;
    auto leaf = pmpool.get_root()->head;
    while (leaf) {
        unique_ptr<KVLeafNode> leafnode(new KVLeafNode());
        leafnode->leaf = leaf;
        leafnode->is_leaf = true;

        // find highest sorting key in leaf, while recovering all hashes
        bool empty_leaf = true;
        string max_key;
        for (int slot = LEAF_KEYS; slot--;) {
            auto kvslot = leaf->slots[slot].get_ro();
            if (kvslot.empty()) continue;
            leafnode->hashes[slot] = kvslot.hash();
            if (leafnode->hashes[slot] == 0) continue;
            const char* key = kvslot.key();
            if (empty_leaf) {
                max_key = string(kvslot.key(), kvslot.get_ks());
                empty_leaf = false;
            } else if (max_key.compare(0, string::npos, kvslot.key(), kvslot.get_ks()) < 0) {
                max_key = string(kvslot.key(), kvslot.get_ks());
            }
            leafnode->keys[slot] = string(key, kvslot.get_ks());
        }

        // use highest sorting key to decide how to recover the leaf
        if (empty_leaf) {
            leaves_prealloc.push_back(leaf);
        } else {
            leaves.push_back({move(leafnode), max_key});
        }

        leaf = leaf->next;  // advance to next linked leaf
    }

    // sort recovered leaves in ascending key order
    leaves.sort([](const KVRecoveredLeaf& lhs, const KVRecoveredLeaf& rhs) {
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
            string split_key = string(max_key);
            auto nextnode = leaves.front().leafnode.get();
            nextnode->parent = prevnode->parent;
            InnerUpdateAfterSplit(prevnode, move(leaves.front().leafnode), &split_key);
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
        251, 175, 119, 215, 81, 14, 79, 191, 103, 49, 181, 143, 186, 157, 0,
        232, 31, 32, 55, 60, 152, 58, 17, 237, 174, 70, 160, 144, 220, 90, 57,
        223, 59, 3, 18, 140, 111, 166, 203, 196, 134, 243, 124, 95, 222, 179,
        197, 65, 180, 48, 36, 15, 107, 46, 233, 130, 165, 30, 123, 161, 209, 23,
        97, 16, 40, 91, 219, 61, 100, 10, 210, 109, 250, 127, 22, 138, 29, 108,
        244, 67, 207, 9, 178, 204, 74, 98, 126, 249, 167, 116, 34, 77, 193,
        200, 121, 5, 20, 113, 71, 35, 128, 13, 182, 94, 25, 226, 227, 199, 75,
        27, 41, 245, 230, 224, 43, 225, 177, 26, 155, 150, 212, 142, 218, 115,
        241, 73, 88, 105, 39, 114, 62, 255, 192, 201, 145, 214, 168, 158, 221,
        148, 154, 122, 12, 84, 82, 163, 44, 139, 228, 236, 205, 242, 217, 11,
        187, 146, 159, 64, 86, 239, 195, 42, 106, 198, 118, 112, 184, 172, 87,
        2, 173, 117, 176, 229, 247, 253, 137, 185, 99, 164, 102, 147, 45, 66,
        231, 52, 141, 211, 194, 206, 246, 238, 56, 110, 78, 248, 63, 240, 189,
        93, 92, 51, 53, 183, 19, 171, 72, 50, 33, 104, 101, 69, 8, 252, 83, 120,
        76, 135, 85, 54, 202, 125, 188, 213, 96, 235, 136, 208, 162, 129, 190,
        132, 156, 38, 47, 1, 7, 254, 24, 4, 216, 131, 89, 21, 28, 133, 37, 153,
        149, 80, 170, 68, 6, 169, 234, 151
};

// Modified Pearson hashing algorithm from RFC 3074
uint8_t KVTree::PearsonHash(const char* data, const size_t size) {
    auto hash = (uint8_t) size;
    for (size_t i = size; i > 0;) {
        hash = PEARSON_LOOKUP_TABLE[hash ^ data[--i]];
    }
    // MODIFICATION START
    return (hash == 0) ? (uint8_t) 1 : hash;                             // 0 reserved for "null"
    // MODIFICATION END
}

// ===============================================================================================
// SLOT CLASS METHODS
// ===============================================================================================

bool KVSlot::empty() {
    if (kv)
        return false;
    else
        return true;
}

void KVSlot::clear() {
    if (kv) {
        char* p = kv.get();
        set_ph_direct(p, 0);
        set_ks_direct(p, 0);
        set_vs_direct(p, 0);
        delete_persistent<char[]>(kv, sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) + get_ks_direct(p) +
                                      get_vs_direct(p) + 2);
        kv = nullptr;
    }
}

void KVSlot::set(const uint8_t hash, const string& key, const string& value) {
    if (kv) {
        char* p = kv.get();
        delete_persistent<char[]>(kv, sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) + get_ks_direct(p) +
                                      get_vs_direct(p) + 2);
    }
    size_t ksize;
    size_t vsize;
    ksize = key.size();
    vsize = value.size();
    size_t size = ksize + vsize + 2 + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t);
    kv = make_persistent<char[]>(size);
    char* p = kv.get();
    set_ph_direct(p, hash);
    set_ks_direct(p, (uint32_t) ksize);
    set_vs_direct(p, (uint32_t) vsize);
    char* kvptr = p + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t);
    memcpy(kvptr, key.data(), ksize);                                       // copy key into buffer
    kvptr += ksize + 1;                                                     // advance ptr past key
    memcpy(kvptr, value.data(), vsize);                                     // copy value into buffer
}

// ===============================================================================================
// Node invariants
// ===============================================================================================

void KVInnerNode::assert_invariants() {
    assert(keycount <= INNER_KEYS);
    for (auto i = 0; i < keycount; ++i) {
        assert(keys[i].size() > 0);
        assert(children[i] != nullptr);
    }
    assert(children[keycount] != nullptr);
    for (auto i = keycount + 1; i < INNER_KEYS + 1; ++i)
        assert(children[i] == nullptr);
}

} // namespace kvtree
} // namespace pmemkv
