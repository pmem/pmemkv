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
#include "persistent_concurrent_hash_map.hpp"
#include "btree/pstring.h"

using pmem::obj::pool;
using pmem::obj::persistent_ptr;

namespace pmemkv {
namespace hash_map_engine {

const string ENGINE = "hash_map";
const size_t MAX_KEY_SIZE = 20;
const size_t MAX_VALUE_SIZE = 200;

template<size_t STR_SIZE>
struct pstring_hash_compare {
    typedef pstring<STR_SIZE> pstrint_type;
    static const size_t hash_multiplier = 11400714819323198485ULL;

    static size_t hash(const pstrint_type& a) {
        size_t h = 0;
        for (const char* c = a.c_str(); c < a.c_str() + a.size(); ++c) {
            h = static_cast<size_t>(*c) ^ (h * hash_multiplier);
        }
        return h;
    }
    static bool equal(const pstrint_type& a, const pstrint_type& b) { return a == b; }
};

class HashMapEngine : public KVEngine {
  public:
    HashMapEngine(const string& path, size_t size);
    ~HashMapEngine();
    string Engine() final { return ENGINE; }
    int64_t Count() final;
    int64_t CountLike(const string& pattern) final;
    using KVEngine::Each;
    void Each(void* context, KVEachCallback* callback) final;
    using KVEngine::EachLike;
    void EachLike(const string& pattern, void* context, KVEachCallback* callback) final;
    KVStatus Exists(const string& key) final;
    using KVEngine::Get;
    void Get(void* context, const string& key, KVGetCallback* callback) final;
    KVStatus Put(const string& key, const string& value) final;
    KVStatus Remove(const string& key) final;
  private:
    typedef pstring<MAX_KEY_SIZE> key_type;
    typedef pstring<MAX_VALUE_SIZE> mapped_type;
    typedef pmem::obj::experimental::persistent_concurrent_hash_map<key_type, mapped_type, pstring_hash_compare<MAX_KEY_SIZE> > hash_map_type;
    struct RootData {
        persistent_ptr<hash_map_type> hash_map_ptr;
    };
    HashMapEngine(const HashMapEngine&);
    void operator=(const HashMapEngine&);
    void Recover();
    pool<RootData> pmpool;
    hash_map_type* my_hash_map;
};

} // namespace hash_map_engine
} // namespace pmemkv
