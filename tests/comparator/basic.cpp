// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

using namespace pmem::kv;

static const int RUNTIME_STATE = 10;

class valid_comparator : public pmem::kv::comparator
{
public:

    valid_comparator(int *runtime_state): runtime_state(runtime_state) {

    }

    ~valid_comparator() {
        delete runtime_state;
    }

    int compare(string_view k1, string_view k2) override {
        UT_ASSERTeq(*runtime_state, RUNTIME_STATE);

        return k1.compare(k2);
    }

    std::string name() override {
        return "valid_cmp";
    }

    int *runtime_state;
};

static void test_stateful_comparator(std::string name, pmem::kv::config &&cfg)
{
	auto s = cfg.put_comparator(std::unique_ptr<valid_comparator>(new valid_comparator(new int(RUNTIME_STATE))));
    UT_ASSERTeq(s, status::OK);

    pmem::kv::db kv;
    s = kv.open(name, std::move(cfg));
    UT_ASSERTeq(s, status::OK);

    s = kv.put("A", "A");
    UT_ASSERTeq(s, status::OK);
    s = kv.put("B", "B");
    UT_ASSERTeq(s, status::OK);
    s = kv.put("C", "C");
    UT_ASSERTeq(s, status::OK);

    std::vector<std::string> keys;

    s = kv.get_all([&](string_view k, string_view v){
        keys.push_back(std::string(k.data(), k.size()));
        return 0;
    });
    UT_ASSERTeq(s, status::OK);

    kv.close();
}

static void test(int argc, char *argv[])
{
	if (argc < 3)
		UT_FATAL("usage: %s engine json_config", argv[0]);

    test_stateful_comparator(argv[1], CONFIG_FROM_JSON(argv[2]));
}

int main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
