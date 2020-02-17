/*
 * Copyright 2015-2019, Intel Corporation
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

/*
 * backtrace.c -- backtrace reporting routines
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "test_backtrace.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_LIBUNWIND

#define UNW_LOCAL_ONLY
#include <dlfcn.h>
#include <libunwind.h>

#define PROCNAMELEN 256
/*
 * test_dump_backtrace -- dump stacktrace to error log using libunwind
 */
void
test_dump_backtrace(void)
{
	unw_context_t context;
	unw_proc_info_t pip;

	pip.unwind_info = NULL;
	int ret = unw_getcontext(&context);
	if (ret) {
		fprintf(stderr, "unw_getcontext: %s [%d]\n", unw_strerror(ret),
			ret);
		return;
	}

	unw_cursor_t cursor;
	ret = unw_init_local(&cursor, &context);
	if (ret) {
		fprintf(stderr, "unw_init_local: %s [%d]\n", unw_strerror(ret),
			ret);
		return;
	}

	ret = unw_step(&cursor);

	char procname[PROCNAMELEN];
	unsigned i = 0;

	while (ret > 0) {
		ret = unw_get_proc_info(&cursor, &pip);
		if (ret) {
			fprintf(stderr, "[%u] unw_get_proc_info: %s [%d]\n", i,
				unw_strerror(ret), ret);
			break;
		}

		unw_word_t off;
		ret = unw_get_proc_name(&cursor, procname, PROCNAMELEN, &off);
		if (ret && ret != -UNW_ENOMEM) {
			if (ret != -UNW_EUNSPEC) {
				fprintf(stderr,
					"[%u] unw_get_proc_name: %s [%d]\n", i,
					unw_strerror(ret), ret);
			}

			strcpy(procname, "?");
		}

		void *ptr = (void *)(pip.start_ip + off);
		Dl_info dlinfo;
		const char *fname = "?";

		if (dladdr(ptr, &dlinfo) && dlinfo.dli_fname &&
		    *dlinfo.dli_fname)
			fname = dlinfo.dli_fname;

		printf("%u: %s (%s%s+0x%lx) [%p]\n", i++, fname, procname,
		       ret == -UNW_ENOMEM ? "..." : "", off, ptr);

		ret = unw_step(&cursor);
		if (ret < 0)
			fprintf(stderr, "unw_step: %s [%d]\n",
				unw_strerror(ret), ret);
	}
}
#else /* USE_LIBUNWIND */

#define BSIZE 100

#ifndef _WIN32

#include <execinfo.h>

/*
 * test_dump_backtrace -- dump stacktrace to error log using libc's backtrace
 */
void
test_dump_backtrace(void)
{
	int j, nptrs;
	void *buffer[BSIZE];
	char **strings;

	nptrs = backtrace(buffer, BSIZE);

	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL) {
		fprintf(stderr, "backtrace_symbols %s\n", strerror(errno));
		return;
	}

	for (j = 0; j < nptrs; j++)
		printf("%u: %s\n", j, strings[j]);

	free(strings);
}

#else /* _WIN32 */

#include <assert.h>
#include <tchar.h>
#include <windows.h>

#include <DbgHelp.h>

/*
 * test_dump_backtrace -- dump stacktrace to error log
 */
void
test_dump_backtrace(void)
{
	void *buffer[BSIZE];
	unsigned nptrs;
	SYMBOL_INFO *symbol;

	HANDLE proc_hndl = GetCurrentProcess();
	SymInitialize(proc_hndl, NULL, TRUE);

	nptrs = CaptureStackBackTrace(0, BSIZE, buffer, NULL);
	symbol = calloc(sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(CHAR), 1);
	symbol->MaxNameLen = MAX_SYM_NAME - 1;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	for (unsigned i = 0; i < nptrs; i++) {
		if (SymFromAddr(proc_hndl, (DWORD64)buffer[i], 0, symbol)) {
			printf("%u: %s [%p]\n", nptrs - i - 1, symbol->Name,
			       buffer[i]);
		} else {
			printf("%u: [%p]\n", nptrs - i - 1, buffer[i]);
		}
	}

	free(symbol);
}

#endif /* _WIN32 */

#endif /* USE_LIBUNWIND */

/*
 * test_sighandler -- fatal signal handler
 */
void
test_sighandler(int sig)
{
	printf("\nSignal %d, backtrace:\n", sig);
	test_dump_backtrace();
	printf("\n");
	exit(128 + sig);
}

/*
 * test_register_sighandlers -- register signal handlers for various fatal
 * signals
 */
void
test_register_sighandlers(void)
{
	signal(SIGSEGV, test_sighandler);
	signal(SIGABRT, test_sighandler);
	signal(SIGILL, test_sighandler);
	signal(SIGFPE, test_sighandler);
	signal(SIGINT, test_sighandler);
#ifndef _WIN32
	signal(SIGALRM, test_sighandler);
	signal(SIGQUIT, test_sighandler);
	signal(SIGBUS, test_sighandler);
#endif
}
