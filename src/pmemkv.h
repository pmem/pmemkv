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

typedef enum {
    FAILED = -1,
    NOT_FOUND = 0,
    OK = 1
} KVStatus;

typedef void(KVAllCallback)(void* context, int keybytes, const char* key);
typedef void(KVAllFunction)(int keybytes, const char* key);
typedef void(KVEachCallback)(void* context, int keybytes, const char* key, int valuebytes, const char* value);
typedef void(KVEachFunction)(int keybytes, const char* key, int valuebytes, const char* value);
typedef void(KVGetCallback)(void* context, int valuebytes, const char* value);
typedef void(KVGetFunction)(int valuebytes, const char* value);
typedef void(KVStartFailureCallback)(void* context, const char* engine, const char* config, const char* msg);

#ifdef __cplusplus

#include <string>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include "rapidjson/document.h"

using std::string;
using std::to_string;

namespace pmemkv {

typedef void(KVAllStringFunction)(const string& key);
typedef void(KVEachStringFunction)(const string& key, const string& value);
typedef void(KVGetStringFunction)(const string& value);

const string LAYOUT = "pmemkv";

class KVEngine {
  public:
    static KVEngine* Start(void* context, const char* engine, const char* config, KVStartFailureCallback* callback);
    static KVEngine* Start(void* context, const string& engine, const string& config);
    static KVEngine* Start(const string& engine, const string& config);
    static void Stop(KVEngine* kv);

    virtual string Engine() = 0;
    virtual void* EngineContext() = 0;

    virtual void All(void* context, KVAllCallback* callback) = 0;
    void All(std::function<KVAllFunction> f);
    void All(std::function<KVAllStringFunction> f);

    virtual void AllAbove(void* context, const string& key, KVAllCallback* callback) = 0;
    void AllAbove(const string& key, std::function<KVAllFunction> f);
    void AllAbove(const string& key, std::function<KVAllStringFunction> f);

    virtual void AllBelow(void* context, const string& key, KVAllCallback* callback) = 0;
    void AllBelow(const string& key, std::function<KVAllFunction> f);
    void AllBelow(const string& key, std::function<KVAllStringFunction> f);

    virtual void AllBetween(void* context, const string& key1, const string& key2, KVAllCallback* callback) = 0;
    void AllBetween(const string& key1, const string& key2, std::function<KVAllFunction> f);
    void AllBetween(const string& key1, const string& key2, std::function<KVAllStringFunction> f);

    virtual int64_t Count() = 0;
    virtual int64_t CountAbove(const string& key) = 0;
    virtual int64_t CountBelow(const string& key) = 0;
    virtual int64_t CountBetween(const string& key1, const string& key2) = 0;

    virtual void Each(void* context, KVEachCallback* callback) = 0;
    void Each(std::function<KVEachFunction> f);
    void Each(std::function<KVEachStringFunction> f);

    virtual void EachAbove(void* context, const string& key, KVEachCallback* callback) = 0;
    void EachAbove(const string& key, std::function<KVEachFunction> f);
    void EachAbove(const string& key, std::function<KVEachStringFunction> f);

    virtual void EachBelow(void* context, const string& key, KVEachCallback* callback) = 0;
    void EachBelow(const string& key, std::function<KVEachFunction> f);
    void EachBelow(const string& key, std::function<KVEachStringFunction> f);

    virtual void EachBetween(void* context, const string& key1, const string& key2, KVEachCallback* callback) = 0;
    void EachBetween(const string& key1, const string& key2, std::function<KVEachFunction> f);
    void EachBetween(const string& key1, const string& key2, std::function<KVEachStringFunction> f);

    virtual KVStatus Exists(const string& key) = 0;

    virtual void Get(void* context, const string& key, KVGetCallback* callback) = 0;
    void Get(const string& key, std::function<KVGetFunction> f);
    void Get(const string& key, std::function<KVGetStringFunction> f);
    KVStatus Get(const string& key, string* value);

    virtual KVStatus Put(const string& key, const string& value) = 0;
    virtual KVStatus Remove(const string& key) = 0;
};

extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

struct KVEngine;
typedef struct KVEngine KVEngine;

KVEngine* kvengine_start(void* context, const char* engine, const char* config, KVStartFailureCallback* callback);
void kvengine_stop(KVEngine* kv);

void kvengine_all(KVEngine* kv, void* context, KVAllCallback* c);
void kvengine_all_above(KVEngine* kv, void* context, int32_t kb, const char* k, KVAllCallback* c);
void kvengine_all_below(KVEngine* kv, void* context, int32_t kb, const char* k, KVAllCallback* c);
void kvengine_all_between(KVEngine* kv, void* context, int32_t kb1, const char* k1,
                          int32_t kb2, const char* k2, KVAllCallback* c);

int64_t kvengine_count(KVEngine* kv);
int64_t kvengine_count_above(KVEngine* kv, int32_t kb, const char* k);
int64_t kvengine_count_below(KVEngine* kv, int32_t kb, const char* k);
int64_t kvengine_count_between(KVEngine* kv, int32_t kb1, const char* k1, int32_t kb2, const char* k2);

void kvengine_each(KVEngine* kv, void* context, KVEachCallback* c);
void kvengine_each_above(KVEngine* kv, void* context, int32_t kb, const char* k, KVEachCallback* c);
void kvengine_each_below(KVEngine* kv, void* context, int32_t kb, const char* k, KVEachCallback* c);
void kvengine_each_between(KVEngine* kv, void* context, int32_t kb1, const char* k1,
                           int32_t kb2, const char* k2, KVEachCallback* c);

int8_t kvengine_exists(KVEngine* kv, int32_t kb, const char* k);
void kvengine_get(KVEngine* kv, void* context, int32_t kb, const char* k, KVGetCallback* c);
int8_t kvengine_get_copy(KVEngine* kv, int32_t kb, const char* k, int32_t maxvaluebytes, char* value);
int8_t kvengine_put(KVEngine* kv, int32_t kb, const char* k, int32_t vb, const char* v);
int8_t kvengine_remove(KVEngine* kv, int32_t kb, const char* k);

#ifdef __cplusplus
}

} // namespace pmemkv
#endif
