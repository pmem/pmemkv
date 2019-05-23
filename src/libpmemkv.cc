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

#include <rapidjson/document.h>
#include <sys/stat.h>

#include "libpmemkv.h"
#include "engine.h"
#include "engines/blackhole.h"

#ifdef ENGINE_VSMAP
#include "engines/vsmap.h"
#endif

#ifdef ENGINE_VCMAP
#include "engines/vcmap.h"
#endif

#ifdef ENGINE_CMAP
#include "engines/cmap.h"
#endif

#ifdef ENGINE_CACHING
#include "engines-experimental/caching.h"
#endif

#ifdef ENGINE_STREE
#include "engines-experimental/stree.h"
#endif

#ifdef ENGINE_TREE3
#include "engines-experimental/tree3.h"
#endif

pmemkv_db* kvengine_start(void* context, const char* engine, const char* config, KVStartFailureCallback* onfail) {
    try {
        if (engine == pmemkv::blackhole::ENGINE) {
            return reinterpret_cast<pmemkv_db*>(new pmemkv::blackhole::Blackhole(context));
        }
#ifdef ENGINE_CACHING
        if (engine == pmemkv::caching::ENGINE) {
            return reinterpret_cast<pmemkv_db*>(new pmemkv::caching::CachingEngine(context, config));
        }
#endif
        // handle traditional engines expecting path & size params
        rapidjson::Document d;
        if (d.Parse(config).HasParseError()) {
            throw std::runtime_error("Config could not be parsed as JSON");
        } else if (!d.HasMember("path") || !d["path"].IsString()) {
            throw std::runtime_error("Config does not include valid path string");
        } else if (d.HasMember("size") && !d["size"].IsInt64()) {
            throw std::runtime_error("Config does not include valid size integer");
        }
        auto path = d["path"].GetString();
        size_t size = d.HasMember("size") ? (size_t) d["size"].GetInt64() : 1073741824;
#ifdef ENGINE_TREE3
        if (engine == pmemkv::tree3::ENGINE) {
            return reinterpret_cast<pmemkv_db*>(new pmemkv::tree3::Tree(context, path, size));
        }
#endif

#ifdef ENGINE_STREE
        if (engine == pmemkv::stree::ENGINE) {
            return reinterpret_cast<pmemkv_db*>(new pmemkv::stree::STree(context, path, size));
        }
#endif

#if defined(ENGINE_VSMAP) || defined(ENGINE_VCMAP)
        struct stat info;
        if ((stat(path, &info) < 0) || !S_ISDIR(info.st_mode)) {
            throw std::runtime_error("Config path is not an existing directory");
        }
#endif

#ifdef ENGINE_VSMAP
        if (engine == pmemkv::vsmap::ENGINE) {
            return reinterpret_cast<pmemkv_db*>(new pmemkv::vsmap::VSMap(context, path, size));
        }
#endif

#ifdef ENGINE_VCMAP
        if (engine == pmemkv::vcmap::ENGINE) {
            return reinterpret_cast<pmemkv_db*>(new pmemkv::vcmap::VCMap(context, path, size));
        }
#endif

#ifdef ENGINE_CMAP
        if (engine == pmemkv::cmap::ENGINE) {
            return reinterpret_cast<pmemkv_db*>(new pmemkv::cmap::CMap(context, path, size));
        }
#endif
        throw std::runtime_error("Unknown engine name");
    } catch (std::exception& e) {
        onfail(context, engine, config, e.what());
        return nullptr;
    }
}

void kvengine_stop(pmemkv_db* kv) {
    // XXX: move dangerous work to stop() method
    delete reinterpret_cast<pmemkv::engine_base*>(kv);
}

void kvengine_all(pmemkv_db* kv, void* context, KVAllCallback* c) {
    reinterpret_cast<pmemkv::engine_base*>(kv)->All(context, c);
}

void kvengine_all_above(pmemkv_db* kv, void* context, int32_t kb, const char* k, KVAllCallback* c) {
    reinterpret_cast<pmemkv::engine_base*>(kv)->AllAbove(context, std::string(k, (size_t) kb), c);
}

