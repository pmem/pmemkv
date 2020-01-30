/*
 * Copyright 2019-2020, Intel Corporation
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

#ifndef LIBPMEMKV_ENGINE_H
#define LIBPMEMKV_ENGINE_H

#include <functional>
#include <memory>
#include <string>

#include "config.h"
#include "libpmemkv.hpp"

namespace pmem
{
namespace kv
{

const std::string LAYOUT = "pmemkv";

class engine_base {
public:
	engine_base();

	virtual ~engine_base();

	static std::unique_ptr<engine_base>
	create_engine(const std::string &name, std::unique_ptr<internal::config> cfg);

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

private:
	static void check_config_null(const std::string &engine_name,
				      std::unique_ptr<internal::config> &cfg);
};

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_ENGINE_H */
