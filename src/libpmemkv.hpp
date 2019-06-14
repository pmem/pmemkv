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

#if __cpp_lib_string_view
using string_view = std::string_view;
#else
class string_view {
public:
	string_view();
	string_view(const char *data, size_t size);
	string_view(const std::string &s);
	string_view(const char *data);

	string_view(const string_view &rhs) = default;
	string_view &operator=(const string_view &rhs) = default;

	const char *data() const;
	std::size_t size() const;

	/**
	 * Compares this string_view with other. Works in the same way as
	 * std::basic_string::compare.
	 *
	 * @return 0 if both character sequences compare equal,
	 *         positive value if this is lexicographically greater than other,
	 *         negative value if this is lexicographically less than other.
	 */
	int compare(const string_view &other);

private:
	const char *_data;
	std::size_t _size;
};
#endif

typedef void all_function(string_view key);
typedef void each_function(string_view key, string_view value);
typedef void get_function(string_view value);

using all_callback = pmemkv_all_callback;
using each_callback = pmemkv_each_callback;
using get_callback = pmemkv_get_callback;

enum class status {
	OK = PMEMKV_STATUS_OK,
	FAILED = PMEMKV_STATUS_FAILED,
	NOT_FOUND = PMEMKV_STATUS_NOT_FOUND,
	NOT_SUPPORTED = PMEMKV_STATUS_NOT_SUPPORTED,
	INVALID_ARGUMENT = PMEMKV_STATUS_INVALID_ARGUMENT,
	CONFIG_PARSING_ERROR = PMEMKV_STATUS_CONFIG_PARSING_ERROR,
	CONFIG_TYPE_ERROR = PMEMKV_STATUS_CONFIG_TYPE_ERROR,
};

class db {
public:
	db();
	~db();

	status open(void *context, const std::string &engine_name, pmemkv_config *config);
	status open(const std::string &engine_name, pmemkv_config *config);

	void close();

	void *engine_context();

	status all(all_callback *callback, void *arg);
	status all(std::function<all_function> f);

	status all_above(string_view key, all_callback *callback, void *arg);
	status all_above(string_view key, std::function<all_function> f);

	status all_below(string_view key, all_callback *callback, void *arg);
	status all_below(string_view key, std::function<all_function> f);

	status all_between(string_view key1, string_view key2, all_callback *callback,
			   void *arg);
	status all_between(string_view key1, string_view key2,
			   std::function<all_function> f);

	status count(std::size_t &cnt);
	status count_above(string_view key, std::size_t &cnt);
	status count_below(string_view key, std::size_t &cnt);
	status count_between(string_view key1, string_view key2, std::size_t &cnt);

	status each(each_callback *callback, void *arg);
	status each(std::function<each_function> f);

	status each_above(string_view key, each_callback *callback, void *arg);
	status each_above(string_view key, std::function<each_function> f);

	status each_below(string_view key, each_callback *callback, void *arg);
	status each_below(string_view key, std::function<each_function> f);

	status each_between(string_view key1, string_view key2, each_callback *callback,
			    void *arg);
	status each_between(string_view key1, string_view key2,
			    std::function<each_function> f);

	status exists(string_view key);

	status get(string_view key, get_callback *callback, void *arg);
	status get(string_view key, std::function<get_function> f);
	status get(string_view key, std::string *value);

	status put(string_view key, string_view value);
	status remove(string_view key);

private:
	pmemkv_db *_db;
};

#if !__cpp_lib_string_view
inline string_view::string_view() : _data(""), _size(0)
{
}

inline string_view::string_view(const char *data, size_t size) : _data(data), _size(size)
{
}

inline string_view::string_view(const std::string &s) : _data(s.c_str()), _size(s.size())
{
}

inline string_view::string_view(const char *data)
    : _data(data), _size(std::char_traits<char>::length(data))
{
}

inline const char *string_view::data() const
{
	return _data;
}

inline std::size_t string_view::size() const
{
	return _size;
}

inline int string_view::compare(const string_view &other)
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
#endif

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

inline status db::all(all_callback *callback, void *arg)
{
	return static_cast<status>(pmemkv_all(this->_db, callback, arg));
}

inline status db::all(std::function<all_function> f)
{
	return static_cast<status>(pmemkv_all(this->_db, callKVAllFunction, &f));
}

inline db::db()
{
	this->_db = nullptr;
}

inline status db::open(void *context, const std::string &engine_name,
		       pmemkv_config *config)
{
	return static_cast<status>(
		pmemkv_open(context, engine_name.c_str(), config, &(this->_db)));
}

inline status db::open(const std::string &engine_name, pmemkv_config *config)
{
	return static_cast<status>(
		pmemkv_open(nullptr, engine_name.c_str(), config, &(this->_db)));
}

inline void db::close()
{
	if (this->_db != nullptr)
		pmemkv_close(this->_db);

	this->_db = nullptr;
}

