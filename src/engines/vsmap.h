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

#pragma once

#include "../engine.h"
#include "pmem_allocator.h"
#include <map>
#include <string>
#include <scoped_allocator>

namespace pmemkv {
namespace vsmap {

const std::string ENGINE = "vsmap";

class VSMap : public pmem::kv::EngineBase {
  public:
    VSMap(void* context, const std::string& path, size_t size);
    ~VSMap();

    std::string Engine() final { return ENGINE; }
    void* EngineContext() { return engine_context; }
    void All(void* context, KVAllCallback* callback) final;
    void AllAbove(void* context, const std::string& key, KVAllCallback* callback) final;
    void AllBelow(void* context, const std::string& key, KVAllCallback* callback) final;
    void AllBetween(void* context, const std::string& key1, const std::string& key2, KVAllCallback* callback) final;
    int64_t Count() final;
    int64_t CountAbove(const std::string& key) final;
    int64_t CountBelow(const std::string& key) final;
    int64_t CountBetween(const std::string& key1, const std::string& key2) final;
    void Each(void* context, KVEachCallback* callback) final;
    void EachAbove(void* context, const std::string& key, KVEachCallback* callback) final;
    void EachBelow(void* context, const std::string& key, KVEachCallback* callback) final;
    void EachBetween(void* context, const std::string& key1, const std::string& key2, KVEachCallback* callback) final;
    KVStatus Exists(const std::string& key) final;
    void Get(void* context, const std::string& key, KVGetCallback* callback) final;
    KVStatus Put(const std::string& key, const std::string& value) final;
    KVStatus Remove(const std::string& key) final;

    using KVEngine::All;
    using KVEngine::AllAbove;
    using KVEngine::AllBelow;
    using KVEngine::AllBetween;
    using KVEngine::Each;
    using KVEngine::EachAbove;
    using KVEngine::EachBelow;
    using KVEngine::EachBetween;
    using KVEngine::Get;
  private:
    using storage_type = std::basic_string<char, std::char_traits<char>, pmem::allocator<char> >;

    using key_type = storage_type;
    using mapped_type = storage_type;
    using map_allocator_type = pmem::allocator<std::pair<key_type, mapped_type> >;
    using map_type = std::map<key_type, mapped_type, std::less<key_type>, std::scoped_allocator_adaptor<map_allocator_type>>;

    void* engine_context;
    map_allocator_type kv_allocator;
    map_type pmem_kv_container;
};

} // namespace vsmap
} // namespace pmemkv
