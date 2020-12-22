// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "out.h"

#include <sstream>
#include <string>

static thread_local std::stringstream error_stream;
static thread_local std::string str;

std::ostream &out_err_stream(const char *func)
{
	error_stream.str(std::string());

	error_stream << "[" << func << "] ";

	return error_stream;
}

const char *out_get_errormsg(void)
{
	str = error_stream.str();
	return str.c_str();
}
