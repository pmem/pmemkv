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

#include <iostream>
#include <regex>
#include <unistd.h>

#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/detail/common.hpp>

#include "cmap.h"

#define DO_LOG 0
#define LOG(msg) if (DO_LOG) std::cout << "[cmap] " << msg << "\n"

using pmem::obj::make_persistent_atomic;
using pmem::obj::transaction;
using pmem::detail::conditional_add_to_tx;

namespace pmemkv {
namespace cmap {

CMap::CMap(const string& path, const size_t size) {
    if (access(path.c_str(), F_OK) != 0 && size > 0) {
        LOG("Creating filesystem pool, path=" << path << ", size=" << to_string(size));
        pmpool = pool<RootData>::create(path.c_str(), LAYOUT, size, S_IRWXU);
    } else {
        LOG("Opening filesystem pool, path=" << path);
        pmpool = pool<RootData>::open(path.c_str(), LAYOUT);
    }
    Recover();
    LOG("Opened ok");
}

CMap::~CMap() {
    LOG("Closing");
    pmpool.close();
    LOG("Closed ok");
}

int64_t CMap::Count() {
    LOG("Count");
    return std::distance(my_hash_map->begin(), my_hash_map->end());
}

int64_t CMap::CountLike(const string& pattern) {
    LOG("Count like pattern=" << pattern);
    try {
        std::regex p(pattern);
        int64_t result = 0;
        for (auto it = my_hash_map->begin(); it != my_hash_map->end(); ++it) {
            auto key = string(it->first.c_str(), it->first.size());
            if (std::regex_match(key, p)) ++result;
        }
        return result;
    } catch (std::regex_error) {
        LOG("Invalid pattern: " << pattern);
        return 0;
    }
}

void CMap::Each(void* context, KVEachCallback* callback) {
    LOG("Each");
    for (auto it = my_hash_map->begin(); it != my_hash_map->end(); ++it) {
        (*callback)(context, (int32_t) it->first.size(), (int32_t) it->second.size(),
                    it->first.c_str(), it->second.c_str());
    }
}

void CMap::EachLike(const string& pattern, void* context, KVEachCallback* callback) {
    LOG("Each like pattern=" << pattern);
    try {
        std::regex p(pattern);
        for (auto it = my_hash_map->begin(); it != my_hash_map->end(); ++it) {
            auto key = string(it->first.c_str(), it->first.size());
            if (std::regex_match(key, p)) {
                (*callback)(context, (int32_t) it->first.size(), (int32_t) it->second.size(),
                            it->first.c_str(), it->second.c_str());
            }
        }
    } catch (std::regex_error) {
        LOG("Invalid pattern: " << pattern);
    }
}

KVStatus CMap::Exists(const string& key) {
    LOG("Exists for key=" << key);
    if (0 == my_hash_map->count(key_type(key))) {
        LOG("  key not found");
        return NOT_FOUND;
    }
    return OK;
}

void CMap::Get(void* context, const string& key, KVGetCallback* callback) {
    LOG("Get using callback for key=" << key);
    hash_map_type::const_accessor acc;
    if (!my_hash_map->find(acc, key_type(key))) {
        LOG("  key not found");
        return;
    }
    (*callback)(context, (int32_t) acc->second.size(), acc->second.c_str());
}

KVStatus CMap::Put(const string& key, const string& value) {
    LOG("Put key=" << key.c_str() << ", value.size=" << to_string(value.size()));
    hash_map_type::accessor res;
    if (!my_hash_map->insert(res, std::make_pair(key_type(key), mapped_type(value)))) {
        // Key already exist.
        // update value
        typename hash_map_type::value_type& entry = *res;
        transaction::manual tx(pmpool);
        conditional_add_to_tx(&(entry.second));
        entry.second = value;
        transaction::commit();
    }
    return OK;
}

KVStatus CMap::Remove(const string& key) {
    LOG("Remove key=" << key.c_str());
    if (my_hash_map->erase(key_type(key))) {
        return OK;
    }
    return NOT_FOUND;
}

void CMap::Recover() {
    auto root_data = pmpool.get_root();
    if (!(root_data->hash_map_ptr)) {
        make_persistent_atomic<hash_map_type>(pmpool, root_data->hash_map_ptr);
    }
    my_hash_map = root_data->hash_map_ptr.get();
}

} // namespace cmap
} // namespace pmemkv
