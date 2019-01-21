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

#include "engines/blackhole.h"
#include "engines/kvtree3.h"
#include "engines/btree.h"
#include "engines/vmap.h"
#include "engines/vcmap.h"

using std::runtime_error;

namespace pmemkv {

KVEngine* KVEngine::Start(const string& engine, const string& config) {
    auto cb = [](void* context, const char* engine, const char* config, const char* msg) {
        throw runtime_error(msg);
    };
    return Start(nullptr, engine.c_str(), config.c_str(), cb);
}

KVEngine* KVEngine::Start(void* context, const char* engine, const char* config, KVStartFailureCallback* onfail) {
    try {
        if (engine == blackhole::ENGINE) {
            return new blackhole::Blackhole();
        } else {  // handle traditional engines expecting path & size params
            rapidjson::Document d;
            if (d.Parse(config).HasParseError()) {
                throw runtime_error("Config could not be parsed as JSON");
            } else if (!d.HasMember("path") || !d["path"].IsString()) {
                throw runtime_error("Config does not include valid path string");
            } else if (d.HasMember("size") && !d["size"].IsInt64()) {
                throw runtime_error("Config does not include valid size integer");
            }
            auto path = d["path"].GetString();
            size_t size = d.HasMember("size") ? (size_t) d["size"].GetInt64() : 1073741824;
            if (engine == kvtree3::ENGINE) {
                return new kvtree3::KVTree(path, size);
            } else if (engine == btree::ENGINE) {
                return new btree::BTree(path, size);
            } else if ((engine == vmap::ENGINE) || (engine == vcmap::ENGINE)) {
                struct stat info;
                if ((stat(path, &info) < 0) || !S_ISDIR(info.st_mode)) {
                    throw runtime_error("Config path is not an existing directory");
                } else if (engine == vmap::ENGINE) {
                    return new vmap::VMap(path, size);
                } else if (engine == vcmap::ENGINE) {
                    return new vcmap::VCMap(path, size);
                }
            }
        }
        throw runtime_error("Unknown engine name");
    } catch (std::exception& e) {
        (*onfail)(context, engine, config, e.what());
        return nullptr;
    }
}

void KVEngine::Stop(KVEngine* kv) {
    auto engine = kv->Engine();
    if (engine == blackhole::ENGINE) {
        delete (blackhole::Blackhole*) kv;
    } else if (engine == kvtree3::ENGINE) {
        delete (kvtree3::KVTree*) kv;
    } else if (engine == btree::ENGINE) {
        delete (btree::BTree*) kv;
    } else if (engine == vmap::ENGINE) {
        delete (vmap::VMap*) kv;
    } else if (engine == vcmap::ENGINE) {
        delete (vcmap::VCMap*) kv;
    }
}

void KVEngine::All(const std::function<KVAllFunction> f) {
    std::function<KVAllFunction> localf = f;
    auto cb = [](void* context, int32_t kb, const char* k) {
        const auto c = ((std::function<KVAllFunction>*) context);
        c->operator()(kb, k);
    };
    All(&localf, cb);
}

void KVEngine::Each(const std::function<KVEachFunction> f) {
    std::function<KVEachFunction> localf = f;
    auto cb = [](void* context, int32_t kb, const char* k, int32_t vb, const char* v) {
        const auto c = ((std::function<KVEachFunction>*) context);
        c->operator()(kb, k, vb, v);
    };
    Each(&localf, cb);
}

struct GetCallbackContext {
    KVStatus result;
    string* value;
};

KVStatus KVEngine::Get(const string& key, string* value) {
    GetCallbackContext cxt = {NOT_FOUND, value};
    auto cb = [](void* context, int32_t vb, const char* v) {
        const auto c = ((GetCallbackContext*) context);
        c->result = OK;
        c->value->append(v, vb);
    };
    Get(&cxt, key, cb);
    return cxt.result;
}

extern "C" KVEngine* kvengine_start(void* context, const char* engine, const char* config,
                                    KVStartFailureCallback* callback) {
    return KVEngine::Start(context, engine, config, callback);
}

extern "C" void kvengine_stop(KVEngine* kv) {
    return KVEngine::Stop(kv);
}

extern "C" void kvengine_all(KVEngine* kv, void* context, KVAllCallback* callback) {
    kv->All(context, callback);
}

extern "C" int64_t kvengine_count(KVEngine* kv) {
    return kv->Count();
}

extern "C" void kvengine_each(KVEngine* kv, void* context, KVEachCallback* callback) {
    kv->Each(context, callback);
}

extern "C" int8_t kvengine_exists(KVEngine* kv, int32_t keybytes, const char* key) {
    return kv->Exists(string(key, (size_t) keybytes));
}

extern "C" void kvengine_get(KVEngine* kv, void* context, const int32_t keybytes, const char* key,
                             KVGetCallback* callback) {
    kv->Get(context, string(key, (size_t) keybytes), callback);
}

struct GetCopyCallbackContext {
    KVStatus result;
    int32_t maxvaluebytes;
    char* value;
};

extern "C" int8_t kvengine_get_copy(KVEngine* kv, int32_t keybytes, const char* key,
                                    int32_t maxvaluebytes, char* value) {
    GetCopyCallbackContext cxt = {NOT_FOUND, maxvaluebytes, value};
    auto cb = [](void* context, int32_t vb, const char* v) {
        const auto c = ((GetCopyCallbackContext*) context);
        if (vb < c->maxvaluebytes) {
            c->result = OK;
            memcpy(c->value, v, vb);
        } else {
            c->result = FAILED;
        }
    };
    memset(value, 0, maxvaluebytes);
    kv->Get(&cxt, string(key, (size_t) keybytes), cb);
    return cxt.result;
}

extern "C" int8_t kvengine_put(KVEngine* kv, const int32_t keybytes, const char* key,
                               const int32_t valuebytes, const char* value) {
    return kv->Put(string(key, (size_t) keybytes), string(value, (size_t) valuebytes));
}

extern "C" int8_t kvengine_remove(KVEngine* kv, const int32_t keybytes, const char* key) {
    return kv->Remove(string(key, (size_t) keybytes));
}

// todo missing test cases for KVEngine static methods & extern C API

} // namespace pmemkv
