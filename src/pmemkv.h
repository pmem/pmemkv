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

const string LAYOUT = "pmemkv";

class KVEngine {
  public:
    static KVEngine* Start(const string& engine, const string& config);
    static KVEngine* Start(void* context, const char* engine, const char* config, KVStartFailureCallback* callback);
    static void Stop(KVEngine* kv);

    virtual string Engine() = 0;

    virtual void All(void* context, KVAllCallback* callback) = 0;
    inline void All(KVAllCallback* callback) { All(nullptr, callback); }
    void All(std::function<KVAllFunction> f);

    virtual int64_t Count() = 0;

    virtual void Each(void* context, KVEachCallback* callback) = 0;
    inline void Each(KVEachCallback* callback) { Each(nullptr, callback); }
    void Each(std::function<KVEachFunction> f);

    virtual KVStatus Exists(const string& key) = 0;

    virtual void Get(void* context, const string& key, KVGetCallback* callback) = 0;
    inline void Get(const string& key, KVGetCallback* callback) { Get(nullptr, key, callback); }
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
void kvengine_all(KVEngine* kv, void* context, KVAllCallback* callback);
int64_t kvengine_count(KVEngine* kv);
void kvengine_each(KVEngine* kv, void* context, KVEachCallback* callback);
int8_t kvengine_exists(KVEngine* kv, int32_t keybytes, const char* key);
void kvengine_get(KVEngine* kv, void* context, int32_t keybytes, const char* key, KVGetCallback* callback);
int8_t kvengine_get_copy(KVEngine* kv, int32_t keybytes, const char* key, int32_t maxvaluebytes, char* value);
int8_t kvengine_put(KVEngine* kv, int32_t keybytes, const char* key, int32_t valuebytes, const char* value);
int8_t kvengine_remove(KVEngine* kv, int32_t keybytes, const char* key);

#ifdef __cplusplus
}

} // namespace pmemkv
#endif
