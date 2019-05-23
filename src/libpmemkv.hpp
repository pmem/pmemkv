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

namespace pmemkv {

typedef void KVAllFunction(int keybytes, const char* key);
typedef void KVEachFunction(int keybytes, const char* key, int valuebytes, const char* value);
typedef void KVGetFunction(int valuebytes, const char* value);
typedef void KVAllStringFunction(const std::string &key);
typedef void KVEachStringFunction(const std::string &key, const std::string &value);
typedef void KVGetStringFunction(const std::string &key);

class KVEngine {
public:
	// XXX - remove context.
	KVEngine(void *context, const std::string& engine_name, const std::string& config);

	~KVEngine();

	void *EngineContext();

	void All(void* context, KVAllCallback* callback);
	void All(std::function<KVAllFunction> f);
	void All(std::function<KVAllStringFunction> f);

	void AllAbove(void* context, const std::string& key, KVAllCallback* callback);
	void AllAbove(const std::string& key, std::function<KVAllFunction> f);
	void AllAbove(const std::string& key, std::function<KVAllStringFunction> f);

	void AllBelow(void* context, const std::string& key, KVAllCallback* callback);
	void AllBelow(const std::string& key, std::function<KVAllFunction> f);
	void AllBelow(const std::string& key, std::function<KVAllStringFunction> f);

	void AllBetween(void* context, const std::string& key1, const std::string& key2, KVAllCallback* callback);
	void AllBetween(const std::string& key1, const std::string& key2, std::function<KVAllFunction> f);
	void AllBetween(const std::string& key1, const std::string& key2, std::function<KVAllStringFunction> f);

	int64_t Count();
	int64_t CountAbove(const std::string& key);
	int64_t CountBelow(const std::string& key);
	int64_t CountBetween(const std::string& key1, const std::string& key2);

	void Each(void* context, KVEachCallback* callback);
	void Each(std::function<KVEachFunction> f);
	void Each(std::function<KVEachStringFunction> f);

	void EachAbove(void* context, const std::string& key, KVEachCallback* callback);
	void EachAbove(const std::string& key, std::function<KVEachFunction> f);
	void EachAbove(const std::string& key, std::function<KVEachStringFunction> f);

	void EachBelow(void* context, const std::string& key, KVEachCallback* callback);
	void EachBelow(const std::string& key, std::function<KVEachFunction> f);
	void EachBelow(const std::string& key, std::function<KVEachStringFunction> f);

	void EachBetween(void* context, const std::string& key1, const std::string& key2, KVEachCallback* callback);
	void EachBetween(const std::string& key1, const std::string& key2, std::function<KVEachFunction> f);
	void EachBetween(const std::string& key1, const std::string& key2, std::function<KVEachStringFunction> f);

	KVStatus Exists(const std::string& key);

	void Get(void* context, const std::string& key, KVGetCallback* callback);
	void Get(const std::string& key, std::function<KVGetFunction> f);
	void Get(const std::string& key, std::function<KVGetStringFunction> f);
	KVStatus Get(const std::string& key, std::string* value);

