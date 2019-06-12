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
	if (DO_LOG)                                                                      \
	std::cout << "[blackhole] " << msg << "\n"

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

void blackhole::all(all_callback *callback, void *arg)
{
	LOG("All");
}

void blackhole::all_above(string_view key, all_callback *callback, void *arg)
{
	LOG("AllAbove for key=" << key.to_string());
}

void blackhole::all_below(string_view key, all_callback *callback, void *arg)
{
	LOG("AllBelow for key=" << key.to_string());
}

void blackhole::all_between(string_view key1, string_view key2, all_callback *callback,
			    void *arg)
{
	LOG("AllBetween for key1=" << key1.to_string() << ", key2=" << key2.to_string());
}

std::size_t blackhole::count()
{
	LOG("Count");
	return 0;
}

std::size_t blackhole::count_above(string_view key)
{
	LOG("CountAbove for key=" << key.to_string());
	return 0;
}

std::size_t blackhole::count_below(string_view key)
{
	LOG("CountBelow for key=" << key.to_string());
	return 0;
}

std::size_t blackhole::count_between(string_view key1, string_view key2)
{
	LOG("CountBetween for key1=" << key1.to_string()
				     << ", key2=" << key2.to_string());
	return 0;
}

void blackhole::each(each_callback *callback, void *arg)
{
	LOG("Each");
}

void blackhole::each_above(string_view key, each_callback *callback, void *arg)
{
	LOG("EachAbove for key=" << key.to_string());
}

void blackhole::each_below(string_view key, each_callback *callback, void *arg)
{
	LOG("EachBelow for key=" << key.to_string());
}

void blackhole::each_between(string_view key1, string_view key2, each_callback *callback,
			     void *arg)
{
	LOG("EachBetween for key1=" << key1.to_string() << ", key2=" << key2.to_string());
}

status blackhole::exists(string_view key)
{
	LOG("Exists for key=" << key.to_string());
	return status::NOT_FOUND;
}

void blackhole::get(string_view key, get_callback *callback, void *arg)
{
	LOG("Get key=" << key.to_string());
}

status blackhole::put(string_view key, string_view value)
{
	LOG("Put key=" << key.to_string()
		       << ", value.size=" << std::to_string(value.size()));
	return status::OK;
}

status blackhole::remove(string_view key)
{
	LOG("Remove key=" << key.to_string());
	return status::OK;
}

} // namespace kv
} // namespace pmem
