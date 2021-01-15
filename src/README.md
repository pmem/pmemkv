pmemkv	{#mainpage}
===========================

Key/Value Datastore for Persistent Memory

All general information about (lib)pmemkv can be found on the webstie:
https://pmem.io/pmemkv

Main code repository location:
https://github.com/pmem/pmemkv
(it contains full sources of all examples' snippets from this documentation)

Here user can found description of C++ API.

### Important classes/functions ###

 * class pmem::kv::db, with its functions:
	* pmem::kv::db::open() to create/open file with data
	* pmem::kv::db::put() to insert data into database
	* pmem::kv::db::get() to read data back
	* pmem::kv::db::remove() to get rid of selected key (ant its value)
 * class pmem::kv::config to setup parameters to open/create database
 * class pmem::kv::tx for grouping operations into a single atomic action
 * class pmem::kv::db::iterator to iterate over records in db
 * enum class pmem::kv::status containing all possible statuses returned by
	most of public functions
