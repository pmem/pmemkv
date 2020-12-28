// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include <libpmemkv.h>

#include "unittest.h"
#include <string.h>

/**
 * Passing null as 'db' or 'config' in C API produces INVALID_ARGUMENT status
 */

void null_db_all_funcs_test()
{
	/**
	 * TEST: null passed as db to pmemkv_* functions
	 */
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

	s = pmemkv_count_equal_above(NULL, key1, strlen(key1), &cnt);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_count_below(NULL, key1, strlen(key1), &cnt);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_count_equal_below(NULL, key1, strlen(key1), &cnt);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_count_between(NULL, key1, strlen(key1), key2, strlen(key2), &cnt);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_get_all(NULL, NULL, NULL);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_get_above(NULL, key1, strlen(key1), NULL, NULL);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_get_equal_above(NULL, key1, strlen(key1), NULL, NULL);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_get_below(NULL, key1, strlen(key1), NULL, NULL);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_get_equal_below(NULL, key1, strlen(key1), NULL, NULL);
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

	pmemkv_tx *tx;
	s = pmemkv_tx_begin(NULL, &tx);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_tx_begin((pmemkv_db *)0x1, NULL);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);
}

void null_config_test(const char *engine)
{
	/**
	 * TEST: null passed as config to pmemkv_open()
	 */
	pmemkv_config *empty_cfg = NULL;
	pmemkv_db *db = NULL;
	int s = pmemkv_open(engine, empty_cfg, &db);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);
}

void null_db_test(const char *engine)
{
	/**
	 * TEST: null passed as db to pmemkv_open()
	 */
	pmemkv_config *cfg = pmemkv_config_new();
	UT_ASSERTne(cfg, NULL);

	int s = pmemkv_open(engine, cfg, NULL);
	UT_ASSERTeq(s, PMEMKV_STATUS_INVALID_ARGUMENT);

	/* Config should be deleted by pmemkv */
}

void null_tx_test(const char *engine)
{
	const char *key1 = "key1";

	int s = pmemkv_tx_put(NULL, key1, strlen(key1), key1, strlen(key1));
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_tx_remove(NULL, key1, strlen(key1));
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_tx_commit(NULL);
	UT_ASSERT(s == PMEMKV_STATUS_INVALID_ARGUMENT);

	pmemkv_tx_end(NULL);
}

void null_iterator_all_funcs_test()
{
	/**
	 * TEST: null passed as iterator to pmemkv_* functions
	 */
	pmemkv_iterator *it1;
	pmemkv_write_iterator *it2;
	size_t cnt;
	const char *key1 = "key1";
	const char *val1;
	char *val2;

	int s = pmemkv_iterator_new(NULL, &it1);
	UT_ASSERTeq(s, PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_write_iterator_new(NULL, &it2);
	UT_ASSERTeq(s, PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_iterator_seek(NULL, key1, strlen(key1));
	UT_ASSERTeq(s, PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_iterator_seek_lower(NULL, key1, strlen(key1));
	UT_ASSERTeq(s, PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_iterator_seek_lower_eq(NULL, key1, strlen(key1));
	UT_ASSERTeq(s, PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_iterator_seek_higher(NULL, key1, strlen(key1));
	UT_ASSERTeq(s, PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_iterator_seek_higher_eq(NULL, key1, strlen(key1));
	UT_ASSERTeq(s, PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_iterator_seek_to_first(NULL);
	UT_ASSERTeq(s, PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_iterator_seek_to_last(NULL);
	UT_ASSERTeq(s, PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_iterator_is_next(NULL);
	UT_ASSERTeq(s, PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_iterator_next(NULL);
	UT_ASSERTeq(s, PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_iterator_prev(NULL);
	UT_ASSERTeq(s, PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_iterator_key(NULL, &val1, &cnt);
	UT_ASSERTeq(s, PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_iterator_read_range(NULL, 0, 10, &val1, &cnt);
	UT_ASSERTeq(s, PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_write_iterator_write_range(NULL, 0, 10, &val2, &cnt);
	UT_ASSERTeq(s, PMEMKV_STATUS_INVALID_ARGUMENT);

	s = pmemkv_write_iterator_commit(NULL);
	UT_ASSERTeq(s, PMEMKV_STATUS_INVALID_ARGUMENT);

	/* returns void */
	pmemkv_write_iterator_abort(NULL);

	/* returns void */
	pmemkv_iterator_delete(NULL);

	/* returns void */
	pmemkv_write_iterator_delete(NULL);
}

int main(int argc, char *argv[])
{
	START();

	if (argc < 2)
		UT_FATAL("usage %s: engine", argv[0]);

	null_db_all_funcs_test();
	null_config_test(argv[1]);
	null_db_test(argv[1]);
	null_iterator_all_funcs_test();

	return 0;
}
