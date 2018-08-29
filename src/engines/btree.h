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

#include "../pmemkv.h"
#include "btree/persistent_b_tree.h"
#include "btree/pstring.h"

using pmem::obj::pool;
using pmem::obj::persistent_ptr;

namespace pmemkv {
namespace btree {

const string ENGINE = "btree";                             // engine identifier
const size_t DEGREE = 64;
const size_t MAX_KEY_SIZE = 20;
const size_t MAX_VALUE_SIZE = 200;

class BTreeEngine : public KVEngine {
  public:
    BTreeEngine(const string& path, size_t size);          // default constructor
    ~BTreeEngine();                                        // default destructor

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
  private:
    BTreeEngine(const BTreeEngine&);
    void operator=(const BTreeEngine&);
    typedef persistent::b_tree<pstring<MAX_KEY_SIZE>,
            pstring<MAX_VALUE_SIZE>, DEGREE> btree_type;
    struct RootData {
        persistent_ptr<btree_type> btree_ptr;
    };
    void Recover();                                        // reload state from persistent pool
    pool<RootData> pmpool;                                 // pool for persistent root
    btree_type* my_btree;
};

} // namespace btree
} // namespace pmemkv
