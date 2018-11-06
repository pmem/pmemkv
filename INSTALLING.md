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
<li><a href="#device_dax">Converting Filesystem DAX to Device DAX</a></li>
<li><a href="#filesystem_dax">Converting Device DAX to Filesystem DAX</a></li>
<li><a href="#pool_set">Using a Pool Set</a></li>
</ul>

<a name="building_from_sources"></a>

Building from Sources
---------------------

**Prerequisites**

* 64-bit Linux (OSX and Windows are not yet supported)
* [memkind](https://github.com/memkind/memkind) - Volatile memory manager
* [PMDK](https://github.com/pmem/pmdk) - Persistent Memory Development Kit
* [libpmemobj-cpp](https://github.com/pmem/libpmemobj-cpp) - C++ bindings for PMDK
* [RapidJSON](https://github.com/tencent/rapidjson) - JSON parser

**Building and running tests**

After cloning sources from GitHub, use provided `make` targets for building and running
tests.

```
git clone https://github.com/pmem/pmemkv
cd pmemkv

make                    # build everything and run tests
make test               # build and run unit tests
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

<a name="fedora"></a>

Installing on Fedora
--------------------

Install required packages:

```
su -c 'dnf install autoconf automake cmake daxctl-devel doxygen gcc gcc-c++ libtool ndctl-devel numactl-devel rapidjson-devel'
```

Configure for proxy if necessary:

```
git config --global http.proxy <YOUR PROXY>
export HTTP_PROXY="<YOUR PROXY>"
export HTTPS_PROXY="<YOUR PROXY>"
```

Install latest PMDK:

```
cd ~
git clone https://github.com/pmem/pmdk
cd pmdk
make -j8
su -c 'make install'
export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig
```

Install latest PMDK C++ bindings:

```
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

```
cd ~
git clone https://github.com/memkind/memkind
cd memkind
./build.sh
su -c 'make install'
```

Build pmemkv:

```
cd ~
git clone https://github.com/pmem/pmemkv
cd pmemkv
make
su -c 'make install'
```

<a name="device_dax"></a>

Converting Filesystem DAX to Device DAX
---------------------------------------

If `ndctl` shows results like those below, then you are using filesystem DAX.

```
ndctl list

{
  "dev":"namespace<X>.0",
  "mode":"fsdax",
  "size":<your-size>,
  "uuid":"<your-uuid>",
  "blockdev":"pmem<X>"
}
```

To convert to device DAX, first unmount your existing filesystem:

```
umount /mnt/<your-mapped-directory>

mount | grep dax   <-- should return nothing now
```

Then convert the namespace to device DAX:

```
sudo ndctl create-namespace -e namespace<X>.0 -f -m devdax

{
  "dev":"namespace<X>.0",
  "mode":"devdax",
  "size":<your-size>,
  "uuid":"<your-uuid>",
  "daxregion":{
    "id":<X>,
    "size":<your-size>,
    "devices":[
      {
        "chardev":"dax<X>.0",
        "size":<your-size>
      }
    ]
  }
}

ll /dev/da*

crw------- 1 root root 238, 0 Jun  8 11:38 /dev/dax2.0
```

Next clear and initialize the DAX device: (using chardev from previous step)

```
pmempool rm --verbose /dev/dax2.0
pmempool create --layout pmemkv obj /dev/dax2.0
```

<a name="filesystem_dax"></a>

Converting Device DAX to Filesystem DAX
---------------------------------------

If `ndctl` shows results like those below, then you are using device DAX.

```
[root@ch6_crpnp_81 ~]# ndctl list
{
  "dev":"namespace<X>.0",
  "mode":"devdax",
  "size":<your-size>,
  "uuid":"<your-uuid>",
  "chardev":"dax<X>.0"
}
```

To convert to filesystem DAX:

```
[root@ch6_crpnp_81 ~]# sudo ndctl create-namespace -e namespace<X>.0 -f -m fsdax
{
  "dev":"namespace<X>.0",
  "mode":"fsdax",
  "size":<your-size>,
  "uuid":"<your-uuid>",
  "blockdev":"pmem<X>"
}
```

Now create and mount filesystem: (using blockdev from previous step)

```
[root@ch6_crpnp_81 ~]# sudo mkfs.ext4 /dev/pmem1
mke2fs 1.43.3 (04-Sep-2016)
Creating filesystem with 64253440 4k blocks and 16064512 inodes
Filesystem UUID: f6050213-c006-4e9a-be0b-c4bce5f1a531
Superblock backups stored on blocks:
        32768, 98304, 163840, 229376, 294912, 819200, 884736, 1605632, 2654208,
        4096000, 7962624, 11239424, 20480000, 23887872

Allocating group tables: done
Writing inode tables: done
Creating journal (262144 blocks): done
Writing superblocks and filesystem accounting information: done

[root@ch6_crpnp_81 ~]# mount -o dax /dev/pmem1 /mnt/pmem
```

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

```
pmempool create --layout pmemkv obj ~/pmemkv.poolset
```
