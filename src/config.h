/*
 * Copyright 2019, Intel Corporation
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

#ifndef LIBPMEMKV_CONFIG_H
#define LIBPMEMKV_CONFIG_H

#include <cstring>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "exceptions.h"
#include "libpmemkv.hpp"
#include "out.h"

namespace pmem
{
namespace kv
{
namespace internal
{

struct config {
public:
	config() = default;

	void put_data(const char *key, const void *value, size_t value_size)
	{
		put(key, value, value_size);
	}

	void put_object(const char *key, void *value, void (*deleter)(void *))
	{
		put(key, value, deleter);
	}

	void put_int64(const char *key, int64_t value)
	{
		put(key, value);
	}

	void put_uint64(const char *key, uint64_t value)
	{
		put(key, value);
	}

	void put_double(const char *key, double value)
	{
		put(key, value);
	}

	void put_string(const char *key, const char *value)
	{
		put(key, value);
	}

	bool get_data(const char *key, const void **value, size_t *value_size)
	{
		auto item = get(key);
		if (!item.second)
			return false;

		if (item.first->item_type != type::DATA)
			throw_type_error(key, item.first->item_type);

		*value = item.first->data.data();
		*value_size = item.first->data.size();

		return true;
	}

	/*
	 * @return 'false' if no item with specified key exists,
	 * 'true' if item was obtained succesfully
	 *
	 * @throw pmem::kv::internal::type_error if item has type different than 'object'
	 */
	bool get_object(const char *key, void **value)
	{
		auto item = get(key);
		if (!item.second)
			return false;

		if (item.first->item_type != type::OBJECT)
			throw_type_error(key, item.first->item_type);

		*value = item.first->object.ptr;

		return true;
	}

	/*
	 * @return 'false' if no item with specified key exists,
	 * 'true' if item was obtained succesfully
	 *
	 * @throw pmem::kv::internal::type_error if item has type different than 'int64'
	 * or 'uint64' (and convertible to int64)
	 */
	bool get_int64(const char *key, int64_t *value)
	{
		auto item = get(key);
		if (!item.second)
			return false;

		if (item.first->item_type == type::INT64)
			*value = item.first->sint64;
		else if (item.first->item_type == type::UINT64) {
			/* Conversion from uint64 allowed */
			if (item.first->uint64 <= std::numeric_limits<int64_t>::max())
				*value = static_cast<int64_t>(item.first->uint64);
			else
				throw config_type_error(
					"Element with key: " + std::string(key) +
					" has value which exceeds int64 range");
		} else
			throw_type_error(key, item.first->item_type);

		return true;
	}

	/*
	 * @return 'false' if no item with specified key exists,
	 * 'true' if item was obtained succesfully
	 *
	 * @throw pmem::kv::internal::type_error if item has type different than 'uint64'
	 * or 'int64' (and convertible to uint64)
	 */
	bool get_uint64(const char *key, uint64_t *value)
	{
		auto item = get(key);
		if (!item.second)
			return false;

		if (item.first->item_type == type::UINT64)
			*value = item.first->uint64;
		else if (item.first->item_type == type::INT64) {
			/* Conversion from int64 allowed */
			if (item.first->sint64 >= 0)
				*value = static_cast<uint64_t>(item.first->sint64);
			else
				throw config_type_error("Element with key: " +
							std::string(key) + " is < 0");
		} else
			throw_type_error(key, item.first->item_type);

		return true;
	}

	/*
	 * @return 'false' if no item with specified key exists,
	 * 'true' if item was obtained succesfully
	 *
	 * @throw pmem::kv::internal::type_error if item has type different than 'double'
	 */
	bool get_double(const char *key, double *value)
	{
		auto item = get(key);
		if (!item.second)
			return false;

		if (item.first->item_type != type::DOUBLE)
			throw_type_error(key, item.first->item_type);

		*value = item.first->double_v;

		return true;
	}

	/*
	 * @return 'false' if no item with specified key exists,
	 * 'true' if item was obtained succesfully
	 *
	 * @throw pmem::kv::internal::type_error if item has type different than 'string'
	 */
	bool get_string(const char *key, const char **value)
	{
		auto item = get(key);
		if (!item.second)
			return false;

		if (item.first->item_type != type::STRING)
			throw_type_error(key, item.first->item_type);

		*value = item.first->string.c_str();

		return true;
	}

private:
	std::string type_names[6] = {"string", "int64", "uint64",
				     "double", "data",  "object"};
	enum class type { STRING, INT64, UINT64, DOUBLE, DATA, OBJECT };

	struct entry {
		entry(int64_t sint64) : sint64(sint64), item_type(type::INT64)
		{
		}
		entry(uint64_t uint64) : uint64(uint64), item_type(type::UINT64)
		{
		}
		entry(double double_v) : double_v(double_v), item_type(type::DOUBLE)
		{
		}
		entry(const char *string) : string(string), item_type(type::STRING)
		{
		}
		entry(void *object, void (*deleter)(void *))
		    : object{object, deleter}, item_type(type::OBJECT)
		{
		}
		entry(const void *data, std::size_t size)
		    : data((const char *)data, (const char *)data + size),
		      item_type(type::DATA)
		{
		}

		~entry()
		{
			if (item_type == type::STRING)
				string.~basic_string();
			if (item_type == type::DATA)
				data.~vector();
			if (item_type == type::OBJECT) {
				if (object.deleter != nullptr)
					object.deleter(object.ptr);
			}
		}

		union {
			int64_t sint64;
			uint64_t uint64;
			double double_v;
			std::string string;
			std::vector<char> data;

			struct {
				void *ptr;
				void (*deleter)(void *);
			} object;
		};

		type item_type;
	};

	std::unordered_map<std::string, entry> umap;

	template <typename... Args>
	void put(const std::string &key, Args &&... args)
	{
		auto ret =
			umap.emplace(std::piecewise_construct, std::forward_as_tuple(key),
				     std::forward_as_tuple(std::forward<Args>(args)...));

		if (!ret.second)
			throw error("Element with key: " + std::string(key) +
				    " already exists");
	}

	std::pair<entry *, bool> get(const std::string &key)
	{
		auto found = umap.find(key);

		if (found == umap.end())
			return {nullptr, false};

		return {&found->second, true};
	}

	void throw_type_error(const std::string &key, type item_type)
	{
		throw config_type_error("Element with key: " + std::string(key) + " is " +
					type_names[static_cast<int>(item_type)]);
	}
};
} /* namespace internal */
} /* namespace kv */
} /* namespace pmem */

#endif
