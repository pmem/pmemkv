---
layout: manual
Content-Style: 'text/css'
title: _MP(PMEMKV, 7)
collection: libpmemkv
header: PMEMKV
date: pmemkv version 0.8
...

[comment]: <> (Copyright 2019, Intel Corporation)

[comment]: <> (Redistribution and use in source and binary forms, with or without)
[comment]: <> (modification, are permitted provided that the following conditions)
[comment]: <> (are met:)
[comment]: <> (    * Redistributions of source code must retain the above copyright)
[comment]: <> (      notice, this list of conditions and the following disclaimer.)
[comment]: <> (    * Redistributions in binary form must reproduce the above copyright)
[comment]: <> (      notice, this list of conditions and the following disclaimer in)
[comment]: <> (      the documentation and/or other materials provided with the)
[comment]: <> (      distribution.)
[comment]: <> (    * Neither the name of the copyright holder nor the names of its)
[comment]: <> (      contributors may be used to endorse or promote products derived)
[comment]: <> (      from this software without specific prior written permission.)

[comment]: <> (THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS)
[comment]: <> ("AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT)
[comment]: <> (LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR)
[comment]: <> (A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT)
[comment]: <> (OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,)
[comment]: <> (SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT)
[comment]: <> (LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,)
[comment]: <> (DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY)
[comment]: <> (THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT)
[comment]: <> ((INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE)
[comment]: <> (OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.)

[comment]: <> (libpmemkv.7 -- man page for libpmemkv)

[NAME](#name)<br />
[DESCRIPTION](#description)<br />
[ENGINES](#engines)<br />
[BINDINGS](#bindings)<br />
[SEE ALSO](#see-also)<br />


# NAME #

**pmemkv** - Key/Value Datastore for Persistent Memory

# DESCRIPTION #

**pmemkv** is a key-value datastore framework optimized for persistent memory. It provides native C API and C++ headers. Support for other languages is described in the BINDINGS section below.

It has mutiple storage engines, each optimized for different use case. They differ in implementation and capabilities:

+ persistence - this is a trade-off between data preservation and performance; persistent engines retain their content and are power fail/crash safe, but are slower; volatile engines are faster, but keep their content only until the database is closed (or application crashes; power fail occurs)

+ concurrency - non-concurrent engines are usually simpler, but can have better performance in single-threaded workloads; in contrary, concurrent engines are more complex, but have some support for multi-threaded access (levels of concurrency depends on engine)

+ keys' ordering - "sorted" engines support querying above/below the given key

Persistent engines usually use libpmemobj++ and PMDK to access NVDIMMs. They can work with files on DAX filesystem (fsdax) or DAX device.

# ENGINES #

| Engine Name  | Description | Persistent? | Concurrent? | Sorted? |
| ------------ | ----------- | ----------- | ----------- | ------- |
| **cmap** | **Concurrent hash map** | **Yes** | **Yes** | **No** |
| vcmap | Volatile concurrent hash map | No | Yes | No |
| vsmap | Volatile sorted hash map | No | No | Yes |
| blackhole | Accepts everything, returns nothing | No | Yes | No |


The design and implementation of **cmap** engine provides best preformance results and stability, hence it should be default choice of use.

Each engine can be manually turned on and off at build time, using CMake options. By default all non-experimental engines (listed here) are switched on and ready to use. Description, usage and list of experimental engines can be found at <https://github.com/pmem/pmemkv>.

## cmap

A persistent concurrent engine, backed by a hashmap.
Internally the engine uses persistent concurrent hash map and persistent string from libpmemobj-cpp library. Persistent string is used as a type of a key and a value.
TBB and libpmemobj-cpp packages are required.
Supports path, size and force_create configuration parameters.

## vcmap

A volatile concurrent engine, backed by memkind.
The engine is built on top of tbb::concurrent\_hash\_map data structure. The hash map uses PMEM C++ allocator to allocate memory. The std::basic\_string with PMEM C++ allocator is used as a type of a key and a value.
Memkind, TBB and libpmemobj-cpp packages are required.
Supports path and size configuration parameters.

## vsmap

A volatile single-threaded sorted engine, backed by memkind.
The engine is built on top of std::map. The map uses PMEM C++ allocator to allocate memory. The std::basic\_string with PMEM C++ allocator is used as a type of a key and a value.
Memkind and libpmemobj-cpp packages are required.
Supports path and size configuration parameters.


## blackhole

A volatile engine that accepts an unlimited amount of data, but never returns anything.
Internally, `blackhole` does not use a persistent pool or any durable structure. The intended use of this engine is to profile and tune high-level bindings, and similar cases when persistence
should be intentionally skipped.
No additional packages are required.
No supported configuration parameters.

# BINDINGS #

Bindings for other languages are available on GitHub. Currently they support only subset of native API.

Existing bindings:

+ Ruby - for details see <https://github.com/pmem/pmemkv-ruby>

+ JNI & Java - for details see <https://github.com/pmem/pmemkv-java>

+ Node.js - for details see <https://github.com/pmem/pmemkv-nodejs>

# SEE ALSO #

**libpmemkv**(3), **pmempool**(1), **libpmemobj**(7) and **<http://pmem.io>**
