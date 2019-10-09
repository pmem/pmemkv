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

#include "test_path.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

std::string test_path;

void print_option(std::string option, std::string description, std::string params = "")
{
	std::cout << "\033[32m  " << option << "\033[0m " << params << std::endl
		  << "      " << description << std::endl;
}

int main(int argc, char *argv[])
{
	std::vector<std::string> args(argv + 1, argv + argc);
	auto help = std::find(args.begin(), args.end(), "--help");
	if (help != args.end()) {
		print_option("--test_dir", "Path passed to engines config.", "PATH");
		std::cout << std::endl;
	}
	auto test_dir_param = std::find(args.begin(), args.end(), "--test_dir");
	if (test_dir_param != args.end()) {
		auto test_dir = std::next(test_dir_param, 1);
		if (test_dir != args.end()) {
			test_path.assign(*test_dir);
		}
	}
	bool gtest_list_tests =
		(std::find(args.begin(), args.end(), "--gtest_list_tests") != args.end());
	if (!gtest_list_tests && test_path.empty()) {
		std::cerr << "Test path not specified" << std::endl;
		exit(1);
	}

	/* gtest is friendly creature, not at all evil,
	 * so is ignoring non-gtest parameters */
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
