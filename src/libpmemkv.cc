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
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
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

#define ERR(msg)                                                                         \
	do {                                                                             \
		std::cerr << "[" << __func__ << "()] " << msg << "\n";                   \
	} while (0)

struct pmemkv_config {
	enum class config_type { STRING, INT64, UINT64, DOUBLE, SUBCONFIG, OBJECT };

	struct entry {
		std::vector<char> value;
		config_type type;
	};

	std::unordered_map<std::string, entry> umap;
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
	for (auto &item : config->umap) {
		if (item.second.type == pmemkv_config::config_type::SUBCONFIG) {
			pmemkv_config *cfg;
			memcpy(&cfg, item.second.value.data(), item.second.value.size());

			pmemkv_config_delete(cfg);
		}
	}

	delete config;
}

static int pmemkv_config_put(pmemkv_config *config, const char *key, const void *value,
			     size_t value_size, pmemkv_config::config_type type)
{
	try {
		std::string mkey(key);
		std::vector<char> v((char *)value, (char *)value + value_size);
		config->umap.insert({mkey, {v, type}});
	} catch (...) {
		return PMEMKV_STATUS_FAILED;
	}

	return PMEMKV_STATUS_OK;
}

int pmemkv_config_put_object(pmemkv_config *config, const char *key, const void *value,
			     size_t value_size)
{
	return pmemkv_config_put(config, key, value, value_size,
				 pmemkv_config::config_type::OBJECT);
}

static int pmemkv_config_get(pmemkv_config *config, const char *key, const void **value,
			     size_t *value_size, pmemkv_config::config_type *type)
{
	try {
		auto found = config->umap.find(key);

		if (found == config->umap.end())
			return PMEMKV_STATUS_NOT_FOUND;

		auto &mvalue = found->second.value;

		if (value)
			*value = mvalue.data();

		if (value_size)
			*value_size = mvalue.size();

		if (type)
			*type = found->second.type;
	} catch (...) {
		return PMEMKV_STATUS_FAILED;
	}

	return PMEMKV_STATUS_OK;
}

int pmemkv_config_get_object(pmemkv_config *config, const char *key, const void **value,
			     size_t *value_size)
{
	return pmemkv_config_get(config, key, value, value_size, nullptr);
}

int pmemkv_config_from_json(pmemkv_config *config, const char *json)
{
	rapidjson::Document doc;
	rapidjson::Value::ConstMemberIterator itr;

	assert(config && json);

	try {
		if (doc.Parse(json).HasParseError())
			return PMEMKV_STATUS_CONFIG_PARSING_ERROR;

		if (doc.HasMember("path") && !doc["path"].IsString())
			throw std::runtime_error("'path' in JSON is not a valid string");
		else if (doc.HasMember("size") && !doc["size"].IsNumber())
			throw std::runtime_error("'size' in JSON is not a valid number");

		for (itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
			if (itr->value.IsString()) {
				auto value = itr->value.GetString();

				auto status = pmemkv_config_put_string(
					config, itr->name.GetString(), value);
				if (status != PMEMKV_STATUS_OK)
					throw std::runtime_error(
						"Inserting a new entry to the config failed");
			} else if (itr->value.IsInt64()) {
				auto value = itr->value.GetInt64();

				auto status = pmemkv_config_put_int64(
					config, itr->name.GetString(), value);
				if (status != PMEMKV_STATUS_OK)
					throw std::runtime_error(
						"Inserting a new entry to the config failed");
			} else if (itr->value.IsDouble()) {
				auto value = itr->value.GetDouble();

				auto status = pmemkv_config_put_double(
					config, itr->name.GetString(), value);
				if (status != PMEMKV_STATUS_OK)
					throw std::runtime_error(
						"Inserting a new entry to the config failed");
			} else if (itr->value.IsTrue() || itr->value.IsFalse()) {
				auto value = itr->value.GetBool();

				auto status = pmemkv_config_put_int64(
					config, itr->name.GetString(), value);
				if (status != PMEMKV_STATUS_OK)
					throw std::runtime_error(
						"Inserting a new entry to the config failed");
			} else if (itr->value.IsObject()) {
				rapidjson::StringBuffer sb;
				rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
				itr->value.Accept(writer);

				auto sub_cfg = pmemkv_config_new();

				if (sub_cfg == nullptr) {
					ERR("Cannot allocate subconfig");
					return PMEMKV_STATUS_FAILED;
				}

				auto status =
					pmemkv_config_from_json(sub_cfg, sb.GetString());
				if (status != PMEMKV_STATUS_OK) {
					pmemkv_config_delete(config);
					throw std::runtime_error(
						"Cannot parse subsconfig");
				}

				status = pmemkv_config_put(
					config, itr->name.GetString(), &sub_cfg,
					sizeof(sub_cfg),
					pmemkv_config::config_type::SUBCONFIG);
				if (status != PMEMKV_STATUS_OK)
					throw std::runtime_error(
						"Inserting a new entry to the config failed");
			} else {
				static std::string kTypeNames[] = {
					"Null",  "False",  "True",  "Object",
					"Array", "String", "Number"};

				throw std::runtime_error(
					"Unsupported data type in JSON string: " +
					kTypeNames[itr->value.GetType()]);
			}
		}
	} catch (const std::exception &exc) {
		ERR(exc.what());
		return PMEMKV_STATUS_CONFIG_PARSING_ERROR;
	} catch (...) {
		ERR("Unspecified failure");
		return PMEMKV_STATUS_CONFIG_PARSING_ERROR;
	}

	return PMEMKV_STATUS_OK;
}

