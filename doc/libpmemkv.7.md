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
[SYNOPSIS](#synopsis)<br />
[DESCRIPTION](#description)<br />
[ENGINES](#engines)<br />
[EXAMPLE](#example)<br />
[SEE ALSO](#see-also)<br />


# NAME #

**pmemkv** - Key/Value Datastore for Persistent Memory

# SYNOPSIS #

## C

```c
#include <libpmemkv.h>

// most popular functions

int pmemkv_open(const char *engine, pmemkv_config *config, pmemkv_db **db);
void pmemkv_close(pmemkv_db *kv);
int pmemkv_put(pmemkv_db *db, const char *k, size_t kb, const char *v, size_t vb);
int pmemkv_get(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_v_callback *c, void *arg);
int pmemkv_remove(pmemkv_db *db, const char *k, size_t kb);
int pmemkv_exists(pmemkv_db *db, const char *k, size_t kb);

// error handling

const char *pmemkv_errormsg(void);
```

## C++

```cpp
#include <libpmemkv.hpp>

using namespace pmem::kv;

// most popular functions

status open(const std::string &engine_name, pmemkv_config *config) noexcept;
void close() noexcept;
status get(string_view key, std::string *value) noexcept;
status put(string_view key, string_view value) noexcept;
status remove(string_view key) noexcept;
status exists(string_view key) noexcept;

// error handling

static std::string errormsg();
```

For full API description see **libpmemkv**(3).

# DESCRIPTION #

**libpmemkv** is a library exposing API for key/value datastore operations.  It's a native C library, but it provides public C and C++ headers.  Bindings in other languages are still under development, but are already partially delivered on github.

It's delivered along with engines with various specifications.  Depends on used engine pmemkv is working on NVDIMMs (persistent engines) or in DRAM (volatile ones).  For detailed information on engines implementations and their features see section below.

To work on persistent memory PMDK and libpmemobj++ are used.  pmemkv can work on file on DAX filesystem (fsdax), a DAX device or on a PMDK poolset file.


# ENGINES #

| Engine Name  | Description | Experimental? | Persistent? | Concurrent? | Sorted? |
| ------------ | ----------- | ------------- | ----------- | ----------- | ------- |
| blackhole | Accepts everything, returns nothing | No | No | Yes | No |
| cmap | Concurrent hash map | No | Yes | Yes | No |
| vcmap | Volatile concurrent hash map | No | No | Yes | No |
| vsmap | Volatile sorted hash map | No | No | No | Yes |
| tree3 | Persistent B+ tree | Yes | Yes | No | No |
| stree | Sorted persistent B+ tree | Yes | Yes | No | Yes |
| caching | Caching for remote Memcached or Redis server | Yes | - | Yes | - |

By default all non-experimental engines are switched on and ready to use. Each engine can be manually turned on and off in source files, using CMake options.

## blackhole

A volatile concurrent engine that accepts an unlimited amount of data, but never returns anything.  It is always enabled (no CMake option is specified to enable/disable this engine).
Internally, `blackhole` does not use a persistent pool or any durable structures.  The intended use of this engine is to profile and tune high-level bindings, and similar cases when persistence
should be intentionally skipped.

## cmap

A persistent concurrent engine, backed by a hashmap.  It is enabled by default.  It can be disabled in CMake using the `ENGINE_CMAP` option.
Internally the engine uses persistent concurrent hash map and persistent string from libpmemobj-cpp library.  Persistent string is used as a type of a key and a value.

## vcmap

A volatile concurrent engine, backed by memkind.  It is enabled by default. It can be disabled in CMake using the `ENGINE_VCMAP` option.
The engine is built on top of tbb::concurrent_hash_map data structure.  The hash map uses PMEM C++ allocator to allocate memory.  The std::basic\_string with PMEM C++ allocator is used as a type of a key and a value.

## vsmap

A volatile single-threaded sorted engine, backed by memkind.  It is enabled by default. It can be disabled in CMake using the `ENGINE_VSMAP` option.
The engine is built on top of std::map.  The map uses PMEM C++ allocator to allocate memory.  The std::basic\_string with PMEM C++ allocator is used as a type of a key and a value.

## tree3

A persistent single-threaded engine, backed by a read-optimized B+ tree.  It is disabled by default.  It can be enabled in CMake using the `ENGINE_TREE3` option.
Internally, `tree3` uses a hybrid fingerprinted B+ tree implementation.  Rather than keeping inner and leaf nodes of the tree in persistent memory, `tree3` uses a hybrid structure where
inner nodes are kept in DRAM and leaf nodes only are kept in persistent memory.  Though `tree3` has to recover all inner nodes when the engine is started, searches are performed in
DRAM except for a final read from persistent memory.

Leaf nodes in `tree3` contain multiple key-value pairs, indexed using 1-byte fingerprints Pearson hashes that speed locating a given key.  Leaf modifications are accelerated using zero-copy updates.

## stree

This engine is disabled by default.  It can be enabled in CMake using the `ENGINE_STREE` option.

## caching

This engine is disabled by default.  It can be enabled in CMake using the `ENGINE_CACHING` option.

# EXAMPLE #

The example below presents standard flow of the application:
* preparing config,
* opening engine,
* adding new key/value pairs, reading data, removing... in general using database,
* and finally closing

< c example to be copied from README >

# SEE ALSO #

**libpmemkv**(3), **pmempool**(1), **libpmemobj**(7) and **<http://pmem.io>**

