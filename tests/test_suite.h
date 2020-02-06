// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

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
