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

| Engine  | Description | Stable? | Thread-Safe? | Each? | Remove? |
| ------- | ----------- | ------- | ------------ | ----- | ------- |
| [blackhole](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#blackhole) | Accepts everything, returns nothing | Yes | Yes | Yes | Yes |
| [kvtree2](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#kvtree2) (default) | Hybrid B+ persistent tree | Yes | No | No | Yes |
| btree | Copy-on-write B+ persistent tree | No | No | Yes | No |
| hashmap | TBB-optimized hash map | No | Yes | Yes | Yes |

<a name="bindings"></a>

Language Bindings
-----------------

`pmemkv` is written in C and C++. Developers can either use native C++ classes directly, or use our `extern "C"` API, or use one of several high-level language bindings that are based on the `extern "C"` API.

![pmemkv-bindings](https://user-images.githubusercontent.com/913363/34419334-6d6252fc-ebc0-11e7-9a34-d78591fb8c40.png)

### C++

```cpp
#include <libpmemkv.h>
#include <string.h>
#include <assert.h>
using namespace pmemkv;

int main() {
    // open the datastore
    KVEngine* kv = new KVEngine("kvtree2", "/dev/shm/mykv", 8388608);  // 8 MB

    // put new key
    KVStatus s = kv->Put("key1", "value1");
    assert(s == OK);

    // read last key back
    string value;
    s = kv->Get("key1", &value);
    assert(s == OK && value == "value1");

    // close the datastore
    delete kv;

    return 0;
}
```

### C

```c
#include <libpmemkv.h>
#include <string.h>
#include <assert.h>

int main() {
    #define VAL_LEN 64
    char value[VAL_LEN];

    /* open the datastore */
    KVEngine* kv = kvengine_open("kvtree2", "/dev/shm/mykv", 8388608);

    /* put new key */
    int32_t len = strlen("value1");
    KVStatus s = kvengine_put(kv, "key1", "value1", &len);
    assert(s == OK);

    /* read last key back */
    len = VAL_LEN;
    s = kvengine_get(kv, "key1", VAL_LEN, value, &len);
    assert(s == OK && !strcmp(value, "value1"));

    /* close the datastore */
    kvengine_close(kv);

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