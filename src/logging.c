// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2014-2020, Intel Corporation */

// #include "valgrind.h"
#include "logging.h"

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *Log_prefix;
static int Log_level;
static FILE *Out_fp;
static unsigned Log_alignment;

// XXX: fix this hardcode
#define SRCVERSION "1.0"

/* no Windows support, use just POSIX version */
// #define util_strerror strerror_r
#define os_getenv getenv

/*
 * util_snprintf -- run snprintf; in case of truncation or a failure
 * return a negative value, or the number of characters printed otherwise.
 */
int util_snprintf(char *str, size_t size, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	int ret = vsnprintf(str, size, format, ap);
	va_end(ap);

	if (ret < 0) {
		// if (!errno)
		//	errno = EIO;
		goto err;
	} else if ((size_t)ret >= size) {
		// errno = ENOBUFS;
		goto err;
	}

	return ret;
err:
	return -1;
}

/*
 * util_getexecname -- return name of current executable
 */
char *util_getexecname(char *path, size_t pathlen)
{
	assert(pathlen != 0);
	ssize_t cc;

	cc = readlink("/proc/self/exe", path, pathlen);
	if (cc == -1) {
		strncpy(path, "unknown", pathlen);
		path[pathlen - 1] = '\0';
	} else {
		path[cc] = '\0';
	}

	return path;
}

/*
 * util_strwinerror -- should never be called on posix OS - abort()
 */
void util_strwinerror(unsigned long err, char *buff, size_t bufflen)
{
	abort();
}

/* extra defines */
#define OS_DIR_SEPARATOR '\\'
#define UTIL_MAX_ERR_MSG 128
#ifndef NO_LIBPTHREAD
#define MAXPRINT 8192 /* maximum expected log line */
#else
#define MAXPRINT 256 /* maximum expected log line for libpmem */
#endif

/*
 * out_init -- initialize the log
 *
 * This is called from the library initialization code.
 */
void out_init(const char *log_prefix, const char *log_level_var, const char *log_file_var,
	      int major_version, int minor_version)
{
	static int once;

	/* only need to initialize the out module once */
	if (once)
		return;
	once++;

	Log_prefix = log_prefix;

#ifdef PMEMKV_USE_LOGGING
	char *log_level;
	char *log_file;

	if ((log_level = os_getenv(log_level_var)) != NULL) {
		Log_level = atoi(log_level);
		if (Log_level < 0) {
			Log_level = 0;
		}
	}

	if ((log_file = os_getenv(log_file_var)) != NULL && log_file[0] != '\0') {

		/* reserve more than enough space for a PID + '\0' */
		char log_file_pid[PATH_MAX];
		size_t len = strlen(log_file);
		if (len > 0 && log_file[len - 1] == '-') {
			if (util_snprintf(log_file_pid, PATH_MAX, "%s%d", log_file,
					  getpid()) < 0) {
				// ERR("snprintf: %d", errno);
				abort();
			}
			log_file = log_file_pid;
		}

		if ((Out_fp = fopen(log_file, "w")) == NULL) {
			// char buff[UTIL_MAX_ERR_MSG];
			// util_strerror(errno, buff, UTIL_MAX_ERR_MSG);
			// fprintf(stderr, "Error (%s): %s=%s: %s\n", log_prefix,
			//	log_file_var, log_file, buff);

			// ERR("snprintf: %d", errno);
			abort();
		}
	}
#endif /* PMEMKV_USE_LOGGING */

	char *log_alignment = os_getenv("PMDK_LOG_ALIGN");
	if (log_alignment) {
		int align = atoi(log_alignment);
		if (align > 0)
			Log_alignment = (unsigned)align;
	}

	if (Out_fp == NULL)
		Out_fp = stderr;
	else
		setlinebuf(Out_fp);

#ifdef PMEMKV_USE_LOGGING
	static char namepath[PATH_MAX];
	LOG_PMDK(1, "pid %d: program: %s", getpid(),
		 util_getexecname(namepath, PATH_MAX));
#endif
	LOG_PMDK(1, "%s version %d.%d", log_prefix, major_version, minor_version);

	static __attribute__((used)) const char *version_msg = "src version: " SRCVERSION;
	LOG_PMDK(1, "%s", version_msg);
#if VG_PMEMCHECK_ENABLED
	/*
	 * Attribute "used" to prevent compiler from optimizing out the variable
	 * when LOG expands to no code (!PMEMKV_USE_LOGGING)
	 */
	static __attribute__((used)) const char *pmemcheck_msg =
		"compiled with support for Valgrind pmemcheck";
	LOG_PMDK(1, "%s", pmemcheck_msg);
#endif /* VG_PMEMCHECK_ENABLED */
#if VG_HELGRIND_ENABLED
	static __attribute__((used)) const char *helgrind_msg =
		"compiled with support for Valgrind helgrind";
	LOG_PMDK(1, "%s", helgrind_msg);
#endif /* VG_HELGRIND_ENABLED */
#if VG_MEMCHECK_ENABLED
	static __attribute__((used)) const char *memcheck_msg =
		"compiled with support for Valgrind memcheck";
	LOG_PMDK(1, "%s", memcheck_msg);
#endif /* VG_MEMCHECK_ENABLED */
#if VG_DRD_ENABLED
	static __attribute__((used)) const char *drd_msg =
		"compiled with support for Valgrind drd";
	LOG_PMDK(1, "%s", drd_msg);
#endif /* VG_DRD_ENABLED */
#if SDS_ENABLED
	static __attribute__((used)) const char *shutdown_state_msg =
		"compiled with support for shutdown state";
	LOG_PMDK(1, "%s", shutdown_state_msg);
#endif
#if NDCTL_ENABLED
	static __attribute__((used)) const char *ndctl_ge_63_msg =
		"compiled with libndctl 63+";
	LOG_PMDK(1, "%s", ndctl_ge_63_msg);
#endif

	// Last_errormsg_key_alloc();
}

