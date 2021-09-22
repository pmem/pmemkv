# Examples

This directory contains C and C++ examples for pmemkv, the library
providing key-value datastore optimized for persistent memory.
For more information about library see libpmemkv(3).

## Building and execution

### Build with sources
To compile examples with current pmemkv sources, follow build steps for your OS
(as described in top-level README). Make sure the BUILD_EXAMPLES options is ON.
The build directory should now contain `examples` sub-directory with all binaries,
ready to run, e.g.:

```sh
cd <build_dir>/examples
./example-pmemkv_basic_c
```

If an example requires additional parameter it will print its usage,
otherwise it will run and print the execution results (if any).

### Standalone build
To compile any example as a standalone application (using pmemkv installed in the OS)
you have to enter selected example's sub-directory and run e.g.:

```sh
cd <repo_dir>/examples/pmemkv_basic_c
mkdir build
cd build
cmake ..
make
./pmemkv_basic_c
```

Similarly to previous section, if an example requires additional parameter
it will print its usage, otherwise it will run and print the execution results (if any).

## Descriptions and additional dependencies:

* pmemkv_basic_c/pmemkv_basic.c -- contains basic example workflow
		of C application

* pmemkv_basic_cpp/pmemkv_basic.cpp -- contains basic example workflow
		of C++ application

* pmemkv_comparator_c/pmemkv_comparator.c -- example of pmemkv used with
		custom comparator (C API)

* pmemkv_comparator_c/pmemkv_comparator.cpp -- example of pmemkv used with
		custom comparator (C++ API)

* pmemkv_config_c/
	* pmemkv_basic_config.c -- example usage for part of the pmemkv config API,
		which should be preferred

	* pmemkv_config.c -- example usage of the part of the pmemkv config API
		to set and get data based on their data types.

		It **requires** to be built:
		* 'rapidjson-devel' package to be installed in the OS and
		* 'BUILD_JSON_CONFIG' pmemkv's CMake variable to be set to ON

* pmemkv_fill_cpp/pmemkv_fill.cpp -- example which calculates how many elements fit
	into pmemkv. It inserts elements with specified key and value size
	to the database until OUT_OF_MEMORY status is returned. It then prints
	number of elements inserted. It may be used to observe the memory overhead
	of a certain engine with specific key/value sizes.

* pmemkv_iterator_c/pmemkv_iterator.c -- example of pmemkv's iterator (C API)

* pmemkv_iterator_cpp/pmemkv_iterator.cpp -- example of pmemkv's iterators (C++ API).
	It shows how to use it in single-threaded and concurrent approach.

	It **requires** to be built:
	* pthread available in the OS

* pmemkv_open_cpp/pmemkv_open_cpp -- contains example of pmemkv usage
		for already existing pools (and poolsets)

* pmemkv_pmemobj_cpp/pmemkv_pmemobj.cpp -- contains example
		of pmemkv supporting multiple engines

* pmemkv_transaction_c/pmemkv_transaction.c -- example with pmemkv transactions (C API)

* pmemkv_transaction_cpp/pmemkv_transaction.cpp -- example with pmemkv transactions (C++ API)
