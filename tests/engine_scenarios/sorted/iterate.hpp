// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#include "unittest.hpp"

#include <algorithm>
#include <vector>

/**
 * Helper class for testing get_* and count_* functions
 */

using namespace pmem::kv;
using kv_pair = std::pair<std::string, std::string>;
using kv_list = std::vector<kv_pair>;

const std::string EMPTY_KEY = "";
const std::string MIN_KEY = std::string(1, char(1));
const std::string MAX_KEY = std::string(4, char(255));

inline kv_list kv_sort(kv_list list)
{
	std::sort(list.begin(), list.end(), [](const kv_pair &lhs, const kv_pair &rhs) {
		return lhs.first < rhs.first;
	});

	return list;
}

#define KV_GET_ALL_CPP_CB_LST(list)                                                      \
	kv.get_all([&](string_view k, string_view v) {                                   \
		list.emplace_back(std::string(k.data(), k.size()),                       \
				  std::string(v.data(), v.size()));                      \
		return 0;                                                                \
	});

#define KV_GET_ALL_C_CB_LST(list)                                                        \
	kv.get_all(                                                                      \
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {      \
			const auto c = ((kv_list *)arg);                                 \
			c->emplace_back(std::string(k, kb), std::string(v, vb));         \
			return 0;                                                        \
		},                                                                       \
		&list)

#define KV_GET_1KEY_CPP_CB_LST(func, key, list)                                          \
	kv.func(key, [&](string_view k, string_view v) {                                 \
		list.emplace_back(std::string(k.data(), k.size()),                       \
				  std::string(v.data(), v.size()));                      \
		return 0;                                                                \
	})

#define KV_GET_1KEY_C_CB_LST(func, key, list)                                            \
	kv.func(                                                                         \
		key,                                                                     \
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {      \
			const auto c = ((kv_list *)arg);                                 \
			c->emplace_back(std::string(k, kb), std::string(v, vb));         \
			return 0;                                                        \
		},                                                                       \
		&list)

#define KV_GET_2KEYS_CPP_CB_LST(func, key1, key2, list)                                  \
	kv.func(key1, key2, [&](string_view k, string_view v) {                          \
		list.emplace_back(std::string(k.data(), k.size()),                       \
				  std::string(v.data(), v.size()));                      \
		return 0;                                                                \
	})

#define KV_GET_2KEYS_C_CB_LST(func, key1, key2, list)                                    \
	kv.func(                                                                         \
		key1, key2,                                                              \
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {      \
			const auto c = ((kv_list *)arg);                                 \
			c->emplace_back(std::string(k, kb), std::string(v, vb));         \
			return 0;                                                        \
		},                                                                       \
		&list)

inline void verify_get_all(pmem::kv::db &kv, const size_t exp_cnt,
			   const kv_list &sorted_exp_result)
{
	ASSERT_SIZE(kv, exp_cnt);

	kv_list result;
	auto s = KV_GET_ALL_CPP_CB_LST(result);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERT(result == sorted_exp_result);
}

inline void verify_get_all_c(pmem::kv::db &kv, const size_t exp_cnt,
			     const kv_list &sorted_exp_result)
{
	ASSERT_SIZE(kv, exp_cnt);

	kv_list result;
	auto s = KV_GET_ALL_C_CB_LST(result);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERT(result == sorted_exp_result);
}

inline void verify_get_above(pmem::kv::db &kv, const std::string key,
			     const size_t exp_cnt, const kv_list &sorted_exp_result)
{
	std::size_t cnt;
	kv_list result;
	ASSERT_STATUS(kv.count_above(key, cnt), status::OK);
	UT_ASSERTeq(cnt, exp_cnt);
	auto s = KV_GET_1KEY_CPP_CB_LST(get_above, key, result);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERT(result == sorted_exp_result);
}

