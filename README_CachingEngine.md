#### Caching engine depends on remote service running Redis/Memcached. If the respective libraries are not installed you will get an error while building.  To install the libraries follow these steps.

#### 1.  ACL (for Redis)


Follow [this](https://www.digitalocean.com/community/tutorials/how-to-install-and-configure-redis-on-ubuntu-16-04) link to install Redis. Follow steps till User creation from the link(Create the Redis User, Group and Directories)

Follow these steps to install Redis libraries :-

- git clone https://github.com/acl-dev/acl.git
- cd acl/lib_acl_cpp
- sudo make
- cd acl/lib_acl
- sudo make
- cd acl/lib_protocol
- sudo make

	  
#### 2. libmemcached (For Memcached)


Follow [this](https://www.digitalocean.com/community/tutorials/how-to-install-and-secure-memcached-on-ubuntu-16-04) link to install Memcached. Follow the steps till Adding Autorized Users and Allowing Access over the private Network. 

Follow these steps to install memcached libraries :-

- wget https://launchpad.net/libmemcached/1.0/0.21/+download/libmemcached-0.21.tar.gz
- tar -xvf libmemcached-0.21.tar.gz
- cd libmemcached-0.21
- ./configure	
-  make && make install


#### 3. Provide the ACL and libmemcached library paths mentioned below in the CMakeLists.txt

- set(LIB_ACL $ENV{HOME}/acl/lib_acl)
- set(LIB_ACL_CPP $ENV{HOME}/acl/lib_acl_cpp)
- set(MEMCACHED_INCLUDE $ENV{HOME}/libmemcached-0.21)
