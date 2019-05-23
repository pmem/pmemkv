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

#ifndef LIBPMEMKV_HPP
#define LIBPMEMKV_HPP

#include <string>
#include <stdexcept>
#include <functional>

#include "libpmemkv.h"

namespace pmem {
namespace kv {

typedef void(all_function_t)(int keybytes, const char* key);
typedef void(each_function_t)(int keybytes, const char* key, int valuebytes, const char* value);
typedef void(get_function_t)(int valuebytes, const char* value);

// XXX: better name
// XXX: in this file we can have only declarations and implementation
// 	in libpmemkv_impl.hpp (included in this file)
class db {
public:
	db(const char* engine_name, const char* config) {
		std::string errormsg;

		// XXX: replace this callback on failure with status
		this->engine = pmemkv_start(&errormsg, engine_name, config,
			[](void* context, const char* engine, const char* config, const char* msg) {
				*((std::string*) context) = msg;
			});

		if (errormsg.size() > 0)
			throw std::runtime_error(errormsg);
	}

	~db() {
		pmemkv_stop(this->engine);
	}

	void All(void* context, pmemkv_all_callback_t* callback) {
		pmemkv_all(this->engine, context, callback);
	}

	// XXX: if C++ API is build on C API there is no way to support std::function
	// There are two solution:
	// 1. implement only version with *context and C-style function pointer
	// 2. implement function_view and export this symbol (our own implementation of callable object)


	/* void All(std::function<KVAllStringFunction> f); */
    	/* void All(std::function<KVFunction> f); */

	/* 
	 *  ...
	 */

private:
	pmemkv_engine* engine;
};

} /* namespace kv */
} /* namespace pmem */

#endif /* LIBPMEMKV_HPP */