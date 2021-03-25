// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#include "engine.h"

// #include "engines/blackhole.h"

namespace pmem
{
namespace kv
{

engine_base::engine_base()
{
}

engine_base::~engine_base()
{
}

std::map<std::string, StorageEngineFactory::TCreateMethod>& StorageEngineFactory::getPairs() {
	static std::map<std::string, StorageEngineFactory::TCreateMethod> s_methods;
	return s_methods;
}

bool StorageEngineFactory::Register(const std::string name, StorageEngineFactory::TCreateMethod factory) {
	auto& pairs = getPairs();
	auto it = pairs.find(name);
	if (it == pairs.end()) {
		pairs[name] = std::move(factory);
		return true;
	}
	return false;
}

std::unique_ptr<engine_base>
StorageEngineFactory::Create(const std::string& name, std::unique_ptr<internal::config> cfg)
{
	auto& pairs = getPairs();
	auto it = pairs.find(name);
	if (it != pairs.end()) {
		return it->second->create(std::move(cfg));
	}
	return nullptr;
}

/* throws internal error (status) if config is null */
void engine_base::check_config_null(const std::string &engine_name,
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
