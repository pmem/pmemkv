# Installing pmemkv

Installation of Key-Value Datastore for Persistent Memory

## Contents

1. [Building from Sources](#building-from-sources)
	- [Prerequisites](#prerequisites)
	- [Building pmemkv and running tests](#building-pmemkv-and-running-tests)
	- [Managing shared library](#managing-shared-library)
	- [Out-of-source builds](#out-of-source-builds)
2. [Installing on Fedora](#installing-on-fedora)
3. [Installing on Ubuntu](#installing-on-ubuntu)
4. [Using Experimental Engines](#using-experimental-engines)
5. [Building packages](#building-packages)
6. [Using a Pool Set](#using-a-pool-set)

## Building from Sources

### Prerequisites

* **Linux 64-bit** (OSX and Windows are not yet supported)
* **libpmem** and **libpmemobj**, which are part of [PMDK](https://github.com/pmem/pmdk) - Persistent Memory Development Kit 1.9.1
* [**libpmemobj-cpp**](https://github.com/pmem/libpmemobj-cpp) - C++ PMDK bindings 1.13.0
* [**memkind**](https://github.com/memkind/memkind) - Volatile memory manager 1.8.0 (required by vsmap & vcmap engines)
* [**TBB**](https://github.com/01org/tbb) - Thread Building Blocks (required by vcmap engine)
* [**RapidJSON**](https://github.com/tencent/rapidjson) - JSON parser 1.0.0 (required by `libpmemkv_json_config` helper library)
* Used only for **testing**:
	* [**pmempool**](https://github.com/pmem/pmdk/tree/master/src/tools/pmempool) - pmempool utility, part of PMDK
	* [**valgrind**](https://github.com/pmem/valgrind) - tool for profiling and memory leak detection. *pmem* forked version with *pmemcheck*
		tool is recommended, but upstream/original [valgrind](https://valgrind.org/) is also compatible (package valgrind-devel is required).
* Used only for **development**:
	* [**pandoc**](https://pandoc.org/) - markup converter to generate manpages
	* [**doxygen**](http://www.doxygen.nl/) - tool for generating documentation from annotated C++ sources
	* [**graphviz**](https://www.graphviz.org/) - graph visualization software required by _doxygen_
	* [**perl**](https://www.perl.org/) - for whitespace checker script
	* [**clang format**](https://clang.llvm.org/docs/ClangFormat.html) - to format and check coding style, version 9.0 is required

Required packages (or their names) for some OSes may differ. Some examples or scripts in this
repository may require additional dependencies, but should not interrupt the build.

See our **[Dockerfiles](utils/docker/images)** (used e.g. on our CI
systems) to get an idea what packages are required to build
the entire pmemkv, with all tests and examples.

### Building pmemkv and running tests

```sh
git clone https://github.com/pmem/pmemkv
cd pmemkv
mkdir ./build
cd ./build
cmake .. -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug    # run CMake, prepare Debug version
make -j$(nproc)                 # build everything
make test                       # run all tests
```

To see the output of failed tests, instead of the last command (`make test`) you can run:

```sh
ctest --output-on-failure
```

Building of the `libpmemkv_json_config` helper library is enabled by default.
If you want to disable it (for example to get rid of the RapidJSON dependency),
instead of a standard `cmake ..` command run:

```sh
cmake .. -DBUILD_JSON_CONFIG=OFF
```

### Managing shared library

To package `pmemkv` as a shared library and install on your system:

```sh
cmake .. [-DCMAKE_BUILD_TYPE=Release]	# prepare e.g. Release version
sudo make install			# install shared library to the default location: /usr/local
sudo make uninstall			# remove shared library and headers
```

To install this library into other locations, pass appropriate value to cmake
using CMAKE_INSTALL_PREFIX variable like this:

```sh
cmake .. -DCMAKE_INSTALL_PREFIX=/usr [-DCMAKE_BUILD_TYPE=Release]
sudo make install		# install to path specified by CMAKE_INSTALL_PREFIX
sudo make uninstall		# remove shared library and headers from path specified by CMAKE_INSTALL_PREFIX
```

### Out-of-source builds

If the standard build does not suit your needs, create your own
out-of-source build and run tests like this:

```sh
cd ~
mkdir mybuild
cd mybuild
cmake ~/pmemkv       # this directory should contain the source code of pmemkv
make -j$(nproc)
make test            # or 'ctest --output-on-failure'
```

## Installing on Fedora

Install required packages (see comprehensive list of packages used in our CI
on a Fedora image in [utils directory](./utils/docker/images/)):

Configure for proxy if necessary:

```sh
git config --global http.proxy <YOUR PROXY>
export HTTP_PROXY="<YOUR PROXY>"
export HTTPS_PROXY="<YOUR PROXY>"
```

Install latest PMDK:

```sh
cd ~
git clone https://github.com/pmem/pmdk
cd pmdk
make -j$(nproc)
su -c 'make install'
export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig
```

Install latest PMDK C++ bindings:

```sh
cd ~
git clone https://github.com/pmem/libpmemobj-cpp
cd libpmemobj-cpp
mkdir build
cd build
cmake ..
make -j$(nproc)
su -c 'make install'
```

Install latest memkind:

```sh
cd ~
git clone https://github.com/memkind/memkind
cd memkind
./autogen.sh
./configure
make
su -c 'make install'
```

Finally [build and install pmemkv from sources](#building-from-sources).

## Installing on Ubuntu

Install required packages (see comprehensive list of packages used in our CI
on a Ubuntu image in [utils directory](./utils/docker/images/)):

```sh
sudo apt install autoconf automake build-essential cmake libdaxctl-dev \
	libndctl-dev libnuma-dev libtbb-dev libtool rapidjson-dev
```

Configure for proxy if necessary:

```sh
git config --global http.proxy <YOUR PROXY>
export HTTP_PROXY="<YOUR PROXY>"
export HTTPS_PROXY="<YOUR PROXY>"
```

Install latest PMDK:

```sh
cd ~
git clone https://github.com/pmem/pmdk
cd pmdk
make -j$(nproc)
sudo make install
```

Install latest PMDK C++ bindings:

```sh
cd ~
git clone https://github.com/pmem/libpmemobj-cpp
cd libpmemobj-cpp
mkdir build
cd build
cmake ..
make -j$(nproc)
sudo make install
```

Install latest memkind:

```sh
cd ~
git clone https://github.com/memkind/memkind
cd memkind
./autogen.sh
./configure
make
sudo make install
```

Finally [build and install pmemkv from sources](#building-from-sources).

## Using Experimental Engines

To enable experimental engine(s) use adequate CMake parameter, e.g.:

```sh
cmake .. -DENGINE_CSMAP=ON
```

Now build will contain selected experimental engine(s) and their dependencies, that are not available by default.

Experimental engines may require additional libraries, see **prerequisites** section of a selected
engine in [ENGINES-experimental.md](doc/ENGINES-experimental.md).

## Building packages

```sh
...
cmake .. -DCPACK_GENERATOR="$GEN" -DCMAKE_INSTALL_PREFIX=/usr [-DCMAKE_BUILD_TYPE=Release]
make -j$(nproc) package
```

$GEN is a type of package generator and can be RPM or DEB.

CMAKE_INSTALL_PREFIX must be set to a destination where packages will be installed.

## Using a Pool Set

First create a pool set descriptor (`~/pmemkv.poolset` in this example):

```
PMEMPOOLSET
1000M /dev/shm/pmemkv1
1000M /dev/shm/pmemkv2
```

Next initialize the pool set:

```sh
pmempool create --layout pmemkv obj ~/pmemkv.poolset
```
