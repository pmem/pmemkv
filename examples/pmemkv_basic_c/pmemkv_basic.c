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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libpmemkv.h>

#define LOG(msg) printf("%s\n", msg)
#define MAX_VAL_LEN 64

const char *PATH = "/dev/shm";

void start_failure_callback(void *context, const char *engine, pmemkv_config *config, const char *msg) {
  printf("ERROR: %s\n", msg);
  exit(-1);
}

void all_callback(const char *k, size_t kb, void *arg) {
  printf("   visited: %s\n", k);
}

int main() {
  LOG("Creating config file");
  pmemkv_config *cfg = pmemkv_config_new();
  assert(cfg != NULL);

  int ret = pmemkv_config_put(cfg, "path", PATH, strlen(PATH) + 1);
  assert(ret == 0);

  LOG("Starting engine");
  pmemkv_db *kv = pmemkv_open(NULL, "vsmap", cfg, &start_failure_callback);
  pmemkv_config_delete(cfg);

  LOG("Putting new key");
  char* key1 = "key1";
  char* value1 = "value1";
  pmemkv_status s = pmemkv_put(kv, key1, strlen(key1), value1, strlen(value1));
  assert(s == PMEMKV_STATUS_OK && pmemkv_count(kv) == 1);

  LOG("Reading key back");
  char val[MAX_VAL_LEN];
  s = pmemkv_get_copy(kv, key1, strlen(key1), val, MAX_VAL_LEN);
  assert(s == PMEMKV_STATUS_OK && !strcmp(val, "value1"));

  LOG("Iterating existing keys");
  char* key2 = "key2";
  char* value2 = "value2";
  char* key3 = "key3";
  char* value3 = "value3";
  pmemkv_put(kv, key2, strlen(key2), value2, strlen(value2));
  pmemkv_put(kv, key3, strlen(key3), value3, strlen(value3));
  pmemkv_all(kv, &all_callback, NULL);

  LOG("Removing existing key");
  s = pmemkv_remove(kv, key1, strlen(key1));
  assert(s == PMEMKV_STATUS_OK && pmemkv_exists(kv, key1, strlen(key1)) == PMEMKV_STATUS_NOT_FOUND);

  LOG("Stopping engine");
  pmemkv_close(kv);

  return 0;
}
