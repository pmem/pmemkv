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

**libpmemkv** is a library exposing API for key-value datastore operations. It's a native C library, but it provides both C and C++ headers. Support for other languages is described in the section below.

This library provides engines with different sets of capabilities. Engines can be classified based on a number of attributes:

+ persistency - this is a trade-off between persistency and performance; persistent engines retain content and are power fail/crash safe, but are slower; volatile engines are faster, but keep their content only until the database is closed (or application crashes; power fail occurs)

+ concurrency - non-concurrent engines are usually simpler, but can have better performance in single-threaded workloads; in contrary, concurrent engines are more complex, but support multi-threaded workloads

+ keys' ordering - "sorted" engines support querying above/below the given key

+ stability - "experimental" engines are not validated yet and may crash or corrupt data

Persistent engines usually use libpmemobj++ and PMDK to access NVDIMMs. They can work with files on DAX filesystem (fsdax) or DAX device.

# ENGINES #

| Engine Name  | Description | Experimental? | Persistent? | Concurrent? | Sorted? |
| ------------ | ----------- | ------------- | ----------- | ----------- | ------- |
| blackhole | Accepts everything, returns nothing | No | No | Yes | No |
| cmap | Concurrent hash map | No | Yes | Yes | No |
| vcmap | Volatile concurrent hash map | No | No | Yes | No |
| vsmap | Volatile sorted hash map | No | No | No | Yes |
| tree3 | Persistent B+ tree | Yes | Yes | No | No |
| stree | Sorted persistent B+ tree | Yes | Yes | No | Yes |
| caching | Caching for remote Memcached or Redis server | Yes | - | No | - |

By default all non-experimental engines are switched on and ready to use. Each engine can be manually turned on and off in source files, using CMake options.

## blackhole

A volatile concurrent engine that accepts an unlimited amount of data, but never returns anything. It is always enabled.
Internally, `blackhole` does not use a persistent pool or any durable structure. The intended use of this engine is to profile and tune high-level bindings, and similar cases when persistence
should be intentionally skipped.

## cmap

A persistent concurrent engine, backed by a hashmap. It is enabled by default.
Internally the engine uses persistent concurrent hash map and persistent string from libpmemobj-cpp library. Persistent string is used as a type of a key and a value.

## vcmap

A volatile concurrent engine, backed by memkind. It is enabled by default.
The engine is built on top of tbb::concurrent\_hash\_map data structure. The hash map uses PMEM C++ allocator to allocate memory. The std::basic\_string with PMEM C++ allocator is used as a type of a key and a value.

## vsmap

A volatile single-threaded sorted engine, backed by memkind. It is enabled by default.
The engine is built on top of std::map. The map uses PMEM C++ allocator to allocate memory. The std::basic\_string with PMEM C++ allocator is used as a type of a key and a value.

## tree3

A persistent single-threaded engine, backed by a read-optimized B+ tree. It is disabled by default.
Internally, `tree3` uses a hybrid fingerprinted B+ tree implementation. Rather than keeping inner and leaf nodes of the tree in persistent memory, `tree3` uses a hybrid structure where inner nodes are kept in DRAM and leaf nodes only are kept in persistent memory.

## stree

A persistent, single-threaded and sorted engine, backed by a B+ tree. It is disabled by default.

## caching

This engine is using a sub engine from the list above to cache requests to external Redis or Memcached server. This engine is disabled by default.

# Bindings #

Bindings for other languages are available on GitHub. Currently they support only subset of native API.

Existing bindings:

+ Ruby - for details see <https://github.com/pmem/pmemkv-ruby>

+ JNI & Java - for details see <https://github.com/pmem/pmemkv-java>

+ Node.js - for details see <https://github.com/pmem/pmemkv-nodejs>

# SEE ALSO #

**libpmemkv**(3), **pmempool**(1), **libpmemobj**(7) and **<http://pmem.io>**
