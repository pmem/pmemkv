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
#include <libpmemobj++/detail/common.hpp>

#include "versioned_b_tree.h"

#define DO_LOG 0
#define LOG(msg) if (DO_LOG) std::cout << "[btree] " << msg << "\n"

using pmem::obj::make_persistent_atomic;
using pmem::obj::transaction;
using pmem::detail::conditional_add_to_tx;

namespace pmemkv {
namespace versioned_b_tree {

BTreeEngine::BTreeEngine(const string& path, const size_t size) {
    if (path.find("/dev/dax") == 0) {
        LOG("Opening device dax pool, path=" << path);
        pmpool = pool<RootData>::open(path.c_str(), LAYOUT);
    } else if (access(path.c_str(), F_OK) != 0) {
        LOG("Creating filesystem pool, path=" << path << ", size=" << to_string(size));
        pmpool = pool<RootData>::create(path.c_str(), LAYOUT, size, S_IRWXU);
    } else {
        LOG("Opening filesystem pool, path=" << path);
        pmpool = pool<RootData>::open(path.c_str(), LAYOUT);
    }
    Recover();
}

BTreeEngine::~BTreeEngine() {
    LOG("Closing");
    pmpool.close();
    LOG("Closed ok");
}

KVStatus BTreeEngine::Get(const int32_t limit, const int32_t keybytes, int32_t* valuebytes,
                        const char* key, char* value) {
    LOG("Get for key=" << key);
    return NOT_FOUND;
}

KVStatus BTreeEngine::Get(const string& key, string* value) {
    LOG("Get for key=" << key.c_str());
    btree_type::iterator it = my_btree->find( pstring<20>(key) );
    if ( it == my_btree->end() ) {
        LOG("Key=" << key.c_str() << " not found");
        return NOT_FOUND;
    }
    value->append( it->second.c_str() );
    return OK;
}

KVStatus BTreeEngine::Put(const string& key, const string& value) {
    LOG("Put key=" << key.c_str() << ", value.size=" << to_string(value.size()));
    std::pair<typename btree_type::iterator, bool> res = my_btree->insert(std::make_pair(pstring<MAX_KEY_SIZE>(key), pstring<MAX_VALUE_SIZE>(value)));
    if(!res.second) { // Key already exist.
        // update value
        typename btree_type::value_type& entry = *res.first;
        transaction::manual tx( pmpool );
        conditional_add_to_tx(&(entry.second));
        entry.second = value;
        transaction::commit();
    }
    return OK;
}

KVStatus BTreeEngine::Remove(const string& key) {
    LOG("Remove key=" << key.c_str());
    return FAILED;
}

void BTreeEngine::Recover() {
    auto root_data = pmpool.get_root();

    if ( root_data->btree_ptr ) {
        my_btree = root_data->btree_ptr.get();
        my_btree->garbage_collection();
    } 
    else {
        make_persistent_atomic<btree_type>(pmpool, root_data->btree_ptr);
        my_btree = root_data->btree_ptr.get();
    }
}

} // namespace versioned_b_tree
} // namespace pmemkv