inline db::~db()
{
	if (this->_db != nullptr)
		pmemkv_close(this->_db);
}

inline status db::all_above(string_view key, all_callback *callback, void *arg)
{
	return static_cast<status>(
		pmemkv_all_above(this->_db, key.data(), key.size(), callback, arg));
}

inline status db::all_above(string_view key, std::function<all_function> f)
{
	return static_cast<status>(pmemkv_all_above(this->_db, key.data(), key.size(),
						    callKVAllFunction, &f));
}

inline status db::all_below(string_view key, all_callback *callback, void *arg)
{
	return static_cast<status>(
		pmemkv_all_below(this->_db, key.data(), key.size(), callback, arg));
}

inline status db::all_below(string_view key, std::function<all_function> f)
{
	return static_cast<status>(pmemkv_all_below(this->_db, key.data(), key.size(),
						    callKVAllFunction, &f));
}

inline status db::all_between(string_view key1, string_view key2, all_callback *callback,
			      void *arg)
{
	return static_cast<status>(pmemkv_all_between(this->_db, key1.data(), key1.size(),
						      key2.data(), key2.size(), callback,
						      arg));
}

inline status db::all_between(string_view key1, string_view key2,
			      std::function<all_function> f)
{
	return static_cast<status>(pmemkv_all_between(this->_db, key1.data(), key1.size(),
						      key2.data(), key2.size(),
						      callKVAllFunction, &f));
}

inline status db::count(std::size_t &cnt)
{
	return static_cast<status>(pmemkv_count(this->_db, &cnt));
}

inline status db::count_above(string_view key, std::size_t &cnt)
{
	return static_cast<status>(
		pmemkv_count_above(this->_db, key.data(), key.size(), &cnt));
}

inline status db::count_below(string_view key, std::size_t &cnt)
{
	return static_cast<status>(
		pmemkv_count_below(this->_db, key.data(), key.size(), &cnt));
}

inline status db::count_between(string_view key1, string_view key2, std::size_t &cnt)
{
	return static_cast<status>(pmemkv_count_between(
		this->_db, key1.data(), key1.size(), key2.data(), key2.size(), &cnt));
}

inline status db::each(each_callback *callback, void *arg)
{
	return static_cast<status>(pmemkv_each(this->_db, callback, arg));
}

inline status db::each(std::function<each_function> f)
{
	return static_cast<status>(pmemkv_each(this->_db, callKVEachFunction, &f));
}

inline status db::each_above(string_view key, each_callback *callback, void *arg)
{
	return static_cast<status>(
		pmemkv_each_above(this->_db, key.data(), key.size(), callback, arg));
}

inline status db::each_above(string_view key, std::function<each_function> f)
{
	return static_cast<status>(pmemkv_each_above(this->_db, key.data(), key.size(),
						     callKVEachFunction, &f));
}

inline status db::each_below(string_view key, each_callback *callback, void *arg)
{
	return static_cast<status>(
		pmemkv_each_below(this->_db, key.data(), key.size(), callback, arg));
}

inline status db::each_below(string_view key, std::function<each_function> f)
{
	return static_cast<status>(pmemkv_each_below(this->_db, key.data(), key.size(),
						     callKVEachFunction, &f));
}

inline status db::each_between(string_view key1, string_view key2,
			       each_callback *callback, void *arg)
{
	return static_cast<status>(pmemkv_each_between(this->_db, key1.data(),
						       key1.size(), key2.data(),
						       key2.size(), callback, arg));
}

inline status db::each_between(string_view key1, string_view key2,
			       std::function<each_function> f)
{
	return static_cast<status>(
		pmemkv_each_between(this->_db, key1.data(), key1.size(), key2.data(),
				    key2.size(), callKVEachFunction, &f));
}

inline status db::exists(string_view key)
{
	return static_cast<status>(pmemkv_exists(this->_db, key.data(), key.size()));
}

inline status db::get(string_view key, get_callback *callback, void *arg)
{
	return static_cast<status>(
		pmemkv_get(this->_db, key.data(), key.size(), callback, arg));
}

inline status db::get(string_view key, std::function<get_function> f)
{
	return static_cast<status>(
		pmemkv_get(this->_db, key.data(), key.size(), callKVGetFunction, &f));
}

inline status db::get(string_view key, std::string *value)
{
	std::pair<status, std::string *> ctx = {status::NOT_FOUND, value};

	pmemkv_get(this->_db, key.data(), key.size(), callGet, &ctx);

	return ctx.first;
}

inline status db::put(string_view key, string_view value)
{
	return static_cast<status>(pmemkv_put(this->_db, key.data(), key.size(),
					      value.data(), value.size()));
}

inline status db::remove(string_view key)
{
	return static_cast<status>(pmemkv_remove(this->_db, key.data(), key.size()));
}

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_HPP */