void kvengine_all_below(pmemkv_db* kv, void* context, int32_t kb, const char* k, KVAllCallback* c) {
    reinterpret_cast<pmemkv::engine_base*>(kv)->AllBelow(context, std::string(k, (size_t) kb), c);
}

void kvengine_all_between(pmemkv_db* kv, void* context, int32_t kb1, const char* k1,
                                     int32_t kb2, const char* k2, KVAllCallback* c) {
    reinterpret_cast<pmemkv::engine_base*>(kv)->AllBetween(context, std::string(k1, (size_t) kb1), std::string(k2, (size_t) kb2), c);
}

int64_t kvengine_count(pmemkv_db* kv) {
    return reinterpret_cast<pmemkv::engine_base*>(kv)->Count();
}

int64_t kvengine_count_above(pmemkv_db* kv, int32_t kb, const char* k) {
    return reinterpret_cast<pmemkv::engine_base*>(kv)->CountAbove(std::string(k, (size_t) kb));
}

int64_t kvengine_count_below(pmemkv_db* kv, int32_t kb, const char* k) {
    return reinterpret_cast<pmemkv::engine_base*>(kv)->CountBelow(std::string(k, (size_t) kb));
}

int64_t kvengine_count_between(pmemkv_db* kv, int32_t kb1, const char* k1, int32_t kb2, const char* k2) {
    return reinterpret_cast<pmemkv::engine_base*>(kv)->CountBetween(std::string(k1, (size_t) kb1), std::string(k2, (size_t) kb2));
}

void kvengine_each(pmemkv_db* kv, void* context, KVEachCallback* c) {
    reinterpret_cast<pmemkv::engine_base*>(kv)->Each(context, c);
}

void kvengine_each_above(pmemkv_db* kv, void* context, int32_t kb, const char* k, KVEachCallback* c) {
    reinterpret_cast<pmemkv::engine_base*>(kv)->EachAbove(context, std::string(k, (size_t) kb), c);
}

void kvengine_each_below(pmemkv_db* kv, void* context, int32_t kb, const char* k, KVEachCallback* c) {
    reinterpret_cast<pmemkv::engine_base*>(kv)->EachBelow(context, std::string(k, (size_t) kb), c);
}

void kvengine_each_between(pmemkv_db* kv, void* context, int32_t kb1, const char* k1,
                                      int32_t kb2, const char* k2, KVEachCallback* c) {
    reinterpret_cast<pmemkv::engine_base*>(kv)->EachBetween(context, std::string(k1, (size_t) kb1), std::string(k2, (size_t) kb2), c);
}

int8_t kvengine_exists(pmemkv_db* kv, int32_t kb, const char* k) {
    return reinterpret_cast<pmemkv::engine_base*>(kv)->Exists(std::string(k, (size_t) kb));
}

void kvengine_get(pmemkv_db* kv, void* context, const int32_t kb, const char* k, KVGetCallback* c) {
    reinterpret_cast<pmemkv::engine_base*>(kv)->Get(context, std::string(k, (size_t) kb), c);
}

struct GetCopyCallbackContext {
    KVStatus result;
    int32_t maxvaluebytes;
    char* value;
};

int8_t kvengine_get_copy(pmemkv_db* kv, int32_t kb, const char* k, int32_t maxvaluebytes, char* value) {
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
    reinterpret_cast<pmemkv::engine_base*>(kv)->Get(&cxt, std::string(k, (size_t) kb), cb);
    return cxt.result;
}

int8_t kvengine_put(pmemkv_db* kv, const int32_t kb, const char* k, const int32_t vb, const char* v) {
    return reinterpret_cast<pmemkv::engine_base*>(kv)->Put(std::string(k, (size_t) kb), std::string(v, (size_t) vb));
}

int8_t kvengine_remove(pmemkv_db* kv, const int32_t kb, const char* k) {
    return reinterpret_cast<pmemkv::engine_base*>(kv)->Remove(std::string(k, (size_t) kb));
}

void *kvengine_engine_context(pmemkv_db* kv) {
    return reinterpret_cast<pmemkv::engine_base*>(kv)->EngineContext();
}
