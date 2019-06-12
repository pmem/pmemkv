# pmemkv

[![Build Status](https://travis-ci.org/pmem/pmemkv.svg?branch=master)](https://travis-ci.org/pmem/pmemkv)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/18408/badge.svg)](https://scan.coverity.com/projects/pmem-pmemkv)
[![Coverage Status](https://codecov.io/github/pmem/pmemkv/coverage.svg?branch=master)](https://codecov.io/gh/pmem/pmemkv/branch/master)

Key/Value Datastore for Persistent Memory

*This is experimental pre-release software and should not be used in
production systems. APIs and file formats may change at any time without
preserving backwards compatibility. All known issues and limitations
are logged as GitHub issues.*

Overview
--------

`pmemkv` is a local/embedded key-value datastore optimized for persistent memory.
Rather than being tied to a single language or backing implementation, `pmemkv`
provides different options for language bindings and storage engines.

<ul>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md">Installation</a></li>
<li><a href="#bindings">Language Bindings</a></li>
<li><a href="#engines">Storage Engines</a></li>
<li><a href="#tools">Tools and Utilities</a></li>
</ul>

<a name="installation"></a>

Installation
------------

<a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md">Installation</a> guide
provides detailed instructions how to build and install `pmemkv` from sources,
build rpm and deb packages and explains usage of experimental engines and pool sets.

<ul>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#building_from_sources">Building From Sources</a></li>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#fedora">Installing on Fedora</a></li>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#ubuntu">Installing on Ubuntu</a></li>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#build_package">Building packages</a></li>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#experimental">Using Experimental Engines</a></li>
</ul>

<a name="bindings"></a>

Language Bindings
-----------------

`pmemkv` is written in C/C++ and includes bindings for Java, Ruby, and Node.js applications.

![pmemkv-bindings](https://user-images.githubusercontent.com/913363/52880816-4651ef00-3120-11e9-9ab4-7eb006b4c7f5.png)

### C++ Example

```cpp
#include <cassert>
#include <iostream>
#include <libpmemkv.hpp>
#include <string>

#define LOG(msg) std::cout << msg << "\n"

using namespace pmem::kv;

int main() {
    LOG("Starting engine");
    db *kv = new db("vsmap", "{\"path\":\"/dev/shm/\"}");

    LOG("Putting new key");
    status s = kv->put("key1", "value1");
    assert(s == status::OK && kv->count() == 1);

    LOG("Reading key back");
    std::string value;
    s = kv->get("key1", &value);
    assert(s == status::OK && value == "value1");

    LOG("Iterating existing keys");
    kv->put("key2", "value2");
    kv->put("key3", "value3");
    kv->all([](const std::string& k) {
        LOG("  visited: " << k);
    });

    LOG("Removing existing key");
    s = kv->remove("key1");
    assert(s == status::OK);
    s = kv->exists("key1");
    assert(s == status::NOT_FOUND);

    LOG("Stopping engine");
    delete kv;
    return 0;
}
```

### C Example

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libpmemkv.h>

#define LOG(msg) printf("%s\n", msg)
#define MAX_VAL_LEN 64

void start_failure_callback(void *context, const char *engine, const char *config, const char *msg) {
    printf("ERROR: %s\n", msg);
    exit(-1);
}

void all_callback(const char *k, size_t kb, void *arg) {
    printf("   visited: %s\n", k);
}

int main() {
    LOG("Starting engine");

    pmemkv_db *kv = pmemkv_open(NULL, "vsmap", "{\"path\":\"/dev/shm/\"}", &start_failure_callback);

    LOG("Putting new key");
    char* key1 = "key1";
    char* value1 = "value1";
    int s = pmemkv_put(kv, key1, strlen(key1), value1, strlen(value1));
    assert(s == PMEMKV_STATUS_OK && pmemkv_count(kv) == 1);

    LOG("Reading key back");
    char val[MAX_VAL_LEN];
    s = pmemkv_get_copy(kv, key1, strlen(key1), val, MAX_VAL_LEN);
    assert(s == PMEMKV_STATUS_OK && !strcmp(val, "value1"));

    LOG("Iterating existing keys");
    char* key2 = "key2";
    char* value2 = "value2";
    char* key3 = "key3";
    char* value3 = "value3";
    pmemkv_put(kv, key2, strlen(key2), value2, strlen(value2));
    pmemkv_put(kv, key3, strlen(key3), value3, strlen(value3));
    pmemkv_all(kv, NULL, &all_callback);

    LOG("Removing existing key");
    s = pmemkv_remove(kv, key1, strlen(key1));
    assert(s == PMEMKV_STATUS_OK && pmemkv_exists(kv, key1, strlen(key1)) == PMEMKV_STATUS_NOT_FOUND);

    LOG("Stopping engine");
    pmemkv_close(kv);
    return 0;
}
```

### Other Languages

These bindings are maintained in separate GitHub repos, but are still kept
in sync with the main `pmemkv` distribution.

* Java - https://github.com/pmem/pmemkv-java
* Node.js - https://github.com/pmem/pmemkv-nodejs
* Ruby - https://github.com/pmem/pmemkv-ruby
* Python - https://github.com/pmem/pmemkv-python (coming soon!)

<a name="engines"></a>

Storage Engines
---------------

`pmemkv` provides multiple storage engines that conform to the same common API, so every engine can be used with
all language bindings and utilities. Engines are loaded by name at runtime.

| Engine Name  | Description | Experimental? | Concurrent? | Sorted? |
| ------------ | ----------- | ------------- | ----------- | ------- |
| [blackhole](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#blackhole) | Accepts everything, returns nothing | No | Yes | No |
| [cmap](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#cmap) | Concurrent hash map | No | Yes | No |
| [vsmap](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#vsmap) | Volatile sorted hash map | No | No | Yes |
| [vcmap](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#vcmap) | Volatile concurrent hash map | No | Yes | No |
| [tree3](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#tree3) | Persistent B+ tree | Yes | No | No |
| [stree](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#stree) | Sorted persistent B+ tree | Yes | No | Yes |
| [caching](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#caching) | Caching for remote Memcached or Redis server | Yes | Yes | - |

[Contributing a new engine](https://github.com/pmem/pmemkv/blob/master/CONTRIBUTING.md#engines) is easy and encouraged!

<a name="tools"></a>

Tools and Utilities
-------------------

Benchmarks, examples and other helpful utilities are available here:

https://github.com/pmem/pmemkv-tools
