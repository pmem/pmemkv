---
title: pmemkv
layout: main
---

# pmemkv

**pmemkv** is a local/embedded key-value datastore optimized for persistent memory.
Rather than being tied to a single language or backing implementation,
**pmemkv** provides different options for language bindings and storage engines.

The **C API** of pmemkv is documented in the following manuals:

| Version branch | libpmemkv(3) | libpmemkv(7) | libpmemkv_config(3) | libpmemkv_json_config(3) |
| -------------- | ------------ | ------------ | ------------------- | ------------------------ |
| master | [libpmemkv.3](./master/manpages/libpmemkv.3.html) | [libpmemkv.7](./master/manpages/libpmemkv.7.html) | [libpmemkv_config.3](./master/manpages/libpmemkv_config.3.html) | [libpmemkv_json_config.3](./master/manpages/libpmemkv_json_config.3.html) |
| stable-1.3 | [libpmemkv.3](./v1.3/manpages/libpmemkv.3.html) | [libpmemkv.7](./v1.3/manpages/libpmemkv.7.html) | [libpmemkv_config.3](./v1.3/manpages/libpmemkv_config.3.html) | [libpmemkv_json_config.3](./v1.3/manpages/libpmemkv_json_config.3.html) |
| stable-1.2 | [libpmemkv.3](./v1.2/manpages/libpmemkv.3.html) | [libpmemkv.7](./v1.2/manpages/libpmemkv.7.html) | [libpmemkv_config.3](./v1.2/manpages/libpmemkv_config.3.html) | [libpmemkv_json_config.3](./v1.2/manpages/libpmemkv_json_config.3.html) |
| stable-1.1 | [libpmemkv.3](./v1.1/manpages/libpmemkv.3.html) | [libpmemkv.7](./v1.1/manpages/libpmemkv.7.html) | [libpmemkv_config.3](./v1.1/manpages/libpmemkv_config.3.html) | [libpmemkv_json_config.3](./v1.1/manpages/libpmemkv_json_config.3.html) |


for the current **master**:

* [libpmemkv.3](./master/manpages/libpmemkv.3.html)
* [libpmemkv.7](./master/manpages/libpmemkv.7.html)
* [libpmemkv_config.3](./master/manpages/libpmemkv_config.3.html)
* [libpmemkv_json_config.3](./master/manpages/libpmemkv_json_config.3.html)

for version **1.3**:

* [libpmemkv.3](./v1.3/manpages/libpmemkv.3.html)
* [libpmemkv.7](./v1.3/manpages/libpmemkv.7.html)
* [libpmemkv_config.3](./v1.3/manpages/libpmemkv_config.3.html)
* [libpmemkv_json_config.3](./v1.3/manpages/libpmemkv_json_config.3.html)

for version **1.2**:

* [libpmemkv.3](./v1.2/manpages/libpmemkv.3.html)
* [libpmemkv.7](./v1.2/manpages/libpmemkv.7.html)
* [libpmemkv_config.3](./v1.2/manpages/libpmemkv_config.3.html)
* [libpmemkv_json_config.3](./v1.2/manpages/libpmemkv_json_config.3.html)

for version **1.1**:

* [libpmemkv.3](./v1.1/manpages/libpmemkv.3.html)
* [libpmemkv.7](./v1.1/manpages/libpmemkv.7.html)
* [libpmemkv_config.3](./v1.1/manpages/libpmemkv_config.3.html)
* [libpmemkv_json_config.3](./v1.1/manpages/libpmemkv_json_config.3.html)

The **C++ API** of pmemkv is documented in the Doxygen documentation listed below:

* [master](./master/doxygen/index.html)
* [v1.3](./v1.3/doxygen/index.html)
* [v1.2](./v1.2/doxygen/index.html)
* [v1.1](./v1.1/doxygen/index.html)

## Language Bindings

Up-to-date overview information about language bindings for pmemkv can be found in
[pmemkv's README on GitHub](https://github.com/pmem/pmemkv#language-bindings).

Each of bindings has its own detailed API documentation on pmem.io:
* **pmemkv-java** [docs](https://pmem.io/pmemkv-java)
* **pmemkv-nodejs** [docs](https://pmem.io/pmemkv-nodejs)
* **pmemkv-python** [docs](https://pmem.io/pmemkv-python)

or just a readme (on GitHub's repository page):
* **pmemkv-ruby** [readme](https://github.com/pmem/pmemkv-ruby)

## Blog entries

The following series of blog articles provides an introduction to **pmemkv**:

* [Introduction](https://pmem.io/2017/02/21/pmemkv-intro.html)
* [Zero-copy leaf splits in pmemkv](https://pmem.io/2017/03/09/pmemkv-zero-copy-leaf-splits.html)
* [Benchmarking with different storage engines using pmemkv](https://pmem.io/2017/12/27/pmemkv-benchmarking-engines.html)
* [Language bindings for pmemkv](https://pmem.io/2020/03/04/pmemkv-bindings.html)
* [API overview of pmemkv-java binding](https://pmem.io/2020/10/30/pmemkv-java-binding.html)

## Releases' support status

Currently all branches/releases are fully supported. Latest releases can be
seen on the ["releases" tab on the Github page](https://github.com/pmem/pmemkv/releases).

| Version branch | First release date | Last patch release | Maintenance status |
| -------------- | ------------------ | ------------------ | ------------------ |
| stable-1.3 | Oct 02, 2020 | N/A | Full |
| stable-1.2 | May 29, 2020 | N/A | Full |
| stable-1.1 | Jan 31, 2020 | N/A | Full |
| stable-1.0 | Oct 4, 2019 | 1.0.3 (Oct 06, 2020) | EOL |

Possible statuses:
1. Full maintenance:
	* All/most of bugs fixed (if possible),
	* Patch releases issued based on a number of fixes and their severity,
	* At least one release at the end of the maintenance period,
	* Full support for at least a year since the initial release.
2. Limited scope:
	* Only critical bugs (security, data integrity, etc.) will be backported,
	* Patch versions will be released when needed (based on severity of found issues),
	* Branch will remain in "limited maintanance" status based on original release availability in popular distros,
3. EOL:
	* No support,
	* No bug fixes,
	* No official releases.

## Archived documentation

For version **1.0.3**:

* [C++ Doxygen docs](./v1.0/doxygen/index.html)
* C manpages:
  * [libpmemkv.3](./v1.0/manpages/libpmemkv.3.html)
  * [libpmemkv.7](./v1.0/manpages/libpmemkv.7.html)
  * [libpmemkv_config.3](./v1.0/manpages/libpmemkv_config.3.html)
  * [libpmemkv_json_config.3](./v1.0/manpages/libpmemkv_json_config.3.html)
