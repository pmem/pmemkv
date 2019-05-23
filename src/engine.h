/*
 * Copyright 2019, Intel Corporation
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

#ifndef LIBPMEMKV_ENGINE_H
#define LIBPMEMKV_ENGINE_H

#include <string>
#include <functional>

#include "libpmemkv.h"

namespace pmemkv {

using status = KVStatus;

const std::string LAYOUT = "pmemkv";

typedef void AllCallback(void* context, int keybytes, const char* key);
typedef void EachCallback(void* context, int keybytes, const char* key, int valuebytes, const char* value);
typedef void GetCallback(void* context, int valuebytes, const char* value);

class engine_base {
  public:
    engine_base() {}

    virtual ~engine_base() {}

    virtual std::string Engine() = 0;

    virtual void* EngineContext() = 0;

    virtual void All(void* context, AllCallback* callback) = 0;

    virtual void AllAbove(void* context, const std::string& key, AllCallback* callback) = 0;

    virtual void AllBelow(void* context, const std::string& key, AllCallback* callback) = 0;

    virtual void AllBetween(void* context, const std::string& key1, const std::string& key2, AllCallback* callback) = 0;

    virtual int64_t Count() = 0;

    virtual int64_t CountAbove(const std::string& key) = 0;

    virtual int64_t CountBelow(const std::string& key) = 0;

    virtual int64_t CountBetween(const std::string& key1, const std::string& key2) = 0;

    virtual void Each(void* context, EachCallback* callback) = 0;

    virtual void EachAbove(void* context, const std::string& key, EachCallback* callback) = 0;

    virtual void EachBelow(void* context, const std::string& key, EachCallback* callback) = 0;

    virtual void EachBetween(void* context, const std::string& key1, const std::string& key2, EachCallback* callback) = 0;

    virtual status Exists(const std::string& key) = 0;

    virtual void Get(void* context, const std::string& key, GetCallback* callback) = 0;

    virtual status Put(const std::string& key, const std::string& value) = 0;

    virtual status Remove(const std::string& key) = 0;
};

} /* namespace pmemkv */

#endif /* LIBPMEMKV_ENGINE_H */
