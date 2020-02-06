# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2017-2019, Intel Corporation

# FIXME hardcoded paths is a VERY bad idea
set(LIB_ACL $ENV{HOME}/work/libacl/lib_acl)
set(LIB_ACL_CPP $ENV{HOME}/work/libacl/lib_acl_cpp)
set(LIB_PROTOCOL $ENV{HOME}/work/libacl/lib_protocol/lib)

include_directories(SYSTEM ${LIB_ACL}/include ${LIB_ACL_CPP}/include/acl_cpp)
link_directories(${LIB_ACL_CPP}/lib ${LIB_PROTOCOL} ${LIB_ACL}/lib)
