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

#ifndef LIBPMEMKV_HPP
#define LIBPMEMKV_HPP

#include <string>
#include <stdexcept>
#include <functional>
#include <utility>

#include "libpmemkv.h"

namespace pmem {
namespace kv {

typedef void all_function(std::size_t keybytes, const char *key);
typedef void each_function(std::size_t keybytes, const char *key, std::size_t valuebytes, const char *value);
typedef void get_function(std::size_t valuebytes, const char *value);
typedef void all_string_function(const std::string &key);
typedef void each_string_function(const std::string &key, const std::string &value);
typedef void get_string_function(const std::string &key);

using all_callback = pmemkv_all_callback;
using each_callback = pmemkv_each_callback;
using get_callback = pmemkv_get_callback;
using start_failure_callback = pmemkv_start_failure_callback;

using status = pmemkv_status;

class db {
public:
	// XXX - remove context.
	db(void *context, const std::string& engine_name, const std::string& config, pmemkv_start_failure_callback* callback);
	db(void *context, const std::string& engine_name, const std::string& config);
	db(const std::string& engine_name, const std::string& config);

	~db();

	void *EngineContext();

	void all(void *context, all_callback* callback);
	void all(std::function<all_function> f);
	void all(std::function<all_string_function> f);

	void all_above(void *context, const std::string& key, all_callback* callback);
	void all_above(const std::string& key, std::function<all_function> f);
	void all_above(const std::string& key, std::function<all_string_function> f);

	void all_below(void *context, const std::string& key, all_callback* callback);
	void all_below(const std::string& key, std::function<all_function> f);
	void all_below(const std::string& key, std::function<all_string_function> f);

	void all_between(void *context, const std::string& key1, const std::string& key2, all_callback* callback);
	void all_between(const std::string& key1, const std::string& key2, std::function<all_function> f);
	void all_between(const std::string& key1, const std::string& key2, std::function<all_string_function> f);

	std::size_t count();
	std::size_t count_above(const std::string& key);
	std::size_t count_below(const std::string& key);
	std::size_t count_beetween(const std::string& key1, const std::string& key2);

	void each(void *context, each_callback* callback);
	void each(std::function<each_function> f);
	void each(std::function<each_string_function> f);

	void each_above(void *context, const std::string& key, each_callback* callback);
	void each_above(const std::string& key, std::function<each_function> f);
	void each_above(const std::string& key, std::function<each_string_function> f);

	void each_below(void *context, const std::string& key, each_callback* callback);
	void each_below(const std::string& key, std::function<each_function> f);
	void each_below(const std::string& key, std::function<each_string_function> f);

	void each_between(void *context, const std::string& key1, const std::string& key2, each_callback* callback);
	void each_between(const std::string& key1, const std::string& key2, std::function<each_function> f);
	void each_between(const std::string& key1, const std::string& key2, std::function<each_string_function> f);

	status exists(const std::string& key);

	void get(void *context, const std::string& key, get_callback* callback);
	void get(const std::string& key, std::function<get_function> f);
	void get(const std::string& key, std::function<get_string_function> f);
	status get(const std::string& key, std::string* value);

