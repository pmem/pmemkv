// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#include "kvdk_sorted.h"

#include <../out.h>

namespace pmem
{
namespace kv
{

status kvdk_sorted::status_mapper(storage_engine::Status s)
{
	switch(s)
	{
		case storage_engine::Status::Ok: return status::OK;
		case storage_engine::Status::NotFound: return status::NOT_FOUND;
		case storage_engine::Status::MemoryOverflow: return status::OUT_OF_MEMORY;
		case storage_engine::Status::PmemOverflow: return status::OUT_OF_MEMORY;
		case storage_engine::Status::NotSupported: return status::NOT_SUPPORTED;
		default: return status::UNKNOWN_ERROR;
//		case  MapError,
//		case  BatchOverflow,
//		case  TooManyWriteThreads,
//		case  InvalidDataSize,
//		case  IOError,
//		case  InvalidConfiguration,
	}
}

kvdk_sorted::kvdk_sorted(std::unique_ptr<internal::config> cfg)
{
	LOG("Started ok");
	storage_engine::Configs engine_configs;
	engine_configs.pmem_file_size = cfg->get_size();
	engine_configs.pmem_segment_blocks = (1ull << 10);
	engine_configs.hash_bucket_num = (1ull << 20);

	auto status = storage_engine::Engine::Open(cfg->get_path(), &engine, engine_configs, stdout);
	(void)status;
	assert(status == storage_engine::Status::Ok);
}

kvdk_sorted::~kvdk_sorted()
{
	LOG("Stopped ok");
	delete engine;
}

std::string kvdk_sorted::name()
{
	return "kvdk_sorted";
}

status kvdk_sorted::count_all(std::size_t &cnt)
{
	LOG("count_all");

	cnt = 0;

	auto iter = engine->NewSortedIterator(collection);
	if (!iter)
		return status::OK;
	
	for (iter->SeekToFirst(); iter->Valid(); iter->Next())
		++cnt;
	return status::OK;
}

status kvdk_sorted::count_above(string_view key, std::size_t &cnt)
{
	std::string key_copy(key.data(), key.size());
	LOG("count_above for key=" << key_copy);

	cnt = 0;

	auto iter = engine->NewSortedIterator(collection);
	if (!iter)
		return status::OK;
	
	for (iter->SeekToLast(); iter->Valid() && (iter->Key() > key_copy); iter->Prev())
		++cnt;
	return status::OK;
}

status kvdk_sorted::count_equal_above(string_view key, std::size_t &cnt)
{
	std::string key_copy(key.data(), key.size());
	LOG("count_equal_above for key=" << key_copy);

	cnt = 0;

	auto iter = engine->NewSortedIterator(collection);
	if (!iter)
		return status::OK;
	
	for (iter->SeekToLast(); iter->Valid() && (iter->Key() >= key_copy); iter->Prev())
		++cnt;
	return status::OK;
}

status kvdk_sorted::count_equal_below(string_view key, std::size_t &cnt)
{
	std::string key_copy(key.data(), key.size());
	LOG("count_equal_below for key=" << key_copy);

	cnt = 0;

	auto iter = engine->NewSortedIterator(collection);
	if (!iter)
		return status::OK;
	
	for (iter->SeekToFirst(); iter->Valid() && (iter->Key() <= key_copy); iter->Next())
		++cnt;
	return status::OK;
}

status kvdk_sorted::count_below(string_view key, std::size_t &cnt)
{
	std::string key_copy(key.data(), key.size());
	LOG("count_below for key=" << key_copy);

	cnt = 0;

	auto iter = engine->NewSortedIterator(collection);
	if (!iter)
		return status::OK;
	
	for (iter->SeekToFirst(); iter->Valid() && (iter->Key() < key_copy); iter->Next())
		++cnt;
	return status::OK;
}

// count elements in range [key1, key2]
status kvdk_sorted::count_between(string_view key1, string_view key2, std::size_t &cnt)
{
	std::string key1_copy(key1.data(), key1.size());
	std::string key2_copy(key2.data(), key2.size());
	LOG("count_between for key1=" << key1_copy << ", key2=" << key2_copy);

	cnt = 0;

	auto iter = engine->NewSortedIterator(collection);
	if (!iter)
		return status::OK;
	
	for (iter->Seek(key1_copy); iter->Valid() && (iter->Key() <= key2_copy); iter->Next())
		++cnt;
	return status::OK;
}

status kvdk_sorted::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");

