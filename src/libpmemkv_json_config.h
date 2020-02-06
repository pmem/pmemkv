// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019, Intel Corporation */

#ifndef LIBPMEMKV_JSON_H
#define LIBPMEMKV_JSON_H

#include "libpmemkv.h"

#ifdef __cplusplus
extern "C" {
#endif

int pmemkv_config_from_json(pmemkv_config *config, const char *jsonconfig);
const char *pmemkv_config_from_json_errormsg(void);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* LIBPMEMKV_JSON_H */