inline void verify_get_above_c(pmem::kv::db &kv, const std::string key,
			       const size_t exp_cnt, const kv_list &sorted_exp_result)
{
	std::size_t cnt;
	kv_list result;
	ASSERT_STATUS(kv.count_above(key, cnt), status::OK);
	UT_ASSERTeq(cnt, exp_cnt);
	auto s = KV_GET_1KEY_C_CB_LST(get_above, key, result);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERT(result == sorted_exp_result);
}

inline void verify_get_equal_above(pmem::kv::db &kv, const std::string key,
				   const size_t exp_cnt, const kv_list &sorted_exp_result)
{
	std::size_t cnt;
	kv_list result;
	ASSERT_STATUS(kv.count_equal_above(key, cnt), status::OK);
	UT_ASSERTeq(cnt, exp_cnt);
	auto s = KV_GET_1KEY_CPP_CB_LST(get_equal_above, key, result);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERT(result == sorted_exp_result);
}

inline void verify_get_equal_above_c(pmem::kv::db &kv, const std::string key,
				     const size_t exp_cnt,
				     const kv_list &sorted_exp_result)
{
	std::size_t cnt;
	kv_list result;
	ASSERT_STATUS(kv.count_equal_above(key, cnt), status::OK);
	UT_ASSERTeq(cnt, exp_cnt);
	auto s = KV_GET_1KEY_C_CB_LST(get_equal_above, key, result);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERT(result == sorted_exp_result);
}

inline void verify_get_below(pmem::kv::db &kv, const std::string key,
			     const size_t exp_cnt, const kv_list &sorted_exp_result)
{
	std::size_t cnt;
	kv_list result;
	ASSERT_STATUS(kv.count_below(key, cnt), status::OK);
	UT_ASSERTeq(cnt, exp_cnt);
	auto s = KV_GET_1KEY_CPP_CB_LST(get_below, key, result);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERT(result == sorted_exp_result);
}

inline void verify_get_below_c(pmem::kv::db &kv, const std::string key,
			       const size_t exp_cnt, const kv_list &sorted_exp_result)
{
	std::size_t cnt;
	kv_list result;
	ASSERT_STATUS(kv.count_below(key, cnt), status::OK);
	UT_ASSERTeq(cnt, exp_cnt);
	auto s = KV_GET_1KEY_C_CB_LST(get_below, key, result);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERT(result == sorted_exp_result);
}

inline void verify_get_equal_below(pmem::kv::db &kv, const std::string key,
				   const size_t exp_cnt, const kv_list &sorted_exp_result)
{
	std::size_t cnt;
	kv_list result;
	ASSERT_STATUS(kv.count_equal_below(key, cnt), status::OK);
	UT_ASSERTeq(cnt, exp_cnt);
	auto s = KV_GET_1KEY_CPP_CB_LST(get_equal_below, key, result);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERT(result == sorted_exp_result);
}

inline void verify_get_equal_below_c(pmem::kv::db &kv, const std::string key,
				     const size_t exp_cnt,
				     const kv_list &sorted_exp_result)
{
	std::size_t cnt;
	kv_list result;
	ASSERT_STATUS(kv.count_equal_below(key, cnt), status::OK);
	UT_ASSERTeq(cnt, exp_cnt);
	auto s = KV_GET_1KEY_C_CB_LST(get_equal_below, key, result);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERT(result == sorted_exp_result);
}

inline void verify_get_between(pmem::kv::db &kv, const std::string key1,
			       const std::string key2, const size_t exp_cnt,
			       const kv_list &sorted_exp_result)
{
	std::size_t cnt;
	kv_list result;
	ASSERT_STATUS(kv.count_between(key1, key2, cnt), status::OK);
	UT_ASSERTeq(cnt, exp_cnt);
	auto s = KV_GET_2KEYS_CPP_CB_LST(get_between, key1, key2, result);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERT(result == sorted_exp_result);
}

