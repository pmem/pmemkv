# Experimental Storage Engines for pmemkv

- [tree3](#tree3)
- [csmap](#csmap)
- [radix](#radix)
- [stree](#stree)

# tree3

A persistent single-threaded engine, backed by a read-optimized B+ tree.
It is disabled by default. It can be enabled in CMake using the `ENGINE_TREE3` option.

### Configuration

Configuration must specify a `path` to a PMDK persistent pool (with layout "pmemkv_tree3"), which can be a file (on a DAX filesystem),
a DAX device, or a PMDK poolset file.

* **path** -- Path to the database file
	+ type: string
* **force_create** -- If 0, pmemkv opens file specified by 'path', otherwise it creates it
	+ type: uint64_t
	+ default value: 0
* **size** --  Only needed when force_create is not 0, specifies size of the database [in bytes]
	+ type: uint64_t
	+ min value: 8388608 (8MB)

### Internals

Internally, `tree3` uses a hybrid fingerprinted B+ tree implementation. Rather than keeping
inner and leaf nodes of the tree in persistent memory, `tree3` uses a hybrid structure where
inner nodes are kept in DRAM and leaf nodes only are kept in persistent memory. Though `tree3`
has to recover all inner nodes when the engine is started, searches are performed in
DRAM except for a final read from persistent memory.

![pmemkv-intro](https://cloud.githubusercontent.com/assets/913363/25543024/289f06d8-2c12-11e7-86e4-a1f0df891659.png)

Leaf nodes in `tree3` contain multiple key-value pairs, indexed using 1-byte fingerprints
([Pearson hashes](https://en.wikipedia.org/wiki/Pearson_hashing)) that speed locating
a given key. Leaf modifications are accelerated using
[zero-copy updates](https://pmem.io/2017/03/09/pmemkv-zero-copy-leaf-splits.html).

### Prerequisites

No additional packages are required.

# csmap

A persistent, concurrent and sorted engine, backed by a skip list.
It is disabled by default. It can be enabled in CMake using the `ENGINE_CSMAP` option (requires C++14 support).

All methods of csmap are thread safe. Put, get, count_\* and get_\* scale with the number of threads.
Remove method is currently implemented to take a global lock - it blocks all other threads.

### Configuration

* **path** -- Path to the database file (layout "pmemkv_csmap")
	+ type: string
* **force_create** -- If 0, pmemkv opens the file specified by 'path', otherwise it creates the file
	+ type: uint64_t
	+ default value: 0
* **size** --  Only needed when force_create is not 0, specifies size of the database [in bytes]
	+ type: uint64_t

### Prerequisites

No additional packages are required.

# radix

A persistent, sorted (without custom comparator support) engine, backed by a radix tree.
It is disabled by default. It can be enabled in CMake using the `ENGINE_RADIX` option.

### Configuration

* **path** -- Path to the database file (layout "pmemkv_csmap")
	+ type: string
* **force_create** -- If 0, pmemkv opens the file specified by 'path', otherwise it creates the file
	+ type: uint64_t
	+ default value: 0
* **size** --  Only needed when force_create is not 0, specifies size of the database [in bytes]
	+ type: uint64_t

### Prerequisites

No additional packages are required.

# stree

A persistent, single-threaded and sorted engine, backed by a B+ tree.
It is disabled by default. It can be enabled in CMake using the `ENGINE_STREE` option.

### Configuration

* **path** -- Path to the database file (layout "pmemkv_stree")
	+ type: string
* **force_create** -- If 0, pmemkv opens file specified by 'path', otherwise it creates it
	+ type: uint64_t
	+ default value: 0
* **size** --  Only needed when force_create is not 0, specifies size of the database [in bytes]
	+ type: uint64_t

### Internals

(TBD)

### Prerequisites

No additional packages are required.

# Related Work
---------

**pmse**

`tree3` has a lot in common with [pmse](https://github.com/pmem/pmse)
-- both implementations rely on PMDK internally, although
they expose different APIs externally. Both `pmse` and `tree3` are based on a B+ tree
implementation. The biggest difference is that the `pmse`
tree keeps inner and leaf nodes in persistent memory,
where `tree3` keeps inner nodes in DRAM and leaf nodes in
persistent memory. (This means that `tree3` has to recover
all inner nodes when the engine is started)

**FPTree**

This research paper describes a hybrid DRAM/NVM tree design (similar
to the `tree3` storage engine) but this paper doesn't provide any code, and
omits certain important implementation details.

Beyond providing a clean-room implementation, the design of `tree3`
differs from FPTree in several important areas:

1. `tree3` is written using PMDK C++ bindings, which exerts influence on
its design and implementation. `tree3` uses generic PMDK transactions
(i.e. `transaction::run()` closures), there is no need for micro-logging
structures as described in the FPTree paper to make internal delete and
split operations safe. `tree3` also adjusts sizes of data structures
(to fit PMDK primitive types) for best cache-line optimization.

2. FPTree does not specify a hash method implementation, where `tree3`
uses a Pearson hash (RFC 3074).

3. Within its persistent leaves, FPTree uses an array of key hashes with
a separate visibility bitmap to track what hash slots are occupied.
`tree3` takes a different approach and uses key hashes themselves to track
visibility. This relies on a specially modified Pearson hash function,
where a hash value of zero always indicates the slot is unused.
This optimization eliminates the cost of using and maintaining
visibility bitmaps as well as cramming more hashes into a single
cache-line, and affects the implementation of every primitive operation
in the tree.

4. `tree3` caches key hashes in DRAM (in addition to storing these as
part of the persistent leaf). This speeds leaf operations, especially with
slower media, for what seems like an acceptable rise in DRAM usage.

5. Within its persistent leaves, `tree3` combines hash, key and value
into a single slot type (`KVSlot`). This leads to improved leaf split
performance and reduced write amplification, since splitting can be
performed by swapping pointers to slots without copying any key or
value data stored in the slots. `KVSlot` internally stores key and
value to a single persistent buffer, which minimizes the number of
persistent allocations and improves storage efficiency with larger
keys and values.

**cpp_map**

Use of PMDK C++ bindings by `tree3` was lifted from this example program.
Many thanks to [@tomaszkapela](https://github.com/tomaszkapela)
for providing a great example to follow!

# Archived engines

**caching**

It was present in releases <= 1.2
That engine was using a sub engine (one of other engines) to cache requests to external Redis or Memcached server.