int pmemkv_config_put_int64(pmemkv_config *config, const char *key, int64_t value)
{
	return pmemkv_config_put(config, key, &value, sizeof(value),
				 pmemkv_config::config_type::INT64);
}

int pmemkv_config_put_uint64(pmemkv_config *config, const char *key, uint64_t value)
{
	return pmemkv_config_put(config, key, &value, sizeof(value),
				 pmemkv_config::config_type::UINT64);
	;
}

int pmemkv_config_put_double(pmemkv_config *config, const char *key, double value)
{
	return pmemkv_config_put(config, key, &value, sizeof(value),
				 pmemkv_config::config_type::DOUBLE);
	;
}

int pmemkv_config_put_string(pmemkv_config *config, const char *key, const char *value)
{
	return pmemkv_config_put(config, key, value,
				 std::char_traits<char>::length(value) + 1,
				 pmemkv_config::config_type::STRING);
}

int pmemkv_config_get_int64(pmemkv_config *config, const char *key, int64_t *value)
{
	const void *data;
	size_t value_size;
	pmemkv_config::config_type type;

	auto status = pmemkv_config_get(config, key, &data, &value_size, &type);
	if (status != PMEMKV_STATUS_OK)
		return status;

	if (type == pmemkv_config::config_type::INT64) {
		*value = *(static_cast<const int64_t *>(data));
		return PMEMKV_STATUS_OK;
	} else if (type == pmemkv_config::config_type::UINT64) {
		/* conversion from uint64 allowed */
		auto uval = *(static_cast<const uint64_t *>(data));
		if (uval < std::numeric_limits<int64_t>::max()) {
			*value = *(static_cast<const int64_t *>(data));
			return PMEMKV_STATUS_OK;
		}
	}

	return PMEMKV_STATUS_CONFIG_TYPE_ERROR;
}

int pmemkv_config_get_uint64(pmemkv_config *config, const char *key, uint64_t *value)
{
	const void *data;
	size_t value_size;
	pmemkv_config::config_type type;

	auto status = pmemkv_config_get(config, key, &data, &value_size, &type);
	if (status != PMEMKV_STATUS_OK)
		return status;

	if (type == pmemkv_config::config_type::UINT64) {
		*value = *(static_cast<const uint64_t *>(data));
		return PMEMKV_STATUS_OK;
	} else if (type == pmemkv_config::config_type::INT64) {
		/* conversion from int64 allowed */
		auto sval = *(static_cast<const int64_t *>(data));
		if (sval >= 0) {
			*value = *(static_cast<const uint64_t *>(data));
			return PMEMKV_STATUS_OK;
		}
	}

	return PMEMKV_STATUS_CONFIG_TYPE_ERROR;
}

