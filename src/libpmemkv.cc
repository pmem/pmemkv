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

#include "engine.h"
#include "engines/blackhole.h"
#include "libpmemkv.h"

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

#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

struct pmemkv_config {
	std::unordered_map<std::string, std::vector<char>> umap;
};

extern "C" {

pmemkv_config *pmemkv_config_new(void)
{
	try {
		return new pmemkv_config;
	} catch (...) {
		return nullptr;
	}
}

void pmemkv_config_delete(pmemkv_config *config)
{
	delete config;
}

int pmemkv_config_put(pmemkv_config *config, const char *key, const void *value,
		      size_t value_size)
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

ssize_t pmemkv_config_get(pmemkv_config *config, const char *key, void *buffer,
			  size_t buffer_len, size_t *value_size)
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

int pmemkv_config_from_json(pmemkv_config *config, const char *jsonconfig)
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
			throw std::runtime_error("Input string is not a valid JSON");

		if (doc.HasMember("path") && !doc["path"].IsString())
			throw std::runtime_error("'path' in JSON is not a valid string");
		else if (doc.HasMember("size") && !doc["size"].IsNumber())
			throw std::runtime_error("'size' in JSON is not a valid number");

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

			if (pmemkv_config_put(config, itr->name.GetString(), value,
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

pmemkv_status pmemkv_open(void *context, const char *engine_c_str, pmemkv_config *config,
			  pmemkv_db **db)
{
	if (db == nullptr)
		return PMEMKV_STATUS_INVALID_ARGUMENT;

	try {
		std::string engine = engine_c_str;

		if (engine == "blackhole") {
			*db = reinterpret_cast<pmemkv_db *>(
				new pmem::kv::blackhole(context));

			return PMEMKV_STATUS_OK;
		}
#ifdef ENGINE_CACHING
		if (engine == "caching") {
			*db = reinterpret_cast<pmemkv_db *>(
				new pmem::kv::caching(context, config));

			return PMEMKV_STATUS_OK;
		}
#endif
		// handle traditional engines expecting path & size params
		size_t length;
		if (pmemkv_config_get(config, "path", NULL, 0, &length))
			throw std::runtime_error(
				"JSON does not contain a valid path string");
		auto path = std::unique_ptr<char[]>(new char[length]);
		if (pmemkv_config_get(config, "path", path.get(), length, NULL) != length)
			throw std::runtime_error(
				"Cannot get a 'path' string from the config");

		size_t size = 0;
		pmemkv_config_get(config, "size", &size, sizeof(size_t), NULL);
#ifdef ENGINE_TREE3
		if (engine == "tree3") {
			*db = reinterpret_cast<pmemkv_db *>(
				new pmem::kv::tree3(context, path.get(), size));

			return PMEMKV_STATUS_OK;
		}
#endif

#ifdef ENGINE_STREE
		if (engine == "stree") {
			*db = reinterpret_cast<pmemkv_db *>(
				new pmem::kv::stree(context, path.get(), size));

			return PMEMKV_STATUS_OK;
		}
#endif

#ifdef ENGINE_CMAP
		if (engine == "cmap") {
			*db = reinterpret_cast<pmemkv_db *>(
				new pmem::kv::cmap(context, path.get(), size));

			return PMEMKV_STATUS_OK;
		}
#endif

#if defined(ENGINE_VSMAP) || defined(ENGINE_VCMAP)
		struct stat info;
		if ((stat(path.get(), &info) < 0) || !S_ISDIR(info.st_mode)) {
			throw std::runtime_error(
				"Config path is not an existing directory");
		}
#endif

#ifdef ENGINE_VSMAP
		if (engine == "vsmap") {
			*db = reinterpret_cast<pmemkv_db *>(
				new pmem::kv::vsmap(context, path.get(), size));

			return PMEMKV_STATUS_OK;
		}
#endif

#ifdef ENGINE_VCMAP
		if (engine == "vcmap") {
			*db = reinterpret_cast<pmemkv_db *>(
				new pmem::kv::vcmap(context, path.get(), size));

			return PMEMKV_STATUS_OK;
		}
#endif
		throw std::runtime_error("Unknown engine name");
	} catch (std::exception &e) {
		std::cerr << e.what() << "\n";
		*db = nullptr;

		return PMEMKV_STATUS_FAILED;
	}
}

void pmemkv_close(pmemkv_db *db)
{
	delete reinterpret_cast<pmem::kv::engine_base *>(db);
}

pmemkv_status pmemkv_all(pmemkv_db *db, pmemkv_all_callback *c, void *arg)
{
	return (pmemkv_status) reinterpret_cast<pmem::kv::engine_base *>(db)->all(c, arg);
}

pmemkv_status pmemkv_all_above(pmemkv_db *db, const char *k, size_t kb,
			       pmemkv_all_callback *c, void *arg)
{
	return (pmemkv_status) reinterpret_cast<pmem::kv::engine_base *>(db)->all_above(
		std::string(k, (size_t)kb), c, arg);
}

pmemkv_status pmemkv_all_below(pmemkv_db *db, const char *k, size_t kb,
			       pmemkv_all_callback *c, void *arg)
{
	return (pmemkv_status) reinterpret_cast<pmem::kv::engine_base *>(db)->all_below(
		std::string(k, (size_t)kb), c, arg);
}

pmemkv_status pmemkv_all_between(pmemkv_db *db, const char *k1, size_t kb1,
				 const char *k2, size_t kb2, pmemkv_all_callback *c,
				 void *arg)
{
	return (pmemkv_status) reinterpret_cast<pmem::kv::engine_base *>(db)->all_between(
		std::string(k1, (size_t)kb1), std::string(k2, (size_t)kb2), c, arg);
}

pmemkv_status pmemkv_count(pmemkv_db *db, size_t *cnt)
{
	return (pmemkv_status) reinterpret_cast<pmem::kv::engine_base *>(db)->count(*cnt);
}

pmemkv_status pmemkv_count_above(pmemkv_db *db, const char *k, size_t kb, size_t *cnt)
{
	return (pmemkv_status) reinterpret_cast<pmem::kv::engine_base *>(db)->count_above(
		std::string(k, kb), *cnt);
}

pmemkv_status pmemkv_count_below(pmemkv_db *db, const char *k, size_t kb, size_t *cnt)
{
	return (pmemkv_status) reinterpret_cast<pmem::kv::engine_base *>(db)->count_below(
		std::string(k, kb), *cnt);
}

pmemkv_status pmemkv_count_between(pmemkv_db *db, const char *k1, size_t kb1,
				   const char *k2, size_t kb2, size_t *cnt)
{
	return (pmemkv_status) reinterpret_cast<pmem::kv::engine_base *>(db)
		->count_between(std::string(k1, kb1), std::string(k2, kb2), *cnt);
}

pmemkv_status pmemkv_each(pmemkv_db *db, pmemkv_each_callback *c, void *arg)
{
	return (pmemkv_status) reinterpret_cast<pmem::kv::engine_base *>(db)->each(c,
										   arg);
}

pmemkv_status pmemkv_each_above(pmemkv_db *db, const char *k, size_t kb,
				pmemkv_each_callback *c, void *arg)
{
	return (pmemkv_status) reinterpret_cast<pmem::kv::engine_base *>(db)->each_above(
		std::string(k, (size_t)kb), c, arg);
}

pmemkv_status pmemkv_each_below(pmemkv_db *db, const char *k, size_t kb,
				pmemkv_each_callback *c, void *arg)
{
	return (pmemkv_status) reinterpret_cast<pmem::kv::engine_base *>(db)->each_below(
		std::string(k, (size_t)kb), c, arg);
}

pmemkv_status pmemkv_each_between(pmemkv_db *db, const char *k1, size_t kb1,
				  const char *k2, size_t kb2, pmemkv_each_callback *c,
				  void *arg)
{
	return (pmemkv_status) reinterpret_cast<pmem::kv::engine_base *>(db)
		->each_between(std::string(k1, (size_t)kb1), std::string(k2, (size_t)kb2),
			       c, arg);
}

pmemkv_status pmemkv_exists(pmemkv_db *db, const char *k, size_t kb)
{
	return (pmemkv_status) reinterpret_cast<pmem::kv::engine_base *>(db)->exists(
		std::string(k, (size_t)kb));
}

pmemkv_status pmemkv_get(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_callback *c,
			 void *arg)
{
	return (pmemkv_status) reinterpret_cast<pmem::kv::engine_base *>(db)->get(
		std::string(k, (size_t)kb), c, arg);
}

struct GetCopyCallbackContext {
	pmemkv_status result;
	size_t maxvaluebytes;
	char *value;
};

pmemkv_status pmemkv_get_copy(pmemkv_db *db, const char *k, size_t kb, char *value,
			      size_t maxvaluebytes)
{
	GetCopyCallbackContext cxt = {PMEMKV_STATUS_NOT_FOUND, maxvaluebytes, value};
	auto cb = [](const char *v, size_t vb, void *arg) {
		const auto c = ((GetCopyCallbackContext *)arg);
		if (vb < c->maxvaluebytes) {
			c->result = PMEMKV_STATUS_OK;
			memcpy(c->value, v, vb);
		} else {
			c->result = PMEMKV_STATUS_FAILED;
		}
	};
	memset(value, 0, maxvaluebytes);
	reinterpret_cast<pmem::kv::engine_base *>(db)->get(std::string(k, (size_t)kb), cb,
							   &cxt);
	return cxt.result;
}

pmemkv_status pmemkv_put(pmemkv_db *db, const char *k, size_t kb, const char *v,
			 size_t vb)
{
	return (pmemkv_status) reinterpret_cast<pmem::kv::engine_base *>(db)->put(
		std::string(k, (size_t)kb), std::string(v, (size_t)vb));
}

pmemkv_status pmemkv_remove(pmemkv_db *db, const char *k, size_t kb)
{
	return (pmemkv_status) reinterpret_cast<pmem::kv::engine_base *>(db)->remove(
		std::string(k, (size_t)kb));
}

void *pmemkv_engine_context(pmemkv_db *db)
{
	return reinterpret_cast<pmem::kv::engine_base *>(db)->engine_context();
}

} /* extern "C" */
