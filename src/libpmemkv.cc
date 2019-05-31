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

pmemkv_db *pmemkv_start(void *context, const char *engine_c_str, const char *config, pmemkv_start_failure_callback* onfail) {
    try {
        std::string engine = engine_c_str;

        if (engine == "blackhole") {
            return reinterpret_cast<pmemkv_db*>(new pmem::kv::blackhole(context));
        }
#ifdef ENGINE_CACHING
        if (engine == "caching") {
            return reinterpret_cast<pmemkv_db*>(new pmem::kv::caching(context, config));
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
        if (engine == "tree3") {
            return reinterpret_cast<pmemkv_db*>(new pmem::kv::tree3(context, path, size));
        }
#endif

#ifdef ENGINE_STREE
        if (engine == "stree") {
            return reinterpret_cast<pmemkv_db*>(new pmem::kv::stree(context, path, size));
        }
#endif

#ifdef ENGINE_CMAP
        if (engine == "cmap") {
            return reinterpret_cast<pmemkv_db*>(new pmem::kv::cmap(context, path, size));
        }
#endif

#if defined(ENGINE_VSMAP) || defined(ENGINE_VCMAP)
        struct stat info;
        if ((stat(path, &info) < 0) || !S_ISDIR(info.st_mode)) {
            throw std::runtime_error("Config path is not an existing directory");
        }
#endif

#ifdef ENGINE_VSMAP
        if (engine == "vsmap") {
            return reinterpret_cast<pmemkv_db*>(new pmem::kv::vsmap(context, path, size));
        }
#endif

#ifdef ENGINE_VCMAP
        if (engine == "vcmap") {
            return reinterpret_cast<pmemkv_db*>(new pmem::kv::vcmap(context, path, size));
        }
#endif
        throw std::runtime_error("Unknown engine name");
    } catch (std::exception& e) {
        onfail(context, engine_c_str, config, e.what());
        return nullptr;
    }
}

void pmemkv_stop(pmemkv_db *db) {
    // XXX: move dangerous work to stop() method
    delete reinterpret_cast<pmem::kv::engine_base*>(db);
}

void pmemkv_all(pmemkv_db *db, void *context, pmemkv_all_callback* c) {
    reinterpret_cast<pmem::kv::engine_base*>(db)->all(context, c);
}

void pmemkv_all_above(pmemkv_db *db, void *context, const char *k, size_t kb, pmemkv_all_callback* c) {
    reinterpret_cast<pmem::kv::engine_base*>(db)->all_above(context, std::string(k, (size_t) kb), c);
}

void pmemkv_all_below(pmemkv_db *db, void *context, const char *k, size_t kb, pmemkv_all_callback* c) {
    reinterpret_cast<pmem::kv::engine_base*>(db)->all_below(context, std::string(k, (size_t) kb), c);
}

void pmemkv_all_between(pmemkv_db *db, void *context, const char *k1, size_t kb1,
                                     const char *k2, size_t kb2, pmemkv_all_callback* c) {
    reinterpret_cast<pmem::kv::engine_base*>(db)->all_between(context, std::string(k1, (size_t) kb1), std::string(k2, (size_t) kb2), c);
}

size_t pmemkv_count(pmemkv_db *db) {
    return reinterpret_cast<pmem::kv::engine_base*>(db)->count();
}

size_t pmemkv_count_above(pmemkv_db *db, const char *k, size_t kb) {
    return reinterpret_cast<pmem::kv::engine_base*>(db)->count_above(std::string(k, (size_t) kb));
}

size_t pmemkv_count_below(pmemkv_db *db, const char *k, size_t kb) {
    return reinterpret_cast<pmem::kv::engine_base*>(db)->count_below(std::string(k, (size_t) kb));
}

size_t pmemkv_count_between(pmemkv_db *db, const char *k1, size_t kb1, const char *k2, size_t kb2) {
    return reinterpret_cast<pmem::kv::engine_base*>(db)->count_between(std::string(k1, (size_t) kb1), std::string(k2, (size_t) kb2));
}

void pmemkv_each(pmemkv_db *db, void *context, pmemkv_each_callback* c) {
    reinterpret_cast<pmem::kv::engine_base*>(db)->each(context, c);
}

void pmemkv_each_above(pmemkv_db *db, void *context, const char *k, size_t kb, pmemkv_each_callback* c) {
    reinterpret_cast<pmem::kv::engine_base*>(db)->each_above(context, std::string(k, (size_t) kb), c);
}

void pmemkv_each_below(pmemkv_db *db, void *context, const char *k, size_t kb, pmemkv_each_callback* c) {
    reinterpret_cast<pmem::kv::engine_base*>(db)->each_below(context, std::string(k, (size_t) kb), c);
}

void pmemkv_each_between(pmemkv_db *db, void *context, const char *k1, size_t kb1,
                                      const char *k2, size_t kb2, pmemkv_each_callback* c) {
    reinterpret_cast<pmem::kv::engine_base*>(db)->each_between(context, std::string(k1, (size_t) kb1), std::string(k2, (size_t) kb2), c);
}

pmemkv_status pmemkv_exists(pmemkv_db *db, const char *k, size_t kb) {
    return (pmemkv_status) reinterpret_cast<pmem::kv::engine_base*>(db)->exists(std::string(k, (size_t) kb));
}

void pmemkv_get(pmemkv_db *db, void *context, const char *k, size_t kb, pmemkv_get_callback* c) {
    reinterpret_cast<pmem::kv::engine_base*>(db)->get(context, std::string(k, (size_t) kb), c);
}

struct GetCopyCallbackContext {
    pmemkv_status result;
    size_t maxvaluebytes;
    char* value;
};

pmemkv_status pmemkv_get_copy(pmemkv_db *db, const char *k, size_t kb, char* value, size_t maxvaluebytes) {
    GetCopyCallbackContext cxt = {PMEMKV_STATUS_NOT_FOUND, maxvaluebytes, value};
    auto cb = [](void *context, const char *v, size_t vb) {
        const auto c = ((GetCopyCallbackContext*) context);
        if (vb < c->maxvaluebytes) {
            c->result = PMEMKV_STATUS_OK;
            memcpy(c->value, v, vb);
        } else {
            c->result = PMEMKV_STATUS_FAILED;
        }
    };
    memset(value, 0, maxvaluebytes);
    reinterpret_cast<pmem::kv::engine_base*>(db)->get(&cxt, std::string(k, (size_t) kb), cb);
    return cxt.result;
}

pmemkv_status pmemkv_put(pmemkv_db *db, const char *k, size_t kb, const char *v, size_t vb) {
    return (pmemkv_status) reinterpret_cast<pmem::kv::engine_base*>(db)->put(std::string(k, (size_t) kb), std::string(v, (size_t) vb));
}

pmemkv_status pmemkv_remove(pmemkv_db *db, const char *k, size_t kb) {
    return (pmemkv_status) reinterpret_cast<pmem::kv::engine_base*>(db)->remove(std::string(k, (size_t) kb));
}

void *pmemkv_engine_context(pmemkv_db *db) {
    return reinterpret_cast<pmem::kv::engine_base*>(db)->engine_context();
}
