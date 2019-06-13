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

#include <cassert>
#include <iostream>
#include <libpmemkv.hpp>
#include <string>

#define LOG(msg) std::cout << msg << "\n"

using namespace pmem::kv;

const std::string PATH = "/dev/shm/";

int main() {
    LOG("Creating config file");
    pmemkv_config *cfg = pmemkv_config_new();
    assert(cfg != nullptr);

    int ret = pmemkv_config_put(cfg, "path", PATH.c_str(), PATH.size() + 1);
    assert(ret == 0);

    LOG("Starting engine");
    db *kv = new db("vsmap", cfg);
    assert(kv != nullptr);

    pmemkv_config_delete(cfg);

    LOG("Putting new key");
    status s = kv->put("key1", "value1");
    assert(s == status::OK && kv->count() == 1);

    LOG("Reading key back");
    std::string value;
    s = kv->get("key1", &value);
    assert(s == status::OK && value == "value1");

    LOG("Iterating existing keys");
    kv->put("key2", "value2");
    kv->put("key3", "value3");
    kv->all([](const std::string& k) {
        LOG("  visited: " << k);
    });

    LOG("Removing existing key");
    s = kv->remove("key1");
    assert(s == status::OK);
    s = kv->exists("key1");
    assert(s == status::NOT_FOUND);

    LOG("Stopping engine");
    delete kv;

    return 0;
}
