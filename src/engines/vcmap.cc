/*
 * Copyright 2018, Intel Corporation
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

#include "vcmap.h"

#include <iostream>

#define DO_LOG 0
#define LOG(msg) if (DO_LOG) std::cout << "[tbb_hash_map] " << msg << "\n"

namespace pmemkv {
namespace tbb_hash_map {

TbbHashMap::TbbHashMap(const string& path, size_t size) : kv_allocator(path, size), ch_allocator(kv_allocator),
    pmem_kv_container(std::scoped_allocator_adaptor<kv_allocator_t>(kv_allocator)) { LOG("Opened ok"); }

TbbHashMap::~TbbHashMap() { LOG("Closed ok"); }

int64_t TbbHashMap::Count() {
    int64_t result = 0;
    for (auto& iterator : pmem_kv_container) result++; 
    return result;
}

int64_t TbbHashMap::CountLike(const string& pattern) {
    LOG("Count like pattern=" << pattern);
}

KVStatus TbbHashMap::Exists(const string& key) {
    LOG("Exists for key=" << key);
    map_t::const_accessor result;
    const bool result_found = pmem_kv_container.find(result, pmem_string(key.c_str(), key.size(), ch_allocator));
    return (result_found ? OK : NOT_FOUND);
}

void TbbHashMap::Each(void* context, KVEachCallback* callback) {
    LOG("Each");
    for (auto& iterator : pmem_kv_container) {
        (*callback)(context, (int32_t)iterator.first.size(), (int32_t)iterator.second.size(),
                iterator.first.c_str(), iterator.second.c_str());
    }
}

void TbbHashMap::EachLike(const string& pattern, void* context, KVEachCallback* callback) {
    LOG("Each like pattern=" << pattern);
}

void TbbHashMap::Get(void* context, const string& key, KVGetCallback* callback) {
    LOG("Get key=" << key);
    map_t::const_accessor result;
    const bool result_found = pmem_kv_container.find(result, pmem_string(key.c_str(), key.size(), ch_allocator));
    if (!result_found) {
        LOG("  key not found");
        return;
    }
    (*callback)(context, (int32_t) result->second.size(), result->second.c_str());
}

KVStatus TbbHashMap::Put(const string& key, const string& value) {    
    LOG("Put key=" << key << ", value.size=" << to_string(value.size()));    
    map_t::value_type kv_pair { pmem_string(key.c_str(), key.size(), ch_allocator), pmem_string(value.c_str(), value.size(), ch_allocator) };    
    bool result = pmem_kv_container.insert(kv_pair);

    if (!result) {
        map_t::accessor result_found;
        pmem_kv_container.find(result_found, kv_pair.first);
        result_found->second = kv_pair.second;
    }

    return OK;
}

KVStatus TbbHashMap::Remove(const string& key) {
    LOG("Remove key=" << key);
    size_t erased = pmem_kv_container.erase(pmem_string(key.c_str(), key.size(), ch_allocator));
    if (erased == 1) {
        return OK;
    }
    return NOT_FOUND;
}

} // namespace tbb_hash_map
} // namespace pmemkv
