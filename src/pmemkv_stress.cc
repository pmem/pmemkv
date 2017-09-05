/*
 * Copyright 2017, Intel Corporation
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
#include <random>
#include <unistd.h>
#include <sys/time.h>
#include "pmemkv.h"

#define LOG(msg) std::cout << msg << "\n"

using namespace pmemkv;

static int count = 950000;
static string path = "/dev/shm/pmemkv";
static size_t size = ((size_t) 1024 * 1024 * 1000);
static size_t value_length = 800;

static const char* USAGE =
        "Usage: pmemkv_stress [engine] [command] [value-length] [path] [size]\n"
                "  engine=default|kvtree\n"
                "  command=a|r|w|gr|gs|pr|ps|rr|rs\n"
                "  value-length=<positive integer>\n"
                "  path=DAX device|filesystem DAX mount|pool set\n"
                "  size=pool size in MB|0 for device DAX";

static unsigned long current_millis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long long) (tv.tv_sec) * 1000 + (unsigned long long) (tv.tv_usec) / 1000;
}

static KVTree* open() {
    auto started = current_millis();
    auto kv = new KVTree(path, size);
    LOG("   in " << current_millis() - started << " ms");
    return kv;
}

static string formatKey(int i) {
    string key = std::to_string(i);
    key.append(20 - key.length(), 'X');
    return key;
}

static string formatValue() {
    string val = string();
    val.append(value_length, 'X');
    return val;
}

static void testGetRandom(KVTree* kv) {
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(1, count - 1);
    int failures = 0;
    auto started = current_millis();
    for (int i = 0; i < count; i++) {
        std::string value;
        if (kv->Get(formatKey(distribution(generator)), &value) != OK) failures++;
    }
    LOG("   in " << current_millis() - started << " ms, failures=" + std::to_string(failures));
}

static void testGetSequential(KVTree* kv) {
    int failures = 0;
    auto started = current_millis();
    for (int i = 0; i < count; i++) {
        std::string value;
        if (kv->Get(formatKey(i), &value) != OK) failures++;
    }
    LOG("   in " << current_millis() - started << " ms, failures=" + std::to_string(failures));
}

static void testPutRandom(KVTree* kv) {
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(1, count - 1);
    auto value = formatValue();
    auto started = current_millis();
    for (int i = 0; i < count; i++) {
        if (kv->Put(formatKey(distribution(generator)), value) != OK) {
            std::cout << "Out of space at key " << std::to_string(i) << "\n";
            exit(-42);
        }
    }
    LOG("   in " << current_millis() - started << " ms");
}

static void testPutSequential(KVTree* kv) {
    auto value = formatValue();
    auto started = current_millis();
    for (int i = 0; i < count; i++) {
        if (kv->Put(formatKey(i), value) != OK) {
            std::cout << "Out of space at key " << std::to_string(i) << "\n";
            exit(-42);
        }
    }
    LOG("   in " << current_millis() - started << " ms");
}

static void testRemoveRandom(KVTree* kv) {
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(1, count - 1);
    auto started = current_millis();
    for (int i = 0; i < count; i++) {
        kv->Remove(formatKey(distribution(generator)));
    }
    LOG("   in " << current_millis() - started << " ms");
}

static void testRemoveSequential(KVTree* kv) {
    auto started = current_millis();
    for (int i = 0; i < count; i++) kv->Remove(formatKey(i));
    LOG("   in " << current_millis() - started << " ms");
}

static void usage_exit(int exit_code) {
    ((exit_code == EXIT_SUCCESS) ? std::cout : std::cerr) << USAGE << "\n";
    exit(exit_code);
}

int main(int argc, char** argv) {
    string engine;
    string command;

    // validate command line args
    if (argc != 6) {
        usage_exit(EXIT_FAILURE);
    } else {
        engine = argv[1];
        command = argv[2];
        value_length = std::stoul(argv[3]);
        path = argv[4];
        size = std::stoul(argv[5]) * (1024 * 1024);
        count = (int) (size / 1100);
        if (engine != "default" and engine != "kvtree")
            usage_exit(EXIT_FAILURE);
        else if (command != "a" and command != "r" and command != "w" and command != "gr"
                and command != "gs" and command != "pr" and command != "ps"
                and command != "rr" and command != "rs")
            usage_exit(EXIT_FAILURE);
    }

    // open datastore
    LOG("Opening engine=" + engine + ", path=" + path);
    KVTree* kv = open();

    // run requested command
    if (command == "a") {
        LOG("Inserting " << count << " sequential values");
        testPutSequential(kv);
        LOG("Getting " << count << " sequential values");
        testGetSequential(kv);
        LOG("Updating " << count << " sequential values");
        testPutSequential(kv);
        LOG("Getting " << count << " sequential values");
        testGetSequential(kv);
        LOG("Updating " << count << " random values");
        testPutRandom(kv);
        LOG("Getting " << count << " random values");
        testGetRandom(kv);
        LOG("Removing " << count << " sequential values");
        testRemoveSequential(kv);
    } else if (command == "r") {
        for (int i = 0; i < 8; i++) {
            LOG("Getting " << count << " random values");
            testGetRandom(kv);
            LOG("Getting " << count << " sequential values");
            testGetSequential(kv);
        }
    } else if (command == "w") {
        LOG("Inserting " << count << " sequential values");
        testPutSequential(kv);
        LOG("Updating " << count << " sequential values");
        testPutSequential(kv);
        LOG("Updating " << count << " random values");
        testPutRandom(kv);
    } else if (command == "gr") {
        LOG("Getting " << count << " random values");
        testGetRandom(kv);
    } else if (command == "gs") {
        LOG("Getting " << count << " sequential values");
        testGetSequential(kv);
    } else if (command == "pr") {
        LOG("Putting " << count << " random values");
        testPutRandom(kv);
    } else if (command == "ps") {
        LOG("Putting " << count << " sequential values");
        testPutSequential(kv);
    } else if (command == "rr") {
        LOG("Removing " << count << " random values");
        testRemoveRandom(kv);
    } else if (command == "rs") {
        LOG("Removing " << count << " sequential values");
        testRemoveSequential(kv);
    }
    delete kv;

    LOG("Finished");
    return 0;
}
