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

class string_view;

typedef void all_function(string_view key);
typedef void each_function(string_view key, string_view value);
typedef void get_function(string_view value);

using all_callback = pmemkv_all_callback;
using each_callback = pmemkv_each_callback;
using get_callback = pmemkv_get_callback;
using start_failure_callback = pmemkv_start_failure_callback;

enum class status {
	FAILED = PMEMKV_STATUS_FAILED,
	NOT_FOUND = PMEMKV_STATUS_NOT_FOUND,
	OK = PMEMKV_STATUS_OK,
};

class string_view {
public:
	string_view() : _data(""), _size(0)
	{
	}

	string_view(const char *data, size_t size) : _data(data), _size(size)
	{
	}

	string_view(const std::string &s) : _data(s.c_str()), _size(s.size())
	{
	}
	string_view(const char *data)
	    : _data(data), _size(std::char_traits<char>::length(data))
	{
	}

	string_view(const string_view &rhs) = default;

	string_view &operator=(const string_view &rhs) = default;

	const char *data() const
	{
		return _data;
	}

	std::size_t size() const
	{
		return _size;
	}

	std::string to_string() const
	{
		return std::string(_data, _size);
	}

	int compare(const string_view &other)
	{
		int ret = std::char_traits<char>::compare(data(), other.data(),
							  std::min(size(), other.size()));
		if (ret != 0)
			return ret;
		if (size() < other.size())
			return -1;
		if (size() > other.size())
			return 1;
		return 0;
	}

private:
	const char *_data;
	std::size_t _size;
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

	void all_above(string_view key, all_callback *callback, void *arg);
	void all_above(string_view key, std::function<all_function> f);

	void all_below(string_view key, all_callback *callback, void *arg);
	void all_below(string_view key, std::function<all_function> f);

	void all_between(string_view key1, string_view key2, all_callback *callback,
			 void *arg);
	void all_between(string_view key1, string_view key2,
			 std::function<all_function> f);

	std::size_t count();
	std::size_t count_above(string_view key);
	std::size_t count_below(string_view key);
	std::size_t count_between(string_view key1, string_view key2);

	void each(each_callback *callback, void *arg);
	void each(std::function<each_function> f);

	void each_above(string_view key, each_callback *callback, void *arg);
	void each_above(string_view key, std::function<each_function> f);

	void each_below(string_view key, each_callback *callback, void *arg);
	void each_below(string_view key, std::function<each_function> f);

	void each_between(string_view key1, string_view key2, each_callback *callback,
			  void *arg);
	void each_between(string_view key1, string_view key2,
			  std::function<each_function> f);

	status exists(string_view key);

	void get(string_view key, get_callback *callback, void *arg);
	void get(string_view key, std::function<get_function> f);
	status get(string_view key, std::string *value);

	status put(string_view key, string_view value);
	status remove(string_view key);

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
	(*reinterpret_cast<std::function<all_function> *>(arg))(
		string_view(key, keybytes));
}

static inline void callKVEachFunction(const char *key, size_t keybytes, const char *value,
				      size_t valuebytes, void *arg)
{
	(*reinterpret_cast<std::function<each_function> *>(arg))(
		string_view(key, keybytes), string_view(value, valuebytes));
}

static inline void callKVGetFunction(const char *value, size_t valuebytes, void *arg)
{
	(*reinterpret_cast<std::function<all_function> *>(arg))(
		string_view(value, valuebytes));
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

inline void db::all_above(string_view key, all_callback *callback, void *arg)
{
	pmemkv_all_above(this->_db, key.data(), key.size(), callback, arg);
}

inline void db::all_above(string_view key, std::function<all_function> f)
{
	pmemkv_all_above(this->_db, key.data(), key.size(), callKVAllFunction, &f);
}

inline void db::all_below(string_view key, all_callback *callback, void *arg)
{
	pmemkv_all_below(this->_db, key.data(), key.size(), callback, arg);
}

inline void db::all_below(string_view key, std::function<all_function> f)
{
	pmemkv_all_below(this->_db, key.data(), key.size(), callKVAllFunction, &f);
}

inline void db::all_between(string_view key1, string_view key2, all_callback *callback,
			    void *arg)
{
	pmemkv_all_between(this->_db, key1.data(), key1.size(), key2.data(), key2.size(),
			   callback, arg);
}

inline void db::all_between(string_view key1, string_view key2,
			    std::function<all_function> f)
{
	pmemkv_all_between(this->_db, key1.data(), key1.size(), key2.data(), key2.size(),
			   callKVAllFunction, &f);
}

inline std::size_t db::count()
{
	return pmemkv_count(this->_db);
}

inline std::size_t db::count_above(string_view key)
{
	return pmemkv_count_above(this->_db, key.data(), key.size());
}

inline std::size_t db::count_below(string_view key)
{
	return pmemkv_count_below(this->_db, key.data(), key.size());
}

inline std::size_t db::count_between(string_view key1, string_view key2)
{
	return pmemkv_count_between(this->_db, key1.data(), key1.size(), key2.data(),
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

inline void db::each_above(string_view key, each_callback *callback, void *arg)
{
	pmemkv_each_above(this->_db, key.data(), key.size(), callback, arg);
}

inline void db::each_above(string_view key, std::function<each_function> f)
{
	pmemkv_each_above(this->_db, key.data(), key.size(), callKVEachFunction, &f);
}

inline void db::each_below(string_view key, each_callback *callback, void *arg)
{
	pmemkv_each_below(this->_db, key.data(), key.size(), callback, arg);
}

inline void db::each_below(string_view key, std::function<each_function> f)
{
	pmemkv_each_below(this->_db, key.data(), key.size(), callKVEachFunction, &f);
}

inline void db::each_between(string_view key1, string_view key2, each_callback *callback,
			     void *arg)
{
	pmemkv_each_between(this->_db, key1.data(), key1.size(), key2.data(), key2.size(),
			    callback, arg);
}

inline void db::each_between(string_view key1, string_view key2,
			     std::function<each_function> f)
{
	pmemkv_each_between(this->_db, key1.data(), key1.size(), key2.data(), key2.size(),
			    callKVEachFunction, &f);
}

inline status db::exists(string_view key)
{
	return (status)pmemkv_exists(this->_db, key.data(), key.size());
}

inline void db::get(string_view key, get_callback *callback, void *arg)
{
	pmemkv_get(this->_db, key.data(), key.size(), callback, arg);
}

inline void db::get(string_view key, std::function<get_function> f)
{
	pmemkv_get(this->_db, key.data(), key.size(), callKVGetFunction, &f);
}

inline status db::get(string_view key, std::string *value)
{
	std::pair<status, std::string *> ctx = {status::NOT_FOUND, value};

	pmemkv_get(this->_db, key.data(), key.size(), callGet, &ctx);

	return ctx.first;
}

inline status db::put(string_view key, string_view value)
{
	return (status)pmemkv_put(this->_db, key.data(), key.size(), value.data(),
				  value.size());
}

inline status db::remove(string_view key)
{
	return (status)pmemkv_remove(this->_db, key.data(), key.size());
}

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_HPP */
