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
#include "pmem_allocator.h"
#include <map>
#include <string>
#include <scoped_allocator>

namespace pmemkv {
namespace std_map {

const string ENGINE = "std_map";                         // engine identifier

class StdMap : public KVEngine {

    typedef pmem::allocator<char> ch_allocator_t;
    typedef std::basic_string<char, std::char_traits<char>, ch_allocator_t > pmem_string;
    typedef pmem::allocator<std::pair<pmem_string, pmem_string> > kv_allocator_t;
    typedef std::map<pmem_string, pmem_string, std::less<pmem_string>, std::scoped_allocator_adaptor<kv_allocator_t > > map_t;

    kv_allocator_t kv_allocator;
    ch_allocator_t ch_allocator;
    map_t pmem_kv_container;

public:
    StdMap(const string& path, size_t size);           // default constructor
    ~StdMap();                                          // default destructor

    string Engine() final { return ENGINE; }               // engine identifier

    int64_t Count() final;                                 // count all keys

    using KVEngine::Each;                                  // iterate over all keys & values
    void Each(void* context,                               // (with context)
            KVEachCallback* callback) final;

    KVStatus Exists(const string& key) final;              // does key have a value?

    using KVEngine::Get;                                   // pass value to callback
    void Get(void* context,                                // (with context)
            const string& key,
            KVGetCallback* callback) final;

    KVStatus Put(const string& key,                        // store key and value
            const string& value) final;
    KVStatus Remove(const string& key) final;              // remove value for key
};

} // namespace std_map
} // namespace pmemkv
