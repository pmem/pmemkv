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

#include <functional>
#include <stdexcept>
#include <string>
#include <utility>

#include "libpmemkv.h"

namespace pmem
{
namespace kv
{

typedef void all_function(const char *key, std::size_t keybytes);
typedef void each_function(const char *key, std::size_t keybytes, const char *value,
			   std::size_t valuebytes);
typedef void get_function(const char *value, std::size_t valuebytes);
typedef void all_string_function(const std::string &key);
typedef void each_string_function(const std::string &key, const std::string &value);
typedef void get_string_function(const std::string &key);

using all_callback = pmemkv_all_callback;
using each_callback = pmemkv_each_callback;
using get_callback = pmemkv_get_callback;
using start_failure_callback = pmemkv_start_failure_callback;

enum class status {
	FAILED = PMEMKV_STATUS_FAILED,
	NOT_FOUND = PMEMKV_STATUS_NOT_FOUND,
	OK = PMEMKV_STATUS_OK,
};

class db {
public:
	db(void *context, const std::string &engine_name, pmemkv_config *config,
	   start_failure_callback *callback);
	db(const std::string &engine_name, pmemkv_config *config);

	~db();

	void *engine_context();

	void all(all_callback *callback, void *arg);
	void all(std::function<all_function> f);
	void all(std::function<all_string_function> f);

	void all_above(const std::string &key, all_callback *callback, void *arg);
	void all_above(const std::string &key, std::function<all_function> f);
	void all_above(const std::string &key, std::function<all_string_function> f);

	void all_below(const std::string &key, all_callback *callback, void *arg);
	void all_below(const std::string &key, std::function<all_function> f);
	void all_below(const std::string &key, std::function<all_string_function> f);

	void all_between(const std::string &key1, const std::string &key2,
			 all_callback *callback, void *arg);
	void all_between(const std::string &key1, const std::string &key2,
			 std::function<all_function> f);
	void all_between(const std::string &key1, const std::string &key2,
			 std::function<all_string_function> f);

	std::size_t count();
	std::size_t count_above(const std::string &key);
	std::size_t count_below(const std::string &key);
	std::size_t count_between(const std::string &key1, const std::string &key2);

	void each(each_callback *callback, void *arg);
	void each(std::function<each_function> f);
	void each(std::function<each_string_function> f);

	void each_above(const std::string &key, each_callback *callback, void *arg);
	void each_above(const std::string &key, std::function<each_function> f);
	void each_above(const std::string &key, std::function<each_string_function> f);

	void each_below(const std::string &key, each_callback *callback, void *arg);
	void each_below(const std::string &key, std::function<each_function> f);
	void each_below(const std::string &key, std::function<each_string_function> f);

	void each_between(const std::string &key1, const std::string &key2,
			  each_callback *callback, void *arg);
	void each_between(const std::string &key1, const std::string &key2,
			  std::function<each_function> f);
	void each_between(const std::string &key1, const std::string &key2,
			  std::function<each_string_function> f);

	status exists(const std::string &key);

	void get(const std::string &key, get_callback *callback, void *arg);
	void get(const std::string &key, std::function<get_function> f);
	void get(const std::string &key, std::function<get_string_function> f);
	status get(const std::string &key, std::string *value);

