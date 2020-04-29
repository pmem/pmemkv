// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#ifndef LIBPMEMKV_EXCEPTIONS_H
#define LIBPMEMKV_EXCEPTIONS_H

#include <stdexcept>

#include "libpmemkv.h"

namespace pmem
{
namespace kv
{
namespace internal
{

struct error : std::runtime_error {
	error(const std::string &msg, int status_code = PMEMKV_STATUS_UNKNOWN_ERROR)
	    : std::runtime_error(msg), status_code(status_code)
	{
	}
	int status_code;
};

struct not_supported : error {
	not_supported(const std::string &msg) : error(msg, PMEMKV_STATUS_NOT_SUPPORTED)
	{
	}
};

struct invalid_argument : error {
	invalid_argument(const std::string &msg)
	    : error(msg, PMEMKV_STATUS_INVALID_ARGUMENT)
	{
	}
};

struct config_parsing_error : error {
	config_parsing_error(const std::string &msg)
	    : error(msg, PMEMKV_STATUS_CONFIG_PARSING_ERROR)
	{
	}
};

struct config_type_error : error {
	config_type_error(const std::string &msg)
	    : error(msg, PMEMKV_STATUS_CONFIG_TYPE_ERROR)
	{
	}
};

struct wrong_engine_name : error {
	wrong_engine_name(const std::string &msg)
	    : error(msg, PMEMKV_STATUS_WRONG_ENGINE_NAME)
	{
	}
};

struct comparator_mismatch : error {
	comparator_mismatch(const std::string &msg)
	    : error(msg, PMEMKV_STATUS_COMPARATOR_MISMATCH)
	{
	}
};

} /* namespace internal */
} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_EXCEPTIONS_H */
