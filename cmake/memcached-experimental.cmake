# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2017-2019, Intel Corporation

# FIXME hardcoded path is a VERY bad idea
set(MEMCACHED_INCLUDE $ENV{HOME}/work/libmemcached)

include_directories(${MEMCACHED_INCLUDE})
