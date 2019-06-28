---
layout: manual
Content-Style: 'text/css'
title: _MP(PMEMKV, 3)
collection: libpmemkv
header: PMEMKV
date: pmemkv version 0.8
...

[comment]: <> (Copyright 2019, Intel Corporation)

[comment]: <> (Redistribution and use in source and binary forms, with or without)
[comment]: <> (modification, are permitted provided that the following conditions)
[comment]: <> (are met:)
[comment]: <> (    * Redistributions of source code must retain the above copyright)
[comment]: <> (      notice, this list of conditions and the following disclaimer.)
[comment]: <> (    * Redistributions in binary form must reproduce the above copyright)
[comment]: <> (      notice, this list of conditions and the following disclaimer in)
[comment]: <> (      the documentation and/or other materials provided with the)
[comment]: <> (      distribution.)
[comment]: <> (    * Neither the name of the copyright holder nor the names of its)
[comment]: <> (      contributors may be used to endorse or promote products derived)
[comment]: <> (      from this software without specific prior written permission.)

[comment]: <> (THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS)
[comment]: <> ("AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT)
[comment]: <> (LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR)
[comment]: <> (A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT)
[comment]: <> (OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,)
[comment]: <> (SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT)
[comment]: <> (LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,)
[comment]: <> (DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY)
[comment]: <> (THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT)
[comment]: <> ((INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE)
[comment]: <> (OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.)

[comment]: <> (libpmemkv.3 -- man page for libpmemkv)

[NAME](#name)<br />
[SYNOPSIS](#synopsis)<br />
[DESCRIPTION](#description)<br />
[EXAMPLE](#example)<br />
[SEE ALSO](#see-also)<br />


# NAME #

**pmemkv** - Key/Value Datastore for Persistent Memory

# SYNOPSIS #

```c
#include <libpmemkv.h>

typedef int pmemkv_get_kv_callback(const char *key, size_t keybytes, const char *value,
				size_t valuebytes, void *arg);
typedef void pmemkv_get_v_callback(const char *value, size_t valuebytes, void *arg);

int pmemkv_open(const char *engine, pmemkv_config *config, pmemkv_db **db);
void pmemkv_close(pmemkv_db *kv);

int pmemkv_count_all(pmemkv_db *db, size_t *cnt);
int pmemkv_count_above(pmemkv_db *db, const char *k, size_t kb, size_t *cnt);
int pmemkv_count_below(pmemkv_db *db, const char *k, size_t kb, size_t *cnt);
int pmemkv_count_between(pmemkv_db *db, const char *k1, size_t kb1, const char *k2,
			 size_t kb2, size_t *cnt);

int pmemkv_get_all(pmemkv_db *db, pmemkv_get_kv_callback *c, void *arg);
int pmemkv_get_above(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_kv_callback *c,
		     void *arg);
int pmemkv_get_below(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_kv_callback *c,
		     void *arg);
int pmemkv_get_between(pmemkv_db *db, const char *k1, size_t kb1, const char *k2,
		       size_t kb2, pmemkv_get_kv_callback *c, void *arg);

int pmemkv_exists(pmemkv_db *db, const char *k, size_t kb);

int pmemkv_get(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_v_callback *c,
	       void *arg);
int pmemkv_get_copy(pmemkv_db *db, const char *k, size_t kb, char *buffer,
		    size_t buffer_size, size_t *value_size);
int pmemkv_put(pmemkv_db *db, const char *k, size_t kb, const char *v, size_t vb);

int pmemkv_remove(pmemkv_db *db, const char *k, size_t kb);

const char *pmemkv_errormsg(void);
```

For pmemkv configuration API description see **libpmemkv_config**(3).
For general pmemkv information, engines descriptions and bindings details see **libpmemkv**(7).

# DESCRIPTION #

Keys and values stored in pmemkv database can be arbitrary binary data and can contain multiple null characters.
Every function which accepts key expects `const char *k` pointer to data and its size as `size_t`.

Some of the functions (mainly range-query API) are not guaranteed to be implemented by all engines.
If an engine does not support a certain function, it will return PMEMKV_STATUS_NOT_SUPPORTED.

`int pmemkv_open(const char *engine, pmemkv_config *config, pmemkv_db **db);`

:	Opens pmemkv database and stores a pointer to *pmemkv_db* instance in `*db`.
	`engine` parameter specifies which engine should be used (see **libpmemkv**(7) for list of available engines).
	`config` parameter specifies configuration (see **libpmemkv_config**(3) for details).

`void pmemkv_close(pmemkv_db *kv);`

:	Closes pmemkv database.

`int pmemkv_count_all(pmemkv_db *db, size_t *cnt);`

:	Stores number of elements in `db` to variable `*cnt`.

`int pmemkv_count_above(pmemkv_db *db, const char *k, size_t kb, size_t *cnt);`

:	Stores number of elements in `db` which keys are greater than
	specified key to `*cnt`. Key is passed as pointer `const char *k` and size `kb`.

`int pmemkv_count_below(pmemkv_db *db, const char *k, size_t kb, size_t *cnt);`

:	Stores number of elements in `db` which keys are less than
	specified key to `*cnt`. Key is passed as pointer `const char *k` and size `kb`.

`int pmemkv_count_between(pmemkv_db *db, const char *k1, size_t kb1, const char *k2, size_t kb2, size_t *cnt);`

:	Stores number of elements in `db` which keys are less than key1 and greater than key2
	to `*cnt`. key1 is passed as pointer `const char *k1` and size `kb1`. key2 is passed as pointer `const char *k2` and size `kb2`.

`int pmemkv_get_all(pmemkv_db *db, pmemkv_get_kv_callback *c, void *arg);`

:	Executes function `c` for every element stored in `db`. Arguments
	passed to callback are: pointer to key, size of the key, pointer to value, size of
	the value and `arg` specified by the user.
	Callback `c` can stop iteration by returning non-zero value. In that case *pmemkv_get_all()* returns
	PMEMKV_STATUS_STOPPED_BY_CV. Returning 0 will continue iteration.

`int pmemkv_get_above(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_kv_callback *c, void *arg);`

:	Executes function `c` for every element stored in `db` which keys are greater than
	specified key. Key is passed as pointer `const char *k` and size `kb`. Arguments
	passed to `c` are: pointer to key, size of the key, pointer to value, size of
	the value and `arg` specified by the user.
	Callback `c` can stop iteration by returning non-zero value. In that case *pmemkv_get_above()* returns
	PMEMKV_STATUS_STOPPED_BY_CV. Returning 0 will continue iteration.

`int pmemkv_get_below(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_kv_callback *c, oid *arg);`

:	Executes function `c` for every element stored in `db` which keys are less than
	specified key. Key is passed as pointer `const char *k` and size `kb`. Arguments
	passed to `c` are: pointer to key, size of the key, pointer to value, size of
	the value and `arg` specified by the user.
	Callback `c` can stop iteration by returning non-zero value. In that case *pmemkv_get_below()* returns
	PMEMKV_STATUS_STOPPED_BY_CV. Returning 0 will continue iteration.

`int pmemkv_get_between(pmemkv_db *db, const char *k1, size_t kb1, const char *k2, size_t kb2, pmemkv_get_kv_callback *c, void *arg);`

:	Executes function `c` for every element stored in `db` which keys are less than
	key1 and greater than key2. key1 is passed as pointer `const char *k1` and size `kb1`.
	key2 is passed as pointer `const char *k2` and size `kb2` Arguments
	passed to `c` are: pointer to key, size of the key, pointer to value, size of
	the value and `arg` specified by the user.
	Callback `c` can stop iteration by returning non-zero value. In that case *pmemkv_get_between()* returns
	PMEMKV_STATUS_STOPPED_BY_CV. Returning 0 will continue iteration.

`int pmemkv_exists(pmemkv_db *db, const char *k, size_t kb);`

:	Checks existence of element with specified key. Key is passed as pointer `const char *k` and size `kb`.
	If element is present PMEMKV_STATUS_OK is returned, otherwise PMEMKV_STATUS_NOT_FOUND is returned.
	Other possible return values are described in *ERRORS* section.

`int pmemkv_get(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_v_callback *c, void *arg);`

:	Executes function `c` on element with specified key. Key is passed as pointer `const char *k` and size `kb`.
	If element is present and no error occurs function returns PMEMKV_STATUS_OK. If element does not exists PMEMKV_STATUS_NOT_FOUND
	is returned. Other possible return values are described in *ERRORS* section.
	Callback `c` is called with following parameters: const pointer to the value, size of the value and `arg` specified by the user.
	The Pointer to value points to the location where data is actually stored (no copy occurs).
	This function is guaranteed to be implemented by all engines.

`int pmemkv_get_copy(pmemkv_db *db, const char *k, size_t kb, char *buffer, size_t buffer_size, size_t *value_size);`

:	Copies value of element with specified key to user provided buffer. Key is passed as pointer `const char *k` and size `kb`.
	`buffer` pointer points to the buffer, `buffer_size` specifies the buffer size and `*value_size` is set by this function
	to size of the value. If provided buffer size is less than value size function returns PMEMKV_STATUS_FAILED. Otherwise, in
	absence of any errors, PMEMKV_STATUS_OK is returned. Other possible return values are described in *ERRORS* section.
	This function is guaranteed to be implemented by all engines.

`int pmemkv_put(pmemkv_db *db, const char *k, size_t kb, const char *v, size_t vb);`

:	Puts data pointed by `v` to pmemkv database. `vb` specifies size of the value. Key is passed as pointer `const char *k` and size `kb`.
	This function copies data to the database. This function is guaranteed to be implemented by all engines.

`int pmemkv_remove(pmemkv_db *db, const char *k, size_t kb);`

:	Removes element with specified key. Key is passed as pointer `const char *k` and size `kb`.
	This function is guaranteed to be implemented by all engines.

`const char *pmemkv_errormsg(void);`

:	Returns human readable string describing last error.

## ERRORS ##

Each function, except for *pmemkv_close()* and *pmemkv_errormsg()* returns status.
Possible return values are:

+ **PMEMKV_STATUS_OK** -- no error
+ **PMEMKV_STATUS_FAILED** -- unspecified error
+ **PMEMKV_STATUS_NOT_FOUND** -- element not found
+ **PMEMKV_STATUS_NOT_SUPPORTED** -- function is not implemented by current engine
+ **PMEMKV_STATUS_INVALID_ARGUMENT** -- argument to function has wrong value
+ **PMEMKV_STATUS_CONFIG_PARSING_ERROR** -- parsing data to config failed
+ **PMEMKV_STATUS_CONFIG_TYPE_ERROR** -- element has different type than expected
+ **PMEMKV_STATUS_STOPPED_BY_CB** -- iteration was stopped by user callback

# EXAMPLE #

```c
#include <assert.h>
#include <libpmemkv.h>
#include <stdio.h>
#include <string.h>

#define MAX_VAL_LEN 64

const char *PATH = "/dev/shm";
const uint64_t SIZE = 1024UL * 1024UL * 1024UL;

int get_kv_callback(const char *k, size_t kb, const char *value, size_t value_bytes,
		     void *arg)
{
	printf("   visited: %s\n", k);

	return 0;
}

int main()
{
	/* Creating config see libpmemkv_config(3).md for more detailed example */
	pmemkv_config *cfg = pmemkv_config_new();
	assert(cfg != NULL);

	int s = pmemkv_config_put_string(cfg, "path", PATH);
	assert(s == PMEMKV_STATUS_OK);
	s = pmemkv_config_put_uint64(cfg, "size", SIZE);
	assert(s == PMEMKV_STATUS_OK);

	/* Opening pmemkv databse with 'vsmap' engine */
	pmemkv_db *db = NULL;
	s = pmemkv_open("vsmap", cfg, &db);
	assert(s == PMEMKV_STATUS_OK);
	assert(db != NULL);

	/* Putting new key */
	const char *key1 = "key1";
	const char *value1 = "value1";
	s = pmemkv_put(db, key1, strlen(key1), value1, strlen(value1));
	assert(s == PMEMKV_STATUS_OK);

	size_t cnt;
	s = pmemkv_count_all(db, &cnt);
	assert(s == PMEMKV_STATUS_OK);
	assert(cnt == 1);

	/* Reading key back */
	char val[MAX_VAL_LEN];
	s = pmemkv_get_copy(db, key1, strlen(key1), val, MAX_VAL_LEN, NULL);
	assert(s == PMEMKV_STATUS_OK);
	assert(!strcmp(val, "value1"));

	/* Iterating existing keys */
	const char *key2 = "key2";
	const char *value2 = "value2";
	const char *key3 = "key3";
	const char *value3 = "value3";
	pmemkv_put(db, key2, strlen(key2), value2, strlen(value2));
	pmemkv_put(db, key3, strlen(key3), value3, strlen(value3));
	pmemkv_get_all(db, &get_kv_callback, NULL);

	/* Removing existing key */
	s = pmemkv_remove(db, key1, strlen(key1));
	assert(s == PMEMKV_STATUS_OK);
	assert(pmemkv_exists(db, key1, strlen(key1)) == PMEMKV_STATUS_NOT_FOUND);

	/* Closing database */
	pmemkv_close(db);

	return 0;
}
```

# SEE ALSO #

**libpmemkv**(7), **libpmemkv_config**(3) and **<http://pmem.io>**
