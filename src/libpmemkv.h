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

#ifndef LIBPMEMKV_H
#define LIBPMEMKV_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PMEMKV_STATUS_FAILED = -1,
    PMEMKV_STATUS_NOT_FOUND = 0,
    PMEMKV_STATUS_OK = 1,
} pmemkv_status;

typedef struct pmemkv_db pmemkv_db;

typedef void pmemkv_all_callback(void *context, const char *key, size_t keybytes);
typedef void pmemkv_each_callback(void *context, const char *key, size_t keybytes, const char *value, size_t valuebytes);
typedef void pmemkv_get_callback(void *context, const char *value, size_t valuebytes);
typedef void pmemkv_start_failure_callback(void *context, const char *engine, const char *config, const char *msg);

pmemkv_db *pmemkv_new(void *context, const char *engine, const char *config, pmemkv_start_failure_callback* onfail);
void pmemkv_delete(pmemkv_db *db);

void pmemkv_all(pmemkv_db *db, void *context, pmemkv_all_callback* c);
void pmemkv_all_above(pmemkv_db *db, void *context, const char *k, size_t kb, pmemkv_all_callback* c);
void pmemkv_all_below(pmemkv_db *db, void *context, const char *k, size_t kb, pmemkv_all_callback* c);
void pmemkv_all_between(pmemkv_db *db, void *context, const char *k1, size_t kb1,
                          const char *k2, size_t kb2, pmemkv_all_callback* c);

size_t pmemkv_count(pmemkv_db *db);
size_t pmemkv_count_above(pmemkv_db *db, const char *k, size_t kb);
size_t pmemkv_count_below(pmemkv_db *db, const char *k, size_t kb);
size_t pmemkv_count_between(pmemkv_db *db, const char *k1, size_t kb1, const char *k2, size_t kb2);

void pmemkv_each(pmemkv_db *db, void *context, pmemkv_each_callback* c);
void pmemkv_each_above(pmemkv_db *db, void *context, const char *k, size_t kb, pmemkv_each_callback* c);
void pmemkv_each_below(pmemkv_db *db, void *context, const char *k, size_t kb, pmemkv_each_callback* c);
void pmemkv_each_between(pmemkv_db *db, void *context, const char *k1, size_t kb1,
                           const char *k2, size_t kb2, pmemkv_each_callback* c);

pmemkv_status pmemkv_exists(pmemkv_db *db, const char *k, size_t kb);
void pmemkv_get(pmemkv_db *db, void *context, const char *k, size_t kb, pmemkv_get_callback* c);
pmemkv_status pmemkv_get_copy(pmemkv_db *db, const char *k, size_t kb, char* value, size_t maxvaluebytes);
pmemkv_status pmemkv_put(pmemkv_db *db, const char *k, size_t kb, const char *v, size_t vb);
pmemkv_status pmemkv_remove(pmemkv_db *db, const char *k, size_t kb);

// XXX: remove when config structure is merged
void *pmemkv_engine_context(pmemkv_db *db);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* LIBPMEMKV_H */
