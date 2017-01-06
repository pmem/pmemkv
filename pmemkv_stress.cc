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
#include <sys/time.h>
#include "pmemkv.h"

#define LOG(msg) std::cout << msg << "\n"

using namespace pmemkv;

const unsigned long COUNT = 3000000;
const std::string PATH = "/dev/shm/pmemkv";

const char* LOREM_IPSUM_120 = " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer non vestibulum lectus. Suspendisse metus leo volutpa.";
const char* LOREM_IPSUM_248 = " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Ut vulputate neque egestas, hendrerit nibh in, tristique urna. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec non orci mattis, cursus nisl eu, aliquam felis. Ut euismod ame.";
const char* LOREM_IPSUM_504 = " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aliquam et varius velit, in venenatis augue. Mauris volutpat consectetur suscipit. Nam velit sem, consectetur quis euismod id, ornare non turpis. Curabitur tempor ut turpis vitae tincidunt. Praesent malesuada dapibus congue. Nullam eu sollicitudin ex, eget ullamcorper massa. Phasellus feugiat dictum augue ac molestie. Cras non augue lacinia, laoreet elit eleifend, maximus sapien. Proin gravida congue neque, in tempor sem euismod ut. Nullami.";

unsigned long current_millis() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (unsigned long long) (tv.tv_sec) * 1000 + (unsigned long long) (tv.tv_usec) / 1000;
}

KVTree* open() {
  auto started = current_millis();
  auto kv = new KVTree(PATH);
  LOG("   in " << current_millis() - started << " ms");
  return kv;
}

void testDelete(KVTree* kv) {
  auto started = current_millis();
  for (int i = 0; i < COUNT; i++) kv->Delete(std::to_string(i));
  LOG("   in " << current_millis() - started << " ms");
}

void testGetSequential(KVTree* kv) {
  auto started = current_millis();
  for (int i = 0; i < COUNT; i++) {
    std::string value;
    kv->Get(std::to_string(i), &value);
  }
  LOG("   in " << current_millis() - started << " ms");
}

void testGetRandom(KVTree* kv) {
  std::random_device random_device;
  std::mt19937 generator(random_device());
  std::uniform_int_distribution<> distribution(1, COUNT - 1);
  auto started = current_millis();
  for (int i = 0; i < COUNT; i++) {
    std::string value;
    kv->Get(std::to_string(distribution(generator)), &value);
  }
  LOG("   in " << current_millis() - started << " ms");
}

void testPut(KVTree* kv) {
  auto started = current_millis();
  for (int i = 0; i < COUNT; i++) {
    kv->Put(std::to_string(i), std::to_string(i) + LOREM_IPSUM_504);
  }
  LOG("   in " << current_millis() - started << " ms");
}

int main() {
  LOG("\nRecovering");
  KVTree* kv = open();
  LOG("Inserting " << COUNT << " values");
  testPut(kv);
  LOG("Getting " << COUNT << " sequential values");
  testGetSequential(kv);
  LOG("Getting " << COUNT << " random values");
  testGetRandom(kv);
  delete kv;

  LOG("\nRecovering");
  kv = open();
  LOG("Getting " << COUNT << " random values");
  testGetRandom(kv);
  LOG("Getting " << COUNT << " sequential values");
  testGetSequential(kv);
  LOG("Updating " << COUNT << " values");
  testPut(kv);
  LOG("Deleting " << COUNT << " values");
  testDelete(kv);
  LOG("Reinserting " << COUNT << " values");
  testPut(kv);
  delete kv;

  LOG("\nFinished");
  return 0;
}
