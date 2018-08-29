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

typedef enum {                                             // status enumeration
    FAILED = -1,                                           // operation failed
    NOT_FOUND = 0,                                         // key not located
    OK = 1                                                 // successful completion
} KVStatus;

typedef void(KVEachCallback)(void* context,                // callback function for Each operation
                             int keybytes,
                             int valuebytes,
                             const char* key,
                             const char* value);

typedef void(KVGetCallback)(void* context,                 // callback function for Get operation
                            int valuebytes,
                            const char* value);

#ifdef __cplusplus

#include <string>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using std::string;
using std::to_string;

namespace pmemkv {

const string LAYOUT = "pmemkv";                            // pool layout identifier

class KVEngine {                                           // storage engine implementations
  public:
    static KVEngine* Open(const string& engine,            // open storage engine
                          const string& path,              // path to persistent pool
                          size_t size);                    // size used when creating pool
    static void Close(KVEngine* kv);                       // close storage engine

    virtual string Engine() = 0;                           // engine identifier

    virtual int64_t Count() = 0;                           // count all keys
    virtual int64_t CountLike(const string& pattern) = 0;  // count all keys matching pattern

    inline void Each(KVEachCallback* callback) {           // iterate over all keys
        Each(nullptr, callback);
    }
    virtual void Each(void* context,                       // iterate over all keys with context
                      KVEachCallback* callback) = 0;
    inline void EachLike(const string& pattern,            // iterate over matching keys
                         KVEachCallback* callback) {
        EachLike(pattern, nullptr, callback);
    }
    virtual void EachLike(const string& pattern,           // iterate over matching keys with context
                          void* context,
                          KVEachCallback* callback) = 0;

    virtual KVStatus Exists(const string& key) = 0;        // does key have a value?

    inline void Get(const string& key,                     // pass value to callback
                    KVGetCallback* callback) {
        Get(nullptr, key, callback);
    }
    virtual void Get(void* context,                        // pass value to callback with context
                     const string& key,
                     KVGetCallback* callback) = 0;
    KVStatus Get(const string& key, string* value);        // append value to string

    virtual KVStatus Put(const string& key,                // store key and value
                         const string& value) = 0;
    virtual KVStatus Remove(const string& key) = 0;        // remove value for key
};

extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

struct KVEngine;                                           // define types as simple structs
typedef struct KVEngine KVEngine;

KVEngine* kvengine_open(const char* engine,                // open storage engine
                        const char* path,
                        size_t size);

void kvengine_close(KVEngine* kv);                         // close storage engine

int64_t kvengine_count(KVEngine* kv);                      // count all keys

int64_t kvengine_count_like(KVEngine* kv,                  // count all keys matching pattern
                            int32_t patternbytes,
                            const char* pattern);

void kvengine_each(KVEngine* kv,                           // iterate over all keys
                   void* context,
                   KVEachCallback* callback);

void kvengine_each_like(KVEngine* kv,                      // iterate over matching keys
                        int32_t patternbytes,
                        const char* pattern,
                        void* context,
                        KVEachCallback* callback);

int8_t kvengine_exists(KVEngine* kv,                       // does key have a value?
                       int32_t keybytes,
                       const char* key);

int8_t kvengine_exists_like(KVEngine* kv,                  // does pattern have a match?
                            int32_t patternbytes,
                            const char* pattern);

void kvengine_get(KVEngine* kv,                            // pass value to callback with context
                  void* context,
                  int32_t keybytes,
                  const char* key,
                  KVGetCallback* callback);

int8_t kvengine_put(KVEngine* kv,                          // store key and value
                    int32_t keybytes,
                    int32_t valuebytes,
                    const char* key,
                    const char* value);

int8_t kvengine_remove(KVEngine* kv,                       // remove value for key
                       int32_t keybytes,
                       const char* key);

#ifdef __cplusplus
}

} // namespace pmemkv
#endif
