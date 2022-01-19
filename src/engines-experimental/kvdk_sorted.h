// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2022, Intel Corporation */

#ifndef LIBPMEMKV_KVDK_SORTED_H
#define LIBPMEMKV_KVDK_SORTED_H

#include <memory>

#include "kvdk/engine.hpp"

#include "../engine.h"

#include "kvdk_status.h"

namespace pmem
{
namespace kv
{

namespace storage_engine = KVDK_NAMESPACE;

class kvdk_sorted : public engine_base {
	class kvdk_iterator;
	class kvdk_const_iterator;

public:
	kvdk_sorted(std::unique_ptr<internal::config> cfg);
	~kvdk_sorted();

	std::string name() final;

	status count_all(std::size_t &cnt) final;
	status count_above(string_view key, std::size_t &cnt) final;
	status count_equal_above(string_view key, std::size_t &cnt) final;
	status count_equal_below(string_view key, std::size_t &cnt) final;
	status count_below(string_view key, std::size_t &cnt) final;
	status count_between(string_view key1, string_view key2, std::size_t &cnt) final;

	status get_all(get_kv_callback *callback, void *arg) final;
	status get_above(string_view key, get_kv_callback *callback, void *arg) final;
	status get_equal_above(string_view key, get_kv_callback *callback,
			       void *arg) final;
	status get_equal_below(string_view key, get_kv_callback *callback,
			       void *arg) final;
	status get_below(string_view key, get_kv_callback *callback, void *arg) final;
	status get_between(string_view key1, string_view key2, get_kv_callback *callback,
			   void *arg) final;

	status exists(string_view key) final;

	status get(string_view key, get_v_callback *callback, void *arg) final;

	status put(string_view key, string_view value) final;

	status remove(string_view key) final;

	internal::iterator_base *new_iterator() final;
	internal::iterator_base *new_const_iterator() final;

private:
	const std::string collection{"global_collection"};
	// XXX: change to uniqe_ptr
	storage_engine::Engine *engine;
};

class kvdk_sorted::kvdk_iterator : public internal::iterator_base {
public:
	status seek(string_view key) final;

	result<string_view> key() final;

	result<pmem::obj::slice<const char *>> read_range(size_t pos, size_t n) final;

private:
	std::string name();
};

class kvdk_sorted::kvdk_const_iterator : public internal::iterator_base {
public:
	status seek(string_view key) final;

	result<string_view> key() final;

	result<pmem::obj::slice<const char *>> read_range(size_t pos, size_t n) final;

	explicit kvdk_const_iterator(std::shared_ptr<kvdk::Iterator> iter)
	    : iterator{iter}
	{
	}

private:
	std::string name();

	std::shared_ptr<kvdk::Iterator> iterator;
	std::string key_local;
};

class kvdk_sorted_factory : public engine_base::factory_base {
public:
	std::unique_ptr<engine_base>
	create(std::unique_ptr<internal::config> cfg) override
	{
		return std::unique_ptr<engine_base>(new kvdk_sorted(std::move(cfg)));
	};
	std::string get_name() override
	{
		return "kvdk_sorted";
	};
};

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_KVDK_SORTED_H */