	status put(const std::string &key, const std::string &value);
	status remove(const std::string &key);

private:
	pmemkv_db *_db;
};

/*
 * All functions which will be called by C code must be declared as extern "C"
 * to ensure they have C linkage. It is needed because it is possible that
 * C and C++ functions use different calling conventions.
 */
extern "C" {
static inline void callKVAllFunction(const char *key, size_t keybytes, void *arg)
{
	(*reinterpret_cast<std::function<all_function> *>(arg))(key, keybytes);
}

static inline void callKVAllStringFunction(const char *key, size_t keybytes, void *arg)
{
	(*reinterpret_cast<std::function<all_string_function> *>(arg))(
		std::string(key, keybytes));
}

static inline void callKVEachFunction(const char *key, size_t keybytes, const char *value,
				      size_t valuebytes, void *arg)
{
	(*reinterpret_cast<std::function<each_function> *>(arg))(key, keybytes, value,
								 valuebytes);
}

static inline void callKVEachStringFunction(const char *key, size_t keybytes,
					    const char *value, size_t valuebytes,
					    void *arg)
{
	(*reinterpret_cast<std::function<each_string_function> *>(arg))(
		std::string(key, keybytes), std::string(value, valuebytes));
}

static inline void callKVGetFunction(const char *value, size_t valuebytes, void *arg)
{
	(*reinterpret_cast<std::function<get_function> *>(arg))(value, valuebytes);
}

static inline void callKVGetStringFunction(const char *value, size_t valuebytes,
					   void *arg)
{
	(*reinterpret_cast<std::function<all_string_function> *>(arg))(
		std::string(value, valuebytes));
}

static inline void callGet(const char *v, size_t vb, void *arg)
{
	auto c = reinterpret_cast<std::pair<status, std::string *> *>(arg);
	c->first = status::OK;
	c->second->append(v, vb);
}

static inline void callOnStartCallback(void *context, const char *engine,
				       pmemkv_config *config, const char *msg)
{
	auto *string = reinterpret_cast<std::string *>(context);
	*string = msg;
}
}

inline void *db::engine_context()
{
	return pmemkv_engine_context(this->_db);
}

inline void db::all(all_callback *callback, void *arg)
{
	pmemkv_all(this->_db, callback, arg);
}

inline void db::all(std::function<all_function> f)
{
	pmemkv_all(this->_db, callKVAllFunction, &f);
}

inline void db::all(std::function<all_string_function> f)
{
	pmemkv_all(this->_db, callKVAllStringFunction, &f);
}

inline db::db(void *context, const std::string &engine_name, pmemkv_config *config,
	      start_failure_callback *callback)
{
	this->_db = pmemkv_open(context, engine_name.c_str(), config, callback);
}

inline db::db(const std::string &engine_name, pmemkv_config *config)
{
	std::string str;
	this->_db = pmemkv_open(&str, engine_name.c_str(), config, callOnStartCallback);

	if (this->_db == nullptr)
		throw std::runtime_error(str);
}

inline db::~db()
{
	pmemkv_close(this->_db);
}

inline void db::all_above(const std::string &key, all_callback *callback, void *arg)
{
	pmemkv_all_above(this->_db, key.c_str(), key.size(), callback, arg);
}

inline void db::all_above(const std::string &key, std::function<all_function> f)
{
	pmemkv_all_above(this->_db, key.c_str(), key.size(), callKVAllFunction, &f);
}

inline void db::all_above(const std::string &key, std::function<all_string_function> f)
{
	pmemkv_all_above(this->_db, key.c_str(), key.size(), callKVAllStringFunction, &f);
}

inline void db::all_below(const std::string &key, all_callback *callback, void *arg)
{
	pmemkv_all_below(this->_db, key.c_str(), key.size(), callback, arg);
}

inline void db::all_below(const std::string &key, std::function<all_function> f)
{
	pmemkv_all_below(this->_db, key.c_str(), key.size(), callKVAllFunction, &f);
}

inline void db::all_below(const std::string &key, std::function<all_string_function> f)
{
	pmemkv_all_below(this->_db, key.c_str(), key.size(), callKVAllStringFunction, &f);
}

inline void db::all_between(const std::string &key1, const std::string &key2,
			    all_callback *callback, void *arg)
{
	pmemkv_all_between(this->_db, key1.c_str(), key1.size(), key2.c_str(),
			   key2.size(), callback, arg);
}

inline void db::all_between(const std::string &key1, const std::string &key2,
			    std::function<all_function> f)
{
	pmemkv_all_between(this->_db, key1.c_str(), key1.size(), key2.c_str(),
			   key2.size(), callKVAllFunction, &f);
}

inline void db::all_between(const std::string &key1, const std::string &key2,
			    std::function<all_string_function> f)
{
	pmemkv_all_between(this->_db, key1.c_str(), key1.size(), key2.c_str(),
			   key2.size(), callKVAllStringFunction, &f);
}

inline std::size_t db::count()
{
	return pmemkv_count(this->_db);
}

inline std::size_t db::count_above(const std::string &key)
{
	return pmemkv_count_above(this->_db, key.c_str(), key.size());
}

inline std::size_t db::count_below(const std::string &key)
{
	return pmemkv_count_below(this->_db, key.c_str(), key.size());
}

inline std::size_t db::count_between(const std::string &key1, const std::string &key2)
{
	return pmemkv_count_between(this->_db, key1.c_str(), key1.size(), key2.c_str(),
				    key2.size());
}

inline void db::each(each_callback *callback, void *arg)
{
	pmemkv_each(this->_db, callback, arg);
}

inline void db::each(std::function<each_function> f)
{
	pmemkv_each(this->_db, callKVEachFunction, &f);
}

inline void db::each(std::function<each_string_function> f)
{
	pmemkv_each(this->_db, callKVEachStringFunction, &f);
}

inline void db::each_above(const std::string &key, each_callback *callback, void *arg)
{
	pmemkv_each_above(this->_db, key.c_str(), key.size(), callback, arg);
}

inline void db::each_above(const std::string &key, std::function<each_function> f)
{
	pmemkv_each_above(this->_db, key.c_str(), key.size(), callKVEachFunction, &f);
}

inline void db::each_above(const std::string &key, std::function<each_string_function> f)
{
	pmemkv_each_above(this->_db, key.c_str(), key.size(), callKVEachStringFunction,
			  &f);
}

inline void db::each_below(const std::string &key, each_callback *callback, void *arg)
{
	pmemkv_each_below(this->_db, key.c_str(), key.size(), callback, arg);
}

inline void db::each_below(const std::string &key, std::function<each_function> f)
{
	pmemkv_each_below(this->_db, key.c_str(), key.size(), callKVEachFunction, &f);
}

inline void db::each_below(const std::string &key, std::function<each_string_function> f)
{
	pmemkv_each_below(this->_db, key.c_str(), key.size(), callKVEachStringFunction,
			  &f);
}

inline void db::each_between(const std::string &key1, const std::string &key2,
			     each_callback *callback, void *arg)
{
	pmemkv_each_between(this->_db, key1.c_str(), key1.size(), key2.c_str(),
			    key2.size(), callback, arg);
}

inline void db::each_between(const std::string &key1, const std::string &key2,
			     std::function<each_function> f)
{
	pmemkv_each_between(this->_db, key1.c_str(), key1.size(), key2.c_str(),
			    key2.size(), callKVEachFunction, &f);
}

inline void db::each_between(const std::string &key1, const std::string &key2,
			     std::function<each_string_function> f)
{
	pmemkv_each_between(this->_db, key1.c_str(), key1.size(), key2.c_str(),
			    key2.size(), callKVEachStringFunction, &f);
}

inline status db::exists(const std::string &key)
{
	return (status)pmemkv_exists(this->_db, key.c_str(), key.size());
}

inline void db::get(const std::string &key, get_callback *callback, void *arg)
{
	pmemkv_get(this->_db, key.c_str(), key.size(), callback, arg);
}

inline void db::get(const std::string &key, std::function<get_function> f)
{
	pmemkv_get(this->_db, key.c_str(), key.size(), callKVGetFunction, &f);
}

inline void db::get(const std::string &key, std::function<get_string_function> f)
{
	pmemkv_get(this->_db, key.c_str(), key.size(), callKVGetStringFunction, &f);
}

inline status db::get(const std::string &key, std::string *value)
{
	std::pair<status, std::string *> ctx = {status::NOT_FOUND, value};

	pmemkv_get(this->_db, key.c_str(), key.size(), callGet, &ctx);

	return ctx.first;
}

inline status db::put(const std::string &key, const std::string &value)
{
	return (status)pmemkv_put(this->_db, key.c_str(), key.size(), value.c_str(),
				  value.size());
}

inline status db::remove(const std::string &key)
{
	return (status)pmemkv_remove(this->_db, key.c_str(), key.size());
}

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_HPP */
