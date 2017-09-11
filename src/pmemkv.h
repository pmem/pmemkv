/*
 * Copyright 2017, Intel Corporation
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
    static KVEngine* Open(const string& engine,            // open specified engine
                          const string& path,              // path to persistent pool
                          size_t size);                    // size of persistent pool
    static void Close(KVEngine* kv);                       // close specified engine

    virtual string Engine() = 0;                           // engine identifier
    virtual KVStatus Get(const string& key,                // copy value for key to buffer
                         int32_t limit,                    // maximum bytes to copy to buffer
                         char* value,                      // binary-safe value buffer
                         int32_t* valuebytes) = 0;         // buffer bytes actually copied
    virtual KVStatus Get(const string& key,                // append value for key to std::string
                         string* value) = 0;
    virtual KVStatus Put(const string& key,                // copy value for key from std::string
                         const string& value) = 0;
    virtual KVStatus Remove(const string& key) = 0;        // remove value for key
};

extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
struct KVEngine;
typedef struct KVEngine KVEngine;
KVEngine* kvengine_open(const char* engine,                // open requested engine
                        const char* path,                  // path to persistent pool
                        size_t size);                      // size of persistent pool

void kvengine_close(KVEngine* kv);                         // close engine instance

int8_t kvengine_get(KVEngine* kv,                          // copy value for key to buffer
                    const char* key,                       // key as C-style string
                    int32_t limit,                         // maximum bytes to copy to buffer
                    char* value,                           // binary-safe value buffer
                    int32_t* valuebytes);                  // buffer bytes actually copied

int8_t kvengine_put(KVEngine* kv,                          // copy value for key from buffer
                    const char* key,                       // key as C-style string
                    const char* value,                     // binary-safe value buffer
                    const int32_t* valuebytes);            // buffer bytes available to copy

int8_t kvengine_remove(KVEngine* kv,                       // remove value for key
                       const char* key);                   // key as C-style string

#ifdef __cplusplus
}

} // namespace pmemkv
#endif
