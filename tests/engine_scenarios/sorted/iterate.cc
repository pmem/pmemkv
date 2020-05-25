// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "iterate.hpp"

/**
 * Common tests for all count_* and get_* (except get_all) methods for sorted engines.
 */

static void CountTest(pmem::kv::db &kv)
{
	/**
	 * TEST: all count_* methods with basic keys (without any special chars in keys)
	 */
	add_basic_keys(kv);

	std::size_t cnt;
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == 6);

	/* insert new key */
	UT_ASSERTeq(kv.put("BD", "7"), status::OK);
	UT_ASSERT(kv.count_all(cnt) == status::OK && cnt == 7);

	cnt = std::numeric_limits<std::size_t>::max();
	UT_ASSERT(kv.count_above("", cnt) == status::OK && cnt == 7);
	UT_ASSERT(kv.count_above("A", cnt) == status::OK && cnt == 6);
	UT_ASSERT(kv.count_above("B", cnt) == status::OK && cnt == 3);
	UT_ASSERT(kv.count_above("BC", cnt) == status::OK && cnt == 1);
	UT_ASSERT(kv.count_above("BD", cnt) == status::OK && cnt == 0);
	cnt = 1;
	UT_ASSERT(kv.count_above("ZZ", cnt) == status::OK && cnt == 0);

	cnt = 0;
	UT_ASSERT(kv.count_equal_above("", cnt) == status::OK && cnt == 7);
	cnt = 0;
	UT_ASSERT(kv.count_equal_above("A", cnt) == status::OK && cnt == 7);
	UT_ASSERT(kv.count_equal_above("AA", cnt) == status::OK && cnt == 6);
	UT_ASSERT(kv.count_equal_above("B", cnt) == status::OK && cnt == 4);
	UT_ASSERT(kv.count_equal_above("BC", cnt) == status::OK && cnt == 2);
	UT_ASSERT(kv.count_equal_above("BD", cnt) == status::OK && cnt == 1);
	UT_ASSERT(kv.count_equal_above("Z", cnt) == status::OK && cnt == 0);

	cnt = 1;
	UT_ASSERT(kv.count_below("", cnt) == status::OK && cnt == 0);
	cnt = 10;
	UT_ASSERT(kv.count_below("A", cnt) == status::OK && cnt == 0);
	UT_ASSERT(kv.count_below("B", cnt) == status::OK && cnt == 3);
	UT_ASSERT(kv.count_below("BC", cnt) == status::OK && cnt == 5);
	UT_ASSERT(kv.count_below("BD", cnt) == status::OK && cnt == 6);
	UT_ASSERT(kv.count_below("ZZZZZ", cnt) == status::OK && cnt == 7);

	cnt = 256;
	UT_ASSERT(kv.count_equal_below("", cnt) == status::OK && cnt == 0);
	UT_ASSERT(kv.count_equal_below("A", cnt) == status::OK && cnt == 1);
	UT_ASSERT(kv.count_equal_below("B", cnt) == status::OK && cnt == 4);
	cnt = 257;
	UT_ASSERT(kv.count_equal_below("BA", cnt) == status::OK && cnt == 4);
	UT_ASSERT(kv.count_equal_below("BC", cnt) == status::OK && cnt == 6);
	UT_ASSERT(kv.count_equal_below("BD", cnt) == status::OK && cnt == 7);
	cnt = 258;
	UT_ASSERT(kv.count_equal_below("ZZZZZZ", cnt) == status::OK && cnt == 7);

	cnt = 1024;
	UT_ASSERT(kv.count_between("", "ZZZZ", cnt) == status::OK && cnt == 7);
	UT_ASSERT(kv.count_between("", "A", cnt) == status::OK && cnt == 0);
	UT_ASSERT(kv.count_between("", "B", cnt) == status::OK && cnt == 3);
	UT_ASSERT(kv.count_between("A", "B", cnt) == status::OK && cnt == 2);
	UT_ASSERT(kv.count_between("A", "BD", cnt) == status::OK && cnt == 5);
	UT_ASSERT(kv.count_between("B", "ZZ", cnt) == status::OK && cnt == 3);

	cnt = 1024;
	UT_ASSERT(kv.count_between("", "", cnt) == status::OK && cnt == 0);
	cnt = 1025;
	UT_ASSERT(kv.count_between("A", "A", cnt) == status::OK && cnt == 0);
	cnt = 1026;
	UT_ASSERT(kv.count_between("AC", "A", cnt) == status::OK && cnt == 0);
	cnt = 1027;
	UT_ASSERT(kv.count_between("B", "A", cnt) == status::OK && cnt == 0);
	cnt = 1028;
	UT_ASSERT(kv.count_between("BD", "A", cnt) == status::OK && cnt == 0);
	cnt = 1029;
	UT_ASSERT(kv.count_between("ZZZ", "B", cnt) == status::OK && cnt == 0);
}

int main(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	return run_engine_tests(argv[1], argv[2],
				{
					CountTest,
				});
}