/*
 * out_fini -- close the log file
 *
 * This is called to close log file before process stop.
 */
void out_fini(void)
{
	if (Out_fp != NULL && Out_fp != stderr) {
		fclose(Out_fp);
		Out_fp = stderr;
	}

	// Last_errormsg_fini();
}

/*
 * out_print_func -- default print_func, goes to stderr or Out_fp
 */
static void out_print_func(const char *s)
{
	/* to suppress drd false-positive */
	/* XXX: confirm real nature of this issue: pmem/issues#863 */
#ifdef SUPPRESS_FPUTS_DRD_ERROR
	VALGRIND_ANNOTATE_IGNORE_READS_BEGIN();
	VALGRIND_ANNOTATE_IGNORE_WRITES_BEGIN();
#endif
	fputs(s, Out_fp);
#ifdef SUPPRESS_FPUTS_DRD_ERROR
	VALGRIND_ANNOTATE_IGNORE_READS_END();
	VALGRIND_ANNOTATE_IGNORE_WRITES_END();
#endif
}

/*
 * calling Print(s) calls the current print_func...
 */
typedef void (*Print_func)(const char *s);
typedef int (*Vsnprintf_func)(char *str, size_t size, const char *format, va_list ap);
static Print_func Print = out_print_func;
static Vsnprintf_func Vsnprintf = vsnprintf;

/*
 * out_set_print_func -- allow override of print_func used by out module
 */
void out_set_print_func(void (*print_func)(const char *s))
{
	LOG_PMDK(3, "print %p", print_func);

	Print = (print_func == NULL) ? out_print_func : print_func;
}

/*
 * out_set_vsnprintf_func -- allow override of vsnprintf_func used by out module
 */
void out_set_vsnprintf_func(int (*vsnprintf_func)(char *str, size_t size,
						  const char *format, va_list ap))
{
	LOG_PMDK(3, "vsnprintf %p", vsnprintf_func);

	Vsnprintf = (vsnprintf_func == NULL) ? vsnprintf : vsnprintf_func;
}

/*
 * out_snprintf -- (internal) custom snprintf implementation
 */
FORMAT_PRINTF(3, 4)
static int out_snprintf(char *str, size_t size, const char *format, ...)
{
	int ret;
	va_list ap;

	va_start(ap, format);
	ret = Vsnprintf(str, size, format, ap);
	va_end(ap);

	return (ret);
}

/*
 * out_common -- common output code, all output goes through here
 */
static void out_common(const char *file, int line, const char *func, int level,
		       const char *suffix, const char *fmt, va_list ap)
{
	// int oerrno = errno;
	char buf[MAXPRINT];
	unsigned cc = 0;
	int ret;
	const char *sep = "";
	char errstr[UTIL_MAX_ERR_MSG] = "";

	// unsigned long olast_error = 0;
	// #ifdef _WIN32
	//	if (fmt && fmt[0] == '!' && fmt[1] == '!')
	//		olast_error = GetLastError();
	// #endif

	if (file) {
		const char *f = strrchr(file, OS_DIR_SEPARATOR);
		if (f)
			file = f + 1;
		ret = out_snprintf(&buf[cc], MAXPRINT - cc, "<%s>: <%d> [%s:%d %s] ",
				   Log_prefix, level, file, line, func);
		if (ret < 0) {
			Print("out_snprintf failed");
			// goto end;
			return;
		}
		cc += (unsigned)ret;
		if (cc < Log_alignment) {
			memset(buf + cc, ' ', Log_alignment - cc);
			cc = Log_alignment;
		}
	}

	if (fmt) {
		if (*fmt == '!') {
			sep = ": ";
			fmt++;
			if (*fmt == '!') {
				fmt++;
				/* it will abort on non Windows OS */
				// util_strwinerror(olast_error, errstr,
				// UTIL_MAX_ERR_MSG);
			} else {
				// util_strerror(oerrno, errstr, UTIL_MAX_ERR_MSG);
			}
		}
		ret = Vsnprintf(&buf[cc], MAXPRINT - cc, fmt, ap);
		if (ret < 0) {
			Print("Vsnprintf failed");
			// goto end;
			return;
		}
		cc += (unsigned)ret;
	}

	out_snprintf(&buf[cc], MAXPRINT - cc, "%s%s%s", sep, errstr, suffix);

	Print(buf);

	// end:
	//	errno = oerrno;
	// #ifdef _WIN32
	//	SetLastError(olast_error);
	// #endif
}

/*
 * out -- output a line, newline added automatically
 */
void out(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	out_common(NULL, 0, NULL, 0, "\n", fmt, ap);

	va_end(ap);
}

/*
 * out_nonl -- output a line, no newline added automatically
 */
void out_nonl(int level, const char *fmt, ...)
{
	va_list ap;

	if (Log_level < level)
		return;

	va_start(ap, fmt);
	out_common(NULL, 0, NULL, level, "", fmt, ap);

	va_end(ap);
}

/*
 * out_log -- output a log line if Log_level >= level
 */
void out_log(const char *file, int line, const char *func, int level, const char *fmt,
	     ...)
{
	va_list ap;

	if (Log_level < level)
		return;

	va_start(ap, fmt);
	out_common(file, line, func, level, "\n", fmt, ap);

	va_end(ap);
}
