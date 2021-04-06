// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2021, Intel Corporation */

#ifndef LIBPMEMKV_ENGINE_H
#define LIBPMEMKV_ENGINE_H

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

/**
 * engine_base is a top-level interface for implementing new storage engine.
 *
 * All storage engines should implement its factory (engine_base::factory_base).
 * To activate new storage engine create object of factory_registerer and pass
 * factory instance.
 */

class engine_base {
	using iterator = internal::iterator_base;

public:
	engine_base() = default;
	virtual ~engine_base() = default;

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

	/**
	 * factory_base is an interface for engine factory.
	 * Should be implemented for registration purposes.
	 */
	class factory_base {
	public:
		factory_base() = default;
		virtual ~factory_base() = default;
		virtual std::unique_ptr<engine_base>
			create(std::unique_ptr<internal::config>) = 0;
		virtual std::string get_name() = 0;
	};
};

/**
 * storage_engine_factory is a class for handling auto-registering factories.
 * Provides simple to use mechanism for factory registration without creating
 * dependencies for newly added engines.
 */
class storage_engine_factory {
public:
	using factory_type = std::unique_ptr<engine_base::factory_base>;

	storage_engine_factory() = delete;
	static bool register_factory(factory_type factory);
	static std::unique_ptr<engine_base>
	create_engine(const std::string &name, std::unique_ptr<internal::config> cfg);

private:
	static std::map<std::string, factory_type> &get_engine_factories();
	static std::string get_names();
};

/**
 * This class is intended to successfully initialize storage_engine_factory
 * despite optimization level due to [basic.stc.static]:
 * "If a variable with static storage duration has initialization or a destructor
 * with side effects, it shall not be eliminated even if it appears to be unused".
 */
class factory_registerer {
public:
	factory_registerer() = delete;
	factory_registerer(storage_engine_factory::factory_type factory)
	{
		storage_engine_factory::register_factory(std::move(factory));
	}
};

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_ENGINE_H */
