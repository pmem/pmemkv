# Installing pmemkv
Key/Value Datastore for Persistent Memory

*This is experimental pre-release software and should not be used in
production systems. APIs and file formats may change at any time without
preserving backwards compatibility. All known issues and limitations
are logged as GitHub issues.*

Contents
--------

<ul>
<li><a href="#fedora">Installing on Fedora</a></li>
<li><a href="#building_from_sources">Building from Sources</a></li>
<li><a href="#device_dax">Configuring Device DAX</a></li>
</ul>

<a name="fedora"></a>

Installing on Fedora
--------------------

Update installed packages first:

```
su -c 'dnf update'
```

Install required packages:

```
su -c 'dnf install autoconf cmake gcc-c++ libpmemobj++-devel nvml-tools'
```

Build pmemkv: (skip proxy steps if you have none)

```
git config --global http.proxy <YOUR PROXY>
git clone https://github.com/pmem/pmemkv.git
cd pmemkv
export HTTP_PROXY="<YOUR PROXY>"
export HTTPS_PROXY="<YOUR PROXY>"
make
```

<a name="building_from_sources"></a>

Building from Sources
---------------------

**Prerequisites**

* 64-bit Linux (OSX and Windows are not yet supported)
* [NVML](https://github.com/pmem/nvml) (install binary package or build from source)
* `make` and `cmake` (version 3.6 or higher)
* `g++` (version 5.4 or higher)

**Building and running tests**

After cloning sources from GitHub, use provided `make` targets for building and running
tests and example programs.

```
git clone https://github.com/pmem/pmemkv.git
cd pmemkv

make                    # build everything and run tests
make test               # build and run unit tests
make example            # build and run simple example
make stress             # build stress test utility
make clean              # remove build files
```

**Managing shared library**

To package `pmemkv` as a shared library and install on your system:
 
```
sudo make install       # install to /usr/local/{include,lib}
sudo make uninstall     # remove shared library and headers
```

To install this library into other locations, use the `prefix` variable like this:

```
sudo make install prefix=/usr/local
sudo make uninstall prefix=/usr/local
```

**Out-of-source builds**

If the standard build does not suit your needs, create your own
out-of-source build and run tests like this:

```
cd ~
mkdir mybuild
cd mybuild
cmake ~/pmemkv
make
PMEM_IS_PMEM_FORCE=1 ./pmemkv_test
```

<a name="device_dax"></a>

Configuring Device DAX
----------------------

If `ndctl` reports memory mode, as shown below, then you are using filesystem DAX.

```
ndctl list

{
  "dev":"namespace2.0",
  "mode":"memory",
  "size":<your-size>,
  "uuid":"<your-uuid>",
  "blockdev":"pmem2"
}
```

Here's how you switch over to device DAX:

```
1. Unmount your existing filesystem DAX mount

umount /mnt/<your-mapped-directory>

mount | grep dax   <-- should return nothing now


2. Convert namespace to DAX (name comes from ndctl output above)

sudo ndctl create-namespace -e namespace2.0 -f -m dax

{
  "dev":"namespace2.0",
  "mode":"dax",
  "size":266352984064,
  "uuid":"ac489425-ce96-43de-a728-e6d35bf44e11"
}


3. Verify DAX device now present

ll /dev/da*

crw------- 1 root root 238, 0 Jun  8 11:38 /dev/dax2.0


4. Clear contents of DAX device

pmempool rm --verbose /dev/dax2.0


5. Initialize pmemkv on DAX device

pmempool create --layout pmemkv obj /dev/dax2.0


6. Reference your DAX device by its path

./pmemkv_stress w /dev/dax2.0 1000

```
