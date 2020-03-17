
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
int
main(int argc, char *argv[])
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
	remove(path);

	if (is_pmem)
		return 0;
	else
		return 1;
}