inline void verify_get_between_c(pmem::kv::db &kv, const std::string key1,
				 const std::string key2, const size_t exp_cnt,
				 const kv_list &sorted_exp_result)
{
	std::size_t cnt;
	kv_list result;
	ASSERT_STATUS(kv.count_between(key1, key2, cnt), status::OK);
	UT_ASSERTeq(cnt, exp_cnt);
	auto s = KV_GET_2KEYS_C_CB_LST(get_between, key1, key2, result);
	ASSERT_STATUS(s, status::OK);
	UT_ASSERT(result == sorted_exp_result);
}

const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		       "abcdefghijklmnopqrstuvwxyz";
const size_t charset_size = strlen(charset);

inline void add_basic_keys(pmem::kv::db &kv)
{
	ASSERT_STATUS(kv.put("A", "1"), status::OK);
	ASSERT_STATUS(kv.put("AB", "2"), status::OK);
	ASSERT_STATUS(kv.put("AC", "3"), status::OK);
	ASSERT_STATUS(kv.put("B", "4"), status::OK);
	ASSERT_STATUS(kv.put("BB", "5"), status::OK);
	ASSERT_STATUS(kv.put("BC", "6"), status::OK);
}

inline void add_ext_keys(pmem::kv::db &kv)
{
	ASSERT_STATUS(kv.put("aaa", "1"), status::OK);
	ASSERT_STATUS(kv.put("bbb", "2"), status::OK);
	ASSERT_STATUS(kv.put("ccc", "3"), status::OK);
	ASSERT_STATUS(kv.put("rrr", "4"), status::OK);
	ASSERT_STATUS(kv.put("sss", "5"), status::OK);
	ASSERT_STATUS(kv.put("ttt", "6"), status::OK);
	ASSERT_STATUS(kv.put("yyy", "记!"), status::OK);
}

/*
 * Generates incremental keys using each char from global variable charset.
 * List looks e.g. like: "A", "AA", ..., "B", "BB", ...
 */
inline std::vector<std::string> gen_incr_keys(const size_t max_key_len)
{
	std::vector<std::string> keys;

	for (size_t i = 0; i < charset_size; i++) {
		std::string key = "";
		char curr_char = charset[i];
		for (size_t j = 0; j < max_key_len; j++) {
			key += curr_char;
			keys.push_back(key);
		}
	}

	return keys;
}

/* generates 'cnt' unique keys with random content, using global variable charset */
inline std::vector<std::string> gen_rand_keys(const size_t cnt, const size_t max_key_len)
{
	std::vector<std::string> keys;
	std::string gen_key;

	for (size_t k = 0; k < cnt; k++) {
		do {
			/* various lengths of key, min: 2 */
			size_t key_len = 2 + ((size_t)rand() % (max_key_len - 1));
			gen_key.clear();
			gen_key.reserve(key_len);
			for (size_t i = 0; i < key_len; i++) {
				gen_key.push_back(charset[(size_t)rand() % charset_size]);
			}
		} while (std::find(keys.begin(), keys.end(), gen_key) != keys.end());
		keys.push_back(std::move(gen_key));
	}

	return keys;
}

/* Custom comparator */
class reverse_comparator {
public:
	reverse_comparator() : runtime_data(new int(RUNTIME_DATA_VAL))
	{
	}

	reverse_comparator(reverse_comparator &&cmp)
	{
		this->runtime_data = cmp.runtime_data;
		cmp.runtime_data = nullptr;
	}

	reverse_comparator(const reverse_comparator &cmp) = delete;

	reverse_comparator &operator=(const reverse_comparator &cmp) = delete;

	~reverse_comparator()
	{
		delete runtime_data;
	}

	int compare(string_view key1, string_view key2) const
	{
		UT_ASSERT(*runtime_data == RUNTIME_DATA_VAL);

		return key2.compare(key1);
	}

	std::string name()
	{
		return "reverse_comparator";
	}

private:
	static constexpr int RUNTIME_DATA_VAL = 0xABC;
	int *runtime_data;
};
