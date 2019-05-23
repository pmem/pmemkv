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

#include <libpmemkv.h>

#include "engines/blackhole.h"
#include "engine.h"
//#include "engines/vsmap.h"
//#include "engines/vcmap.h"
//#include "engines/cmap.h"
#ifdef EXPERIMENTAL
#include "engines-experimental/tree3.h"
#include "engines-experimental/stree.h"
#include "engines-experimental/caching.h"
#endif

using pmemkv_engine = pmem::kv::EngineBase;

// XXX: remove start failure callback - instead use out parameter for
// pmemkv_engine and return status.
pmemkv_engine* pmemkv_start(void* context, const char* engine, const char* config,
                                    KVStartFailureCallback* callback) {
    try {
        // XXX: use enums instead of strings / factory method
        // XXX: create config structure
        if (std::string(engine) == "blackhole") {
            return new pmem::kv::Blackhole();
            /*
#ifdef EXPERIMENTAL
        } else if (engine == "caching") {
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
            if (engine == "tee3") {
                return new tree3::Tree(context, path, size);
            } else if (engine == stree::ENGINE) {
                return new stree::STree(context, path, size);
            } else if ((engine == vsmap::ENGINE) || (engine == vcmap::ENGINE)) {
#else
	    if ((engine == "vsmap") || (engine == "vcmap")) {
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
        */
        }
    } catch (std::exception& e) {
        (*callback)(context, engine, config, e.what());
        return nullptr;
    }
}

void pmemkv_stop(pmemkv_engine* kv) {
    // XXX: move dangerous work to stop() method
    delete kv;
}

void pmemkv_all(pmemkv_engine* kv, void* context, KVAllCallback* c) {
    kv->All(context, c);
}

void pmemkv_all_above(pmemkv_engine* kv, void* context, int32_t kb, const char* k, KVAllCallback* c) {
    kv->AllAbove(context, std::string(k, (size_t) kb), c);
}

void pmemkv_all_below(pmemkv_engine* kv, void* context, int32_t kb, const char* k, KVAllCallback* c) {
    kv->AllBelow(context, std::string(k, (size_t) kb), c);
}

void pmemkv_all_between(pmemkv_engine* kv, void* context, int32_t kb1, const char* k1,
                                     int32_t kb2, const char* k2, KVAllCallback* c) {
    kv->AllBetween(context, std::string(k1, (size_t) kb1), std::string(k2, (size_t) kb2), c);
}

int64_t pmemkv_count(pmemkv_engine* kv) {
    return kv->Count();
}

int64_t pmemkv_count_above(pmemkv_engine* kv, int32_t kb, const char* k) {
    return kv->CountAbove(std::string(k, (size_t) kb));
}

int64_t pmemkv_count_below(pmemkv_engine* kv, int32_t kb, const char* k) {
    return kv->CountBelow(std::string(k, (size_t) kb));
}

int64_t pmemkv_count_between(pmemkv_engine* kv, int32_t kb1, const char* k1, int32_t kb2, const char* k2) {
    return kv->CountBetween(std::string(k1, (size_t) kb1), std::string(k2, (size_t) kb2));
}

void pmemkv_each(pmemkv_engine* kv, void* context, KVEachCallback* c) {
    kv->Each(context, c);
}

void pmemkv_each_above(pmemkv_engine* kv, void* context, int32_t kb, const char* k, KVEachCallback* c) {
    kv->EachAbove(context, std::string(k, (size_t) kb), c);
}

void pmemkv_each_below(pmemkv_engine* kv, void* context, int32_t kb, const char* k, KVEachCallback* c) {
    kv->EachBelow(context, std::string(k, (size_t) kb), c);
}

void pmemkv_each_between(pmemkv_engine* kv, void* context, int32_t kb1, const char* k1,
                                      int32_t kb2, const char* k2, KVEachCallback* c) {
    kv->EachBetween(context, std::string(k1, (size_t) kb1), std::string(k2, (size_t) kb2), c);
}

int8_t pmemkv_exists(pmemkv_engine* kv, int32_t kb, const char* k) {
    return kv->Exists(std::string(k, (size_t) kb));
}

void pmemkv_get(pmemkv_engine* kv, void* context, const int32_t kb, const char* k, KVGetCallback* c) {
    kv->Get(context, std::string(k, (size_t) kb), c);
}

struct GetCopyCallbackContext {
    KVStatus result;
    int32_t maxvaluebytes;
    char* value;
};

int8_t pmemkv_get_copy(pmemkv_engine* kv, int32_t kb, const char* k, int32_t maxvaluebytes, char* value) {
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

int8_t pmemkv_put(pmemkv_engine* kv, const int32_t kb, const char* k, const int32_t vb, const char* v) {
    return kv->Put(std::string(k, (size_t) kb), std::string(v, (size_t) vb));
}

int8_t pmemkv_remove(pmemkv_engine* kv, const int32_t kb, const char* k) {
    return kv->Remove(std::string(k, (size_t) kb));
}
