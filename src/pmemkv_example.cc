/*
 * Copyright 2017-2018, Intel Corporation
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

#include <iostream>
#include "pmemkv.h"

#define LOG(msg) std::cout << msg << "\n"

using namespace pmemkv;

int main() {
    LOG("Opening datastore");
    KVEngine* kv = KVEngine::Open("kvtree", "/dev/shm/pmemkv", PMEMOBJ_MIN_POOL);

    LOG("Putting new value");
    KVStatus s = kv->Put("key1", "value1");
    assert(s == OK);
    string value;
    s = kv->Get("key1", &value);
    assert(s == OK && value == "value1");

    LOG("Replacing existing value");
    string value2;
    s = kv->Get("key1", &value2);
    assert(s == OK && value2 == "value1");
    s = kv->Put("key1", "value_replaced");
    assert(s == OK);
    string value3;
    s = kv->Get("key1", &value3);
    assert(s == OK && value3 == "value_replaced");

    LOG("Removing existing value");
    s = kv->Remove("key1");
    assert(s == OK);
    string value4;
    s = kv->Get("key1", &value4);
    assert(s == NOT_FOUND);

    LOG("Closing datastore");
    delete kv;

    LOG("Finished successfully");
    return 0;
}
