---
title: pmemkv
layout: main
---

# pmemkv

**pmemkv** is a local/embedded key-value datastore optimized for persistent memory.
Rather than being tied to a single language or backing implementation,
**pmemkv** provides different options for language bindings and storage engines.

The **C API** of pmemkv is documented in the following manuals:

for the current **master**:

* [libpmemkv.3](./master/manpages/libpmemkv.3.html)
* [libpmemkv.7](./master/manpages/libpmemkv.7.html)
* [libpmemkv_config.3](./master/manpages/libpmemkv_config.3.html)
* [libpmemkv_json_config.3](./master/manpages/libpmemkv_json_config.3.html)

for version **1.1**:

* [libpmemkv.3](./v1.1/manpages/libpmemkv.3.html)
* [libpmemkv.7](./v1.1/manpages/libpmemkv.7.html)
* [libpmemkv_config.3](./v1.1/manpages/libpmemkv_config.3.html)
* [libpmemkv_json_config.3](./v1.1/manpages/libpmemkv_json_config.3.html)

for version **1.0**:

* [libpmemkv.3](./v1.0/manpages/libpmemkv.3.html)
* [libpmemkv.7](./v1.0/manpages/libpmemkv.7.html)
* [libpmemkv_config.3](./v1.0/manpages/libpmemkv_config.3.html)
* [libpmemkv_json_config.3](./v1.0/manpages/libpmemkv_json_config.3.html)

The **C++ API** of pmemkv is documented in the Doxygen documentation listed below:

* [master](./master/doxygen/index.html)
* [v1.1](./v1.1/doxygen/index.html)
* [v1.0](./v1.0/doxygen/index.html)

## Releases' support status

Currently all branches/releases are fully supported. Latest releases can be
seen on the ["releases" tab on the Github page](https://github.com/pmem/pmemkv/releases).

| Version branch | First release date | Maintenance status |
| -------------- | ------------------ | ------------------ |
| stable-1.1 | Jan 31, 2020 | Full |
| stable-1.0 | Oct 4, 2019 | Full |

Possible statuses:
1. Full maintenance:
	* All/most of bugs fixed (if possible),
	* Patch releases issued based on a number of fixes and their severity,
	* At least one release at the end of the maintenance period,
	* Full support for at least a year since the initial release.
2. Limited scope:
	* Only critical bugs (security, data integrity, etc.) will be backported,
	* Patch versions will be released when needed (based on severity of found issues),
	* Branch will remain in "limited maintenance" status based on original release availability in popular distros,
	* Announcement about transition to EOL at least a half year before.
3. EOL:
	* No support,
	* No bug fixes,
	* No official releases.

# Blog entries

The following series of blog articles provides an introduction to **pmemkv**:

* [Introduction](https://pmem.io/2017/02/21/pmemkv-intro.html)
* [Zero-copy leaf splits in pmemkv](https://pmem.io/2017/03/09/pmemkv-zero-copy-leaf-splits.html)
* [Benchmarking with different storage engines using pmemkv](https://pmem.io/2017/12/27/pmemkv-benchmarking-engines.html)
