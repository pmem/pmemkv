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

static std::string USAGE = "Usage: pmemkv_stress [r|w] [path] [size in MB]";
static int count = 10000000;
static std::string path = "/dev/shm/pmemkv";
static size_t size = ((size_t) (1024 * 1024) * (int) (9176 * 1.1));

static const char* LOREM_IPSUM_200 =
        " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec facilisis ipsum ipsum, nec sollicitudin nulla pharetra at. Sed accumsan ut felis sed ornare. Aliquam maximus dui congue ipsum cras amet.";
static const char* LOREM_IPSUM_400 =
        " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vestibulum eu tristique lorem. Integer sodales mi non ullamcorper faucibus. Suspendisse efficitur tortor mi. Phasellus facilisis placerat molestie. Proin tempus mollis volutpat. Quisque gravida hendrerit erat et commodo. Integer eu dolor et velit auctor pharetra non id sapien. Duis consectetur magna ut odio aliquam, vestibulum bibendum sed.";
static const char* LOREM_IPSUM_800 =
        " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Phasellus bibendum ante sem, vel tincidunt odio aliquet vitae. Vestibulum eget laoreet sem. Donec ultrices scelerisque odio quis pretium. Proin semper in diam nec scelerisque. Fusce id ornare dui. Ut hendrerit interdum cursus. Praesent fringilla non nibh eget rhoncus. In eget est nec enim imperdiet scelerisque. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Donec ac egestas mi. Aenean a urna dui. Suspendisse urna nisl, rhoncus eget risus non, ultricies aliquam ante. Curabitur libero ante, imperdiet a consequat id, cursus eget eros. Suspendisse semper arcu odio, id pellentesque lacus eleifend sed. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Ut metus.";

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

static void testGetRandom(KVTree* kv) {
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(1, count - 1);
    auto started = current_millis();
    for (int i = 0; i < count; i++) {
        std::string value;
        kv->Get(formatKey(distribution(generator)), &value);
    }
    LOG("   in " << current_millis() - started << " ms");
}

static void testGetSequential(KVTree* kv) {
    auto started = current_millis();
    for (int i = 0; i < count; i++) {
        std::string value;
        kv->Get(formatKey(i), &value);
    }
    LOG("   in " << current_millis() - started << " ms");
}

static void testPut(KVTree* kv) {
    auto started = current_millis();
    for (int i = 0; i < count; i++) {
        if (kv->Put(formatKey(i), LOREM_IPSUM_800) != OK) {
            std::cout << "Out of space at key " << std::to_string(i) << "\n";
            exit(-42);
        }
    }
    LOG("   in " << current_millis() - started << " ms");
}

static void testRemove(KVTree* kv) {
    auto started = current_millis();
    for (int i = 0; i < count; i++) kv->Remove(formatKey(i));
    LOG("   in " << current_millis() - started << " ms");
}

static void usage_exit(int exit_code) {
    ((exit_code == EXIT_SUCCESS) ? std::cout : std::cerr) << USAGE << "\n";
    exit(exit_code);
}

int main(int argc, char** argv) {
    // assume reading by default when present
    bool reading = (access(path.c_str(), F_OK) == 0);

    // validate command line args if present
    if (argc > 1) {
        std::string command(argv[1]);
        if (command == "h" or command == "-help" or command == "--help")
            usage_exit(EXIT_SUCCESS);
        else if (argc != 4)
            usage_exit(EXIT_FAILURE);
        else if (command == "r" or command == "read")
            reading = true;
        else if (command == "w" or command == "write")
            reading = false;
        else usage_exit(EXIT_FAILURE);
        path = argv[2];
        size = std::stoul(argv[3]) * (1024 * 1024);
        count = (int) (size / 1100);
    }

    // run read or write workload
    if (reading) {
        LOG("\nOpening for reads: " + path);
        KVTree* kv = open();
        for (int i = 0; i < 8; i++) {
            LOG("Getting " << count << " random values");
            testGetRandom(kv);
            LOG("Getting " << count << " sequential values");
            testGetSequential(kv);
        }
        delete kv;
    } else {
        LOG("\nOpening for writes: " + path);
        KVTree* kv = open();
        LOG("Inserting " << count << " values");
        testPut(kv);
//      LOG("Updating " << count << " values");
//      testPut(kv);
//      LOG("Removing " << count << " values");
//      testRemove(kv);
//      LOG("Reinserting " << count << " values");
//      testPut(kv);
        delete kv;
    }
    LOG("\nFinished");
    return 0;
}
