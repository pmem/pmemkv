// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * result.cpp -- tests result class, its members and related exception.
 */

#include <libpmemobj++/slice.hpp>

#include "../common/unittest.hpp"
#include "libpmemkv.hpp"

using slice = pmem::obj::slice<const char *>;
template <typename T>
using result = pmem::kv::result<T>;

/* number of possible statuses */
const size_t number_of_statuses = 13;

class moveable {
public:
	moveable(int val) : move(false), x(val)
	{
	}

	moveable(const moveable &other) = default;

	moveable(moveable &&other)
	{
		x = other.x;
		other.x = -1;
		move = false;
		other.move = true;
	}

	moveable &operator=(moveable &&other)
	{
		x = other.x;
		other.x = -1;
		other.move = true;

		return *this;
	}

	bool moved()
	{
		return move && x == -1;
	}

	int get()
	{
		return x;
	}

private:
	bool move;
	int x;
};

static void basic_test()
{
	const std::string str = "abcdefgh";

	/* result with correct value */
	result<slice> res1(slice(&str[0], &str[str.size()]));
	UT_ASSERT(res1.is_ok());
	ASSERT_STATUS(res1.get_status(), pmem::kv::status::OK);
	UT_ASSERT(res1 == pmem::kv::status::OK);
	UT_ASSERT(pmem::kv::status::OK == res1);
	UT_ASSERT(res1 != pmem::kv::status::NOT_FOUND);
	UT_ASSERT(pmem::kv::status::NOT_FOUND != res1);
	UT_ASSERTeq(str.compare(res1.get_value().begin()), 0);

	/* result without value */
	for (size_t i = 1; i < number_of_statuses; ++i) {
		result<slice> res2(static_cast<pmem::kv::status>(i));
		UT_ASSERT(!res2.is_ok());
		UT_ASSERTeq(res2.get_status(), static_cast<pmem::kv::status>(i));
		UT_ASSERT(res2 == static_cast<pmem::kv::status>(i));
		UT_ASSERT(static_cast<pmem::kv::status>(i) == res2);
		UT_ASSERT(res2 != pmem::kv::status::OK);
		UT_ASSERT(pmem::kv::status::OK != res2);

		bool exception_thrown = false;
		try {
			res2.get_value();
		} catch (pmem::kv::bad_result_access &e) {
			UT_ASSERTeq(std::string(e.what()).compare(
					    "bad_result_access (value doesn't exist)"),
				    0);
			exception_thrown = true;
		}
		UT_ASSERT(exception_thrown);
	}

	/* test copy ctor */
	result<slice> result1(res1);
	UT_ASSERT(result1.is_ok());
	ASSERT_STATUS(result1.get_status(), pmem::kv::status::OK);
	UT_ASSERTeq(str.compare(result1.get_value().begin()), 0);

	/* test copy assignment */
	result<slice> result2(pmem::kv::status::NOT_FOUND);
	result2 = res1;
	UT_ASSERT(result2.is_ok());
	ASSERT_STATUS(result2.get_status(), pmem::kv::status::OK);
	UT_ASSERTeq(str.compare(result2.get_value().begin()), 0);

	/* test move ctor */
	result<moveable> to_move(moveable(10));
	auto &ref = to_move.get_value();

	result<moveable> move_result(std::move(to_move));

	UT_ASSERT(move_result.is_ok());
	ASSERT_STATUS(move_result.get_status(), pmem::kv::status::OK);
	UT_ASSERTeq(move_result.get_value().get(), 10);
	ASSERT_STATUS(to_move.get_status(), pmem::kv::status::UNKNOWN_ERROR);
	UT_ASSERT(ref.moved());

	/* test move assignment*/
	result<moveable> to_move2(moveable(10));
	auto &ref2 = to_move2.get_value();

	result<moveable> move_result2(pmem::kv::status::NOT_FOUND);
	move_result2 = std::move(to_move2);

	UT_ASSERT(move_result2.is_ok());
	ASSERT_STATUS(move_result2.get_status(), pmem::kv::status::OK);
	UT_ASSERTeq(move_result2.get_value().get(), 10);
	ASSERT_STATUS(to_move2.get_status(), pmem::kv::status::UNKNOWN_ERROR);
	UT_ASSERT(ref2.moved());

	/* test ctor with rvalue reference to OkType */
	moveable to_move3(10);

	result<moveable> move_result3(std::move(to_move3));
	UT_ASSERT(move_result3.is_ok());
	ASSERT_STATUS(move_result3.get_status(), pmem::kv::status::OK);
	UT_ASSERTeq(move_result3.get_value().get(), 10);
	UT_ASSERT(to_move3.moved());
}

