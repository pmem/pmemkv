# pmemkv

[![Travis build status](https://travis-ci.org/pmem/pmemkv.svg?branch=master)](https://travis-ci.org/pmem/pmemkv)
[![GHA build status](https://github.com/pmem/pmemkv/workflows/pmemkv/badge.svg?branch=master)](https://github.com/pmem/pmemkv/actions)
[![PMEMKV version](https://img.shields.io/github/tag/pmem/pmemkv.svg)](https://github.com/pmem/pmemkv/releases/latest)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/18408/badge.svg)](https://scan.coverity.com/projects/pmem-pmemkv)
[![Coverage Status](https://codecov.io/github/pmem/pmemkv/coverage.svg?branch=master)](https://codecov.io/gh/pmem/pmemkv/branch/master)

Key/Value Datastore for Persistent Memory

## Overview

`pmemkv` is a local/embedded key-value datastore optimized for persistent memory.
Rather than being tied to a single language or backing implementation, `pmemkv`
provides different options for language bindings and storage engines.
For more information, see https://pmem.io/pmemkv.

The C++ API of pmemkv is documented in the Doxygen documentation listed below:

- [master](https://pmem.io/pmemkv/master/doxygen/index.html)
- [v1.1](https://pmem.io/pmemkv/v1.1/doxygen/index.html)
- [v1.0](https://pmem.io/pmemkv/v1.0/doxygen/index.html)

There is also a small helper library `pmemkv_json_config` provided.
See its [manual](doc/libpmemkv_json_config.3.md) for details.

- [Installation](#installation)
- [Language Bindings](#language-bindings)
- [Storage Engines](#storage-engines)
- [Tools and Utilities](#tools-and-utilities)

## Installation

[Installation guide](INSTALLING.md)
provides detailed instructions how to build and install `pmemkv` from sources,
build rpm and deb packages and explains usage of experimental engines and pool sets.

- [Building from Sources](INSTALLING.md#building-from-sources)
- [Installing on Fedora](INSTALLING.md#installing-on-fedora)
- [Installing on Ubuntu](INSTALLING.md#installing-on-ubuntu)
- [Using Experimental Engines](INSTALLING.md#using-experimental-engines)
- [Building packages](INSTALLING.md#building-packages)
- [Using a Pool Set](INSTALLING.md#using-a-pool-set)

## Language Bindings

`pmemkv` is written in C/C++ and it is used by bindings for Java, Node.js,
Python, and Ruby applications.

![pmemkv-bindings](https://user-images.githubusercontent.com/12031346/65962933-ff6bfc00-e459-11e9-9552-d6326e9c0684.png)

### C/C++ Examples

Examples for C and C++ can be found within this repository in [examples directory](./examples/).

### Other Languages

Abovementioned bindings are maintained in separate GitHub repositories, but are still kept
in sync with the main `pmemkv` distribution.

* Java - https://github.com/pmem/pmemkv-java
	* \+ Java Native Interface - https://github.com/pmem/pmemkv-jni
* Node.js - https://github.com/pmem/pmemkv-nodejs
* Python - https://github.com/pmem/pmemkv-python
* Ruby - https://github.com/pmem/pmemkv-ruby

## Storage Engines

`pmemkv` provides multiple storage engines that conform to the same common API, so every engine can be used with
all language bindings and utilities. Engines are loaded by name at runtime.

| Engine Name  | Description | Experimental? | Concurrent? | Sorted? |
| ------------ | ----------- | ------------- | ----------- | ------- |
| [blackhole](doc/libpmemkv.7.md#blackhole) | Accepts everything, returns nothing | No | Yes | No |
| [cmap](doc/libpmemkv.7.md#cmap) | Concurrent hash map | No | Yes | No |
| [vsmap](doc/libpmemkv.7.md#vsmap) | Volatile sorted hash map | No | No | Yes |
| [vcmap](doc/libpmemkv.7.md#vcmap) | Volatile concurrent hash map | No | Yes | No |
| [tree3](ENGINES-experimental.md#tree3) | Persistent B+ tree | Yes | No | No |
| [stree](ENGINES-experimental.md#stree) | Sorted persistent B+ tree | Yes | No | Yes |
| [caching](ENGINES-experimental.md#caching) | Caching for remote Memcached or Redis server | Yes | No | - |

The production quality engines are described in the [libpmemkv(7)](doc/libpmemkv.7.md#engines) manual
and the experimental engines are described in the [ENGINES-experimental.md](ENGINES-experimental.md) file.

[Contributing a new engine](CONTRIBUTING.md#creating-new-engines) is easy and encouraged!

## Tools and Utilities

Benchmarks' scripts and other helpful utilities are available here:

https://github.com/pmem/pmemkv-tools

