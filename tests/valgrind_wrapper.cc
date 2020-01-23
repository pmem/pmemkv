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
