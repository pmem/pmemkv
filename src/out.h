// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2021, Intel Corporation */

#ifndef LIBPMEMKV_OUT_H
#define LIBPMEMKV_OUT_H

#include <ostream>

std::ostream &out_err_stream(const char *func);

#define DO_LOG 0
#define LOG(msg)                                                                         \
	do {                                                                             \
		if (DO_LOG)                                                              \
			std::cout << "[" << name() << "] " << msg << "\n";               \
	} while (0)

#define ERR() out_err_stream(__func__)

const char *out_get_errormsg(void);
void set_last_status(int status);

#endif /* LIBPMEMKV_OUT_H */