int pmemkv_config_get_double(pmemkv_config *config, const char *key, double *value)
{
	const void *data;
	size_t value_size;
	pmemkv_config::config_type type;

	auto status = pmemkv_config_get(config, key, &data, &value_size, &type);
	if (status != PMEMKV_STATUS_OK)
		return status;

	if (type != pmemkv_config::config_type::DOUBLE)
		return PMEMKV_STATUS_CONFIG_TYPE_ERROR;

	*value = *((const double *)data);

	return PMEMKV_STATUS_OK;
}

int pmemkv_config_get_string(pmemkv_config *config, const char *key, const char **value)
{
	const void *data;
	size_t value_size;
	pmemkv_config::config_type type;

	auto status = pmemkv_config_get(config, key, &data, &value_size, &type);
	if (status != PMEMKV_STATUS_OK)
		return status;

	if (type != pmemkv_config::config_type::STRING)
		return PMEMKV_STATUS_CONFIG_TYPE_ERROR;

	*value = (const char *)data;

	return PMEMKV_STATUS_OK;
}

int pmemkv_open(void *context, const char *engine_c_str, pmemkv_config *config,
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
		const char *path;

		auto status = pmemkv_config_get_string(config, "path", &path);
		if (status != PMEMKV_STATUS_OK)
			throw std::runtime_error(
				"JSON does not contain a valid path string");

		size_t size;

		status = pmemkv_config_get_uint64(config, "size", &size);
		if (status != PMEMKV_STATUS_OK)
			throw std::runtime_error("Cannot get 'size' from the config");

#ifdef ENGINE_TREE3
		if (engine == "tree3") {
			*db = reinterpret_cast<pmemkv_db *>(
				new pmem::kv::tree3(context, path, size));

			return PMEMKV_STATUS_OK;
		}
#endif

#ifdef ENGINE_STREE
		if (engine == "stree") {
			*db = reinterpret_cast<pmemkv_db *>(
				new pmem::kv::stree(context, path, size));

			return PMEMKV_STATUS_OK;
		}
#endif

#ifdef ENGINE_CMAP
		if (engine == "cmap") {
			*db = reinterpret_cast<pmemkv_db *>(
				new pmem::kv::cmap(context, path, size));

			return PMEMKV_STATUS_OK;
		}
#endif

#if defined(ENGINE_VSMAP) || defined(ENGINE_VCMAP)
		struct stat info;
		if ((stat(path, &info) < 0) || !S_ISDIR(info.st_mode)) {
			throw std::runtime_error(
				"Config path is not an existing directory");
		}
#endif

#ifdef ENGINE_VSMAP
		if (engine == "vsmap") {
			*db = reinterpret_cast<pmemkv_db *>(
				new pmem::kv::vsmap(context, path, size));

			return PMEMKV_STATUS_OK;
		}
#endif

#ifdef ENGINE_VCMAP
		if (engine == "vcmap") {
			*db = reinterpret_cast<pmemkv_db *>(
				new pmem::kv::vcmap(context, path, size));

			return PMEMKV_STATUS_OK;
		}
#endif
		throw std::runtime_error("Unknown engine name");
	} catch (std::exception &e) {
		ERR(e.what());
		*db = nullptr;

		return PMEMKV_STATUS_FAILED;
	} catch (...) {
		ERR("Unspecified failure");
		return PMEMKV_STATUS_FAILED;
	}
}

void pmemkv_close(pmemkv_db *db)
{
	try {
		delete reinterpret_cast<pmem::kv::engine_base *>(db);
	} catch (const std::exception &exc) {
		ERR(exc.what());
	} catch (...) {
		ERR("Unspecified failure");
	}
}

int pmemkv_all(pmemkv_db *db, pmemkv_all_callback *c, void *arg)
{
	try {
		return (int)reinterpret_cast<pmem::kv::engine_base *>(db)->all(c, arg);
	} catch (const std::exception &exc) {
		ERR(exc.what());
		return PMEMKV_STATUS_FAILED;
	} catch (...) {
		ERR("Unspecified failure");
		return PMEMKV_STATUS_FAILED;
	}
}

int pmemkv_all_above(pmemkv_db *db, const char *k, size_t kb, pmemkv_all_callback *c,
		     void *arg)
{
	try {
		return (int)reinterpret_cast<pmem::kv::engine_base *>(db)->all_above(
			std::string(k, (size_t)kb), c, arg);
	} catch (const std::exception &exc) {
		ERR(exc.what());
		return PMEMKV_STATUS_FAILED;
	} catch (...) {
		ERR("Unspecified failure");
		return PMEMKV_STATUS_FAILED;
	}
}

