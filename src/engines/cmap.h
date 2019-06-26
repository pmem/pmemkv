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
#include "../polymorphic_string.h"

#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#define LIBPMEMOBJ_CPP_USE_TBB_RW_MUTEX 1
#include <libpmemobj++/experimental/concurrent_hash_map.hpp>

namespace std
{
template <>
struct hash<pmem::kv::polymorphic_string> {
	/* hash multiplier used by fibonacci hashing */
	static const size_t hash_multiplier = 11400714819323198485ULL;

	size_t operator()(const pmem::kv::polymorphic_string &str)
	{
		size_t h = 0;
		for (size_t i = 0; i < str.size(); ++i) {
			h = static_cast<size_t>(str[i]) ^ (h * hash_multiplier);
		}
		return h;
	}
};
}

namespace pmem
{
namespace kv
{

class cmap : public engine_base {
public:
	cmap(const std::string &path, size_t size);
	~cmap();

	cmap(const cmap &) = delete;
	cmap &operator=(const cmap &) = delete;

	std::string name() final;

	status count_all(std::size_t &cnt) final;

	status get_all(get_kv_callback *callback, void *arg) final;

	status exists(string_view key) final;

	status get(string_view key, get_v_callback *callback, void *arg) final;

	status put(string_view key, string_view value) final;

	status remove(string_view key) final;

private:
	using string_t = pmem::kv::polymorphic_string;
	using map_t = pmem::obj::experimental::concurrent_hash_map<string_t, string_t>;

	struct RootData {
		pmem::obj::persistent_ptr<map_t> map_ptr;
	};
	using pool_t = pmem::obj::pool<RootData>;

	void Recover();
	pool_t pmpool;
	map_t *container;
};

} /* namespace kv */
} /* namespace pmem */