	status put(const std::string& key, const std::string& value);
	status remove(const std::string& key);

private:
	pmemkv_db* kv;
};

/*
 * All functions which will be called by C code must be declared as extern "C"
 * to ensure they have C linkage. It is needed because it is possible that
 * C and C++ functions use different calling conventions.
 */
extern "C" {
	static inline void callKVAllFunction(void *context, std::size_t keybytes, const char *key) {
		(*reinterpret_cast<std::function<all_function>*>(context))(keybytes, key);
	}

	static inline void callKVAllStringFunction(void *context, std::size_t keybytes, const char *key) {
		(*reinterpret_cast<std::function<all_string_function>*>(context))(std::string(key, keybytes));
	}

	static inline void callKVEachFunction(void *context, std::size_t keybytes, const char *key, std::size_t valuebytes, const char *value) {
		(*reinterpret_cast<std::function<each_function>*>(context))(keybytes, key, valuebytes, value);
	}

	static inline void callKVEachStringFunction(void *context, std::size_t keybytes, const char *key, std::size_t valuebytes, const char *value) {
		(*reinterpret_cast<std::function<each_string_function>*>(context))(std::string(key, keybytes), std::string(value, valuebytes));
	}

	static inline void callKVGetFunction(void *context, std::size_t valuebytes, const char *value) {
		(*reinterpret_cast<std::function<get_function>*>(context))(valuebytes, value);
	}

	static inline void callKVGetStringFunction(void *context, std::size_t valuebytes, const char *value) {
		(*reinterpret_cast<std::function<all_string_function>*>(context))(std::string(value, valuebytes));
	}

	static inline void callGet(void *context, size_t vb, const char *v) {
		auto c = reinterpret_cast<std::pair<status, std::string*>*>(context);
		c->first = OK;
		c->second->append(v, vb);
	}

	static inline void callOnStartCallback(void *context, const char *engine, const char *config, const char *msg) {
		auto string = *reinterpret_cast<std::string*>(context);
		string = msg;
	}
}

inline void *db::EngineContext() {
	return pmemkv_engine_context(this->kv);
}

inline void db::all(void *context, all_callback* callback) {
	pmemkv_all(this->kv, context, callback);
}

inline void db::all(std::function<all_function> f) {
	pmemkv_all(this->kv, &f, callKVAllFunction);
}

inline void db::all(std::function<all_string_function> f) {
	pmemkv_all(this->kv, &f, callKVAllStringFunction);
}

inline db::db(void *context, const std::string& engine_name, const std::string& config, start_failure_callback* callback) {
	this->kv = pmemkv_new(context, engine_name.c_str(), config.c_str(), callback);
}

inline db::db(const std::string& engine_name, const std::string& config) {
	std::string str;
	this->kv = pmemkv_new(&str, engine_name.c_str(), config.c_str(), callOnStartCallback);

	if (this->kv == nullptr)
		throw std::runtime_error(str);
}


inline db::~db() {
	pmemkv_delete(this->kv);
}

inline void db::all_above(void *context, const std::string& key, all_callback* callback) {
	pmemkv_all_above(this->kv, context, key.size(), key.c_str(), callback);
}

inline void db::all_above(const std::string& key, std::function<all_function> f) {
	pmemkv_all_above(this->kv, &f, key.size(), key.c_str(), callKVAllFunction);
}

inline void db::all_above(const std::string& key, std::function<all_string_function> f) {
	pmemkv_all_above(this->kv, &f, key.size(), key.c_str(), callKVAllStringFunction);
}

inline void db::all_below(void *context, const std::string& key, all_callback* callback) {
	pmemkv_all_below(this->kv, context, key.size(), key.c_str(), callback);
}

inline void db::all_below(const std::string& key, std::function<all_function> f) {
	pmemkv_all_below(this->kv, &f, key.size(), key.c_str(), callKVAllFunction);
}

inline void db::all_below(const std::string& key, std::function<all_string_function> f) {
	pmemkv_all_below(this->kv, &f, key.size(), key.c_str(), callKVAllStringFunction);
}

inline void db::all_between(void *context, const std::string& key1, const std::string& key2, all_callback* callback) {
	pmemkv_all_between(this->kv, context, key1.size(), key1.c_str(), key2.size(), key2.c_str(), callback);
}

inline void db::all_between(const std::string& key1, const std::string& key2, std::function<all_function> f) {
	pmemkv_all_between(this->kv, &f, key1.size(), key1.c_str(), key2.size(), key2.c_str(), callKVAllFunction);
}

inline void db::all_between(const std::string& key1, const std::string& key2, std::function<all_string_function> f) {
	pmemkv_all_between(this->kv, &f, key1.size(), key1.c_str(), key2.size(), key2.c_str(), callKVAllStringFunction);
}

inline std::size_t db::count() {
	return pmemkv_count(this->kv);
}

inline std::size_t db::count_above(const std::string& key) {
	return pmemkv_count_above(this->kv, key.size(), key.c_str());
}

inline std::size_t db::count_below(const std::string& key) {
	return pmemkv_count_below(this->kv, key.size(), key.c_str());
}

inline std::size_t db::count_beetween(const std::string& key1, const std::string& key2) {
	return pmemkv_count_between(this->kv, key1.size(), key1.c_str(), key2.size(), key2.c_str());
}

inline void db::each(void *context, each_callback* callback) {
	pmemkv_each(this->kv, context, callback);
}

inline void db::each(std::function<each_function> f) {
	pmemkv_each(this->kv, &f, callKVEachFunction);
}

inline void db::each(std::function<each_string_function> f) {
	pmemkv_each(this->kv, &f, callKVEachStringFunction);
}

inline void db::each_above(void *context, const std::string& key, each_callback* callback) {
	pmemkv_each_above(this->kv, context, key.size(), key.c_str(), callback);
}

inline void db::each_above(const std::string& key, std::function<each_function> f) {
	pmemkv_each_above(this->kv, &f, key.size(), key.c_str(), callKVEachFunction);
}

inline void db::each_above(const std::string& key, std::function<each_string_function> f) {
	pmemkv_each_above(this->kv, &f, key.size(), key.c_str(), callKVEachStringFunction);
}

inline void db::each_below(void *context, const std::string& key, each_callback* callback) {
	pmemkv_each_below(this->kv, context, key.size(), key.c_str(), callback);
}

inline void db::each_below(const std::string& key, std::function<each_function> f) {
	pmemkv_each_below(this->kv, &f, key.size(), key.c_str(), callKVEachFunction);
}

inline void db::each_below(const std::string& key, std::function<each_string_function> f) {
	pmemkv_each_below(this->kv, &f, key.size(), key.c_str(), callKVEachStringFunction);
}

inline void db::each_between(void *context, const std::string& key1, const std::string& key2, each_callback* callback) {
	pmemkv_each_between(this->kv, context, key1.size(), key1.c_str(), key2.size(), key2.c_str(), callback);
}

inline void db::each_between(const std::string& key1, const std::string& key2, std::function<each_function> f) {
	pmemkv_each_between(this->kv, &f, key1.size(), key1.c_str(), key2.size(), key2.c_str(), callKVEachFunction);
}

inline void db::each_between(const std::string& key1, const std::string& key2, std::function<each_string_function> f) {
	pmemkv_each_between(this->kv, &f, key1.size(), key1.c_str(), key2.size(), key2.c_str(), callKVEachStringFunction);
}

inline pmemkv_status db::exists(const std::string& key) {
	return pmemkv_exists(this->kv, key.size(), key.c_str());
}

inline void db::get(void *context, const std::string& key, get_callback* callback) {
	pmemkv_get(this->kv, context, key.size(), key.c_str(), callback);
}

inline void db::get(const std::string& key, std::function<get_function> f) {
	pmemkv_get(this->kv, &f, key.size(), key.c_str(), callKVGetFunction);
}

inline void db::get(const std::string& key, std::function<get_string_function> f) {
	pmemkv_get(this->kv, &f, key.size(), key.c_str(), callKVGetStringFunction);
}

inline status db::get(const std::string& key, std::string* value) {
	std::pair<status, std::string*> ctx = {NOT_FOUND, value};

	pmemkv_get(this->kv, &ctx, key.size(), key.c_str(), callGet);

	return ctx.first;
}

inline status db::put(const std::string& key, const std::string& value) {
	return pmemkv_put(this->kv, key.size(), key.c_str(), value.size(), value.c_str());
}

inline status db::remove(const std::string& key) {
	return pmemkv_remove(this->kv, key.size(), key.c_str());
}

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_HPP */
