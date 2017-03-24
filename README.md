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
<li><a href="#sample_code">Sample Code</a></li>
<li><a href="#related_work">Related Work</a></li>
<li><a href="#configuring_clion_project">Configuring CLion Project</a></li>
</ul>

<a name="overview"/>

Overview
--------

`pmemkv` is a local/embedded key-value datastore optimized for
peristent memory whose API is similar to RocksDB
(but is not drop-in compatible with RocksDB). `pmemkv` has
a lot in common with [pmse](https://github.com/pmem/pmse)
-- both implementations rely on NVML internally, although
they expose different APIs externally.

Both `pmse` and `pmemkv` are based on a B+ tree
implementation. The main difference is that the `pmse`
tree keeps inner and leaf nodes in persistent memory,
where `pmemkv` keeps inner nodes in DRAM and leaf nodes in
persistent memory. (This means that `pmemkv` has to recover
all inner nodes when the datastore is first opened)

Another difference is that leaf nodes
in `pmemkv` contain a small hash table based on a 1-byte
[Pearson hash](https://en.wikipedia.org/wiki/Pearson_hashing),
which limits the maximum size of the leaf, but boosts efficiency.

![pmemkv-intro](https://cloud.githubusercontent.com/assets/913363/22454311/21f15aa8-e742-11e6-9821-e3af513e279b.png)

<a name="installation"/>

Installation
------------

Start with Ubuntu 16.04 (either desktop or server distribution) or other
64-bit Linux distribution. OSX and Windows are not yet supported by `pmemkv`,
so don't use those.

To build `pmemkv`, you'll need make and g++ installed. No external libraries
are required to be installed beforehand.

```
make                # build everything from scratch and run tests
make thirdparty     # download and build dependencies
make clean          # remove build files
make example        # build and run example program
make stress         # build and run stress test
make test           # build and run unit tests
```

Building with CMake is partially supported -- there are some
[known issues](https://github.com/pmem/pmemkv/issues/42) to avoid.

<a name="sample_code"/>

Sample Code
-----------

We are using `/dev/shm` to
[emulate persistent memory](http://pmem.io/2016/02/22/pm-emulation.html)
in this simple example.

```
using namespace pmemkv;

int main() {
    // open the datastore
    KVTree* kv = new KVTree("/dev/shm/pmemkv", 8388608); // 8 MB

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

<a name="related_work"/>

Related Work
------------

**cpp_map**

Use of NVML C++ bindings by `pmemkv` was lifted from this example program.
Many thanks to [@tomaszkapela](https://github.com/tomaszkapela)
for providing a great example to follow!

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

<a name="configuring_clion_project"/>

Configuring CLion Project
-------------------------

Obviously the use of an IDE is a personal preference. CLion is not
required to be a contributor, but it's very easy to configure if
you have a valid license.

Use wizard to open project:

* From Welcome screen, select "Open Project"
* select root directory
* do not overwrite CMakeLists.txt if prompted (use our version)
* `src` directory should be automatically detected
* mark `3rdparty` directory as excluded

Set code style to match pmse:
* Start with [Google C++ Style](https://google.github.io/styleguide/cppguide.html)
* Indent 4 spaces, 8 spaces for continuation
* Max 100 chars per line
* Space after '*' and '&' (rather than before)