int pmemkv_all_below(pmemkv_db *db, const char *k, size_t kb, pmemkv_all_callback *c,
		     void *arg)
{
	try {
		return (int)reinterpret_cast<pmem::kv::engine_base *>(db)->all_below(
			std::string(k, (size_t)kb), c, arg);
	} catch (const std::exception &exc) {
		ERR(exc.what());
		return PMEMKV_STATUS_FAILED;
	} catch (...) {
		ERR("Unspecified failure");
		return PMEMKV_STATUS_FAILED;
	}
}

int pmemkv_all_between(pmemkv_db *db, const char *k1, size_t kb1, const char *k2,
		       size_t kb2, pmemkv_all_callback *c, void *arg)
{
	try {
		return (int)reinterpret_cast<pmem::kv::engine_base *>(db)->all_between(
			std::string(k1, (size_t)kb1), std::string(k2, (size_t)kb2), c,
			arg);
	} catch (const std::exception &exc) {
		ERR(exc.what());
		return PMEMKV_STATUS_FAILED;
	} catch (...) {
		ERR("Unspecified failure");
		return PMEMKV_STATUS_FAILED;
	}
}

int pmemkv_count(pmemkv_db *db, size_t *cnt)
{
	try {
		return (int)reinterpret_cast<pmem::kv::engine_base *>(db)->count(*cnt);
	} catch (const std::exception &exc) {
		ERR(exc.what());
		return PMEMKV_STATUS_FAILED;
	} catch (...) {
		ERR("Unspecified failure");
		return PMEMKV_STATUS_FAILED;
	}
}

int pmemkv_count_above(pmemkv_db *db, const char *k, size_t kb, size_t *cnt)
{
	try {
		return (int)reinterpret_cast<pmem::kv::engine_base *>(db)->count_above(
			std::string(k, kb), *cnt);
	} catch (const std::exception &exc) {
		ERR(exc.what());
		return PMEMKV_STATUS_FAILED;
	} catch (...) {
		ERR("Unspecified failure");
		return PMEMKV_STATUS_FAILED;
	}
}

int pmemkv_count_below(pmemkv_db *db, const char *k, size_t kb, size_t *cnt)
{
	try {
		return (int)reinterpret_cast<pmem::kv::engine_base *>(db)->count_below(
			std::string(k, kb), *cnt);
	} catch (const std::exception &exc) {
		ERR(exc.what());
		return PMEMKV_STATUS_FAILED;
	} catch (...) {
		ERR("Unspecified failure");
		return PMEMKV_STATUS_FAILED;
	}
}

int pmemkv_count_between(pmemkv_db *db, const char *k1, size_t kb1, const char *k2,
			 size_t kb2, size_t *cnt)
{
	try {
		return (int)reinterpret_cast<pmem::kv::engine_base *>(db)->count_between(
			std::string(k1, kb1), std::string(k2, kb2), *cnt);
	} catch (const std::exception &exc) {
		ERR(exc.what());
		return PMEMKV_STATUS_FAILED;
	} catch (...) {
		ERR("Unspecified failure");
		return PMEMKV_STATUS_FAILED;
	}
}

int pmemkv_each(pmemkv_db *db, pmemkv_each_callback *c, void *arg)
{
	try {
		return (int)reinterpret_cast<pmem::kv::engine_base *>(db)->each(c, arg);
	} catch (const std::exception &exc) {
		ERR(exc.what());
		return PMEMKV_STATUS_FAILED;
	} catch (...) {
		ERR("Unspecified failure");
		return PMEMKV_STATUS_FAILED;
	}
}

int pmemkv_each_above(pmemkv_db *db, const char *k, size_t kb, pmemkv_each_callback *c,
		      void *arg)
{
	try {
		return (int)reinterpret_cast<pmem::kv::engine_base *>(db)->each_above(
			std::string(k, (size_t)kb), c, arg);
	} catch (const std::exception &exc) {
		ERR(exc.what());
		return PMEMKV_STATUS_FAILED;
	}
}

