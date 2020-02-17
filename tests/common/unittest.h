/*
 * Copyright 2020, Intel Corporation
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

#ifndef PMEMKV_UT_ASSERT_HPP
#define PMEMKV_UT_ASSERT_HPP

#ifdef __cplusplus
extern "C" {
#endif

#include "test_backtrace.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define START() test_register_sighandlers()

static inline void UT_OUT(const char *format, ...)
{
	va_list args_list;
	va_start(args_list, format);
	vfprintf(stdout, format, args_list);
	va_end(args_list);

	fprintf(stdout, "\n");
}

static inline void UT_FATAL(const char *format, ...)
{
	va_list args_list;
	va_start(args_list, format);
	vfprintf(stderr, format, args_list);
	va_end(args_list);

	fprintf(stderr, "\n");

	abort();
}

/* assert a condition is true at runtime */
#define UT_ASSERT(cnd)                                                                   \
	((void)((cnd) ||                                                                 \
		(UT_FATAL("%s:%d %s - assertion failure: %s, errormsg: ", __FILE__,      \
			  __LINE__, __func__, #cnd, pmemkv_errormsg()),                  \
		 0)))

/* assertion with extra info printed if assertion fails at runtime */
#define UT_ASSERTinfo(cnd, info)                                                         \
	((void)((cnd) ||                                                                 \
		(UT_FATAL("%s:%d %s - assertion failure: %s (%s = %s), errormsg: %s",    \
			  __FILE__, __LINE__, __func__, #cnd, #info, info,               \
			  pmemkv_errormsg()),                                            \
		 0)))

/* assert two integer values are equal at runtime */
#define UT_ASSERTeq(lhs, rhs)                                                            \
	((void)(((lhs) == (rhs)) ||                                                      \
		(UT_FATAL("%s:%d %s - assertion failure: %s (0x%llx) == %s "             \
			  "(0x%llx), errormsg: %s",                                      \
			  __FILE__, __LINE__, __func__, #lhs, (unsigned long long)(lhs), \
			  #rhs, (unsigned long long)(rhs), pmemkv_errormsg()),           \
		 0)))

/* assert two integer values are not equal at runtime */
#define UT_ASSERTne(lhs, rhs)                                                            \
	((void)(((lhs) != (rhs)) ||                                                      \
		(UT_FATAL("%s:%d %s - assertion failure: %s (0x%llx) != %s "             \
			  "(0x%llx), errormsg: %s",                                      \
			  __FILE__, __LINE__, __func__, #lhs, (unsigned long long)(lhs), \
			  #rhs, (unsigned long long)(rhs), pmemkv_errormsg()),           \
		 0)))

#ifdef __cplusplus
}
#endif

#endif /* PMEMKV_UT_ASSERT_HPP */
