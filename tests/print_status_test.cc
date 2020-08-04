// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include <libpmemkv.hpp>

#include <iostream>
#include <sstream>

#include "unittest.hpp"

static const std::string statuses[] = {"OK",
				       "UNKNOWN_ERROR",
				       "NOT_FOUND",
				       "NOT_SUPPORTED",
				       "INVALID_ARGUMENT",
				       "CONFIG_PARSING_ERROR",
				       "CONFIG_TYPE_ERROR",
				       "STOPPED_BY_CB",
				       "OUT_OF_MEMORY",
				       "WRONG_ENGINE_NAME",
				       "TRANSACTION_SCOPE_ERROR",
				       "DEFRAG_ERROR",
				       "COMPARATOR_MISMATCH"};

void test_status_print(int status_no)
{
	std::ostringstream target;
	target << statuses[status_no] << " (" << status_no << ")";

	std::ostringstream oss;
	auto s = static_cast<pmem::kv::status>(status_no);
	oss << s;

	UT_ASSERT(oss.str() == target.str());
}

int main()
{
	for (size_t i = 0; i < sizeof(statuses) / sizeof(statuses[0]); i++)
		test_status_print(i);

	return 0;
}
