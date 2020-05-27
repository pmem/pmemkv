// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

/*
 * check_is_pmem.cpp -- check if path points to a directory on pmem.
 */

#include <cstdio>
#include <iostream>
#include <libpmem.h>

/*
 * return value is:
 * - 0 when path points to pmem
 * - 1 when path points to non-pmem
 * - 2 when error occurred
 */
int main(int argc, char *argv[])
{
	if (argc != 2) {
		std::cerr << "usage: " << argv[0] << " filepath\n";
		return 2;
	}

	auto path = argv[1];

	int is_pmem;
	size_t size;
	int flags = PMEM_FILE_CREATE;

#ifdef _WIN32
	void *addr = pmem_map_fileU(path, 4096, flags, 0, &size, &is_pmem);
#else
	void *addr = pmem_map_file(path, 4096, flags, 0, &size, &is_pmem);
#endif
	if (addr == nullptr) {
		perror("pmem_map_file failed");
		return 2;
	}

	pmem_unmap(addr, size);
	if (remove(path) != 0) {
		perror("remove(path) failed");
		return 2;
	}

	if (is_pmem)
		return 0;
	else
		return 1;
}
