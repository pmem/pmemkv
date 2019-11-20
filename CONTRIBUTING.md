# Contributing to pmemkv

- [Opening New Issues](#opening-new-issues)
- [Code Style](#code-style)
- [Submitting Pull Requests](#submitting-pull-requests)
- [Creating New Engines](#creating-new-engines)
- [Creating Experimental Engines](#creating-experimental-engines)

# Opening New Issues

Please log bugs or suggestions as [GitHub issues](https://github.com/pmem/pmemkv/issues).
Details such as OS and PMDK version are always appreciated.

# Code Style

* See `.clang-format` file in the repository for details
* Indent with tabs (width: 8)
* Max 90 chars per line
* Space before '*' and '&' (rather than after)

If you want to check and format your source code properly you can use CMake's `DEVELOPER_MODE`
and `CHECK_CPP_STYLE` options. When enabled additional checks are switched on
(cppstyle, whitespaces and headers).

```sh
cmake .. -DDEVELOPER_MODE=ON -DCHECK_CPP_STYLE=ON
```

If you just want to format your code you can make adequate target:
```sh
make cppformat
```

**NOTE**: We're using specific clang-format - version exactly **9.0** is required.

# Submitting Pull Requests

We take outside code contributions to `PMEMKV` through GitHub pull requests.

**NOTE: If you do decide to implement code changes and contribute them,
please make sure you agree your contribution can be made available under the
[BSD-style License used for PMEMKV](LICENSE).**

**NOTE: Submitting your changes also means that you certify the following:**

```
Developer's Certificate of Origin 1.1

By making a contribution to this project, I certify that:

(a) The contribution was created in whole or in part by me and I
    have the right to submit it under the open source license
    indicated in the file; or

(b) The contribution is based upon previous work that, to the best
    of my knowledge, is covered under an appropriate open source
    license and I have the right under that license to submit that
    work with modifications, whether created in whole or in part
    by me, under the same open source license (unless I am
    permitted to submit under a different license), as indicated
    in the file; or

(c) The contribution was provided directly to me by some other
    person who certified (a), (b) or (c) and I have not modified
    it.

(d) I understand and agree that this project and the contribution
    are public and that a record of the contribution (including all
    personal information I submit with it, including my sign-off) is
    maintained indefinitely and may be redistributed consistent with
    this project or the open source license(s) involved.
```

In case of any doubt, the gatekeeper may ask you to certify the above in writing,
i.e. via email or by including a `Signed-off-by:` line at the bottom
of your commit comments.

To improve tracking of who is the author of the contribution, we kindly ask you
to use your real name (not an alias) when committing your changes to PMEMKV:
```
Author: Random J Developer <random@developer.example.org>
```

# Creating New Engines

There are several motivations to create a `pmemkv` storage engine:

* Using a new/different implementation strategy
* Trying out a significant change to an existing engine
* Creating a new version of an existing engine with some tweaks

Next we'll walk you through the steps of creating a new engine.

### Picking Engine Name

* Relatively short (users will have to type this!)
* Formatted in all lower-case
* No whitespace or special characters
* Names should use common prefixes to denote capabilities:
  - prefix of "v" denotes volatile (persistent if omitted), appears first
  - prefix of "c" denotes concurrent (single-threaded if omitted), appears second
  - prefix of "s" denotes sorted (unsorted if omitted), appears last
* For this example: `mytree` (persistent, single-threaded, unsorted)

### Creating Engine Header

* Create `src/engines/mytree.h` header file
* For new engines, use `blackhole.h` as a template
* Define engine class in `pmem::kv` namespace
* Use snake_case for implementation class name (`class my_tree`)

### Creating Engine Implementation

* Create `src/engines/mytree.cc` implementation file
* For new engines, use `blackhole.cc` as a template
* Use `pmem::kv` namespace defined by the header

### Providing Unit Test

* Create `tests/engines/mytree_test.cc` for unit tests
* For new engines, use `blackhole_test.cc` as a template
* For stable engines, just copy existing tests (eventually replace the original)

### Updating Build System

* In `CMakeLists.txt`:
    * Add a build option for a new engine with a name like `ENGINE_MYTREE`
    and use it to ifdef all includes, dependencies and linking you may add
    * Add definition of the new option, like `-DENGINE_MYTREE`, so it can
    be used to ifdef engine-specific code (e.g. in `libpmemkv.cc`), like:
    ```
    #ifdef ENGINE_MYTREE
    ...
    #endif
    ```
    * Add `src/engines/mytree.h` and `src/engines/mytree.cc` to `SOURCE_FILES`
    * Use `pkg_check_modules` and/or `find_package` for upstream libraries
* In `tests/CMakeLists.txt`:
    * Add `engines/mytree_test.cc` to `TEST_FILES`
* CMake build and `make test` should complete without any errors now

### Updating Common Source

* In `src/engine.cc`:
    * Add `#include "engines/mytree.h"` (within `#ifdef ENGINE_MYTREE` clause)
    * Update `create_engine` to return new `my_tree` instances
* Build & verify engine now works with high-level bindings (see [README](README.md#language-bindings) for information on current bindings)

### Documentation

* In `README.md`, link `mytree` in the table of supported engines
* Update manpages in `doc` directory

# Creating Experimental Engines

The instructions above describe creating an engine that is considered stable. If you want,
you can mark an engine as experimental and not include it in a build by default.

Experimental engines are unsupported prototypes. These engines are still being actively
developed, optimized, reviewed or documented, but they are still expected to have proper
automated and passing tests.

### Experimental Code

There are subdirectories `engines-experimental` (in `src` and `tests` directories) where all
experimental source files should be placed.

Whole engine-specific code (located in common source files) should be ifdef'd out using
(newly) defined option (`ENGINE_MYTREE` in this case).

### Experimental Build Assets

Extend existing section in `CMakeLists.txt` if your experimental engine requires libraries that
are not yet included in the default build.

```cmake
if(ENGINE_MYTREE)
    include(foo-experimental)
endif()
```

As noted in the example above, the experimental CMake module should use `-experimental` suffix in the file name.

### Documentation

* In `ENGINES-experimental.md`, add `mytree` section
