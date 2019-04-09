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
<li><a href="#pool_set">Using a Pool Set</a></li>
</ul>

<a name="building_from_sources"></a>

Building from Sources
---------------------

**Prerequisites**

* 64-bit Linux (OSX and Windows are not yet supported)
* [PMDK](https://github.com/pmem/pmdk) - Persistent Memory Development Kit
* [libpmemobj-cpp](https://github.com/pmem/libpmemobj-cpp) - C++ bindings for PMDK
* [memkind](https://github.com/memkind/memkind) - Volatile memory manager
* [RapidJSON](https://github.com/tencent/rapidjson) - JSON parser
* [TBB](https://github.com/01org/tbb) - Thread Building Blocks

**Building and running tests**

After cloning sources from GitHub, use provided `make` targets for building and running
tests.

```sh
git clone https://github.com/pmem/pmemkv
cd pmemkv

make                    # build everything and run all tests
make clean              # remove all build & temporary files
make build              # build shared library without running tests
make test               # rebuild shared library and run all tests
```

**Managing shared library**

To package `pmemkv` as a shared library and install on your system:
 
```sh
sudo make install       # install to /usr/local/{include,lib}
sudo make uninstall     # remove shared library and headers
```

To install this library into other locations, use the `prefix` variable like this:

```sh
sudo make install prefix=/usr/local
sudo make uninstall prefix=/usr/local
```

**Out-of-source builds**

If the standard build does not suit your needs, create your own
out-of-source build and run tests like this:

```sh
cd ~
mkdir mybuild
cd mybuild
cmake ~/pmemkv
make
PMEM_IS_PMEM_FORCE=1 ./pmemkv_test
```

<a name="fedora"></a>

Installing on Fedora
--------------------

Install required packages:

```sh
su -c 'dnf install autoconf automake cmake daxctl-devel doxygen gcc gcc-c++ libtool ndctl-devel numactl-devel rapidjson-devel tbb-devel'
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

Build pmemkv:

```sh
cd ~
git clone https://github.com/pmem/pmemkv
cd pmemkv
make
su -c 'make install'
```

<a name="ubuntu"></a>

Installing on Ubuntu
--------------------

Install required packages:

```sh
sudo apt install autoconf automake cmake libdaxctl-dev doxygen gcc g++ libtool libndctl-dev libnuma-dev rapidjson-dev libtbb-dev
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

Build pmemkv:

```sh
cd ~
git clone https://github.com/pmem/pmemkv
cd pmemkv
make
sudo make install
```

<a name="experimental"></a>

Using Experimental Engines
--------------------------

First build or install `pmemkv` as described above.

Install client libraries for Memcached:

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

Edit `CMakeLists.txt` to enable experimental features:

```sh
option(EXPERIMENTAL "use experimental features" ON)
```

Now `make` will include experimental engines and other features that are not available by default.

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
