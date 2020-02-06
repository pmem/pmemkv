// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2019, Intel Corporation */

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
