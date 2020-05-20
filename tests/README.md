# Content

This directory contains tests for pmemkv. They are grouped into directories:

- **c_api** - most of pmemkv's tests use C++ API, these tests use only C API
- **compatibility** - compatibility checks between different pmemkv versions
- **config** - tests for all functions in config's API
- **engine_scenarios** - test scenarios parametrized with at least engine name;
	they are grouped into sub-sections related to specific group of engines' capabilities
- **engines** - tests and scripts related to pmemkv's engines
- **engines-experimental** - tests and scripts related to experimental engines
- and additional test(s) in main directory

# Tests execution

Before executing tests it's required to build pmemkv's sources and tests.
See [INSTALLING.md](../INSTALLING.md) for details. It's worth mention, there are
CMake's options related to tests - see all options in top-level CMakeLists.txt
with prefix `TESTS_`. When working on emulated pmem it's useful to set
`TESTS_USE_FORCED_PMEM=ON` - it speeds up executing of concurrent tests.

To run all tests execute:

```sh
make test
```

To see output on failed tests, you can instead run:

```sh
ctest --output-on-failure
```

There are also many other parameters to use in `ctest` command.
To see the full list see [ctest(1) manpage](https://cmake.org/cmake/help/latest/manual/ctest.1.html).
