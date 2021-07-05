# Contributing to pmemkv

- [Opening New Issues](#opening-new-issues)
- [Code Style](#code-style)
- [Submitting Pull Requests](#submitting-pull-requests)
- [Creating New Engines](#creating-new-engines)
- [Creating Experimental Engines](#creating-experimental-engines)
- [Extending Public API](#extending-public-api)
- [Configuring GitHub fork](#configuring-github-fork)

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

If you add a new feature, implement a new engine or fix a critical bug please append
appropriate entry to ChangeLog under newest release.

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

# Adding new dependency

Adding each new dependency (including new docker image and package) should be done in
a separate commit. The commit message should be:

```
New dependency: dependency_name

license: SPDX license tag
origin: https://dependency_origin.com
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
* For this example: `mytree`

### Creating Engine Header

* Create `src/engines/mytree.h` header file
* For new engines, use `blackhole.h` as a template
* Define engine class in `pmem::kv` namespace
* Use snake_case for implementation class name (`class my_tree`)

### Creating Engine Implementation

* Create `src/engines/mytree.cc` implementation file
* For new engines, use `blackhole.cc` as a template
* Use `pmem::kv` namespace defined by the header
* implement engine interface with its factory
* register engine factory (e.g. look at the blackhole)

### Providing Unit Test

* Select testcases (based on your engine's capabilities) you wish to use
 from `tests/engine_scenarios` (or, optionally, create your own)
* Write `.cmake` scripts for running selected testcases and put them in `tests/engines` directory.
 If engine is based on pmemobj or memkind you can just use scripts
 from `tests/engines/pmemobj_based` or `tests/engines/memkind_based`
* If extraordinary tests are required, consider adding them in `tests/engines`
* Update `tests/wrong_engine_name_test.cc` test with the new engine
* In `tests/CMakeLists.txt`:
	* Check if all new files are covered by patterns for cppstyle and whitespace checks
	* Add selected testcases to `tests/CMakeLists.txt` using `add_engine_test` function
	(see for comparison, current sections for testcases of existing engines)
	* If engine-specific tests were written, build and add them separately
	(e.g. using `build_test` and `add_test_generic` functions defined in `tests/ctest_helpers.cmake`)

### Updating Build System

* In `CMakeLists.txt`:
    * Add a build option for a new engine with a name like `ENGINE_MYTREE`
    and use it to ifdef all includes, dependencies and linking you may add
    * Add definition of the new option, like `-DENGINE_MYTREE`, so it can
    be used to ifdef engine-specific code, like:
    ```
    #ifdef ENGINE_MYTREE
    ...
    #endif
    ```
    * Add `src/engines/mytree.h` and `src/engines/mytree.cc` to `SOURCE_FILES`
    * Use `pkg_check_modules` and/or `find_package` for upstream libraries
* CMake build and `make test` should complete without any errors now

### Updating Common Source

* In script(s) executed in CIs (at least in `utils/docker/run-test-building.sh` and `utils/docker/images/install-bindings-dependencies.sh`) add a check/build for new engine
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

* In `doc/ENGINES-experimental.md`, add `mytree` section

# Extending Public API

When adding a new public function, you have to make sure to update:
- manpages `libpmemkv.3` or `libpmemkv_config.3` in `doc` dir
- `doc/CMakeLists.txt` with new manpage link
- map file with debug symbols (`src/libpmemkv.map`)
- source and header files (incl. libpmemkv.h, libpmemkv.cc, libpmemkv.hpp)
- engines' sources if needed (in `src/engines`)
- appropriate examples, to show usage
- tests (incl. compatibility, c_api and more...)
- ChangeLog with new entry for next release

# Configuring GitHub fork

To build and submit documentation as an automatically generated pull request,
the repository has to be properly configured.

* [Personal access token](https://docs.github.com/en/github/authenticating-to-github/creating-a-personal-access-token) for GitHub account has to be generated.
  * Such personal access token has to be set in pmemkv repository's
  [secrets](https://docs.github.com/en/actions/configuring-and-managing-workflows/creating-and-storing-encrypted-secrets)
  as `DOC_UPDATE_GITHUB_TOKEN` variable.

* `DOC_UPDATE_BOT_NAME` secret variable has to be set. In most cases it will be
  the same as GitHub account name.

* `DOC_REPO_OWNER` secret variable has to be set. Name of the GitHub account,
  which will be target to make an automatic pull request with documentation.
  In most cases it will be the same as GitHub account name.

To enable automatic images pushing to GitHub Container Registry, following variables:

* `CONTAINER_REG` existing environment variable (defined in workflow files, in .github/ directory)
  has to be updated to contain proper GitHub Container Registry address (to forking user's container registry),

* `GH_CR_USER` secret variable has to be set up - an account (with proper permissions) to publish
  images to the Container Registry (tab **Packages** in your GH profile/organization).

* `GH_CR_PAT` secret variable also has to be set up - Personal Access Token
  (with only read & write packages permissions), to be generated as described
  [here](https://docs.github.com/en/free-pro-team@latest/github/authenticating-to-github/creating-a-personal-access-token#creating-a-token)
  for selected account (user defined in above variable).
