// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#ifndef PMEMKV_UNITTEST_H
#define PMEMKV_UNITTEST_H

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
		(UT_FATAL("%s:%d %s - assertion failure: %s, errormsg: %s", __FILE__,    \
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

#endif /* PMEMKV_UNITTEST_H */
