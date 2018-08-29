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

#pragma once

#include <vector>
#include "../pmemkv.h"

using std::move;
using std::unique_ptr;
using std::vector;
using pmem::obj::p;
using pmem::obj::persistent_ptr;
using pmem::obj::make_persistent;
using pmem::obj::transaction;
using pmem::obj::delete_persistent;
using pmem::obj::pool;

namespace pmemkv {
namespace kvtree3 {

const string ENGINE = "kvtree3";                           // engine identifier

#define INNER_KEYS 4                                       // maximum keys for inner nodes
#define INNER_KEYS_MIDPOINT (INNER_KEYS / 2)               // halfway point within the node
#define INNER_KEYS_UPPER ((INNER_KEYS / 2) + 1)            // index where upper half of keys begins
#define LEAF_KEYS 48                                       // maximum keys in tree nodes
#define LEAF_KEYS_MIDPOINT (LEAF_KEYS / 2)                 // halfway point within the node

class KVSlot {
  public:
    uint8_t hash() const { return get_ph(); }
    uint8_t hash_direct(char *p) const { return *((uint8_t *)(p + sizeof(uint32_t) + sizeof(uint32_t))); }
    const char* key() const { return ((char *)(kv.get()) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t)); }
    const char* key_direct(char *p) const { return (p + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t)); }
    const uint32_t keysize() const { return get_ks(); }
    const uint32_t keysize_direct(char *p) const { return *((uint32_t *)(p)); }
    const char* val() const { return ((char *)(kv.get()) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) + get_ks() + 1); }
    const char* val_direct(char *p) const { return (p + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) + *((uint32_t *)(p)) + 1); }
    const uint32_t valsize() const { return get_vs(); }
    const uint32_t valsize_direct(char *p) const { return *((uint32_t *)(p + sizeof(uint32_t))); }
    void clear();
    void set(const uint8_t hash, const string& key, const string& value);
    void set_ph(uint8_t v) {*((uint8_t *)((char *)(kv.get()) + sizeof(uint32_t) + sizeof(uint32_t))) = v;}
    void set_ph_direct(char *p, uint8_t v) {*((uint8_t *)(p + sizeof(uint32_t) + sizeof(uint32_t))) = v;}
    void set_ks(uint32_t v) {*((uint32_t *)(kv.get())) = v;}
    void set_ks_direct(char * p, uint32_t v) {*((uint32_t *)(p)) = v;}
    void set_vs(uint32_t v) {*((uint32_t *)((char *)(kv.get()) + sizeof(uint32_t))) = v;}
    void set_vs_direct(char *p, uint32_t v) {*((uint32_t *)((char *)(p) + sizeof(uint32_t))) = v;}
    uint8_t get_ph() const {return *((uint8_t *)((char *)(kv.get()) + sizeof(uint32_t) + sizeof(uint32_t)));}
    uint8_t get_ph_direct(char *p) const {return *((uint8_t *)((char *)(p) + sizeof(uint32_t) + sizeof(uint32_t)));}
    uint32_t get_ks() const {return *((uint32_t *)(kv.get()));}
    uint32_t get_ks_direct(char *p) const {return *((uint32_t *)(p));}
    uint32_t get_vs() const {return *((uint32_t *)((char *)(kv.get()) + sizeof(uint32_t)));}
    uint32_t get_vs_direct(char *p) const {return *((uint32_t *)((char *)(p) + sizeof(uint32_t)));}
    bool empty();
  private:
    persistent_ptr<char[]> kv;                             // buffer for key & value
};

struct KVLeaf {
    p<KVSlot> slots[LEAF_KEYS];                            // array of slot containers
    persistent_ptr<KVLeaf> next;                           // next leaf in unsorted list
};

struct KVRoot {                                            // persistent root object
    persistent_ptr<KVLeaf> head;                           // head of linked list of leaves
};

struct KVInnerNode;

struct KVNode {                                            // volatile nodes of the tree
    bool is_leaf = false;                                  // indicate inner or leaf node
    KVInnerNode* parent;                                   // parent of this node (null if top)
    virtual ~KVNode() = default;
};

struct KVInnerNode final : KVNode {                        // volatile inner nodes of the tree
    uint8_t keycount;                                      // count of keys in this node
    string keys[INNER_KEYS + 1];                           // child keys plus one overflow slot
    unique_ptr<KVNode> children[INNER_KEYS + 2];           // child nodes plus one overflow slot
    void assert_invariants();
};

struct KVLeafNode final : KVNode {                         // volatile leaf nodes of the tree
    uint8_t hashes[LEAF_KEYS];                             // Pearson hashes of keys
    string keys[LEAF_KEYS];                                // keys stored in this leaf
    persistent_ptr<KVLeaf> leaf;                           // pointer to persistent leaf
};

struct KVRecoveredLeaf {                                   // temporary wrapper used for recovery
    unique_ptr<KVLeafNode> leafnode;                       // leaf node being recovered
    string max_key;                                        // highest sorting key present
};

class KVTree : public KVEngine {                           // hybrid B+ tree engine
  public:
    KVTree(const string& path, size_t size);               // default constructor
    ~KVTree();                                             // default destructor

    string Engine() final { return ENGINE; }               // engine identifier

    int64_t Count() final;                                 // count all keys
    int64_t CountLike(const string& pattern) final;        // count all keys matching pattern

    using KVEngine::Each;                                  // iterate over all keys
    void Each(void* context,                               // iterate over all keys with context
              KVEachCallback* callback) final;
    using KVEngine::EachLike;                              // iterate over matching keys
    void EachLike(const string& pattern,                   // iterate over matching keys with context
                  void* context,
                  KVEachCallback* callback) final;

    KVStatus Exists(const string& key) final;              // does key have a value?

    using KVEngine::Get;                                   // pass value to callback
    void Get(void* context,                                // pass value to callback with context
             const string& key,
             KVGetCallback* callback) final;

    KVStatus Put(const string& key,                        // store key and value
                 const string& value) final;
    KVStatus Remove(const string& key) final;              // remove value for key
  protected:
    KVLeafNode* LeafSearch(const string& key);             // find node for key
    void LeafFillEmptySlot(KVLeafNode* leafnode,           // write first unoccupied slot found
                           uint8_t hash,
                           const string& key,
                           const string& value);
    bool LeafFillSlotForKey(KVLeafNode* leafnode,          // write slot for matching key if found
                            uint8_t hash,
                            const string& key,
                            const string& value);
    void LeafFillSpecificSlot(KVLeafNode* leafnode,        // write slot at specific index
                              uint8_t hash,
                              const string& key,
                              const string& value,
                              int slot);
    void LeafSplitFull(KVLeafNode* leafnode,               // split full leaf into two leaves
                       uint8_t hash,
                       const string& key,
                       const string& value);
    void InnerUpdateAfterSplit(KVNode* node,               // update parents after leaf split
                               unique_ptr<KVNode> newnode,
                               string* split_key);
    uint8_t PearsonHash(const char* data,                  // calculate 1-byte hash for string
                        size_t size);
    void Recover();                                        // reload state from persistent pool
  private:
    KVTree(const KVTree&);                                 // prevent copying
    void operator=(const KVTree&);                         // prevent assigning
    vector<persistent_ptr<KVLeaf>> leaves_prealloc;        // persisted but unused leaves
    const string pmpath;                                   // path when constructed
    pool<KVRoot> pmpool;                                   // pool for persistent root
    unique_ptr<KVNode> tree_top;                           // pointer to uppermost inner node
};

} // namespace kvtree
} // namespace pmemkv
