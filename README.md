# pmemkv

[![Build Status](https://travis-ci.org/pmem/pmemkv.svg?branch=master)](https://travis-ci.org/pmem/pmemkv)
[![PMEMKV version](https://img.shields.io/github/tag/pmem/pmemkv.svg)](https://github.com/pmem/pmemkv/releases/latest)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/18408/badge.svg)](https://scan.coverity.com/projects/pmem-pmemkv)
[![Coverage Status](https://codecov.io/github/pmem/pmemkv/coverage.svg?branch=master)](https://codecov.io/gh/pmem/pmemkv/branch/master)

Key/Value Datastore for Persistent Memory

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

![pmemkv-bindings](https://user-images.githubusercontent.com/12031346/60346522-9b015280-99bb-11e9-9aab-8b8c9c9b7acd.png)

### C++ Example

```cpp
#include <cassert>
#include <iostream>
#include <libpmemkv.hpp>

#define LOG(msg) std::cout << msg << "\n"

using namespace pmem::kv;

const std::string PATH = "/dev/shm/";
const uint64_t SIZE = ((uint64_t)(1024 * 1024 * 1024));

int main()
{
	LOG("Creating config");
	pmemkv_config *cfg = pmemkv_config_new();
	assert(cfg != nullptr);

	int ret = pmemkv_config_put_string(cfg, "path", PATH.c_str());
	assert(ret == PMEMKV_STATUS_OK);
	ret = pmemkv_config_put_uint64(cfg, "size", SIZE);
	assert(ret == PMEMKV_STATUS_OK);

	LOG("Starting engine");
	db *kv = new db();
	assert(kv != nullptr);
	status s = kv->open("vsmap", cfg);
	assert(s == status::OK);

	LOG("Putting new key");
	s = kv->put("key1", "value1");
	assert(s == status::OK);

	size_t cnt;
	s = kv->count_all(cnt);
	assert(s == status::OK && cnt == 1);

	LOG("Reading key back");
	std::string value;
	s = kv->get("key1", &value);
	assert(s == status::OK && value == "value1");

	LOG("Iterating existing keys");
	kv->put("key2", "value2");
	kv->put("key3", "value3");
	kv->get_all([](string_view k, string_view v) { LOG("  visited: " << k.data()); return 0; });

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
#include <assert.h>
#include <libpmemkv.h>
#include <stdio.h>
#include <string.h>

#define LOG(msg) printf("%s\n", msg)
#define MAX_VAL_LEN 64

const char *PATH = "/dev/shm";
const uint64_t SIZE = ((uint64_t)(1024 * 1024 * 1024));

int get_kv_callback(const char *k, size_t kb, const char *value, size_t value_bytes,
		     void *arg)
{
	printf("   visited: %s\n", k);

	return 0;
}

int main()
{
	LOG("Creating config");
	pmemkv_config *cfg = pmemkv_config_new();
	assert(cfg != NULL);

	int s = pmemkv_config_put_string(cfg, "path", PATH);
	assert(s == PMEMKV_STATUS_OK);
	s = pmemkv_config_put_uint64(cfg, "size", SIZE);
	assert(s == PMEMKV_STATUS_OK);

	LOG("Starting engine");
	pmemkv_db *db = NULL;
	s = pmemkv_open("vsmap", cfg, &db);
	assert(s == PMEMKV_STATUS_OK && db != NULL);

	LOG("Putting new key");
	char *key1 = "key1";
	char *value1 = "value1";
	s = pmemkv_put(db, key1, strlen(key1), value1, strlen(value1));
	assert(s == PMEMKV_STATUS_OK);

	size_t cnt;
	s = pmemkv_count_all(db, &cnt);
	assert(s == PMEMKV_STATUS_OK && cnt == 1);

	LOG("Reading key back");
	char val[MAX_VAL_LEN];
	s = pmemkv_get_copy(db, key1, strlen(key1), val, MAX_VAL_LEN, NULL);
	assert(s == PMEMKV_STATUS_OK && !strcmp(val, "value1"));

	LOG("Iterating existing keys");
	char *key2 = "key2";
	char *value2 = "value2";
	char *key3 = "key3";
	char *value3 = "value3";
	pmemkv_put(db, key2, strlen(key2), value2, strlen(value2));
	pmemkv_put(db, key3, strlen(key3), value3, strlen(value3));
	pmemkv_get_all(db, &get_kv_callback, NULL);

	LOG("Removing existing key");
	s = pmemkv_remove(db, key1, strlen(key1));
	assert(s == PMEMKV_STATUS_OK &&
	       pmemkv_exists(db, key1, strlen(key1)) == PMEMKV_STATUS_NOT_FOUND);

	LOG("Stopping engine");
	pmemkv_close(db);

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
| [caching](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#caching) | Caching for remote Memcached or Redis server | Yes | No | - |

[Contributing a new engine](https://github.com/pmem/pmemkv/blob/master/CONTRIBUTING.md#engines) is easy and encouraged!

<a name="tools"></a>

Tools and Utilities
-------------------

Benchmarks, examples and other helpful utilities are available here:

https://github.com/pmem/pmemkv-tools
