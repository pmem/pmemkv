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
#include <vector>

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

	~config()
	{
		for (auto &item : umap) {
			if (item.second.element_type == type::OBJECT) {
				void *object;
				memcpy(&object, item.second.value.data(),
				       item.second.value.size());

				if (item.second.deleter != nullptr)
					item.second.deleter(object);
			}
		}
	}

	status put_data(const char *key, const void *value, size_t value_size)
	{
		return put(key, value, value_size, type::DATA);
	}

	status put_object(const char *key, void *value, void (*deleter)(void *))
	{
		try {
			std::string mkey(key);
			std::vector<char> v((char *)&value,
					    (char *)&value + sizeof(void *));
			auto ret = umap.insert({mkey, {v, deleter, type::OBJECT}});

			if (!ret.second) {
				ERR() << "Element with key: " << key << " already exists";
				return status::FAILED;
			}
		} catch (...) {
			ERR() << "Unspecified failure";
			return status::FAILED;
		}

		return status::OK;
	}

	status put_int64(const char *key, int64_t value)
	{
		return put(key, &value, sizeof(value), type::INT64);
	}

	status put_uint64(const char *key, uint64_t value)
	{
		return put(key, &value, sizeof(value), type::UINT64);
	}

	status put_double(const char *key, double value)
	{
		return put(key, &value, sizeof(value), type::DOUBLE);
	}

	status put_string(const char *key, const char *value)
	{
		return put(key, value, std::char_traits<char>::length(value) + 1,
			   type::STRING);
	}

	status get_data(const char *key, const void **value, size_t *value_size)
	{
		return get(key, value, value_size, nullptr);
	}

	status get_object(const char *key, void **value)
	{
		size_t size;
		void *ptr_ptr;

		auto status = get(key, (const void **)&ptr_ptr, &size, nullptr);
		if (status == status::OK && size != sizeof(void *))
			return status::CONFIG_TYPE_ERROR;

		if (status == status::OK)
			memcpy(value, ptr_ptr, sizeof(void *));

		return status;
	}

	status get_int64(const char *key, int64_t *value)
	{
		const void *data;
		size_t value_size;
		type type;

		auto status = get(key, &data, &value_size, &type);
		if (status != status::OK)
			return status;

		if (type == type::INT64) {
			*value = *(static_cast<const int64_t *>(data));
			return status::OK;
		} else if (type == type::UINT64) {
			/* conversion from uint64 allowed */
			auto uval = *(static_cast<const uint64_t *>(data));
			if (uval < std::numeric_limits<int64_t>::max()) {
				*value = *(static_cast<const int64_t *>(data));
				return status::OK;
			}
		}

		return status::CONFIG_TYPE_ERROR;
	}

	status get_uint64(const char *key, uint64_t *value)
	{
		const void *data;
		size_t value_size;
		type type;

		auto status = get(key, &data, &value_size, &type);
		if (status != status::OK)
			return status;

		if (type == type::UINT64) {
			*value = *(static_cast<const uint64_t *>(data));
			return status::OK;
		} else if (type == type::INT64) {
			/* conversion from int64 allowed */
			auto sval = *(static_cast<const int64_t *>(data));
			if (sval >= 0) {
				*value = *(static_cast<const uint64_t *>(data));
				return status::OK;
			}
		}

		return status::CONFIG_TYPE_ERROR;
	}

	status get_double(const char *key, double *value)
	{
		const void *data;
		size_t value_size;
		type type;

		auto status = get(key, &data, &value_size, &type);
		if (status != status::OK)
			return status;

		if (type != type::DOUBLE)
			return status::CONFIG_TYPE_ERROR;

		*value = *((const double *)data);

		return status::OK;
	}

	status get_string(const char *key, const char **value)
	{
		const void *data;
		size_t value_size;
		type type;

		auto status = get(key, &data, &value_size, &type);
		if (status != status::OK)
			return status;

		if (type != type::STRING)
			return status::CONFIG_TYPE_ERROR;

		*value = (const char *)data;

		return status::OK;
	}

private:
	enum class type { STRING, INT64, UINT64, DOUBLE, DATA, OBJECT };

	struct entry {
		std::vector<char> value;
		void (*deleter)(void *);
		type element_type;
	};

	std::unordered_map<std::string, entry> umap;

	status put(const char *key, const void *value, size_t value_size, type type)
	{
		try {
			std::string mkey(key);
			std::vector<char> v((const char *)value,
					    (const char *)value + value_size);
			auto ret = umap.insert({mkey, {v, nullptr, type}});

			if (!ret.second) {
				ERR() << "Element with key: " << key << " already exists";
				return status::FAILED;
			}
		} catch (...) {
			ERR() << "Unspecified failure";
			return status::FAILED;
		}

		return status::OK;
	}

	status get(const char *key, const void **value, size_t *value_size, type *type)
	{
		try {
			auto found = umap.find(key);

			if (found == umap.end())
				return status::NOT_FOUND;

			auto &mvalue = found->second.value;

			if (value)
				*value = mvalue.data();

			if (value_size)
				*value_size = mvalue.size();

			if (type)
				*type = found->second.element_type;
		} catch (...) {
			ERR() << "Unspecified failure";
			return status::FAILED;
		}

		return status::OK;
	}
};
}
}
}

#endif
