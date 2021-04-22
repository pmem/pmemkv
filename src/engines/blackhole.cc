// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2021, Intel Corporation */

#include "blackhole.h"

#include <../out.h>

namespace pmem
{
namespace kv
{

blackhole::blackhole(std::unique_ptr<internal::config> cfg)
{
	LOG("Started ok");
}

blackhole::~blackhole()
{
	LOG("Stopped ok");
}

std::string blackhole::name()
{
	return "blackhole";
}

status blackhole::count_all(std::size_t &cnt)
{
	LOG("count_all");

	cnt = 0;

	return status::OK;
}

status blackhole::count_above(string_view key, std::size_t &cnt)
{
	LOG("count_above for key=" << std::string(key.data(), key.size()));

	cnt = 0;

	return status::OK;
}

status blackhole::count_equal_above(string_view key, std::size_t &cnt)
{
	LOG("count_equal_above for key=" << std::string(key.data(), key.size()));

	cnt = 0;

	return status::OK;
}

status blackhole::count_equal_below(string_view key, std::size_t &cnt)
{
	LOG("count_equal_below for key=" << std::string(key.data(), key.size()));

	cnt = 0;

	return status::OK;
}

status blackhole::count_below(string_view key, std::size_t &cnt)
{
	LOG("count_below for key=" << std::string(key.data(), key.size()));

	cnt = 0;

	return status::OK;
}

status blackhole::count_between(string_view key1, string_view key2, std::size_t &cnt)
{
	LOG("count_between for key1=" << key1.data() << ", key2=" << key2.data());

	cnt = 0;

	return status::OK;
}

status blackhole::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");

	return status::NOT_FOUND;
}

status blackhole::get_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_above for key=" << std::string(key.data(), key.size()));

	return status::NOT_FOUND;
}

status blackhole::get_equal_above(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_above for key=" << std::string(key.data(), key.size()));

	return status::NOT_FOUND;
}

status blackhole::get_equal_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_equal_below for key=" << std::string(key.data(), key.size()));

	return status::NOT_FOUND;
}

status blackhole::get_below(string_view key, get_kv_callback *callback, void *arg)
{
	LOG("get_below for key=" << std::string(key.data(), key.size()));

	return status::NOT_FOUND;
}

status blackhole::get_between(string_view key1, string_view key2,
			      get_kv_callback *callback, void *arg)
{
	LOG("get_between for key1=" << key1.data() << ", key2=" << key2.data());

	return status::NOT_FOUND;
}

status blackhole::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));

	return status::NOT_FOUND;
}

status blackhole::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));

	return status::NOT_FOUND;
}

status blackhole::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));

	return status::OK;
}

status blackhole::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));

	return status::OK;
}

internal::iterator_base *blackhole::new_iterator()
{
	LOG("create write iterator");

	return new blackhole_iterator{};
}

internal::iterator_base *blackhole::new_const_iterator()
{
	LOG("create read iterator");

	return new blackhole_iterator{};
}

/* required for logging */
std::string blackhole::blackhole_iterator::name()
{
	return "blackhole iterator";
}

status blackhole::blackhole_iterator::seek(string_view key)
{
	LOG("seek to key=" << std::string(key.data(), key.size()));

	return status::OK;
}

result<string_view> blackhole::blackhole_iterator::key()
{
	LOG("key");

	return status::NOT_FOUND;
}

result<pmem::obj::slice<const char *>>
blackhole::blackhole_iterator::read_range(size_t pos, size_t n)
{
	LOG("read_range, pos=" << pos << " n=" << n);

	return status::NOT_FOUND;
}

static factory_registerer register_blackhole(
	std::unique_ptr<engine_base::factory_base>(new blackhole_factory));

} // namespace kv
} // namespace pmem
