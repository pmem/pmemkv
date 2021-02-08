// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2021, Intel Corporation */

#include "out.h"

#include <sstream>
#include <string>

static thread_local std::stringstream error_stream;
static thread_local std::string str;
static thread_local pmem::kv::status last_status;

std::ostream &out_err_stream(const char *func)
{
	error_stream.str(std::string());

	error_stream << "[" << func << "] ";

	return error_stream;
}

void set_last_status(pmem::kv::status s)
{
	last_status = s;
}

const char *out_get_errormsg(void)
{
	if (last_status == pmem::kv::status::NOT_FOUND ||
	    last_status == pmem::kv::status::STOPPED_BY_CB)
		return "";
	str = error_stream.str();
	return str.c_str();
}
