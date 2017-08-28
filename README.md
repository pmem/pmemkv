# pmemkv
Key/Value Datastore for Persistent Memory

*This is experimental pre-release software and should not be used in
production systems. APIs and file formats may change at any time without
preserving backwards compatibility. All known issues and limitations
are logged as GitHub issues.*

Contents
--------

<ul>
<li><a href="#overview">Overview</a></li>
<li><a href="#installation">Installation</a></li>
<li><a href="#benchmarking">Benchmarking</a></li>
<li><a href="#language_bindings">Language Bindings</a></li>
<li><a href="#related_work">Related Work</a></li>
</ul>

<a name="overview"></a>

Overview
--------

`pmemkv` is a local/embedded key-value datastore optimized for persistent memory.

Internally, `pmemkv` uses a hybrid fingerprinted B+ tree implementation. Rather than keeping
inner and leaf nodes of the tree in persistent memory, `pmemkv` uses a hybrid structure where
inner nodes are kept in DRAM and leaf nodes only are kept in persistent memory. Though `pmemkv`
has to recover all inner nodes when the datastore is first opened, searches are performed in 
DRAM except for a final read from persistent memory.

![pmemkv-intro](https://cloud.githubusercontent.com/assets/913363/25543024/289f06d8-2c12-11e7-86e4-a1f0df891659.png)

Leaf nodes in `pmemkv` contain multiple key-value pairs, indexed using 1-byte fingerprints
([Pearson hashes](https://en.wikipedia.org/wiki/Pearson_hashing)) that speed locating
a given key. Leaf modifications are accelerated using
[zero-copy updates](http://pmem.io/2017/03/09/pmemkv-zero-copy-leaf-splits.html). 

<a name="installation"></a>

Installation
------------

* [Installing on Fedora (Stable NVML)](https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#fedora_stable_nvml)
* [Installing on Fedora (Latest NVML)](https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#fedora_latest_nvml)
* [Building From Sources](https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#building_from_sources)
* [Configuring Device DAX](https://github.com/pmem/pmemkv/blob/master/INSTALLING.md#device_dax)

<a name="benchmarking"></a>

Benchmarking
------------

The `pmemkv_stress` utility provides some simple read & write benchmarks.

```
Usage:  pmemkv_stress [r|w] [path] [size in MB]
```  

Benchmarking on emulated persistent memory:

```
(/dev/shm exists on Linux by default)

cd pmemkv/bin
rm -rf /dev/shm/pmemkv
PMEM_IS_PMEM_FORCE=1 ./pmemkv_stress w /dev/shm/pmemkv 1000
PMEM_IS_PMEM_FORCE=1 ./pmemkv_stress r /dev/shm/pmemkv 1000
rm -rf /dev/shm/pmemkv
```

Benchmarking on filesystem DAX:

```
(assuming device present at /dev/pmem1)

mkdir /mnt/pmem
mount -o dax /dev/pmem1 /mnt/pmem

cd pmemkv/bin
rm -rf /mnt/pmem/pmemkv
PMEM_IS_PMEM_FORCE=1 ./pmemkv_stress w /mnt/pmem/pmemkv 1000
PMEM_IS_PMEM_FORCE=1 ./pmemkv_stress r /mnt/pmem/pmemkv 1000
rm -rf /mnt/pmem/pmemkv
```

Benchmarking on device DAX:

```
(assuming device present at /dev/dax1.0)

pmempool rm --verbose /dev/dax1.0
pmempool create --layout pmemkv obj /dev/dax1.0

cd pmemkv/bin
./pmemkv_stress w /dev/dax1.0 1000
./pmemkv_stress r /dev/dax1.0 1000
```

<a name="language_bindings"></a>

Language Bindings
-----------------

Developers can use native C/C++ interfaces provided by `pmemkv`, or one of several high-level
bindings that are maintained separately (for Java, Ruby, and Node.js).

### C++

```
using namespace pmemkv;

int main() {
    // open the datastore
    KVTree* kv = new KVTree("/dev/shm/mykv", 8388608);  // 8 MB

    // put new key
    KVStatus s = kv->Put("key1", "value1");
    assert(s == OK);

    // read last key back
    std::string value;
    s = kv->Get("key1", &value);
    assert(s == OK && value == "value1");

    // close the datastore
    delete kv;

    return 0;
}
```

### C

```
#include <libpmemkv.h>
#include <string.h>
#include <assert.h>

int main() {
    #define VAL_LEN 64
    char value[VAL_LEN];
    /* open the datastore */
    KVTree* kv = kvtree_open("/dev/shm/mykv", 8388608);

    /* put new key */
    int32_t len = strlen("value1");
    KVStatus s = kvtree_put(kv, "key1", "value1", &len);
    assert(s == OK);

    /* read last key back */
    len = VAL_LEN;
    s = kvtree_get(kv, "key1", VAL_LEN, value, &len);
    assert(s == OK && !strcmp(value, "value1"));

    // close the datastore
    kvtree_close(kv);

    return 0;
}
```

### Other Languages
 
* Java - https://github.com/pmem/pmemkv-java
* Node.js - https://github.com/pmem/pmemkv-nodejs
* Ruby - https://github.com/pmem/pmemkv-ruby

<a name="related_work"></a>

Related Work
------------

**pmse**

`pmemkv` has a lot in common with [pmse](https://github.com/pmem/pmse)
-- both implementations rely on NVML internally, although
they expose different APIs externally. Both `pmse` and `pmemkv` are based on a B+ tree
implementation. The biggest difference is that the `pmse`
tree keeps inner and leaf nodes in persistent memory,
where `pmemkv` keeps inner nodes in DRAM and leaf nodes in
persistent memory. (This means that `pmemkv` has to recover
all inner nodes when the datastore is first opened)

**FPTree**

This research paper describes a hybrid DRAM/NVM tree design (similar
to `pmemkv`) but doesn't provide any code, and even in describing the
design omits certain important implementation details.

Beyond providing a clean-room implementation, the design of `pmemkv` differs
from FPTree in several important areas:

1. `pmemkv` is written using NVML C++ bindings, which exerts influence on
its design and implementation. `pmemkv` uses generic NVML transactions
(ie. `transaction::exec_tx()` closures), there is no need for micro-logging
structures as described in the FPTree paper to make internal delete and
split operations safe. `pmemkv` also adjusts sizes of data structures
(to fit NVML primitive types) for best cache-line optimization.

2. FPTree does not specify a hash method implementation, where `pmemkv`
uses a Pearson hash (RFC 3074).

3. Within its persistent leaves, FPTree uses an array of key hashes with
a separate visibility bitmap to track what hash slots are occupied.
`pmemkv` takes a different approach and uses key hashes themselves to track
visibility. This relies on a specially modified Pearson hash function,
where a hash value of zero always indicates the slot is unused.
This optimization eliminates the cost of using and maintaining
visibility bitmaps as well as cramming more hashes into a single
cache-line, and affects the implementation of every primitive operation
in the tree.

4. `pmemkv` caches key hashes in DRAM (in addition to storing these as
part of the persistent leaf). This speeds leaf operations, especially with
slower media, for what seems like an acceptable rise in DRAM usage.

5. Within its persistent leaves, `pmemkv` combines hash, key and value
into a single slot type (`KVSlot`). This leads to improved leaf split
performance and reduced write amplification, since splitting can be
performed by swapping pointers to slots without copying any key or
value data stored in the slots. `KVSlot` internally stores key and
value to a single persistent buffer, which minimizes the number of
persistent allocations and improves storage efficiency with larger 
keys and values.

**cpp_map**

Use of NVML C++ bindings by `pmemkv` was lifted from this example program.
Many thanks to [@tomaszkapela](https://github.com/tomaszkapela)
for providing a great example to follow!
