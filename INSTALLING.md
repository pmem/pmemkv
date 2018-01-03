# Installing pmemkv
Key/Value Datastore for Persistent Memory

*This is experimental pre-release software and should not be used in
production systems. APIs and file formats may change at any time without
preserving backwards compatibility. All known issues and limitations
are logged as GitHub issues.*

Contents
--------

<ul>
<li><a href="#fedora_stable_pmdk">Installing on Fedora (Stable PMDK)</a></li>
<li><a href="#fedora_latest_pmdk">Installing on Fedora (Latest PMDK)</a></li>
<li><a href="#building_from_sources">Building from Sources</a></li>
<li><a href="#device_dax">Converting Filesystem DAX to Device DAX</a></li>
<li><a href="#filesystem_dax">Converting Device DAX to Filesystem DAX</a></li>
<li><a href="#pool_set">Using a Pool Set</a></li>
</ul>

<a name="fedora_stable_pmdk"></a>

Installing on Fedora (Stable PMDK)
----------------------------------

Install required packages:

```
su -c 'dnf install autoconf cmake gcc-c++ libpmemobj++-devel nvml-tools'
```

Build pmemkv: (skip proxy steps if you have none)

```
git config --global http.proxy <YOUR PROXY>
cd ~
git clone https://github.com/pmem/pmemkv.git
cd pmemkv
export HTTP_PROXY="<YOUR PROXY>"
export HTTPS_PROXY="<YOUR PROXY>"
make
```
<a name="fedora_latest_pmdk"></a>

Installing on Fedora (Latest PMDK)
----------------------------------

Install required packages:

```
su -c 'dnf install autoconf cmake gcc-c++ glib2-devel'
```

Install latest PMDK: (skip proxy steps if you have none)

```
git config --global http.proxy <YOUR PROXY>
cd ~
git clone https://github.com/pmem/pmdk.git
cd pmdk
su -c 'make install'
```

Build pmemkv: (skip proxy steps if you have none)

```
cd ~
git clone https://github.com/pmem/pmemkv.git
cd pmemkv
export HTTP_PROXY="<YOUR PROXY>"
export HTTPS_PROXY="<YOUR PROXY>"
export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig
make
```

<a name="building_from_sources"></a>

Building from Sources
---------------------

**Prerequisites**

* 64-bit Linux (OSX and Windows are not yet supported)
* [PMDK](https://github.com/pmem/pmdk) (install binary package or build from source)
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
make bench              # build and run benchmarks
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

Converting Filesystem DAX to Device DAX
---------------------------------------

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

To convert to device DAX, first unmount your existing filesystem:

```
umount /mnt/<your-mapped-directory>

mount | grep dax   <-- should return nothing now
```

Then convert the namespace to DAX: (name comes from ndctl output above)

```
sudo ndctl create-namespace -e namespace2.0 -f -m dax

{
  "dev":"namespace2.0",
  "mode":"dax",
  "size":266352984064,
  "uuid":"ac489425-ce96-43de-a728-e6d35bf44e11"
}

ll /dev/da*

crw------- 1 root root 238, 0 Jun  8 11:38 /dev/dax2.0
```

Next clear and initialize the DAX device:

```
pmempool rm --verbose /dev/dax2.0
pmempool create --layout pmemkv obj /dev/dax2.0
```

Now pass the device DAX device as a parameter to `pmemkv` like this:

```
./pmemkv_bench --db=/dev/dax2.0
```

<a name="filesystem_dax"></a>

Converting Device DAX to Filesystem DAX
---------------------------------------

If `ndctl` reports 'dax' mode, as shown below, then you are using device DAX.

```
[root@ch6_crpnp_81 ~]# ndctl list
{
  "dev":"namespace2.0",
  "mode":"dax",
  "size":<your-size>,
  "uuid":"<your-uuid>"
}
```

To convert to filesystem DAX:

```
[root@ch6_crpnp_81 ~]# sudo ndctl create-namespace -e namespace2.0 -f -m memory
{
  "dev":"namespace2.0",
  "mode":"memory",
  "size":263182090240,
  "uuid":"ef45a74a-6d43-458b-b186-87ff83e86ecc",
  "blockdev":"pmem2"
}
```

Now create and mount filesystem: (using block device name from previous step)

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

Now pass the filesystem DAX device as a parameter to `pmemkv` like this:

```
PMEM_IS_PMEM_FORCE=1 ./pmemkv_bench --db=/mnt/pmem/pmemkv
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

Now pass the pool set as a parameter to `pmemkv` like this:

```
PMEM_IS_PMEM_FORCE=1 ./pmemkv_bench --db=~/pmemkv.poolset
```