	auto iter = engine->NewSortedIterator(collection);
	if (!iter)
		return status::NOT_FOUND;

	for (iter->SeekToFirst(); iter->Valid(); iter->Next())
	{
		auto key = iter->Key();
		auto value = iter->Value();
		auto ret = callback(key.data(), key.size(), value.data(), value.size(), arg);
		if (ret != 0 )
			return status::STOPPED_BY_CB;
	}
	return status::OK;
}

status kvdk_sorted::get_above(string_view key, get_kv_callback *callback, void *arg)
{
	std::string key_copy(key.data(), key.size());
	LOG("get_above for key=" << key_copy);

	auto iter = engine->NewSortedIterator(collection);
	if (!iter)
		return status::NOT_FOUND;
	
	for (iter->SeekToLast(); iter->Valid() && (iter->Key() > key_copy); iter->Prev())
	{
		auto key = iter->Key();
		auto value = iter->Value();
		auto ret = callback(key.data(), key.size(), value.data(), value.size(), arg);
		if (ret != 0 )
			return status::STOPPED_BY_CB;
	}
	return status::OK;
}

status kvdk_sorted::get_equal_above(string_view key, get_kv_callback *callback, void *arg)
{
	std::string key_copy(key.data(), key.size());
	LOG("get_equal_above for key=" << key_copy);

	auto iter = engine->NewSortedIterator(collection);
	if (!iter)
		return status::NOT_FOUND;
	
	for (iter->SeekToLast(); iter->Valid() && (iter->Key() >= key_copy); iter->Prev())
	{
		auto key = iter->Key();
		auto value = iter->Value();
		auto ret = callback(key.data(), key.size(), value.data(), value.size(), arg);
		if (ret != 0 )
			return status::STOPPED_BY_CB;
	}
	return status::OK;
}

status kvdk_sorted::get_equal_below(string_view key, get_kv_callback *callback, void *arg)
{
	std::string key_copy(key.data(), key.size());
	LOG("get_equal_below for key=" << key_copy);

	auto iter = engine->NewSortedIterator(collection);
	if (!iter)
		return status::NOT_FOUND;
	
	for (iter->SeekToFirst(); iter->Valid() && (iter->Key() <= key_copy); iter->Next())
	{
		auto key = iter->Key();
		auto value = iter->Value();
		auto ret = callback(key.data(), key.size(), value.data(), value.size(), arg);
		if (ret != 0 )
			return status::STOPPED_BY_CB;
	}
	return status::OK;
}

status kvdk_sorted::get_below(string_view key, get_kv_callback *callback, void *arg)
{
	std::string key_copy(key.data(), key.size());
	LOG("get_below for key=" << key_copy);

	auto iter = engine->NewSortedIterator(collection);
	if (!iter)
		return status::NOT_FOUND;
	
	for (iter->SeekToFirst(); iter->Valid() && (iter->Key() < key_copy); iter->Next())
	{
		auto key = iter->Key();
		auto value = iter->Value();
		auto ret = callback(key.data(), key.size(), value.data(), value.size(), arg);
		if (ret != 0 )
			return status::STOPPED_BY_CB;
	}
	return status::OK;
}

status kvdk_sorted::get_between(string_view key1, string_view key2,
			      get_kv_callback *callback, void *arg)
{
	std::string key1_copy(key1.data(), key1.size());
	std::string key2_copy(key2.data(), key2.size());
	LOG("get_between key1=" << key1_copy << ", key2=" << key2_copy);

	auto iter = engine->NewSortedIterator(collection);
	if (!iter)
		return status::NOT_FOUND;
	
	for (iter->Seek(key1_copy); iter->Valid() && (iter->Key() <= key2_copy); iter->Next())
	{
		auto key = iter->Key();
		auto value = iter->Value();
		auto ret = callback(key.data(), key.size(), value.data(), value.size(), arg);
		if (ret != 0 )
			return status::STOPPED_BY_CB;
	}
	return status::OK;
}

status kvdk_sorted::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	std::string value;
	return status_mapper(engine->SGet(collection, key, &value));
}

