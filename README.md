# pmemkv
Key/Value Datastore for Persistent Memory

*This is experimental pre-release software and should not be used in
production systems. File formats may change at any time without
preserving compatibility.*

Contents
--------

<ul>
<li><a href="#overview">Overview</a></li>
<li><a href="#project_structure">Project Structure</a></li>
<li><a href="#installation">Installation</a></li>
<li><a href="#configuring_clion_project">Configuring CLion Project</a></li>
<li><a href="#related_work">Related Work</a></li>
</ul>

<a name="overview"/>

Overview
--------

![pmemkv-intro](https://cloud.githubusercontent.com/assets/913363/22454311/21f15aa8-e742-11e6-9821-e3af513e279b.png)

<a name="project_structure"/>

Project Structure
-----------------

-	src/pmemkv.h (class header)
-	src/pmemkv.cc (class implementation)
-	src/pmemkv_example.cc (small example program)
-	src/pmemkv_stress.cc (stress test utility)
-	src/pmemkv_test.cc (unit tests using Google C++ Testing Framework)

<a name="installation"/>

Installation
------------

Start with Ubuntu 16.04 (either desktop or server distribution) or other
64-bit Linux distribution. OSX and Windows are not yet supported by pmemkv,
so don't use those.

To build pmemkv, you'll need make and g++ installed. No external libraries
are required to be installed prior to using our makefile.

```
make                # build everything from scratch and run tests
make thirdparty     # download and build dependencies
make clean          # remove build files
make example        # build and run example program
make stress         # build and run stress test
make test           # build and run unit tests
```

Configuring CLion Project
-------------------------

Obviously the use of an IDE is a personal preference. CLion is not
required for pmemkv development, but it's very easy to configure if
you have a valid license.

Use wizard to open project:

-	From Welcome screen, select "Open Project"
-	select root directory
-	do not "Overwrite CMakeLists.txt" if prompted

Set code style to match pmse:
* Start with [Google C++ Style](https://google.github.io/styleguide/cppguide.html)
* Indent 4 spaces, 8 spaces for continuation
* Max 100 chars per line
* Space after '*' and '&' (rather than before)

<a name="related_work"/>

Related Work
------------

**FPTree**

This research paper describes a hybrid dram/pmem tree design (similar
to pmemkv) but doesn't provide any code, and even in describing the
design omits certain important implementation details.

Beyond providing a clean-room implementation, the design of pmemkv differs
from FPTree in several important areas:

1. FPTree does not specify a hash method implementation, where pmemkv
uses a Pearson hash (RFC 3074).

2. Within its persistent leaves, FPTree uses an array of key hashes with
a separate visibility bitmap to track what hash slots are occupied.
pmemkv takes a different approach and uses only an array of key hashes
(no bitmaps). pmemkv relies on a specially modified Pearson hash function,
where a hash value of zero always indicates the slot is unused by
convention. This optimization eliminates the cost of using and maintaining
visibility bitmaps as well as cramming more hashes into a single
cache-line, and affects the implementation of every primitive operation
in the tree.

3. pmemkv additionally caches this array of key hashes in DRAM (in addition
to storing as part of the persistent leaf). This speeds leaf operations,
especially with slower media, for what seems like an acceptable rise in
DRAM usage.

4. pmemkv is written using NVML C++ bindings, which exerts influence on
its design and implementation. pmemkv uses generic NVML transactions
(ie. transaction::exec_tx() closures), there is no need for micro-logging
structures as described in the FPTree paper to make internal delete and
split operations safe. Preallocation of relatively large leaf objects
(an important facet of the FPTree design) is slower when using NVML then
allocating objects immediately when they are needed, so pmemkv
intentionally avoids this design pattern. pmemkv also adjusts sizes of
data structures (conforming specifically to NVML primitive types) for
best cache-line optimization.

**cpp_map**

Use of NVML C++ bindings by pmemkv was lifted from this example program.
Many thanks to @tomaszkapela for providing a great example to follow!
