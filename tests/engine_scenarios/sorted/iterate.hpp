// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

/**
 * Helper class for testing get_* functions
 */

#define KV_GET_1KEY_CPP_CB(func, key, res)                                               \
	kv.func(key, [&](string_view k, string_view v) {                                 \
		res.append(k.data(), k.size())                                           \
			.append(",")                                                     \
			.append(v.data(), v.size())                                      \
			.append("|");                                                    \
		return 0;                                                                \
	})

#define KV_GET_1KEY_C_CB(func, key, res)                                                 \
	kv.func(                                                                         \
		key,                                                                     \
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {      \
			const auto c = ((std::string *)arg);                             \
			c->append(std::string(k, kb))                                    \
				.append(",")                                             \
				.append(std::string(v, vb))                              \
				.append("|");                                            \
			return 0;                                                        \
		},                                                                       \
		&res)

#define KV_GET_2KEYS_CPP_CB(func, key1, key2, res)                                       \
	kv.func(key1, key2, [&](string_view k, string_view v) {                          \
		res.append(k.data(), k.size())                                           \
			.append(",")                                                     \
			.append(v.data(), v.size())                                      \
			.append("|");                                                    \
		return 0;                                                                \
	})

#define KV_GET_2KEYS_C_CB(func, key1, key2, res)                                         \
	kv.func(                                                                         \
		key1, key2,                                                              \
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {      \
			const auto c = ((std::string *)arg);                             \
			c->append(std::string(k, kb))                                    \
				.append(",")                                             \
				.append(std::string(v, vb))                              \
				.append("|");                                            \
			return 0;                                                        \
		},                                                                       \
		&res)

using namespace pmem::kv;

inline void add_basic_keys(pmem::kv::db &kv)
{
	UT_ASSERTeq(kv.put("A", "1"), status::OK);
	UT_ASSERTeq(kv.put("AB", "2"), status::OK);
	UT_ASSERTeq(kv.put("AC", "3"), status::OK);
	UT_ASSERTeq(kv.put("B", "4"), status::OK);
	UT_ASSERTeq(kv.put("BB", "5"), status::OK);
	UT_ASSERTeq(kv.put("BC", "6"), status::OK);
}

inline void add_ext_keys(pmem::kv::db &kv)
{
	UT_ASSERT(kv.put("aaa", "1") == status::OK);
	UT_ASSERT(kv.put("bbb", "2") == status::OK);
	UT_ASSERT(kv.put("ccc", "3") == status::OK);
	UT_ASSERT(kv.put("rrr", "4") == status::OK);
	UT_ASSERT(kv.put("sss", "5") == status::OK);
	UT_ASSERT(kv.put("ttt", "6") == status::OK);
	UT_ASSERT(kv.put("yyy", "记!") == status::OK);
}
