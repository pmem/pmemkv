/*
 * Copyright 2019, Intel Corporation
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

/*
 * pmemkv_open.cpp -- example usage of pmemkv with already existing pools.
 */

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <libpmemkv.hpp>

#define LOG(msg) std::cout << msg << std::endl

using namespace pmem::kv;

/**
 * This example expects a path to already created pool.
 *
 * To create a pool use one of the following commands.
 *
 * For regular pools use:
 * pmempool create -l -s 1G "pmemkv" obj path_to_a_pool
 *
 * For poolsets use:
 * pmempool create -l "pmemkv" obj ../examples/example.poolset
 */
int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " pool\n";
		exit(1);
	}

	/* See libpmemkv_config(3) for more detailed example of creating a config */
	LOG("Creating config");
	config cfg;

	status s = cfg.put_string("path", argv[1]);
	assert(s == status::OK);

	LOG("Opening pmemkv database with 'cmap' engine");
	db *kv = new db();
	assert(kv != nullptr);
	s = kv->open("cmap", std::move(cfg));
	assert(s == status::OK);

	LOG("Putting new key");
	s = kv->put("key1", "value1");
	assert(s == status::OK);

	size_t cnt;
	s = kv->count_all(cnt);
	assert(s == status::OK && cnt == 1);

	LOG("Reading key back");
	std::string value;
	s = kv->get("key1", &value);
	assert(s == status::OK && value == "value1");

	LOG("Iterating existing keys");
	kv->put("key2", "value2");
	kv->put("key3", "value3");
	kv->get_all([](string_view k, string_view v) {
		LOG("  visited: " << k.data());
		return 0;
	});

	LOG("Removing existing key");
	s = kv->remove("key1");
	assert(s == status::OK);
	s = kv->exists("key1");
	assert(s == status::NOT_FOUND);

	LOG("Closing database");
	delete kv;

	return 0;
}
