// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#include "engine.h"

namespace pmem
{
namespace kv
{

std::map<std::string, storage_engine_factory::factory_type> &
storage_engine_factory::get_engine_factories()
{
	static std::map<std::string, factory_type> factory_objects;
	return factory_objects;
}

bool storage_engine_factory::register_factory(factory_type factory)
{
	auto factory_name = factory->get_name();
	return get_engine_factories().emplace(factory_name, std::move(factory)).second;
}

std::unique_ptr<engine_base>
storage_engine_factory::create_engine(const std::string &name,
				      std::unique_ptr<internal::config> cfg)
{
	auto &pairs = get_engine_factories();
	auto it = pairs.find(name);
	if (it != pairs.end()) {
		return it->second->create(std::move(cfg));
	}
	throw internal::wrong_engine_name("Unknown engine name \"" + name +
					  "\". Available engines: " + get_names());
}

std::string storage_engine_factory::get_names()
{
	auto &factories = get_engine_factories();
	if (factories.empty()) {
		return "";
	}
	std::string separator = ", ";
	std::string result;
	for (auto &entry : get_engine_factories()) {
		result += entry.first + separator;
	}
	return result.erase(result.rfind(separator), separator.length());
}

/* throws internal error (status) if config is null */
void check_config_null(const std::string &engine_name,
		       std::unique_ptr<internal::config> &cfg)
{
	if (!cfg) {
		throw internal::invalid_argument("Config cannot be null for the '" +
						 engine_name + "' engine");
	}
}

status engine_base::count_all(std::size_t &cnt)
{
	return status::NOT_SUPPORTED;
}

status engine_base::count_above(string_view key, std::size_t &cnt)
{
	return status::NOT_SUPPORTED;
}

status engine_base::count_equal_above(string_view key, std::size_t &cnt)
{
	return status::NOT_SUPPORTED;
}

status engine_base::count_equal_below(string_view key, std::size_t &cnt)
{
	return status::NOT_SUPPORTED;
}

status engine_base::count_below(string_view key, std::size_t &cnt)
{
	return status::NOT_SUPPORTED;
}
status engine_base::count_between(string_view key1, string_view key2, std::size_t &cnt)
{
	return status::NOT_SUPPORTED;
}

status engine_base::get_all(get_kv_callback *callback, void *arg)
{
	return status::NOT_SUPPORTED;
}

status engine_base::get_above(string_view key, get_kv_callback *callback, void *arg)
{
	return status::NOT_SUPPORTED;
}

status engine_base::get_equal_above(string_view key, get_kv_callback *callback, void *arg)
{
	return status::NOT_SUPPORTED;
}

status engine_base::get_equal_below(string_view key, get_kv_callback *callback, void *arg)
{
	return status::NOT_SUPPORTED;
}

status engine_base::get_below(string_view key, get_kv_callback *callback, void *arg)
{
	return status::NOT_SUPPORTED;
}

status engine_base::get_between(string_view key1, string_view key2,
				get_kv_callback *callback, void *arg)
{
	return status::NOT_SUPPORTED;
}

status engine_base::exists(string_view key)
{
	return status::NOT_SUPPORTED;
}

status engine_base::defrag(double start_percent, double amount_percent)
{
	return status::NOT_SUPPORTED;
}

internal::transaction *engine_base::begin_tx()
{
	throw internal::not_supported("Transactions are not supported in this engine");
}

engine_base::iterator *engine_base::new_iterator()
{
	throw internal::not_supported("Iterators are not supported in this engine");
}

engine_base::iterator *engine_base::new_const_iterator()
{
	throw internal::not_supported("Iterators are not supported in this engine");
}

} // namespace kv
} // namespace pmem
