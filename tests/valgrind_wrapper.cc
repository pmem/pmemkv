// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <unistd.h>
#include <vector>

std::string get_filename(std::string path)
{
	size_t pos = path.find_last_of("/\\");
	return path.substr(pos + 1);
}

std::string get_cwd_rel_path(std::string path)
{
	size_t pos = path.find_last_of("/\\");
	return path.substr(0, pos);
}

int main(int argc, char *argv[])
{
	std::map<std::string, std::pair<std::string, const char *>> tool_options;
	tool_options["helgrind"] = std::make_pair("H", "--tool=helgrind");
	tool_options["drd"] = std::make_pair("D", "--tool=drd");
	tool_options["pmemcheck"] = std::make_pair("P", "--tool=pmemcheck");
	tool_options["memcheck"] = std::make_pair("M", "--leak-check=full");

	std::vector<std::string> args(argv + 1, argv + argc);
	std::vector<char *> command;
	std::string app_name = get_filename(std::string(argv[0]));

	if (tool_options.find(app_name) == tool_options.end()) {
		std::cout << "Invalid application name. Use: ";
		for (auto &i : tool_options)
			std::cout << i.first << " ";
		std::cout << std::endl;
		exit(EXIT_FAILURE);
	}
	bool list_tests =
		(std::find(args.begin(), args.end(), "--gtest_list_tests") != args.end());
	if (!list_tests) {
		command.push_back((char *)"valgrind");
		command.push_back((char *)tool_options[app_name].second);
	}
	std::string bin_path = get_cwd_rel_path(std::string(argv[0])) + "/pmemkv_test";
	command.push_back((char *)bin_path.c_str());
	std::string memcheck_filter =
		"--gtest_filter=*TRACERS_*" + tool_options[app_name].first + "*";
	if (list_tests) {
		std::cout << memcheck_filter.c_str();
		command.push_back((char *)memcheck_filter.c_str());
	}
	for (auto &i : args) {
		command.push_back((char *)i.c_str());
	}
	command.push_back(NULL);
	execvp(command[0], command.data());
	exit(EXIT_FAILURE);
}