status kvdk_sorted::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));

	std::string value;
	auto status = engine->SGet(collection, key, &value);
	if (status == storage_engine::Status::Ok){
		callback(value.data(), value.size(), arg);
	}
	return status_mapper(status);
}

status kvdk_sorted::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));
	auto status = engine->SSet(collection ,key, value);

	return status_mapper(status);
}

status kvdk_sorted::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));
	auto status = exists(key);
	if (status == status::OK) {
		return status_mapper(engine->SDelete(collection, key));
	}
	return status;
}

internal::iterator_base *kvdk_sorted::new_iterator()
{
	LOG("create write iterator");

	throw std::runtime_error("kvdk_sorted current does not support write iterator.");
	// return nullptr;
}

internal::iterator_base *kvdk_sorted::new_const_iterator()
{
	LOG("create read iterator");

	auto iter = engine->NewSortedIterator(collection);
	if (!iter)
		return nullptr;

	return new kvdk_const_iterator{iter};
}

/* required for logging */
std::string kvdk_sorted::kvdk_iterator::name()
{
	return "kvdk_sorted iterator";
}

status kvdk_sorted::kvdk_iterator::seek(string_view key)
{
	LOG("seek to key=" << std::string(key.data(), key.size()));
	throw std::runtime_error("kvdk_sorted current does not support write iterator.");
	// return status::OK;
}

result<string_view> kvdk_sorted::kvdk_iterator::key()
{
	LOG("key");
	throw std::runtime_error("kvdk_sorted current does not support write iterator.");
	// return status::NOT_FOUND;
}

result<pmem::obj::slice<const char *>>
kvdk_sorted::kvdk_iterator::read_range(size_t pos, size_t n)
{
	LOG("read_range, pos=" << pos << " n=" << n);
	throw std::runtime_error("kvdk_sorted current does not support write iterator.");
	// return status::NOT_FOUND;
}

/* required for logging */
std::string kvdk_sorted::kvdk_const_iterator::name()
{
	return "kvdk_sorted const iterator";
}

status kvdk_sorted::kvdk_const_iterator::seek(string_view key)
{
	std::string key_copy(key.data(), key.size());
	LOG("seek to key=" << key_copy);
	iterator->Seek(key_copy);
	if (iterator->Valid())
	{
		if (iterator->Key() == key_copy)
			return status::OK;
		else 
			return status::NOT_SUPPORTED;
	}
	return status::UNKNOWN_ERROR;	
}

result<string_view> kvdk_sorted::kvdk_const_iterator::key()
{
	LOG("key");
	if (iterator->Valid())
	{
		key_local = iterator->Key();
		return string_view{key_local};
	}
	// Maybe the iterator reaches end, or actual unknown error occurs.
	return status::UNKNOWN_ERROR;
}

result<pmem::obj::slice<const char *>>
kvdk_sorted::kvdk_const_iterator::read_range(size_t pos, size_t n)
{
	LOG("read_range, pos=" << pos << " n=" << n);
	throw std::runtime_error("kvdk_sorted current does not support random access.");
	// return status::NOT_FOUND;
}

static factory_registerer register_kvdk(
	std::unique_ptr<engine_base::factory_base>(new kvdk_sorted_factory));

} // namespace kv
} // namespace pmem
