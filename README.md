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
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md">Installation</a>
<ul>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#fedora_stable_pmdk">Installing on Fedora (Stable PMDK)</a>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#fedora_latest_pmdk">Installing on Fedora (Latest PMDK)</a>
<li><a href="https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#building_from_sources">Building From Sources</a>
</ul>
</li>
<li><a href="#engines">Storage Engines</a></li>
<li><a href="#bindings">Language Bindings</a></li>
<li><a href="#benchmarking">Benchmarking</a></li>
</ul>

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

| Engine  | Description | Thread-Safe? |
| ------- | ----------- | ------------ | 
| [kvtree](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#kvtree) | Hybrid B+ persistent tree | No |
| [blackhole](https://github.com/pmem/pmemkv/blob/master/ENGINES.md#blackhole) | Accepts everything, returns nothing | Yes |

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
    KVEngine* kv = new KVEngine("kvtree", "/dev/shm/mykv", 8388608);  // 8 MB

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
    KVEngine* kv = kvengine_open("kvtree", "/dev/shm/mykv", 8388608);

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

<a name="benchmarking"></a>

Benchmarking
------------

The `pmemkv_bench` utility provides some simple read & write benchmarks. This is
much like the `db_bench` utility included with LevelDB and RocksDB, although the
list of supported parameters is slightly different.

```
pmemkv_bench
--engine=<name>            (storage engine name, default: kvtree)
--db=<location>            (path to persistent pool, default: /dev/shm/pmemkv)
--db_size_in_gb=<integer>  (size of persistent pool in GB, default: 1)
--histogram=<0|1>          (show histograms when reporting latencies)
--num=<integer>            (number of keys to place in database, default: 1000000)
--reads=<integer>          (number of read operations, default: 1000000)
--threads=<integer>        (number of concurrent threads, default: 1)
--value_size=<integer>     (size of values in bytes, default: 100)
--benchmarks=<name>,       (comma-separated list of benchmarks to run)
    fillseq                (load N values in sequential key order into fresh db)
    fillrandom             (load N values in random key order into fresh db)
    overwrite              (replace N values in random key order)
    readseq                (read N values in sequential key order)
    readrandom             (read N values in random key order)
    readmissing            (read N missing values in random key order)
    deleteseq              (delete N values in sequential key order)
    deleterandom           (delete N values in random key order)
```  

Benchmarking on emulated persistent memory:

```
PMEM_IS_PMEM_FORCE=1 ./bin/pmemkv_bench
rm -rf /dev/shm/pmemkv
```

Benchmarking on filesystem DAX:

```
(assuming /dev/pmemX device mounted as /mnt/pmem filesystem)
PMEM_IS_PMEM_FORCE=1 ./bin/pmemkv_bench --db=/mnt/pmem/pmemkv
rm -rf /mnt/pmem/pmemkv
```

Benchmarking on device DAX:

```
(assuming /dev/dax1.0 device present)
./bin/pmemkv_bench --db=/dev/dax1.0
```
