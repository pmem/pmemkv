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

`pmemkv` does not currently provide install packages, but our
<a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md">installation</a> guide
provides detailed instructions, including use of experimental engines and pool sets. 

<ul>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#building_from_sources">Building From Sources</a></li>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#fedora">Installing on Fedora</a></li>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#ubuntu">Installing on Ubuntu</a></li>
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
#include <libpmemkv.h>

#define LOG(msg) std::cout << msg << "\n"

using namespace pmemkv;

int main() {
    LOG("Starting engine");
    KVEngine* kv = KVEngine::Start("vsmap", "{\"path\":\"/dev/shm/\"}");

    LOG("Putting new key");
    KVStatus s = kv->Put("key1", "value1");
    assert(s == OK && kv->Count() == 1);

    LOG("Reading key back");
    string value;
    s = kv->Get("key1", &value);
    assert(s == OK && value == "value1");

    LOG("Iterating existing keys");
    kv->Put("key2", "value2");
    kv->Put("key3", "value3");
    kv->All([](const string& k) {
        LOG("  visited: " << k);
    });

    LOG("Removing existing key");
    s = kv->Remove("key1");
    assert(s == OK && !kv->Exists("key1"));

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

void StartFailureCallback(void* context, const char* engine, const char* config, const char* msg) {
    printf("ERROR: %s\n", msg);
    exit(-1);
}

void AllCallback(void* context, int kb, const char* k) {
    printf("   visited: %s\n", k);
}

int main() {
    LOG("Starting engine");
    KVEngine* kv = kvengine_start(NULL, "vsmap", "{\"path\":\"/dev/shm/\"}", &StartFailureCallback);

    LOG("Putting new key");
    char* key1 = "key1";
    char* value1 = "value1";
    KVStatus s = kvengine_put(kv, strlen(key1), key1, strlen(value1), value1);
    assert(s == OK && kvengine_count(kv) == 1);

    LOG("Reading key back");
    char val[MAX_VAL_LEN];
    s = kvengine_get_copy(kv, strlen(key1), key1, MAX_VAL_LEN, val);
    assert(s == OK && !strcmp(val, "value1"));

    LOG("Iterating existing keys");
    char* key2 = "key2";
    char* value2 = "value2";
    char* key3 = "key3";
    char* value3 = "value3";
    kvengine_put(kv, strlen(key2), key2, strlen(value2), value2);
    kvengine_put(kv, strlen(key3), key3, strlen(value3), value3);
    kvengine_all(kv, NULL, &AllCallback);

    LOG("Removing existing key");
    s = kvengine_remove(kv, strlen(key1), key1);
    assert(s == OK && kvengine_exists(kv, strlen(key1), key1) == NOT_FOUND);

    LOG("Stopping engine");
    kvengine_stop(kv);
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
