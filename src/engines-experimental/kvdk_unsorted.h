// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#ifndef LIBPMEMKV_kvdk_H
#define LIBPMEMKV_kvdk_H

#include "../engine.h"

#include "kvdk/engine.hpp"
#include <memory>

namespace pmem
{
namespace kv
{

namespace storage_engine = KVDK_NAMESPACE;

class kvdk_unsorted : public engine_base {
	class kvdk_iterator;

public:
	kvdk_unsorted(std::unique_ptr<internal::config> cfg);
	~kvdk_unsorted();

	std::string name() final;

	status exists(string_view key) final;

	status get(string_view key, get_v_callback *callback, void *arg) final;

	status put(string_view key, string_view value) final;

	status remove(string_view key) final;

private:

	status status_mapper(storage_engine::Status s);
	
	storage_engine::Engine *engine;
};

class kvdk_factory : public engine_base::factory_base {
public:
	std::unique_ptr<engine_base>
	create(std::unique_ptr<internal::config> cfg) override
	{
		return std::unique_ptr<engine_base>(new kvdk_unsorted(std::move(cfg)));
	};
	std::string get_name() override
	{
		return "kvdk_unsorted";
	};
};

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_kvdk_H */
