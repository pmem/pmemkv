// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2021, Intel Corporation */

#ifndef LIBPMEMKV_CONFIG_H
#define LIBPMEMKV_CONFIG_H

#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include "exceptions.h"
#include "libpmemkv.hpp"

namespace pmem
{
namespace kv
{
namespace internal
{

class config {
public:
	config() = default;

	void put_data(const char *key, const void *value, size_t value_size)
	{
		put(key, value, value_size);
	}

	void put_object(
		const char *key, void *value, void (*deleter)(void *),
		void *(*getter)(void *) = [](void *arg) { return arg; })
	{
		put(key, value, deleter, getter);
	}

	void put_int64(const char *key, int64_t value)
	{
		put(key, value);
	}

	void put_uint64(const char *key, uint64_t value)
	{
		put(key, value);
	}

	void put_string(const char *key, const char *value)
	{
		put(key, value);
	}

	/*
	 * @return 'false' if no item with specified key exists,
	 * 'true' if item was obtained successfully
	 *
	 * @throw pmem::kv::internal::type_error if item has type different than 'data'
	 */
	bool get_data(const char *key, const void **value, size_t *value_size)
	{
		auto item = get_checked_item(key, type::DATA);
		if (!item)
			return false;

		*value = item->data.data();
		*value_size = item->data.size();

		return true;
	}

	/*
	 * @return 'false' if no item with specified key exists,
	 * 'true' if item was obtained successfully
	 *
	 * @throw pmem::kv::internal::type_error if item has type different than 'object'
	 */
	bool get_object(const char *key, void **value)
	{
		auto item = get_checked_item(key, type::OBJECT);
		if (item == nullptr)
			return false;

		*value = (*item->object.getter)(item->object.ptr);

		return true;
	}

	/*
	 * @return 'false' if no item with specified key exists,
	 * 'true' if item was obtained successfully
	 *
	 * @throw pmem::kv::internal::type_error if item has type different than 'int64'
	 * or 'uint64' (and convertible to int64)
	 */
	bool get_int64(const char *key, int64_t *value)
	{
		auto item = get(key);
		if (item == nullptr)
			return false;

		if (item->item_type == type::INT64)
			*value = item->sint64;
		else if (item->item_type == type::UINT64) {
			/* Conversion from uint64 allowed */
			if (item->uint64 <=
			    static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
				*value = static_cast<int64_t>(item->uint64);
			else
				throw config_type_error(
					"Item with key: " + std::string(key) +
					" has value which exceeds int64 range");
		} else
			throw_type_error(key, item->item_type, type::INT64);

		return true;
	}

	/*
	 * @return 'false' if no item with specified key exists,
	 * 'true' if item was obtained successfully
	 *
	 * @throw pmem::kv::internal::type_error if item has type different than 'uint64'
	 * or 'int64' (and convertible to uint64)
	 */
	bool get_uint64(const char *key, uint64_t *value)
	{
		auto item = get(key);
		if (item == nullptr)
			return false;

		if (item->item_type == type::UINT64)
			*value = item->uint64;
		else if (item->item_type == type::INT64) {
			/* Conversion from int64 allowed */
			if (item->sint64 >= 0)
				*value = static_cast<uint64_t>(item->sint64);
			else
				throw config_type_error(
					"Item with key: " + std::string(key) + " is < 0");
		} else
			throw_type_error(key, item->item_type, type::UINT64);

		return true;
	}

	/*
	 * @return 'false' if no item with specified key exists,
	 * 'true' if item was obtained successfully
	 *
	 * @throw pmem::kv::internal::type_error if item has type different than 'string'
	 */
	bool get_string(const char *key, const char **value)
	{
		auto item = get_checked_item(key, type::STRING);
		if (item == nullptr)
			return false;

		*value = item->string_v.c_str();
		return true;
	}

	/*
	 * Returns value for path property from config.
	 *
	 * @throw pmem::kv::internal::invalid_argument if item does not exist
	 */
	std::string get_path()
	{
		const char *path;
		if (!get_string("path", &path))
			throw internal::invalid_argument(
				"Config does not contain item with key: \"path\"");

		return std::string(path);
	}

	/*
	 * Returns value for size property from config.
	 *
	 * @throw pmem::kv::internal::invalid_argument if item does not exist
	 */
	uint64_t get_size()
	{
		std::size_t size;
		if (!get_uint64("size", &size))
			throw internal::invalid_argument(
				"Config does not contain item with key: \"size\"");

		return size;
	}

private:
	std::string type_names[6] = {"string", "int64", "uint64", "data", "object"};
	enum class type { STRING, INT64, UINT64, DATA, OBJECT };

	struct variant {
		variant(int64_t sint64) : sint64(sint64), item_type(type::INT64)
		{
		}
		variant(uint64_t uint64) : uint64(uint64), item_type(type::UINT64)
		{
		}
		variant(const char *string_v)
		    : string_v(string_v), item_type(type::STRING)
		{
		}
		variant(void *object, void (*deleter)(void *), void *(*getter)(void *))
		    : object{object, deleter, getter}, item_type(type::OBJECT)
		{
		}
		variant(const void *data, std::size_t size)
		    : data((const char *)data, (const char *)data + size),
		      item_type(type::DATA)
		{
		}

		~variant()
		{
			if (item_type == type::STRING)
				string_v.~basic_string();
			else if (item_type == type::DATA)
				data.~vector();
			else if (item_type == type::OBJECT && object.deleter != nullptr)
				object.deleter(object.ptr);
		}

		union {
			int64_t sint64;
			uint64_t uint64;
			std::string string_v;
			std::vector<char> data;

			struct {
				void *ptr;
				void (*deleter)(void *);
				void *(*getter)(void *);
			} object;
		};

		type item_type;
	};

	std::unordered_map<std::string, variant> umap;

	template <typename... Args>
	void put(const std::string &key, Args &&... args)
	{
		auto ret =
			umap.emplace(std::piecewise_construct, std::forward_as_tuple(key),
				     std::forward_as_tuple(std::forward<Args>(args)...));

		if (!ret.second)
			throw error("Item with key: " + key + " already exists");
	}

	variant *get(const std::string &key)
	{
		auto found = umap.find(key);

		if (found == umap.end())
			return nullptr;

		return &found->second;
	}

	void throw_type_error(const std::string &key, type item_type, type expected_type)
	{
		throw config_type_error(
			"Item with key: " + key + " is " +
			type_names[static_cast<int>(item_type)] +
			". Expected: " + type_names[static_cast<int>(expected_type)]);
	}

	struct variant *get_checked_item(const char *key, type expected_type)
	{
		auto item = get(key);
		if (!item)
			return nullptr;

		if (item->item_type != expected_type)
			throw_type_error(key, item->item_type, expected_type);
		return item;
	}
};

} /* namespace internal */
} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_CONFIG_H */
