pmemkv	{#mainpage}
===========================

Key/Value Datastore for Persistent Memory

All general information about (lib)pmemkv can be found on webstie:
https://pmem.io/pmemkv

Here user can found description of C++ API.

### Important classes/functions ###

 * class pmem::kv::db, with its functions:
	* pmem::kv::db::open() to create/open file with data
	* pmem::kv::db::put() to insert data into database
	* pmem::kv::db::get() to read data back
	* pmem::kv::db::remove() to get rid of selected key (ant its value)
 * class pmem::kv::config to setup parameters to open/create database