	KVStatus Put(const std::string& key, const std::string& value);
	KVStatus Remove(const std::string& key);

private:
	pmemkv_engine* engine;
};

/*
 * All functions which will be called by C code must be declared as extern "C"
 * to ensure they have C linkage. It is needed because it is possible that
 * C and C++ functions use different calling conventions.
 */
extern "C" {
	inline void callKVAllFunction(void* context, int keybytes, const char* key) {
		(*reinterpret_cast<std::function<KVAllFunction>*>(context))(keybytes, key);
	}

	inline void callKVAllStringFunction(void* context, int keybytes, const char* key) {
		(*reinterpret_cast<std::function<KVAllStringFunction>*>(context))(std::string(key, keybytes));
	}

	inline void callKVEachFunction(void *context, int keybytes, const char* key, int valuebytes, const char* value) {
		(*reinterpret_cast<std::function<KVEachFunction>*>(context))(keybytes, key, valuebytes, value);
	}

	inline void callKVEachStringFunction(void *context, int keybytes, const char* key, int valuebytes, const char* value) {
		(*reinterpret_cast<std::function<KVEachStringFunction>*>(context))(std::string(key, keybytes), std::string(value, valuebytes));
	}

	inline void callKVGetFunction(void* context, int valuebytes, const char* value) {
		(*reinterpret_cast<std::function<KVGetFunction>*>(context))(valuebytes, value);
	}

	inline void callKVGetStringFunction(void* context, int valuebytes, const char* value) {
		(*reinterpret_cast<std::function<KVAllStringFunction>*>(context))(std::string(value, valuebytes));
	}

	inline void callGet(void* context, int32_t vb, const char* v) {
		auto c = reinterpret_cast<std::pair<KVStatus, std::string*>*>(context);
		c->first = OK;
		c->second->append(v, vb);
	}

	inline void callOnStartCallback(void* context, const char* engine, const char* config, const char* msg) {
		return;
	}
}

inline void* KVEngine::EngineContext() {
	return kvengine_engine_context(this->engine);
}

inline void KVEngine::All(void* context, KVAllCallback* callback) {
	kvengine_all(this->engine, context, callback);
}

inline void KVEngine::All(std::function<KVAllFunction> f) {
	kvengine_all(this->engine, &f, callKVAllFunction);
}

inline void KVEngine::All(std::function<KVAllStringFunction> f) {
	kvengine_all(this->engine, &f, callKVAllStringFunction);
}

inline KVEngine::KVEngine(void *context, const std::string& engine_name, const std::string& config) {
	std::string errormsg;

	// XXX - change kvengine_start to return status
	this->engine = kvengine_start(context, engine_name.c_str(), config.c_str(), callOnStartCallback);

	if (this->engine == nullptr)
		throw std::runtime_error("Could not start engine.");
}

inline KVEngine::~KVEngine() {
	kvengine_stop(this->engine);
}

inline void KVEngine::AllAbove(void* context, const std::string& key, KVAllCallback* callback) {
	kvengine_all_above(this->engine, context, key.size(), key.c_str(), callback);
}

inline void KVEngine::AllAbove(const std::string& key, std::function<KVAllFunction> f) {
	kvengine_all_above(this->engine, &f, key.size(), key.c_str(), callKVAllFunction);
}

inline void KVEngine::AllAbove(const std::string& key, std::function<KVAllStringFunction> f) {
	kvengine_all_above(this->engine, &f, key.size(), key.c_str(), callKVAllStringFunction);
}

inline void KVEngine::AllBelow(void* context, const std::string& key, KVAllCallback* callback) {
	kvengine_all_below(this->engine, context, key.size(), key.c_str(), callback);
}

inline void KVEngine::AllBelow(const std::string& key, std::function<KVAllFunction> f) {
	kvengine_all_below(this->engine, &f, key.size(), key.c_str(), callKVAllFunction);
}

inline void KVEngine::AllBelow(const std::string& key, std::function<KVAllStringFunction> f) {
	kvengine_all_below(this->engine, &f, key.size(), key.c_str(), callKVAllStringFunction);
}

inline void KVEngine::AllBetween(void* context, const std::string& key1, const std::string& key2, KVAllCallback* callback) {
	kvengine_all_between(this->engine, context, key1.size(), key1.c_str(), key2.size(), key2.c_str(), callback);
}

inline void KVEngine::AllBetween(const std::string& key1, const std::string& key2, std::function<KVAllFunction> f) {
	kvengine_all_between(this->engine, &f, key1.size(), key1.c_str(), key2.size(), key2.c_str(), callKVAllFunction);
}

inline void KVEngine::AllBetween(const std::string& key1, const std::string& key2, std::function<KVAllStringFunction> f) {
	kvengine_all_between(this->engine, &f, key1.size(), key1.c_str(), key2.size(), key2.c_str(), callKVAllStringFunction);
}

inline int64_t KVEngine::Count() {
	return kvengine_count(this->engine);
}

inline int64_t KVEngine::CountAbove(const std::string& key) {
	return kvengine_count_above(this->engine, key.size(), key.c_str());
}

inline int64_t KVEngine::CountBelow(const std::string& key) {
	return kvengine_count_below(this->engine, key.size(), key.c_str());
}

inline int64_t KVEngine::CountBetween(const std::string& key1, const std::string& key2) {
	return kvengine_count_between(this->engine, key1.size(), key1.c_str(), key2.size(), key2.c_str());
}

inline void KVEngine::Each(void* context, KVEachCallback* callback) {
	kvengine_each(this->engine, context, callback);
}

inline void KVEngine::Each(std::function<KVEachFunction> f) {
	kvengine_each(this->engine, &f, callKVEachFunction);
}

inline void KVEngine::Each(std::function<KVEachStringFunction> f) {
	kvengine_each(this->engine, &f, callKVEachStringFunction);
}

inline void KVEngine::EachAbove(void* context, const std::string& key, KVEachCallback* callback) {
	kvengine_each_above(this->engine, context, key.size(), key.c_str(), callback);
}

inline void KVEngine::EachAbove(const std::string& key, std::function<KVEachFunction> f) {
	kvengine_each_above(this->engine, &f, key.size(), key.c_str(), callKVEachFunction);
}

inline void KVEngine::EachAbove(const std::string& key, std::function<KVEachStringFunction> f) {
	kvengine_each_above(this->engine, &f, key.size(), key.c_str(), callKVEachStringFunction);
}

inline void KVEngine::EachBelow(void* context, const std::string& key, KVEachCallback* callback) {
	kvengine_each_below(this->engine, context, key.size(), key.c_str(), callback);
}

inline void KVEngine::EachBelow(const std::string& key, std::function<KVEachFunction> f) {
	kvengine_each_below(this->engine, &f, key.size(), key.c_str(), callKVEachFunction);
}

inline void KVEngine::EachBelow(const std::string& key, std::function<KVEachStringFunction> f) {
	kvengine_each_below(this->engine, &f, key.size(), key.c_str(), callKVEachStringFunction);
}

inline void KVEngine::EachBetween(void* context, const std::string& key1, const std::string& key2, KVEachCallback* callback) {
	kvengine_each_between(this->engine, context, key1.size(), key1.c_str(), key2.size(), key2.c_str(), callback);
}

inline void KVEngine::EachBetween(const std::string& key1, const std::string& key2, std::function<KVEachFunction> f) {
	kvengine_each_between(this->engine, &f, key1.size(), key1.c_str(), key2.size(), key2.c_str(), callKVEachFunction);
}

inline void KVEngine::EachBetween(const std::string& key1, const std::string& key2, std::function<KVEachStringFunction> f) {
	kvengine_each_between(this->engine, &f, key1.size(), key1.c_str(), key2.size(), key2.c_str(), callKVEachStringFunction);
}

inline KVStatus KVEngine::Exists(const std::string& key) {
	return (KVStatus) kvengine_exists(this->engine, key.size(), key.c_str());
}

inline void KVEngine::Get(void* context, const std::string& key, KVGetCallback* callback) {
	kvengine_get(this->engine, context, key.size(), key.c_str(), callback);
}

inline void KVEngine::Get(const std::string& key, std::function<KVGetFunction> f) {
	kvengine_get(this->engine, &f, key.size(), key.c_str(), callKVGetFunction);
}

inline void KVEngine::Get(const std::string& key, std::function<KVGetStringFunction> f) {
	kvengine_get(this->engine, &f, key.size(), key.c_str(), callKVGetStringFunction);
}

inline KVStatus KVEngine::Get(const std::string& key, std::string* value) {
	std::pair<KVStatus, std::string*> ctx = {NOT_FOUND, value};

	kvengine_get(this->engine, &ctx, key.size(), key.c_str(), callGet);

	return ctx.first;
}

inline KVStatus KVEngine::Put(const std::string& key, const std::string& value) {
	return (KVStatus) kvengine_put(this->engine, key.size(), key.c_str(), value.size(), value.c_str());
}

inline KVStatus KVEngine::Remove(const std::string& key) {
	return (KVStatus) kvengine_remove(this->engine, key.size(), key.c_str());
}

} /* namespace pmemkv */

#endif /* LIBPMEMKV_HPP */
