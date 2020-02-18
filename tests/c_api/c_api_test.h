/*
 * Copyright 2020, Intel Corporation
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

#include <libpmemkv.h>

#include "unittest.h"
#include <string.h>

/* Test if null can be passed as db to pmemkv_* functions */
void check_null_db_test()
{
	size_t cnt;
	const char *key1 = "key1";
	const char *value1 = "value1";
	const char *key2 = "key2";
	char val[10];

	int s = pmemkv_count_all(NULL, &cnt);
	(void)s;
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_count_above(NULL, key1, strlen(key1), &cnt);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_count_below(NULL, key1, strlen(key1), &cnt);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_count_between(NULL, key1, strlen(key1), key2, strlen(key2), &cnt);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_get_all(NULL, NULL, NULL);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_get_above(NULL, key1, strlen(key1), NULL, NULL);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_get_below(NULL, key1, strlen(key1), NULL, NULL);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_get_between(NULL, key1, strlen(key1), key2, strlen(key2), NULL, NULL);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_exists(NULL, key1, strlen(key1));
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_get(NULL, key1, strlen(key1), NULL, NULL);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_get_copy(NULL, key1, strlen(key1), val, 10, &cnt);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_put(NULL, key1, strlen(key1), value1, strlen(value1));
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_remove(NULL, key1, strlen(key1));
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_defrag(NULL, 0, 100);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);
}

void null_config_test(const char *engine)
{
	/* XXX solve it generically, for all tests */
	if (strcmp(engine, "blackhole") == 0)
		return;

	pmemkv_config *empty_cfg = NULL;
	pmemkv_db *db = NULL;
	int s = pmemkv_open(engine, empty_cfg, &db);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);
}
