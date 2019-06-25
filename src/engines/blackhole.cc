/*
 * Copyright 2017-2019, Intel Corporation
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

#include "blackhole.h"
#include <iostream>

#define DO_LOG 0
#define LOG(msg)                                                                         \
	do {                                                                             \
		if (DO_LOG)                                                              \
			std::cout << "[blackhole] " << msg << "\n";                      \
	} while (0)

namespace pmem
{
namespace kv
{

blackhole::blackhole(void *context) : context(context)
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

void *blackhole::engine_context()
{
	return context;
}

status blackhole::count(std::size_t &cnt)
{
	LOG("Count");

	cnt = 0;

	return status::OK;
}

status blackhole::count_above(string_view key, std::size_t &cnt)
{
	LOG("CountAbove for key=" << std::string(key.data(), key.size()));

	cnt = 0;

	return status::OK;
}

status blackhole::count_below(string_view key, std::size_t &cnt)
{
	LOG("CountBelow for key=" << std::string(key.data(), key.size()));

	cnt = 0;

	return status::OK;
}

status blackhole::count_between(string_view key1, string_view key2, std::size_t &cnt)
{
	LOG("CountBetween for key1=" << key1.data() << ", key2=" << key2.data());

	cnt = 0;

	return status::OK;
}

status blackhole::each(each_callback *callback, void *arg)
{
	LOG("Each");

	return status::OK;
}

status blackhole::each_above(string_view key, each_callback *callback, void *arg)
{
	LOG("EachAbove for key=" << std::string(key.data(), key.size()));

	return status::OK;
}

status blackhole::each_below(string_view key, each_callback *callback, void *arg)
{
	LOG("EachBelow for key=" << std::string(key.data(), key.size()));

	return status::OK;
}

status blackhole::each_between(string_view key1, string_view key2,
			       each_callback *callback, void *arg)
{
	LOG("EachBetween for key1=" << key1.data() << ", key2=" << key2.data());

	return status::OK;
}

status blackhole::exists(string_view key)
{
	LOG("Exists for key=" << std::string(key.data(), key.size()));

	return status::NOT_FOUND;
}

status blackhole::get(string_view key, get_callback *callback, void *arg)
{
	LOG("Get key=" << std::string(key.data(), key.size()));

	return status::OK;
}

status blackhole::put(string_view key, string_view value)
{
	LOG("Put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));

	return status::OK;
}

status blackhole::remove(string_view key)
{
	LOG("Remove key=" << std::string(key.data(), key.size()));

	return status::OK;
}

} // namespace kv
} // namespace pmem
