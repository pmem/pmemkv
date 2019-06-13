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

#include <assert.h>
#include <libpmemkv.h>
#include <stdio.h>
#include <string.h>

#define LOG(msg) printf("%s\n", msg)
#define MAX_VAL_LEN 64

const char *PATH = "/dev/shm";
const size_t SIZE = ((size_t)(1024 * 1024 * 1024));

void all_callback(const char *k, size_t kb, void *arg)
{
	printf("   visited: %s\n", k);
}

int main()
{
	LOG("Creating config file");
	pmemkv_config *cfg = pmemkv_config_new();
	assert(cfg != NULL);

	int s = pmemkv_config_put(cfg, "path", PATH, strlen(PATH) + 1);
	assert(s == PMEMKV_STATUS_OK);
	s = pmemkv_config_put(cfg, "size", &SIZE, sizeof(SIZE));
	assert(s == PMEMKV_STATUS_OK);

	LOG("Starting engine");
	pmemkv_db *db = NULL;
	s = pmemkv_open(NULL, "vsmap", cfg, &db);
	assert(s == PMEMKV_STATUS_OK && db != NULL);
	pmemkv_config_delete(cfg);

	LOG("Putting new key");
	char *key1 = "key1";
	char *value1 = "value1";
	s = pmemkv_put(db, key1, strlen(key1), value1, strlen(value1));
	assert(s == PMEMKV_STATUS_OK);

	size_t cnt;
	s = pmemkv_count(db, &cnt);
	assert(s == PMEMKV_STATUS_OK && cnt == 1);

	LOG("Reading key back");
	char val[MAX_VAL_LEN];
	s = pmemkv_get_copy(db, key1, strlen(key1), val, MAX_VAL_LEN);
	assert(s == PMEMKV_STATUS_OK && !strcmp(val, "value1"));

	LOG("Iterating existing keys");
	char *key2 = "key2";
	char *value2 = "value2";
	char *key3 = "key3";
	char *value3 = "value3";
	pmemkv_put(db, key2, strlen(key2), value2, strlen(value2));
	pmemkv_put(db, key3, strlen(key3), value3, strlen(value3));
	pmemkv_all(db, &all_callback, NULL);

	LOG("Removing existing key");
	s = pmemkv_remove(db, key1, strlen(key1));
	assert(s == PMEMKV_STATUS_OK &&
	       pmemkv_exists(db, key1, strlen(key1)) == PMEMKV_STATUS_NOT_FOUND);

	LOG("Stopping engine");
	pmemkv_close(db);

	return 0;
}
