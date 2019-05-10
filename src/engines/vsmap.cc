/*
 * Copyright 2017-2019, Intel Corporation
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

#include "vsmap.h"
#include <libpmemobj++/transaction.hpp>

#include <iostream>

#define DO_LOG 0
#define LOG(msg) if (DO_LOG) std::cout << "[vsmap] " << msg << "\n"

namespace pmemkv {
namespace vsmap {

VSMap::VSMap(void* context, const string& path, size_t size) : engine_context(context),
             kv_allocator(path, size), ch_allocator(kv_allocator),
             pmem_kv_container(std::scoped_allocator_adaptor<kv_allocator_t>(kv_allocator)) {
    LOG("Started ok");
}

VSMap::~VSMap() {
    LOG("Stopped ok");
}

void VSMap::All(void* context, KVAllCallback* callback) {
    LOG("All");
    for (auto& it : pmem_kv_container) (*callback)(context, (int32_t) it.first.size(), it.first.c_str());
}

void VSMap::AllAbove(void* context, const string& key, KVAllCallback* callback) {
    LOG("AllAbove for key=" << key);
    auto it = pmem_kv_container.upper_bound(pmem_string(key.c_str(), key.size(), ch_allocator));
    auto end = pmem_kv_container.end();
    for (; it != end; it++) (*callback)(context, (int32_t) it->first.size(), it->first.c_str());
}

void VSMap::AllBelow(void* context, const string& key, KVAllCallback* callback) {
    LOG("AllBelow for key=" << key);
    auto it = pmem_kv_container.begin();
    auto end = pmem_kv_container.lower_bound(pmem_string(key.c_str(), key.size(), ch_allocator));
    for (; it != end; it++) (*callback)(context, (int32_t) it->first.size(), it->first.c_str());
}

void VSMap::AllBetween(void* context, const string& key1, const string& key2, KVAllCallback* callback) {
    LOG("AllBetween for key1=" << key1 << ", key2=" << key2);
    if (key1 < key2) {
        auto it = pmem_kv_container.upper_bound(pmem_string(key1.c_str(), key1.size(), ch_allocator));
        auto end = pmem_kv_container.lower_bound(pmem_string(key2.c_str(), key2.size(), ch_allocator));
        for (; it != end; it++) (*callback)(context, (int32_t) it->first.size(), it->first.c_str());
    }
}

int64_t VSMap::Count() {
    return pmem_kv_container.size();
}

int64_t VSMap::CountAbove(const string& key) {
    LOG("CountAbove for key=" << key);
    int64_t result = 0;
    auto it = pmem_kv_container.upper_bound(pmem_string(key.c_str(), key.size(), ch_allocator));
    auto end = pmem_kv_container.end();
    for (; it != end; it++) result++;
    return result;
}

int64_t VSMap::CountBelow(const string& key) {
    LOG("CountBelow for key=" << key);
    int64_t result = 0;
    auto it = pmem_kv_container.begin();
    auto end = pmem_kv_container.lower_bound(pmem_string(key.c_str(), key.size(), ch_allocator));
    for (; it != end; it++) result++;
    return result;
}

int64_t VSMap::CountBetween(const string& key1, const string& key2) {
    LOG("CountBetween for key1=" << key1 << ", key2=" << key2);
    int64_t result = 0;
    if (key1 < key2) {
        auto it = pmem_kv_container.upper_bound(pmem_string(key1.c_str(), key1.size(), ch_allocator));
        auto end = pmem_kv_container.lower_bound(pmem_string(key2.c_str(), key2.size(), ch_allocator));
        for (; it != end; it++) result++;
    }
    return result;
}

void VSMap::Each(void* context, KVEachCallback* callback) {
    LOG("Each");
    for (auto& it : pmem_kv_container)
        (*callback)(context, (int32_t) it.first.size(), it.first.c_str(),
                    (int32_t) it.second.size(), it.second.c_str());
}

void VSMap::EachAbove(void* context, const string& key, KVEachCallback* callback) {
    LOG("EachAbove for key=" << key);
    auto it = pmem_kv_container.upper_bound(pmem_string(key.c_str(), key.size(), ch_allocator));
    auto end = pmem_kv_container.end();
    for (; it != end; it++)
        (*callback)(context, (int32_t) it->first.size(), it->first.c_str(),
                    (int32_t) it->second.size(), it->second.c_str());
}

void VSMap::EachBelow(void* context, const string& key, KVEachCallback* callback) {
    LOG("EachBelow for key=" << key);
    auto it = pmem_kv_container.begin();
    auto end = pmem_kv_container.lower_bound(pmem_string(key.c_str(), key.size(), ch_allocator));
    for (; it != end; it++)
        (*callback)(context, (int32_t) it->first.size(), it->first.c_str(),
                    (int32_t) it->second.size(), it->second.c_str());
}

void VSMap::EachBetween(void* context, const string& key1, const string& key2, KVEachCallback* callback) {
    LOG("EachBetween for key1=" << key1 << ", key2=" << key2);
    if (key1 < key2) {
        auto it = pmem_kv_container.upper_bound(pmem_string(key1.c_str(), key1.size(), ch_allocator));
        auto end = pmem_kv_container.lower_bound(pmem_string(key2.c_str(), key2.size(), ch_allocator));
        for (; it != end; it++)
            (*callback)(context, (int32_t) it->first.size(), it->first.c_str(),
                        (int32_t) it->second.size(), it->second.c_str());
    }
}

KVStatus VSMap::Exists(const string& key) {
    LOG("Exists for key=" << key);
    bool r = pmem_kv_container.find(pmem_string(key.c_str(), key.size(), ch_allocator)) != pmem_kv_container.end();
    return (r ? OK : NOT_FOUND);
}

void VSMap::Get(void* context, const string& key, KVGetCallback* callback) {
    LOG("Get key=" << key);
    const auto pos = pmem_kv_container.find(pmem_string(key.c_str(), key.size(), ch_allocator));
    if (pos == pmem_kv_container.end()) {
        LOG("  key not found");
        return;
    }
    (*callback)(context, (int32_t) pos->second.size(), pos->second.c_str());
}

KVStatus VSMap::Put(const string& key, const string& value) {
    LOG("Put key=" << key << ", value.size=" << to_string(value.size()));
    try {
        pmem_kv_container[pmem_string(key.c_str(), key.size(), ch_allocator)] =
                pmem_string(value.c_str(), value.size(), ch_allocator);
        return OK;
    } catch (std::bad_alloc e) {
        LOG("Put failed due to exception, " << e.what());
        return FAILED;
    } catch (pmem::transaction_alloc_error e) {
        LOG("Put failed due to pmem::transaction_alloc_error, " << e.what());
        return FAILED;
    } catch (pmem::transaction_error e) {
        LOG("Put failed due to pmem::transaction_error, " << e.what());
        return FAILED;
    }
}

KVStatus VSMap::Remove(const string& key) {
    LOG("Remove key=" << key);
    try {
        size_t erased = pmem_kv_container.erase(pmem_string(key.c_str(), key.size(), ch_allocator));
        return (erased == 1) ? OK : NOT_FOUND;
    } catch (std::bad_alloc e) {
        LOG("Put failed due to exception, " << e.what());
        return FAILED;
    } catch (pmem::transaction_alloc_error e) {
        LOG("Put failed due to pmem::transaction_alloc_error, " << e.what());
        return FAILED;
    } catch (pmem::transaction_error e) {
        LOG("Put failed due to pmem::transaction_error, " << e.what());
        return FAILED;
    }
}

} // namespace vsmap
} // namespace pmemkv
