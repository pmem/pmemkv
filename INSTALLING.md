# Installing pmemkv
Key/Value Datastore for Persistent Memory

*This is experimental pre-release software and should not be used in
production systems. APIs and file formats may change at any time without
preserving backwards compatibility. All known issues and limitations
are logged as GitHub issues.*

Contents
--------

<ul>
<li><a href="#building_from_sources">Building from Sources</a></li>
<li><a href="#fedora">Installing on Fedora</a></li>
<li><a href="#ubuntu">Installing on Ubuntu</a></li>
<li><a href="#experimental">Using Experimental Engines</a></li>
<li><a href="#build_package">Building packages</a></li>
<li><a href="#pool_set">Using a Pool Set</a></li>
</ul>

<a name="building_from_sources"></a>

Building from Sources
---------------------

**Prerequisites**

* 64-bit Linux (OSX and Windows are not yet supported)
* [PMDK](https://github.com/pmem/pmdk) - Persistent Memory Development Kit
* [libpmemobj-cpp](https://github.com/pmem/libpmemobj-cpp) - C++ bindings for PMDK (required by all engines except blackhole and caching)
* [TBB](https://github.com/01org/tbb) - Thread Building Blocks (requiered by cmap & vcmap engines)
* [memkind](https://github.com/memkind/memkind) - Volatile memory manager (required by vsmap & vcmap engines)
* (optional) [RapidJSON](https://github.com/tencent/rapidjson) - JSON parser (required by `libpmemkv_json_config` helper library)
* Used only for development & testing:
	* [pandoc](https://pandoc.org/) - markup converter to generate manpages
	* [doxygen](http://www.doxygen.nl/) - tool for generating documentation from annotated C++ sources
	* [graphviz](https://www.graphviz.org/) - open source graph visualization software required by _doxygen_
	* [perl](https://www.perl.org/) - for whitespace checker script
	* [clang format 8.0](https://clang.llvm.org/docs/ClangFormat.html) - to format and check coding style

**Building and running tests**

```sh
git clone https://github.com/pmem/pmemkv
cd pmemkv
mkdir ./build
cd ./build
cmake ..                # run CMake
make                    # build everything
make test               # run all tests
```

Instead of the last command (`make test`) you can run

```sh
ctest --output-on-failure
```

to see the output of failed tests.

Building of the `libpmemkv_json_config` helper library is enabled by default.
If you want to disable it (for example to get rid of the RapidJSON dependency)
run:

```sh
cmake .. -DBUILD_JSON_CONFIG=OFF
```

instead of:

```sh
cmake ..
```

**Managing shared library**

To package `pmemkv` as a shared library and install on your system:

```sh
sudo make install		# install shared library to the default location: /usr/local
sudo make uninstall		# remove shared library and headers
```

To install this library into other locations, pass appropriate value to cmake
using CMAKE_INSTALL_PREFIX variable like this:

```sh
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
sudo make install		# install to path specified by CMAKE_INSTALL_PREFIX
sudo make uninstall		# remove shared library and headers from path specified by CMAKE_INSTALL_PREFIX
```

**Out-of-source builds**

If the standard build does not suit your needs, create your own
out-of-source build and run tests like this:

```sh
cd ~
mkdir mybuild
cd mybuild
cmake ~/pmemkv       # this directory should contain the source code of pmemkv
make
make test            # or 'ctest --output-on-failure'
```

<a name="fedora"></a>

Installing on Fedora
--------------------

Install required packages:

```sh
su -c 'dnf install autoconf automake cmake daxctl-devel gcc gcc-c++ \
	libtool ndctl-devel numactl-devel rapidjson-devel tbb-devel'
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
make -j8
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
make
su -c 'make install'
```

Install latest memkind:

```sh
cd ~
git clone https://github.com/memkind/memkind
cd memkind
./build.sh
su -c 'make install'
```

Finally <a href="#building_from_sources">build and install pmemkv from sources</a>.

<a name="ubuntu"></a>

Installing on Ubuntu
--------------------

Install required packages:

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
make -j8
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
make
sudo make install
```

Install latest memkind:

```sh
cd ~
git clone https://github.com/memkind/memkind
cd memkind
./build.sh
sudo make install
```

Finally <a href="#building_from_sources">build and install pmemkv from sources</a>.

<a name="experimental"></a>

Using Experimental Engines
--------------------------

To enable experimental engine(s) use adequate CMake parameter, e.g.:

```sh
cmake .. -DENGINE_CACHING=ON
```

Now build will contain selected experimental engine(s) and their dependencies, that are not available by default.

Additional libraries (not listed above, in prerequisites section) are required only by caching engine. If you want to use it
you need to follow these instructions:

First build and install `pmemkv` as described above. Then, install client libraries for Memcached:

```sh
cd ~
mkdir work
cd work
wget https://launchpad.net/libmemcached/1.0/0.21/+download/libmemcached-0.21.tar.gz
tar -xvf libmemcached-0.21.tar.gz
mv libmemcached-0.21 libmemcached
cd libmemcached
./configure
make
su -c 'make install'
```

Install client libraries for Redis:

```sh
cd ~
mkdir work
cd work
git clone https://github.com/acl-dev/acl.git
mv acl libacl
cd libacl/lib_acl_cpp
make
cd ../lib_acl
make
cd ../lib_protocol
make
```

<a name="build_package"></a>

Building packages
-----------------
```sh
...
cmake .. -DCPACK_GENERATOR="$GEN" -DCMAKE_INSTALL_PREFIX=/usr
make package
```

$GEN is a type of package generator and can be RPM or DEB

CMAKE_INSTALL_PREFIX must be set to a destination where packages will be installed

<a name="pool_set"></a>

Using a Pool Set
----------------

First create a pool set descriptor:  (`~/pmemkv.poolset` in this example)

```
PMEMPOOLSET
1000M /dev/shm/pmemkv1
1000M /dev/shm/pmemkv2
```

Next initialize the pool set:

```sh
pmempool create --layout pmemkv obj ~/pmemkv.poolset
```
