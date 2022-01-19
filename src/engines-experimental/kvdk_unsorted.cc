// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#include "kvdk_unsorted.h"

#include <../out.h>

namespace pmem
{
namespace kv
{

kvdk_unsorted::kvdk_unsorted(std::unique_ptr<internal::config> cfg)
{
	LOG("Started ok");
	storage_engine::Configs engine_configs;
	engine_configs.pmem_file_size = cfg->get_size();
	engine_configs.pmem_segment_blocks = (1ULL << 10);
	engine_configs.hash_bucket_num = (1ULL << 20);

	auto status = storage_engine::Engine::Open(cfg->get_path(), &engine,
						   engine_configs, stdout);
	(void)status;
	assert(status == storage_engine::Status::Ok);
}

kvdk_unsorted::~kvdk_unsorted()
{
	LOG("Stopped ok");
	delete engine;
}

std::string kvdk_unsorted::name()
{
	return "kvdk_unsorted";
}

status kvdk_unsorted::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	std::string value;
	return map_kvdk_status(engine->Get(key, &value));
}

status kvdk_unsorted::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));

	std::string value;
	auto status = engine->Get(key, &value);
	if (status == storage_engine::Status::Ok) {
		callback(value.data(), value.size(), arg);
	}
	return map_kvdk_status(status);
}

status kvdk_unsorted::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));
	auto status = engine->Set(key, value);

	return map_kvdk_status(status);
}

status kvdk_unsorted::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));
	auto status = exists(key);
	if (status == status::OK) {
		return map_kvdk_status(engine->Delete(key));
	}
	return status;
}

static factory_registerer
	register_kvdk(std::unique_ptr<engine_base::factory_base>(new kvdk_factory));

} // namespace kv
} // namespace pmem
