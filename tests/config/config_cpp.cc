// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include <libpmemkv.hpp>

#include "unittest.hpp"

#include <limits>

using namespace pmem::kv;

struct custom_type {
	int a;
	char b;
};

static void deleter(custom_type *ct_ptr)
{
	ct_ptr->a = -1;
	ct_ptr->b = '0';
}

static void simple_test()
{
	auto cfg = new config;
	UT_ASSERT(cfg != nullptr);

	status s = cfg->put_string("string", "abc");
	UT_ASSERTeq(s, status::OK);

	s = cfg->put_int64("int", 123);
	UT_ASSERTeq(s, status::OK);

	custom_type *ptr = new custom_type;
	ptr->a = 10;
	ptr->b = 'a';
	s = cfg->put_object("object_ptr", ptr, nullptr);
	UT_ASSERTeq(s, status::OK);

	s = cfg->put_data("object", ptr);
	UT_ASSERTeq(s, status::OK);

	int array[3] = {1, 15, 77};
	s = cfg->put_data("array", array, 3);
	UT_ASSERTeq(s, status::OK);

	custom_type *ptr_deleter = new custom_type;
	ptr_deleter->a = 11;
	ptr_deleter->b = 'b';
	s = cfg->put_object("object_ptr_with_deleter", ptr_deleter,
			    (void (*)(void *)) & deleter);
	UT_ASSERTeq(s, status::OK);

	std::string value_string;
	s = cfg->get_string("string", value_string);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERT(value_string == "abc");

	int64_t value_int;
	s = cfg->get_int64("int", value_int);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERTeq(value_int, 123);

	custom_type *value_custom_ptr;
	s = cfg->get_object("object_ptr", value_custom_ptr);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERTeq(value_custom_ptr->a, 10);
	UT_ASSERTeq(value_custom_ptr->b, 'a');

	custom_type *value_custom_ptr_deleter;
	s = cfg->get_object("object_ptr_with_deleter", value_custom_ptr_deleter);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERTeq(value_custom_ptr_deleter->a, 11);
	UT_ASSERTeq(value_custom_ptr_deleter->b, 'b');

	custom_type *value_custom;
	size_t value_custom_count;
	s = cfg->get_data("object", value_custom, value_custom_count);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERTeq(value_custom_count, 1U);
	UT_ASSERTeq(value_custom->a, 10);
	UT_ASSERTeq(value_custom->b, 'a');

	int *value_array;
	size_t value_array_count;
	s = cfg->get_data("array", value_array, value_array_count);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERTeq(value_array_count, 3U);
	UT_ASSERTeq(value_array[0], 1);
	UT_ASSERTeq(value_array[1], 15);
	UT_ASSERTeq(value_array[2], 77);

	int64_t none;
	UT_ASSERTeq(cfg->get_int64("non-existent", none), status::NOT_FOUND);

	delete ptr;

	delete cfg;
	cfg = nullptr;

	UT_ASSERTeq(value_custom_ptr_deleter->a, -1);
	UT_ASSERTeq(value_custom_ptr_deleter->b, '0');

	delete ptr_deleter;
}

static void object_unique_ptr_default_deleter_test()
{
	auto cfg = new config;
	UT_ASSERT(cfg != nullptr);

	auto ptr_default = std::unique_ptr<custom_type>(new custom_type);
	ptr_default->a = 10;
	ptr_default->b = 'a';
	auto s = cfg->put_object("object_ptr", std::move(ptr_default));
	UT_ASSERTeq(s, status::OK);

	delete cfg;
}

static void object_unique_ptr_nullptr_test()
{
	auto cfg = new config;
	UT_ASSERT(cfg != nullptr);

	auto ptr = std::unique_ptr<custom_type>(nullptr);
	auto s = cfg->put_object("object_ptr", std::move(ptr));
	UT_ASSERTeq(s, status::OK);

	custom_type *raw_ptr;
	s = cfg->get_object("object_ptr", raw_ptr);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERTeq(raw_ptr, nullptr);

	delete cfg;
}

static void object_unique_ptr_custom_deleter_test()
{
	auto cfg = new config;
	UT_ASSERT(cfg != nullptr);

	static const int TEST_VAL = 0xABC;

	auto custom_deleter = [&](custom_type *ptr) {
		ptr->a = TEST_VAL;
		ptr->b = '0';
	};

	auto ptr_custom = std::unique_ptr<custom_type, decltype(custom_deleter)>(
		new custom_type, custom_deleter);
	ptr_custom->a = 10;
	ptr_custom->b = 'a';

	auto *raw_ptr = ptr_custom.get();

	auto s = cfg->put_object("object_ptr", std::move(ptr_custom));
	UT_ASSERTeq(s, status::OK);

	delete cfg;

	UT_ASSERTeq(raw_ptr->a, TEST_VAL);
	UT_ASSERTeq(raw_ptr->b, '0');
	delete raw_ptr;
}

