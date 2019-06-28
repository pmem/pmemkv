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

`int pmemkv_open(const char *engine, pmemkv_config *config, pmemkv_db **db);`

:	Opens pmemkv database and stores pointer to *pmemkv_db* instance in `*db`.
	`engine` parameter specifies which engine should be used (see **libpmemkv**(7) for list of available engines).
	`config` parameter specifies configuration (see **libpmemkv_config**(3) for details).

`void pmemkv_close(pmemkv_db *kv);`

:	Closes pmemkv database.

`int pmemkv_count_all(pmemkv_db *db, size_t *cnt);`

:	Writes number of all elements in `db` to variable `*cnt`.

`int pmemkv_count_above(pmemkv_db *db, const char *k, size_t kb, size_t *cnt);`

:	Writes number of all elements in `db` which keys compare greater than
	specified key to `*cnt`. Pointer to key to compare is stored in `k` and size in `kb`.
	Data pointed by `k` can contain null characters.

`int pmemkv_count_below(pmemkv_db *db, const char *k, size_t kb, size_t *cnt);`

:	Writes number of all elements in `db` which keys compare less than
	specified key to `*cnt`. Pointer to key to compare is stored in `k` and size in `kb`.
	Data pointed by `k` can contain null characters.

`int pmemkv_count_between(pmemkv_db *db, const char *k1, size_t kb1, const char *k2, size_t kb2, size_t *cnt);`

:	Writes number of all elements in `db` which keys compare less than key1 and greater than key2
	to `*cnt`. Pointer to key1 is stored in `k1` and size in `kb1`. Pointer to key2 is stored in `k2` and size in `kb2`.
	Data pointed by `k1` and `k2` can contain null characters.

`int pmemkv_get_all(pmemkv_db *db, pmemkv_get_kv_callback *c, void *arg);`

:	Calls callback specified by `c` on every element stored in `db`. Arguments
	passed to `c` are: pointer to key, size of the key, pointer to value, size of
	the value and `arg` specified by the user.

`int pmemkv_get_above(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_kv_callback *c, void *arg);`

:	Calls callback specified by `c` on every element stored in `db` which keys compare greater than
	specified key. Pointer to key to compare is stored in `k` and size in `kb`. Arguments
	passed to `c` are: pointer to key, size of the key, pointer to value, size of
	the value and `arg` specified by the user.

`int pmemkv_get_below(pmemkv_db *db, const char *k, size_t kb, pmemkv_get_kv_callback *c, oid *arg);`

:	Calls callback specified by `c` on every element stored in `db` which keys compare less than
	specified key. Pointer to key to compare is stored in `k` and size in `kb`. Arguments
	passed to `c` are: pointer to key, size of the key, pointer to value, size of
	the value and `arg` specified by the user.

`int pmemkv_get_between(pmemkv_db *db, const char *k1, size_t kb1, const char *k2, size_t kb2, pmemkv_get_kv_callback *c, void *arg);`

:	Calls callback specified by `c` on every element stored in `db` which keys compare less than
	key1 and greater than key2. Pointer to key1 is stored in `k1` and size in `kb1`.
	Pointer to key2 is stored in `k2` and size in `kb2`. Arguments
	passed to `c` are: pointer to key, size of the key, pointer to value, size of
	the value and `arg` specified by the user.

# EXAMPLE #


# SEE ALSO #

**libpmemkv**(7), **libpmemkv_config**(3) and **<http://pmem.io>**
