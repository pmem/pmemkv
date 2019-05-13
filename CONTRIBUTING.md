# Contributing to pmemkv

<ul>
<li><a href="#issues">Opening New Issues</a></li>
<li><a href="#style">Code Style</a></li>
<li><a href="#pull_requests">Submitting Pull Requests</a></li>
<li><a href="#engines">Creating New Engines</a></li>
<li><a href="#experimental">Creating Experimental Engines</a></li>
</ul>

<a name="issues"></a>

Opening New Issues
------------------

Please log bugs or suggestions as [GitHub issues](https://github.com/pmem/pmemkv/issues).
Details such as OS and PMDK version are always appreciated.

<a name="style"></a>

Code Style
----------

* Start with [Google C++ Style](https://google.github.io/styleguide/cppguide.html)
* Indent 4 spaces, 8 spaces for continuation
* Max 120 chars per line
* Space after '*' and '&' (rather than before)

<a name="pull_requests"></a>

Submitting Pull Requests
------------------------

We take outside code contributions to `PMEMKV` through GitHub pull requests.

**NOTE: If you do decide to implement code changes and contribute them,
please make sure you agree your contribution can be made available under the
[BSD-style License used for PMEMKV](https://github.com/pmem/pmemkv/blob/master/LICENSE).**

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

<a name="engines"></a>

Creating New Engines
--------------------

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
* Use `pmemkv::mytree` namespace to define internal types
* Define `ENGINE` constant in namespace to export engine name
* Use camel-case for implementation class name (`class MyTree`)

### Creating Engine Implementation

* Create `src/engines/mytree.cc` implementation file
* For new engines, use `blackhole.cc` as a template
* Use `pmemkv::mytree` namespace defined by the header

### Providing Unit Test

* Create `tests/engines/mytree_test.cc` for unit tests
* For new engines, use `blackhole_test.cc` as a template
* For stable engines, just copy existing tests (eventually replace the original)

### Updating Build System

* In `CMakeLists.txt`:
    * Add `src/engines/mytree.h` and `src/engines/mytree.cc` to `SOURCE_FILES`
    * Add `tests/engines/mytree_test.cc` to `pmemkv_test` executable
    * Use `pkg_check_modules` and/or `find_package` for upstream libraries
* `make` should now run to completion without errors

### Updating KVEngine

* In `src/pmemkv.cc`:
    * Add `#include "engines/mytree.h"`
    * Update `KVEngine::Start` to return new `MyTree` instances
    * Update `KVEngine::Stop` to delete `MyTree` instances
* `make` & verify engine now works with high-level bindings

### Documentation

* In `ENGINES.md`, add `mytree` section
* In `README.md`, link `mytree` in the table of supported engines

<a name="experimental"></a>

Creating Experimental Engines
-----------------------------

The instructions above describe creating an engine that is included in the `pmemkv` build by default.
There is also the option of creating experimental engines that are not ready to include by default.
These include engines that are still being actively developed, optimized, reviewed or documented.

Experimental engines are unsupported prototypes, but are still expected to have proper automated tests,
and for all defined tests to pass.

### Experimental Code

Use the `EXPERIMENTAL` conditional define to exclude code from the default build.

```
#ifdef EXPERIMENTAL
...
#endif
```

### Experimental Build Assets

Extend this existing section in `CMakeLists.txt` if your experimental engine requires libraries that
are not yet included in the default build.

```
if(EXPERIMENTAL)
    include(foo-experimental)
endif(EXPERIMENTAL)
```

As noted in the example above, the experimental CMake module should use `-experimental` in the file name.