/* counter of constructor/destructor calls */
class cd_counter {
public:
	static size_t des_cnt, copy_cnt, move_cnt;

	cd_counter()
	{
	}

	cd_counter(const cd_counter &other)
	{
		++cd_counter::copy_cnt;
	}

	cd_counter(cd_counter &&other)
	{
		++cd_counter::move_cnt;
	}

	~cd_counter()
	{
		++cd_counter::des_cnt;
	}

	cd_counter &operator=(const cd_counter &other)
	{
		++cd_counter::copy_cnt;
		return *this;
	}

	cd_counter &operator=(cd_counter &&other)
	{
		++cd_counter::move_cnt;
		return *this;
	}
};

size_t cd_counter::des_cnt;
size_t cd_counter::copy_cnt;
size_t cd_counter::move_cnt;

/* test if OkType's constructors/destructors are properly called */
static void constructor_destructor_test()
{
	cd_counter c;
	{
		result<cd_counter> r(c);
		UT_ASSERTeq(cd_counter::copy_cnt, 1);
	}
	/* test if value inside of the result was destroyed */
	UT_ASSERTeq(cd_counter::des_cnt, 1);

	{
		result<cd_counter> r(c);
		UT_ASSERTeq(cd_counter::copy_cnt, 2);

		result<cd_counter> to_copy(pmem::kv::status::NOT_FOUND);
		/* if you copy an empty result to the ok result, value's destructor should
		 * be called */
		r = to_copy;
		UT_ASSERTeq(cd_counter::des_cnt, 2);
	}
	UT_ASSERTeq(cd_counter::des_cnt, 2);

	{
		/* check copy ctor/assignment */
		result<cd_counter> r1(c);
		result<cd_counter> r2(c);
		UT_ASSERTeq(cd_counter::copy_cnt, 4);
		r1 = r2;
		UT_ASSERTeq(cd_counter::copy_cnt, 5);

		/* if there is only status, value's copy ctor shouldn't be called */
		result<cd_counter> r3(pmem::kv::status::NOT_FOUND);
		result<cd_counter> r4(r3);
		UT_ASSERTeq(cd_counter::copy_cnt, 5);

		/* if there is only status, value's copy assignment operator shouldn't be
		 * called */
		r3 = r4;
		UT_ASSERTeq(cd_counter::copy_cnt, 5);

		/* check move ctor/assignment */
		result<cd_counter> r5(std::move(r1));
		UT_ASSERTeq(cd_counter::move_cnt, 1);

		result<cd_counter> r6(pmem::kv::status::NOT_FOUND);
		r6 = std::move(r2);
		UT_ASSERTeq(cd_counter::move_cnt, 2);

		/* if there is only status, value's move ctor shouldn't be called */
		result<cd_counter> r7(pmem::kv::status::NOT_FOUND);
		result<cd_counter> r8(std::move(r7));
		UT_ASSERTeq(cd_counter::move_cnt, 2);

		/* if there is only status, value's move assignment operator shouldn't be
		 * called */
		r7 = std::move(r8);
		UT_ASSERTeq(cd_counter::move_cnt, 2);

		/* move value to constructor */
		result<cd_counter> r9(std::move(c));
		UT_ASSERTeq(cd_counter::move_cnt, 3);

		/* if there is a value in each result, move assignment operator should be
		 * called */
		result<cd_counter> r10(c);
		r10 = std::move(r9);
		UT_ASSERTeq(cd_counter::move_cnt, 4);

		/* if you move an empty result to the ok result, value's destructor should
		 * be called */
		result<cd_counter> r11(pmem::kv::status::NOT_FOUND);
		r10 = std::move(r11);
		/* previous value in r10 should be destroyed */
		UT_ASSERTeq(cd_counter::des_cnt, 3);
	}
	/* check if values in r5, r6 were destroyed */
	UT_ASSERTeq(cd_counter::des_cnt, 5);
}

static void test(int argc, char *argv[])
{
	basic_test();
	constructor_destructor_test();
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
