# Content

This directory contains tests for pmemkv. They are grouped into directories:

- **c_api** - most of pmemkv's tests use C++ API, these tests use only C API
- **compatibility** - compatibility checks between different pmemkv versions
- **config** - tests for all functions in config's API
- **engine_scenarios** - test scenarios parameterized with at least engine name;
	they are grouped into sub-sections related to specific group of engines' capabilities
- **engines** - tests and scripts related to pmemkv's engines
- **engines-experimental** - tests and scripts related to experimental engines
- and additional test(s) in main directory

# Tests execution

Before executing tests it's required to build pmemkv's sources and tests.
See [INSTALLING.md](../INSTALLING.md) for details. There are CMake's options related
to tests - see all options in top-level CMakeLists.txt with prefix `TESTS_`.
One of them - `TESTS_USE_FORCED_PMEM=ON` speeds up tests execution on emulated pmem.
It's useful for long (e.g. concurrent) tests.

To run all tests execute:

```sh
make test
```

or, if you want to see an output/logs of failed tests, run:

```sh
ctest --output-on-failure
```

There are other parameters to use in `ctest` command.
To see the full list read [ctest(1) manpage](https://cmake.org/cmake/help/latest/manual/ctest.1.html).