int pmemkv_each_below(pmemkv_db *db, const char *k, size_t kb, pmemkv_each_callback *c,
		      void *arg)
{
	try {
		return (int)reinterpret_cast<pmem::kv::engine_base *>(db)->each_below(
			std::string(k, (size_t)kb), c, arg);
	} catch (const std::exception &exc) {
		ERR(exc.what());
		return PMEMKV_STATUS_FAILED;
	} catch (...) {
		ERR("Unspecified failure");
		return PMEMKV_STATUS_FAILED;
	}
}

int pmemkv_each_between(pmemkv_db *db, const char *k1, size_t kb1, const char *k2,
			size_t kb2, pmemkv_each_callback *c, void *arg)
{
	try {
		return (int)reinterpret_cast<pmem::kv::engine_base *>(db)->each_between(
			std::string(k1, (size_t)kb1), std::string(k2, (size_t)kb2), c,
			arg);
	} catch (const std::exception &exc) {
		ERR(exc.what());
		return PMEMKV_STATUS_FAILED;
	} catch (...) {
		ERR("Unspecified failure");
		return PMEMKV_STATUS_FAILED;
	}
}

int pmemkv_exists(pmemkv_db *db, const char *k, size_t kb)
{
	try {
		return (int)reinterpret_cast<pmem::kv::engine_base *>(db)->exists(
			std::string(k, (size_t)kb));
	} catch (const std::exception &exc) {
		ERR(exc.what());
		return PMEMKV_STATUS_FAILED;
	} catch (...) {
		ERR("Unspecified failure");
		return PMEMKV_STATUS_FAILED;
	}
}

int pmemkv_get(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_callback *c, void *arg)
{
	try {
		return (int)reinterpret_cast<pmem::kv::engine_base *>(db)->get(
			std::string(k, (size_t)kb), c, arg);
	} catch (const std::exception &exc) {
		ERR(exc.what());
		return PMEMKV_STATUS_FAILED;
	} catch (...) {
		ERR("Unspecified failure");
		return PMEMKV_STATUS_FAILED;
	}
}

struct GetCopyCallbackContext {
	int result;
	size_t maxvaluebytes;
	char *value;
};

int pmemkv_get_copy(pmemkv_db *db, const char *k, size_t kb, char *value,
		    size_t maxvaluebytes)
{
	try {
		GetCopyCallbackContext cxt = {PMEMKV_STATUS_NOT_FOUND, maxvaluebytes,
					      value};
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
		reinterpret_cast<pmem::kv::engine_base *>(db)->get(
			std::string(k, (size_t)kb), cb, &cxt);
		return cxt.result;
	} catch (const std::exception &exc) {
		ERR(exc.what());
		return PMEMKV_STATUS_FAILED;
	} catch (...) {
		ERR("Unspecified failure");
		return PMEMKV_STATUS_FAILED;
	}
}

int pmemkv_put(pmemkv_db *db, const char *k, size_t kb, const char *v, size_t vb)
{
	try {
		return (int)reinterpret_cast<pmem::kv::engine_base *>(db)->put(
			std::string(k, (size_t)kb), std::string(v, (size_t)vb));
	} catch (const std::exception &exc) {
		ERR(exc.what());
		return PMEMKV_STATUS_FAILED;
	} catch (...) {
		ERR("Unspecified failure");
		return PMEMKV_STATUS_FAILED;
	}
}

int pmemkv_remove(pmemkv_db *db, const char *k, size_t kb)
{
	try {
		return (int)reinterpret_cast<pmem::kv::engine_base *>(db)->remove(
			std::string(k, (size_t)kb));
	} catch (const std::exception &exc) {
		ERR(exc.what());
		return PMEMKV_STATUS_FAILED;
	} catch (...) {
		ERR("Unspecified failure");
		return PMEMKV_STATUS_FAILED;
	}
}

void *pmemkv_engine_context(pmemkv_db *db)
{
	try {
		return reinterpret_cast<pmem::kv::engine_base *>(db)->engine_context();
	} catch (const std::exception &exc) {
		ERR(exc.what());
		return nullptr;
	} catch (...) {
		ERR("Unspecified failure");
		return nullptr;
	}
}

} /* extern "C" */
