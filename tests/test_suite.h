/*
 * Copyright 2019-2020, Intel Corporation
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

#ifndef TEST_SUITE_H
#define TEST_SUITE_H

#include <string>

struct Basic {
	/* path parameter passed to engine config */
	std::string *path;
	/* size parameter passed to engine config */
	uint64_t size;
	/* force_create parameter passed to engine config */
	uint64_t force_create;
	/* engine name */
	const char *engine;
	/* key length */
	size_t key_length;
	/* max size of data  */
	size_t value_length;
	/* size of actually inserted data */
	size_t test_value_length;
	/* test name */
	std::string name;
	/* markers for build system, which tracers should be used:
	 * M - memcheck
	 * P - pmemcheck
	 * H - helgrind
	 * D - drd  */
	std::string tracers;
	/* it specifies if engine should treat path as file or
	 * directory */
	bool use_file;

	std::string get_path()
	{
		std::string abs_path(*path);
		abs_path.append("/" + name);

		return abs_path;
	}
};

std::ostream &operator<<(std::ostream &stream, const Basic &val)
{
	stream << val.name;
	return stream;
}

#endif // TEST_SUITE_H
