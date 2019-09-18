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

} /* namespace internal */
} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_EXCEPTIONS_H */
