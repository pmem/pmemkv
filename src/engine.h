// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2021, Intel Corporation */

#ifndef LIBPMEMKV_ENGINE_H
#define LIBPMEMKV_ENGINE_H

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "config.h"
#include "iterator.h"
#include "libpmemkv.hpp"
#include "transaction.h"

namespace pmem
{
namespace kv
{

void check_config_null(const std::string &engine_name,
		       std::unique_ptr<internal::config> &cfg);

class engine_base {
	using iterator = internal::iterator_base;

public:
	engine_base();
	virtual ~engine_base();

	virtual std::string name() = 0;

	virtual status count_all(std::size_t &cnt);
	virtual status count_above(string_view key, std::size_t &cnt);
	virtual status count_equal_above(string_view key, std::size_t &cnt);
	virtual status count_equal_below(string_view key, std::size_t &cnt);
	virtual status count_below(string_view key, std::size_t &cnt);
	virtual status count_between(string_view key1, string_view key2,
				     std::size_t &cnt);

	virtual status get_all(get_kv_callback *callback, void *arg);
	virtual status get_above(string_view key, get_kv_callback *callback, void *arg);
	virtual status get_equal_above(string_view key, get_kv_callback *callback,
				       void *arg);
	virtual status get_equal_below(string_view key, get_kv_callback *callback,
				       void *arg);
	virtual status get_below(string_view key, get_kv_callback *callback, void *arg);
	virtual status get_between(string_view key1, string_view key2,
				   get_kv_callback *callback, void *arg);

	virtual status exists(string_view key);

	virtual status get(string_view key, get_v_callback *callback, void *arg) = 0;
	virtual status put(string_view key, string_view value) = 0;
	virtual status remove(string_view key) = 0;
	virtual status defrag(double start_percent, double amount_percent);

	virtual internal::transaction *begin_tx();

	virtual iterator *new_iterator();
	virtual iterator *new_const_iterator();

	class factory_base {
	public:
		factory_base() = default;
		virtual ~factory_base() = default;
		virtual std::unique_ptr<engine_base>
			create(std::unique_ptr<internal::config>) = 0;
		virtual std::string get_name() = 0;
	};
};

class storage_engine_factory {
public:
	using factory_object = std::unique_ptr<engine_base::factory_base>;

	storage_engine_factory() = delete;
	static bool Register(factory_object factory);
	static std::unique_ptr<engine_base> Create(const std::string &name,
						   std::unique_ptr<internal::config> cfg);

private:
	static std::map<std::string, factory_object> &get_pairs();
	static std::string get_names();
};

class factory_registerer {
public:
	factory_registerer() = delete;
	factory_registerer(storage_engine_factory::factory_object factory)
	{
		storage_engine_factory::Register(std::move(factory));
	}
};

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_ENGINE_H */
