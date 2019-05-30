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

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <rapidjson/document.h>

#include "engines/blackhole.h"
#include "engines/vsmap.h"
#include "engines/vcmap.h"
#include "engines/cmap.h"
#ifdef EXPERIMENTAL
#include "engines-experimental/tree3.h"
#include "engines-experimental/stree.h"
#include "engines-experimental/caching.h"
#endif

using std::runtime_error;

namespace pmemkv {

KVEngine::~KVEngine() { }

// STATIC METHOD IMPLEMENTATIONS

KVEngine* KVEngine::Start(const std::string& engine, const std::string& config) {
    return Start(nullptr, engine, config);
}

KVEngine* KVEngine::Start(void* context, const std::string& engine, const std::string& config) {
    auto cb = [](void* cxt, const char* engine, const char* config, const char* msg) {
        throw runtime_error(msg);
    };
    return Start(context, engine.c_str(), config.c_str(), cb);
}

KVEngine* KVEngine::Start(void* context, const char* engine, const char* config, KVStartFailureCallback* onfail) {
    try {
        if (engine == blackhole::ENGINE) {
            return new blackhole::Blackhole(context);
#ifdef EXPERIMENTAL
        } else if (engine == caching::ENGINE) {
            return new caching::CachingEngine(context, config);
#endif
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
#ifdef EXPERIMENTAL
            if (engine == tree3::ENGINE) {
                return new tree3::Tree(context, path, size);
            } else if (engine == stree::ENGINE) {
                return new stree::STree(context, path, size);
            } else if ((engine == vsmap::ENGINE) || (engine == vcmap::ENGINE)) {
#else
	    if ((engine == vsmap::ENGINE) || (engine == vcmap::ENGINE)) {
#endif
		struct stat info;
                if ((stat(path, &info) < 0) || !S_ISDIR(info.st_mode)) {
                    throw runtime_error("Config path is not an existing directory");
                } else if (engine == vsmap::ENGINE) {
                    return new vsmap::VSMap(context, path, size);
                } else if (engine == vcmap::ENGINE) {
                    return new vcmap::VCMap(context, path, size);
                }
            } else if (engine == cmap::ENGINE) {
                return new cmap::CMap(context, path, size);
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
#ifdef EXPERIMENTAL
    } else if (engine == tree3::ENGINE) {
        delete (tree3::Tree*) kv;
    } else if (engine == stree::ENGINE) {
        delete (stree::STree*) kv;
    } else if (engine == caching::ENGINE) {
        delete (caching::CachingEngine*) kv;
#endif
    } else if (engine == vsmap::ENGINE) {
        delete (vsmap::VSMap*) kv;
    } else if (engine == vcmap::ENGINE) {
        delete (vcmap::VCMap*) kv;
    } else if (engine == cmap::ENGINE) {
        delete (cmap::CMap*) kv;
    }
}

// API METHOD IMPLEMENTATIONS

auto cb_all_function = [](void* context, int32_t kb, const char* k) {
    const auto c = ((std::function<KVAllFunction>*) context);
    c->operator()(kb, k);
};

auto cb_all_string_function = [](void* context, int32_t kb, const char* k) {
    const auto c = ((std::function<KVAllStringFunction>*) context);
    c->operator()(std::string(k, kb));
};

auto cb_each_function = [](void* context, int32_t kb, const char* k, int32_t vb, const char* v) {
    const auto c = ((std::function<KVEachFunction>*) context);
    c->operator()(kb, k, vb, v);
};

auto cb_each_string_function = [](void* context, int32_t kb, const char* k, int32_t vb, const char* v) {
    const auto c = ((std::function<KVEachStringFunction>*) context);
    c->operator()(std::string(k, kb), std::string(v, vb));
};

void KVEngine::All(const std::function<KVAllFunction> f) {
    auto localf = f;
    All(&localf, cb_all_function);
}

void KVEngine::All(const std::function<KVAllStringFunction> f) {
    auto localf = f;
    All(&localf, cb_all_string_function);
}

void KVEngine::AllAbove(const std::string& key, const std::function<KVAllFunction> f) {
    auto localf = f;
    AllAbove(&localf, key, cb_all_function);
}

void KVEngine::AllAbove(const std::string& key, const std::function<KVAllStringFunction> f) {
    auto localf = f;
    AllAbove(&localf, key, cb_all_string_function);
}

void KVEngine::AllBelow(const std::string& key, const std::function<KVAllFunction> f) {
    auto localf = f;
    AllBelow(&localf, key, cb_all_function);
}

void KVEngine::AllBelow(const std::string& key, const std::function<KVAllStringFunction> f) {
    auto localf = f;
    AllBelow(&localf, key, cb_all_string_function);
}

void KVEngine::AllBetween(const std::string& key1, const std::string& key2, const std::function<KVAllFunction> f) {
    auto localf = f;
    AllBetween(&localf, key1, key2, cb_all_function);
}

void KVEngine::AllBetween(const std::string& key1, const std::string& key2, const std::function<KVAllStringFunction> f) {
    auto localf = f;
    AllBetween(&localf, key1, key2, cb_all_string_function);
}

void KVEngine::Each(const std::function<KVEachFunction> f) {
    auto localf = f;
    Each(&localf, cb_each_function);
}

void KVEngine::Each(const std::function<KVEachStringFunction> f) {
    auto localf = f;
    Each(&localf, cb_each_string_function);
}

void KVEngine::EachAbove(const std::string& key, const std::function<KVEachFunction> f) {
    auto localf = f;
    EachAbove(&localf, key, cb_each_function);
}

void KVEngine::EachAbove(const std::string& key, const std::function<KVEachStringFunction> f) {
    auto localf = f;
    EachAbove(&localf, key, cb_each_string_function);
}

void KVEngine::EachBelow(const std::string& key, const std::function<KVEachFunction> f) {
    auto localf = f;
    EachBelow(&localf, key, cb_each_function);
}

void KVEngine::EachBelow(const std::string& key, const std::function<KVEachStringFunction> f) {
    auto localf = f;
    EachBelow(&localf, key, cb_each_string_function);
}

void KVEngine::EachBetween(const std::string& key1, const std::string& key2, const std::function<KVEachFunction> f) {
    auto localf = f;
    EachBetween(&localf, key1, key2, cb_each_function);
}

void KVEngine::EachBetween(const std::string& key1, const std::string& key2, const std::function<KVEachStringFunction> f) {
    auto localf = f;
    EachBetween(&localf, key1, key2, cb_each_string_function);
}

struct GetCallbackContext {
    KVStatus result;
    std::string* value;
};

KVStatus KVEngine::Get(const std::string& key, std::string* value) {
    GetCallbackContext cxt = {NOT_FOUND, value};
    auto cb = [](void* context, int32_t vb, const char* v) {
        const auto c = ((GetCallbackContext*) context);
        c->result = OK;
        c->value->append(v, vb);
    };
    Get(&cxt, key, cb);
    return cxt.result;
}

void KVEngine::Get(const std::string& key, std::function<KVGetFunction> f) {
    std::function<KVGetFunction> localf = f;
    auto cb = [](void* context, int vb, const char* v) {
        const auto c = ((std::function<KVGetFunction>*) context);
        c->operator()(vb, v);
    };
    Get(&localf, key, cb);
}

void KVEngine::Get(const std::string& key, std::function<KVGetStringFunction> f) {
    std::function<KVGetStringFunction> localf = f;
    auto cb = [](void* context, int vb, const char* v) {
        const auto c = ((std::function<KVGetStringFunction>*) context);
        c->operator()(std::string(v, vb));
    };
    Get(&localf, key, cb);
}

// EXTERN C IMPLEMENTATION

extern "C" KVEngine* kvengine_start(void* context, const char* engine, const char* config,
                                    KVStartFailureCallback* callback) {
    return KVEngine::Start(context, engine, config, callback);
}

extern "C" void kvengine_stop(KVEngine* kv) {
    return KVEngine::Stop(kv);
}

extern "C" void kvengine_all(KVEngine* kv, void* context, KVAllCallback* c) {
    kv->All(context, c);
}

extern "C" void kvengine_all_above(KVEngine* kv, void* context, int32_t kb, const char* k, KVAllCallback* c) {
    kv->AllAbove(context, std::string(k, (size_t) kb), c);
}

extern "C" void kvengine_all_below(KVEngine* kv, void* context, int32_t kb, const char* k, KVAllCallback* c) {
    kv->AllBelow(context, std::string(k, (size_t) kb), c);
}

extern "C" void kvengine_all_between(KVEngine* kv, void* context, int32_t kb1, const char* k1,
                                     int32_t kb2, const char* k2, KVAllCallback* c) {
    kv->AllBetween(context, std::string(k1, (size_t) kb1), std::string(k2, (size_t) kb2), c);
}

extern "C" int64_t kvengine_count(KVEngine* kv) {
    return kv->Count();
}

extern "C" int64_t kvengine_count_above(KVEngine* kv, int32_t kb, const char* k) {
    return kv->CountAbove(std::string(k, (size_t) kb));
}

extern "C" int64_t kvengine_count_below(KVEngine* kv, int32_t kb, const char* k) {
    return kv->CountBelow(std::string(k, (size_t) kb));
}

extern "C" int64_t kvengine_count_between(KVEngine* kv, int32_t kb1, const char* k1, int32_t kb2, const char* k2) {
    return kv->CountBetween(std::string(k1, (size_t) kb1), std::string(k2, (size_t) kb2));
}

extern "C" void kvengine_each(KVEngine* kv, void* context, KVEachCallback* c) {
    kv->Each(context, c);
}

extern "C" void kvengine_each_above(KVEngine* kv, void* context, int32_t kb, const char* k, KVEachCallback* c) {
    kv->EachAbove(context, std::string(k, (size_t) kb), c);
}

extern "C" void kvengine_each_below(KVEngine* kv, void* context, int32_t kb, const char* k, KVEachCallback* c) {
    kv->EachBelow(context, std::string(k, (size_t) kb), c);
}

extern "C" void kvengine_each_between(KVEngine* kv, void* context, int32_t kb1, const char* k1,
                                      int32_t kb2, const char* k2, KVEachCallback* c) {
    kv->EachBetween(context, std::string(k1, (size_t) kb1), std::string(k2, (size_t) kb2), c);
}

extern "C" int8_t kvengine_exists(KVEngine* kv, int32_t kb, const char* k) {
    return kv->Exists(std::string(k, (size_t) kb));
}

extern "C" void kvengine_get(KVEngine* kv, void* context, const int32_t kb, const char* k, KVGetCallback* c) {
    kv->Get(context, std::string(k, (size_t) kb), c);
}

struct GetCopyCallbackContext {
    KVStatus result;
    int32_t maxvaluebytes;
    char* value;
};

extern "C" int8_t kvengine_get_copy(KVEngine* kv, int32_t kb, const char* k, int32_t maxvaluebytes, char* value) {
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
    kv->Get(&cxt, std::string(k, (size_t) kb), cb);
    return cxt.result;
}

extern "C" int8_t kvengine_put(KVEngine* kv, const int32_t kb, const char* k, const int32_t vb, const char* v) {
    return kv->Put(std::string(k, (size_t) kb), std::string(v, (size_t) vb));
}

extern "C" int8_t kvengine_remove(KVEngine* kv, const int32_t kb, const char* k) {
    return kv->Remove(std::string(k, (size_t) kb));
}

// todo missing test cases for KVEngine static methods & extern C API

} // namespace pmemkv