static void integral_conversion_test()
{
	auto cfg = new config;
	UT_ASSERT(cfg != nullptr);

	status s = cfg->put_int64("int", 123);
	UT_ASSERTeq(s, status::OK);

	s = cfg->put_uint64("uint", 123);
	UT_ASSERTeq(s, status::OK);

	s = cfg->put_int64("negative-int", -123);
	UT_ASSERTeq(s, status::OK);

	s = cfg->put_uint64("uint-max", std::numeric_limits<size_t>::max());
	UT_ASSERTeq(s, status::OK);

	int64_t int_s;
	s = cfg->get_int64("int", int_s);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERTeq(int_s, 123);

	size_t int_us;
	s = cfg->get_uint64("int", int_us);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERTeq(int_us, 123U);

	int64_t uint_s;
	s = cfg->get_int64("uint", uint_s);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERTeq(uint_s, 123);

	size_t uint_us;
	s = cfg->get_uint64("uint", uint_us);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERTeq(uint_us, 123U);

	int64_t neg_int_s;
	s = cfg->get_int64("negative-int", neg_int_s);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERTeq(neg_int_s, -123);

	size_t neg_int_us;
	s = cfg->get_uint64("negative-int", neg_int_us);
	UT_ASSERTeq(s, status::CONFIG_TYPE_ERROR);

	int64_t uint_max_s;
	s = cfg->get_int64("uint-max", uint_max_s);
	UT_ASSERTeq(s, status::CONFIG_TYPE_ERROR);

	size_t uint_max_us;
	s = cfg->get_uint64("uint-max", uint_max_us);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERTeq(uint_max_us, std::numeric_limits<size_t>::max());

	delete cfg;
}

static void constructors_test()
{
	auto cfg = new config;
	UT_ASSERT(cfg != nullptr);

	/* assign released C++ config to C config;
	 * it's null because config is lazy initialized */
	pmemkv_config *c_cfg = cfg->release();
	UT_ASSERTeq(c_cfg, nullptr);

	/* put value to C++ config */
	auto s = cfg->put_int64("int", 65535);
	UT_ASSERTeq(s, status::OK);

	/* use move constructor and test if data is still accessible */
	config *move_config = new config(std::move(*cfg));
	int64_t int_s;
	s = move_config->get_int64("int", int_s);
	UT_ASSERTeq(s, status::OK);
	UT_ASSERTeq(int_s, 65535);

	/* release new C++ config and test if data is accessible in C config */
	c_cfg = move_config->release();
	auto ret = pmemkv_config_get_int64(c_cfg, "int", &int_s);
	UT_ASSERTeq(ret, PMEMKV_STATUS_OK);
	UT_ASSERTeq(int_s, 65535);

	/* check if moved config is empty */
	s = move_config->get_int64("int", int_s);
	UT_ASSERTeq(s, status::NOT_FOUND);

	/* cleanup */
	pmemkv_config_delete(c_cfg);
	delete move_config;

	delete cfg;
}

static void not_found_test()
{
	auto cfg = new config;
	UT_ASSERT(cfg != nullptr);

	/* config is nullptr; all gets should return NotFound */
	std::string my_string;
	int64_t my_int;
	uint64_t my_uint;
	custom_type *my_object;
	size_t my_object_count = 0;

	UT_ASSERTeq(cfg->get_string("string", my_string), status::NOT_FOUND);
	UT_ASSERTeq(cfg->get_int64("int", my_int), status::NOT_FOUND);
	UT_ASSERTeq(cfg->get_uint64("uint", my_uint), status::NOT_FOUND);
	UT_ASSERTeq(cfg->get_object("object", my_object), status::NOT_FOUND);
	UT_ASSERTeq(cfg->get_data("data", my_object, my_object_count), status::NOT_FOUND);
	UT_ASSERTeq(my_object_count, 0U);

	/* initialize config with any put */
	cfg->put_int64("init", 0);

	/* all gets should return NotFound when looking for non-existing key */
	UT_ASSERTeq(cfg->get_string("non-existent-string", my_string), status::NOT_FOUND);
	UT_ASSERTeq(cfg->get_int64("non-existent-int", my_int), status::NOT_FOUND);
	UT_ASSERTeq(cfg->get_uint64("non-existent-uint", my_uint), status::NOT_FOUND);
	UT_ASSERTeq(cfg->get_object("non-existent-object_ptr", my_object),
		    status::NOT_FOUND);
	UT_ASSERTeq(cfg->get_data("non-existent-data", my_object, my_object_count),
		    status::NOT_FOUND);
	UT_ASSERTeq(my_object_count, 0U);

	delete cfg;
}

static void test(int argc, char *argv[])
{
	simple_test();
	object_unique_ptr_nullptr_test();
	object_unique_ptr_default_deleter_test();
	object_unique_ptr_custom_deleter_test();
	integral_conversion_test();
	not_found_test();
	constructors_test();
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
