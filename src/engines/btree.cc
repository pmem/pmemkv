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
#include <unistd.h>

#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>

#include "btree.h"

#define DO_LOG 0
#define LOG(msg) if (DO_LOG) std::cout << "[btree] " << msg << "\n"

using pmem::obj::make_persistent_atomic;
using pmem::obj::transaction;
using pmem::detail::conditional_add_to_tx;

namespace pmemkv {
namespace btree {

BTree::BTree(const string& path, const size_t size) {
    if ((access(path.c_str(), F_OK) != 0) && (size > 0)) {
        LOG("Creating filesystem pool, path=" << path << ", size=" << to_string(size));
        pmpool = pool<RootData>::create(path.c_str(), LAYOUT, size, S_IRWXU);
    } else {
        LOG("Opening pool, path=" << path);
        pmpool = pool<RootData>::open(path.c_str(), LAYOUT);
    }
    Recover();
    LOG("Started ok");
}

BTree::~BTree() {
    LOG("Stopping");
    pmpool.close();
    LOG("Stopped ok");
}

void BTree::All(void* context, KVAllCallback* callback) {
    LOG("All");
    for (auto& iterator : *my_btree) {
        (*callback)(context, (int32_t) iterator.first.size(), iterator.first.c_str());
    }
}

int64_t BTree::Count() {
    int64_t result = 0;
    for (auto& iterator : *my_btree) result++;
    return result;
}

void BTree::Each(void* context, KVEachCallback* callback) {
    LOG("Each");
    for (auto& iterator : *my_btree) {
        (*callback)(context, (int32_t) iterator.first.size(), iterator.first.c_str(),
                    (int32_t) iterator.second.size(), iterator.second.c_str());
    }
}

KVStatus BTree::Exists(const string& key) {
    LOG("Exists for key=" << key);
    btree_type::iterator it = my_btree->find(pstring<20>(key));
    if (it == my_btree->end()) {
        LOG("  key not found");
        return NOT_FOUND;
    }
    return OK;
}

void BTree::Get(void* context, const string& key, KVGetCallback* callback) {
    LOG("Get using callback for key=" << key);
    btree_type::iterator it = my_btree->find(pstring<20>(key));
    if (it == my_btree->end()) {
        LOG("  key not found");
        return;
    }
    (*callback)(context, (int32_t) it->second.size(), it->second.c_str());
}

KVStatus BTree::Put(const string& key, const string& value) {
    LOG("Put key=" << key << ", value.size=" << to_string(value.size()));
    try {
        auto result = my_btree->insert(std::make_pair(pstring<MAX_KEY_SIZE>(key), pstring<MAX_VALUE_SIZE>(value)));
        if (!result.second) { // key already exists, so update
            typename btree_type::value_type& entry = *result.first;
            transaction::manual tx(pmpool);
            conditional_add_to_tx(&(entry.second));
            entry.second = value;
            transaction::commit();
        }
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

KVStatus BTree::Remove(const string& key) {
    LOG("Remove key=" << key);
    try {
        auto result = my_btree->erase(key);
        return (result == 1) ? OK : NOT_FOUND;
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

void BTree::Recover() {
    auto root_data = pmpool.root();
    if (root_data->btree_ptr) {
        my_btree = root_data->btree_ptr.get();
        my_btree->garbage_collection();
    } else {
        make_persistent_atomic<btree_type>(pmpool, root_data->btree_ptr);
        my_btree = root_data->btree_ptr.get();
    }
}

} // namespace btree
} // namespace pmemkv
