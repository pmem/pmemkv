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

#include <iostream>
#include "blackhole.h"

#define DO_LOG 0
#define LOG(msg) if (DO_LOG) std::cout << "[blackhole] " << msg << "\n"

namespace pmem {
namespace kv {

blackhole::blackhole(void *context): context(context) {
    LOG("Started ok");
}

blackhole::~blackhole() {
    LOG("Stopped ok");
}

void blackhole::all(all_callback* callback, void *arg) {
    LOG("All");
}

void blackhole::all_above(const std::string& key, all_callback* callback, void *arg) {
    LOG("AllAbove for key=" << key);
}

void blackhole::all_below(const std::string& key, all_callback* callback, void *arg) {
    LOG("AllBelow for key=" << key);
}

void blackhole::all_between(const std::string& key1, const std::string& key2, all_callback* callback, void *arg) {
    LOG("AllBetween for key1=" << key1 << ", key2=" << key2);
}

std::size_t blackhole::count() {
    LOG("Count");
    return 0;
}

std::size_t blackhole::count_above(const std::string& key) {
    LOG("CountAbove for key=" << key);
    return 0;
}

std::size_t blackhole::count_below(const std::string& key) {
    LOG("CountBelow for key=" << key);
    return 0;
}

std::size_t blackhole::count_between(const std::string& key1, const std::string& key2) {
    LOG("CountBetween for key1=" << key1 << ", key2=" << key2);
    return 0;
}

void blackhole::each(each_callback* callback, void *arg) {
    LOG("Each");
}

void blackhole::each_above(const std::string& key, each_callback* callback, void *arg) {
    LOG("EachAbove for key=" << key);
}

void blackhole::each_below(const std::string& key, each_callback* callback, void *arg) {
    LOG("EachBelow for key=" << key);
}

void blackhole::each_between(const std::string& key1, const std::string& key2, each_callback* callback, void *arg) {
    LOG("EachBetween for key1=" << key1 << ", key2=" << key2);
}

status blackhole::exists(const std::string& key) {
    LOG("Exists for key=" << key);
    return status::NOT_FOUND;
}

void blackhole::get(const std::string& key, get_callback* callback, void *arg) {
    LOG("Get key=" << key);
}

status blackhole::put(const std::string& key, const std::string& value) {
    LOG("Put key=" << key << ", value.size=" << std::to_string(value.size()));
    return status::OK;
}

status blackhole::remove(const std::string& key) {
    LOG("Remove key=" << key);
    return status::OK;
}

} // namespace kv
} // namespace pmem
