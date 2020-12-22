// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2014-2020, Intel Corporation */

#ifndef LIBPMEMKV_LOGGING_H
#define LIBPMEMKV_LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>

#ifndef EVALUATE_DBG_EXPRESSIONS
#if defined(PMEMKV_USE_LOGGING) || defined(__clang_analyzer__) ||                        \
	defined(__COVERITY__) || defined(__KLOCWORK__)
#define EVALUATE_DBG_EXPRESSIONS 1
#else
#define EVALUATE_DBG_EXPRESSIONS 0
#endif
#endif

#define FORMAT_PRINTF(a, b) __attribute__((__format__(__printf__, (a), (b))))

/*
 * Enable functions for logging (if turned on in CMake)
 * else define them as void.
 */
#ifdef PMEMKV_USE_LOGGING

/* out_* functions imported from PMDK */
#define OUT_LOG out_log
#define OUT_NONL out_nonl

#else

static __attribute__((always_inline)) inline void
out_log_discard(const char *file, int line, const char *func, int level, const char *fmt,
		...)
{
	(void)file;
	(void)line;
	(void)func;
	(void)level;
	(void)fmt;
}

static __attribute__((always_inline)) inline void out_nonl_discard(int level,
								   const char *fmt, ...)
{
	(void)level;
	(void)fmt;
}

#define OUT_LOG out_log_discard
#define OUT_NONL out_nonl_discard

#endif /* PMEMKV_USE_LOGGING */

/* produce debug/trace output */
#define LOG_PMDK(level, ...)                                                             \
	do {                                                                             \
		if (!EVALUATE_DBG_EXPRESSIONS)                                           \
			break;                                                           \
		OUT_LOG(__FILE__, __LINE__, __func__, level, __VA_ARGS__);               \
	} while (0)

/* produce debug/trace output without prefix and new line */
#define LOG_NONL(level, ...)                                                             \
	do {                                                                             \
		if (!EVALUATE_DBG_EXPRESSIONS)                                           \
			break;                                                           \
		OUT_NONL(level, __VA_ARGS__);                                            \
	} while (0)

void out_init(const char *log_prefix, const char *log_level_var, const char *log_file_var,
	      int major_version, int minor_version);
void out_fini(void);
void out(const char *fmt, ...) FORMAT_PRINTF(1, 2);
void out_nonl(int level, const char *fmt, ...) FORMAT_PRINTF(2, 3);
void out_log(const char *file, int line, const char *func, int level, const char *fmt,
	     ...) FORMAT_PRINTF(5, 6);
void out_set_print_func(void (*print_func)(const char *s));
void out_set_vsnprintf_func(int (*vsnprintf_func)(char *str, size_t size,
						  const char *format, va_list ap));

#ifdef __cplusplus
}
#endif

#endif /* LIBPMEMKV_LOGGING_H */
