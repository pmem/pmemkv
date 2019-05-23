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

#include <unordered_map>
#include <vector>
#include <memory>
#include <iostream>

struct pmemkv_config {
	std::unordered_map<std::string, std::vector<char>> umap;
};

extern "C" {

pmemkv_config *
pmemkv_config_new(void)
{
	try { return new pmemkv_config; } catch(...) { return nullptr; }
}

void
pmemkv_config_delete(pmemkv_config *config)
{
	delete config;
}

int
pmemkv_config_put(pmemkv_config *config, const char *key,
			const void *value, size_t value_size)
{
	try {
		std::string mkey(key);
		std::vector<char> v((char *)value, (char *)value + value_size);
		config->umap.insert({mkey, v});
	} catch (...) {
		return -1;
	}

	return 0;
}

ssize_t
pmemkv_config_get(pmemkv_config *config, const char *key,
			void *buffer, size_t buffer_len,
			size_t *value_size)
{
	size_t len = 0;

	try {
		std::string mkey(key);
		auto found = config->umap.find(mkey);

		if (found == config->umap.end())
			return -1;

		auto mvalue = found->second;

		if (buffer) {
			len = (buffer_len < mvalue.size()) ? buffer_len : mvalue.size();
			memcpy(buffer, mvalue.data(), len);
		}

		if (value_size)
			*value_size = mvalue.size();
	} catch (...) {
		return -1;
	}

	return len;
}

int
pmemkv_config_from_json(pmemkv_config *config, const char *jsonconfig)
{
	rapidjson::Document doc;
	rapidjson::Value::ConstMemberIterator itr;

	union data {
		unsigned uint;
		int sint;
		uint64_t uint64;
		int64_t sint64;
		double db;
	} data;

	assert(config && jsonconfig);

	try {
		if (doc.Parse(jsonconfig).HasParseError())
			throw std::runtime_error(
				"Input string is not a valid JSON");

		if (doc.HasMember("path") && !doc["path"].IsString())
			throw std::runtime_error(
				"'path' in JSON is not a valid string");
		else if (doc.HasMember("size") && !doc["size"].IsNumber())
			throw std::runtime_error(
				"'size' in JSON is not a valid number");

		for (itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
			const void *value = &data;
			size_t value_size;
			if (itr->value.IsString()) {
				value = itr->value.GetString();
				value_size = itr->value.GetStringLength() + 1;
			} else if (itr->value.IsUint()) {
				data.uint = itr->value.GetUint();
				value_size = 4;
			} else if (itr->value.IsInt()) {
				data.sint = itr->value.GetInt();
				value_size = 4;
			} else if (itr->value.IsUint64()) {
				data.uint64 = itr->value.GetUint64();
				value_size = 8;
			} else if (itr->value.IsInt64()) {
				data.sint64 = itr->value.GetInt64();
				value_size = 8;
			} else if (itr->value.IsDouble()) {
				data.db = itr->value.GetDouble();
				value_size = 8;
			} else {
				throw std::runtime_error(
					"Unsupported data type in JSON string");
			}

			if (pmemkv_config_put(config,
					itr->name.GetString(),
					value,
					value_size))
				throw std::runtime_error(
					"Inserting a new entry to the config failed");
		}
	} catch (const std::exception &exc) {
		std::cerr << exc.what() << "\n";
		return -1;
	}

	return 0;
}

pmemkv_db* kvengine_start(void* context, const char* engine, pmemkv_config *config, KVStartFailureCallback* onfail) {
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
        size_t length;
        if (pmemkv_config_get(config, "path", NULL, 0, &length))
                throw std::runtime_error("JSON does not contain a valid path string");
        auto path = std::unique_ptr<char[]>(new char [length]);
        if (pmemkv_config_get(config, "path", path.get(), length, NULL) != length)
                throw std::runtime_error("Cannot get a 'path' string from the config");

        size_t size = 0;
        pmemkv_config_get(config, "size", &size, sizeof(size_t), NULL);
#ifdef ENGINE_TREE3
        if (engine == pmemkv::tree3::ENGINE) {
            return reinterpret_cast<pmemkv_db*>(new pmemkv::tree3::Tree(context, path.get(), size));
        }
#endif

#ifdef ENGINE_STREE
        if (engine == pmemkv::stree::ENGINE) {
            return reinterpret_cast<pmemkv_db*>(new pmemkv::stree::STree(context, path.get(), size));
        }
#endif

#ifdef ENGINE_CMAP
        if (engine == pmemkv::cmap::ENGINE) {
            return reinterpret_cast<pmemkv_db*>(new pmemkv::cmap::CMap(context, path.get(), size));
        }
#endif

#if defined(ENGINE_VSMAP) || defined(ENGINE_VCMAP)
        struct stat info;
        if ((stat(path.get(), &info) < 0) || !S_ISDIR(info.st_mode)) {
            throw std::runtime_error("Config path is not an existing directory");
        }
#endif

#ifdef ENGINE_VSMAP
        if (engine == pmemkv::vsmap::ENGINE) {
            return reinterpret_cast<pmemkv_db*>(new pmemkv::vsmap::VSMap(context, path.get(), size));
        }
#endif

#ifdef ENGINE_VCMAP
        if (engine == pmemkv::vcmap::ENGINE) {
            return reinterpret_cast<pmemkv_db*>(new pmemkv::vcmap::VCMap(context, path.get(), size));
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

} /* extern "C" */
