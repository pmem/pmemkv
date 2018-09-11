# pmemkv
Key/Value Datastore for Persistent Memory

*This is experimental pre-release software and should not be used in
production systems. APIs and file formats may change at any time without
preserving backwards compatibility. All known issues and limitations
are logged as GitHub issues.*

Overview
--------

`pmemkv` is a local/embedded key-value datastore optimized for persistent memory.
Rather than being tied to a single language or backing implementation, `pmemkv`
provides different options for storage engines and language bindings.

<ul>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md">Installation</a></li>
<li><a href="#engines">Storage Engines</a></li>
<li><a href="#bindings">Language Bindings</a></li>
<li><a href="#tools">Tools and Utilities</a></li>
</ul>

<a name="installation"></a>

Installation
------------

`pmemkv` does not currently provide install packages, but building from sources is not difficult.

<ul>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#fedora_stable_pmdk">Installing on Fedora (Stable PMDK)</a></li>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#fedora_latest_pmdk">Installing on Fedora (Latest PMDK)</a></li>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#building_from_sources">Building from Sources</a></li>
</ul>

Our <a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md">installation</a> guide also includes help with configuring DAX and pool sets. 

<a name="engines"></a>

Storage Engines
---------------

`pmemkv` provides multiple storage engines with vastly different implementations. Since all
engines conform to the same common API, any engine can be used with common `pmemkv` utilities
and language bindings. Engines are requested at runtime by name.
[Contributing a new engine](https://github.com/pmem/pmemkv/blob/master/CONTRIBUTING.md#engines)
is easy and encouraged!

![pmemkv-engines](https://user-images.githubusercontent.com/913363/34419331-68619cfe-ebc0-11e7-9443-fa13dc9decbb.png)

### Available Engines

| Engine  | Description | Stable? | Thread-Safe? | Iterable? | Large Values? |
| ------- | ----------- | ------- | ------------ | ----- | ------- |
| [blackhole](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#blackhole) | Accepts everything, returns nothing | Yes | Yes | No | Yes |
| [kvtree3](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#kvtree) | Hybrid B+ persistent tree | Yes | No | Yes | Yes |
| [btree](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#btree) | Copy-on-write B+ persistent tree | Yes | No | Yes | No |
| [cmap](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#cmap) | Concurrent hash map | No | Yes | Yes | Yes |
| [vmap](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#vmap) | Volatile hash map | No | No | Yes | Yes |
| [vcmap](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#vcmap) | Volatile concurrent hash map | No | Yes | Yes | Yes |

<a name="bindings"></a>

Language Bindings
-----------------

`pmemkv` is written in C and C++. Developers can either use native C++ classes directly, or use our `extern "C"` API, or use one of several high-level language bindings that are based on the `extern "C"` API.

![pmemkv-bindings](https://user-images.githubusercontent.com/913363/34419334-6d6252fc-ebc0-11e7-9a34-d78591fb8c40.png)

### C++ Example

```cpp
#include <iostream>
#include "libpmemkv.h"

#define LOG(msg) std::cout << msg << "\n"

using namespace pmemkv;

int main() {
    LOG("Opening datastore");
    KVEngine* kv = KVEngine::Open("kvtree3", "/dev/shm/pmemkv", 1073741824);  // 1 GB pool

    LOG("Putting new key");
    KVStatus s = kv->Put("key1", "value1");
    assert(s == OK);
    assert(kv->Count() == 1);

    LOG("Reading key back");
    string value;
    s = kv->Get("key1", &value);
    assert(s == OK && value == "value1");

    LOG("Iterating existing keys");
    kv->Put("key2", "value2");
    kv->Put("key3", "value3");
    kv->Each([](void* context, int32_t kb, const char* k, int32_t vb, const char* v) {
        LOG("  visited: " << k);
    });

    LOG("Removing existing key");
    s = kv->Remove("key1");
    assert(s == OK);
    assert(!kv->Exists("key1"));

    LOG("Closing datastore");
    delete kv;

    LOG("Finished successfully");
    return 0;
}
```

### C Example

```c
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "libpmemkv.h"

#define LOG(msg) printf("%s\n", msg)
#define MAX_VAL_LEN 64

void MyEachCallback(void* context, int32_t kb, const char* k, int32_t vb, const char* v) {
    printf("   visited: %s\n", k);
}

int main() {
    LOG("Opening datastore");
    KVEngine* kv = kvengine_open("kvtree3", "/dev/shm/pmemkv", 1073741824);  // 1 GB pool

    LOG("Putting new key");
    char* key1 = "key1";
    char* value1 = "value1";
    KVStatus s = kvengine_put(kv, strlen(key1), key1, strlen(value1), value1);
    assert(s == OK);
    assert(kvengine_count(kv) == 1);

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
    kvengine_each(kv, NULL, &MyEachCallback);

    LOG("Removing existing key");
    s = kvengine_remove(kv, strlen(key1), key1);
    assert(s == OK);
    assert(kvengine_exists(kv, strlen(key1), key1) == NOT_FOUND);

    LOG("Closing datastore");
    kvengine_close(kv);

    LOG("Finished successfully");
    return 0;
}
```

### Other Languages

These bindings are maintained in separate GitHub repos, but are still kept
in sync with the main `pmemkv` distribution.
 
* Java - https://github.com/pmem/pmemkv-java
* JNI - https://github.com/pmem/pmemkv-jni
* Node.js - https://github.com/pmem/pmemkv-nodejs
* Ruby - https://github.com/pmem/pmemkv-ruby

<a name="tools"></a>

Tools and Utilities
-------------------

Benchmarks, examples and other helpful utilities are available here:

https://github.com/RobDickinson/pmemkv-tools