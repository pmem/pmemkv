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

#pragma once

#include "../engine.h"
#include "pmem_allocator.h"
#include <scoped_allocator>
#include <string>
#include <tbb/concurrent_hash_map.h>

namespace pmem
{
namespace kv
{

class vcmap : public engine_base {
public:
	vcmap(void *context, const std::string &path, size_t size);
	~vcmap();

	std::string name() final;
	void *engine_context();

	status all(all_callback *callback, void *arg) final;

	status count(std::size_t &cnt) final;

	status each(each_callback *callback, void *arg) final;

	status exists(const std::string &key) final;

	status get(const std::string &key, get_callback *callback, void *arg) final;

	status put(const std::string &key, const std::string &value) final;

	status remove(const std::string &key) final;

private:
	typedef pmem::allocator<char> ch_allocator_t;
	typedef std::basic_string<char, std::char_traits<char>, ch_allocator_t>
		pmem_string;
	typedef pmem::allocator<std::pair<pmem_string, pmem_string>> kv_allocator_t;
	typedef tbb::concurrent_hash_map<pmem_string, pmem_string,
					 tbb::tbb_hash_compare<pmem_string>,
					 std::scoped_allocator_adaptor<kv_allocator_t>>
		map_t;
	void *context;
	kv_allocator_t kv_allocator;
	ch_allocator_t ch_allocator;
	map_t pmem_kv_container;
};

} /* namespace kv */
} /* namespace pmem */
