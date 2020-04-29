// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static constexpr int RUNTIME_DATA_VAL = 0xABC;

class reverse_comparator : public pmem::kv::comparator {
public:
	reverse_comparator(int val) : runtime_data(new int(val))
	{
	}

	~reverse_comparator()
	{
		delete runtime_data;
	}

	int compare(string_view key1, string_view key2) override
	{
		UT_ASSERT(*runtime_data == RUNTIME_DATA_VAL);

		return key2.compare(key1);
	}

	std::string name() override
	{
		return "reverse_comparator";
	}

public:
	int *runtime_data;
};

static void ReverseCompare(std::string engine, pmem::kv::config &&config)
{
	auto cmp = std::unique_ptr<reverse_comparator>(
		new reverse_comparator(RUNTIME_DATA_VAL));
	auto status = config.put_comparator(std::move(cmp));
	UT_ASSERTeq(status, status::OK);

	db kv;
	status = kv.open(engine, std::move(config));
	UT_ASSERTeq(status, status::OK);

	// XXX
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

	auto engine = argv[1];

	ReverseCompare(engine, CONFIG_FROM_JSON(argv[2]));
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
