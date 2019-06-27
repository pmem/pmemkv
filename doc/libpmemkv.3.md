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
[SEE ALSO](#see-also)<br />


# NAME #

**pmemkv** - Key/Value Datastore for Persistent Memory

# SYNOPSIS #

```
#include <libpmemkv.h>

typedef int pmemkv_get_kv_callback(const char *key, size_t keybytes, const char *value,
				    size_t valuebytes, void *arg);
typedef void pmemkv_get_v_callback(const char *value, size_t valuebytes, void *arg);

pmemkv_config *pmemkv_config_new(void);
void pmemkv_config_delete(pmemkv_config *config);
int pmemkv_config_put_data(pmemkv_config *config, const char *key, const void *value,
			   size_t value_size);
int pmemkv_config_put_object(pmemkv_config *config, const char *key, void *value,
			     void (*deleter)(void *));
int pmemkv_config_put_uint64(pmemkv_config *config, const char *key, uint64_t value);
int pmemkv_config_put_int64(pmemkv_config *config, const char *key, int64_t value);
int pmemkv_config_put_double(pmemkv_config *config, const char *key, double value);
int pmemkv_config_put_string(pmemkv_config *config, const char *key, const char *value);
int pmemkv_config_get_data(pmemkv_config *config, const char *key, const void **value,
			   size_t *value_size);
int pmemkv_config_get_object(pmemkv_config *config, const char *key, const void **value);
int pmemkv_config_get_uint64(pmemkv_config *config, const char *key, uint64_t *value);
int pmemkv_config_get_int64(pmemkv_config *config, const char *key, int64_t *value);
int pmemkv_config_get_double(pmemkv_config *config, const char *key, double *value);
int pmemkv_config_get_string(pmemkv_config *config, const char *key, const char **value);
int pmemkv_config_from_json(pmemkv_config *config, const char *jsonconfig);

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

For general pmemkv description, engines descriptions and example see **libpmemkv**(7).

# DESCRIPTION #

...

## CONFIGURATION ##

`pmemkv_config *pmemkv_config_new(void);`

Creates an instance of configuration for pmemkv database. This configuration
stores mapping of keys to values, where a value can be:
* uint64_t
* int64_t
* double
* c-style string
* binary data
* pointer to an object (with accompanying deleter function)

Key should be a null-terminated string.

On failure, NULL is returned.

`void pmemkv_config_delete(pmemkv_config *config);`

Deletes pmemkv_config. Should be called ONLY for configs which were not
passed to pmemkv_open (as this function moves ownership of the config to
the database).

`int pmemkv_config_put_uint64(pmemkv_config *config, const char *key, uint64_t value);`
`int pmemkv_config_put_int64(pmemkv_config *config, const char *key, int64_t value);`
`int pmemkv_config_put_double(pmemkv_config *config, const char *key, double value);`

Puts value `value` to pmemkv_config at key `key`. 

`int pmemkv_config_put_string(pmemkv_config *config, const char *key, const char *value);`

Puts null-terminated string to pmemkv_config.

`int pmemkv_config_put_data(pmemkv_config *config, const char *key, const void *value, size_t value_size);`

Puts copy of binary data pointed to by `value` to pmemkv_config. `value_size`
specifies size of the data.

`int pmemkv_config_put_object(pmemkv_config *config, const char *key, void *value, void (*deleter)(void *));`

Puts pointer `value` to pmemkv_config. `value` can point to any arbitrary
object. `deleter` parameter specified function which is called for `value`
when config is destroyed (using pmemkv_config_delete).

`int pmemkv_config_get_uint64(pmemkv_config *config, const char *key, uint64_t *value);`
`int pmemkv_config_get_int64(pmemkv_config *config, const char *key, int64_t *value);`
`int pmemkv_config_get_double(pmemkv_config *config, const char *key, double *value);`

Gets value of element with key `key`. Value is copied to variable pointed to by
`value`.

`int pmemkv_config_get_string(pmemkv_config *config, const char *key, const char **value);`

Gets pointer to null-terminated string. String is not copied. After successful call
`value` points to string stored in pmemkv_config.

`int pmemkv_config_get_data(pmemkv_config *config, const char *key, const void **value, size_t *value_size);`

Gets pointer to binary data. Data is not copied. After successful call
`value` points to data stored in pmemkv_config and `value_size` holds size of the data.

`int pmemkv_config_get_object(pmemkv_config *config, const char *key, const void **value);`

Gets pointer to an object. After successful call, `value` points the object.

`int pmemkv_config_from_json(pmemkv_config *config, const char *jsonconfig);`

Parses JSON string and puts all elements found in JSON in `config`. Allowed types
in JSON strings and their corresponding types in pmemkv_config are:
* `number` -> int64 for integral types and double for floating point,
* `string` -> const char *,
* `object` (another JSON string) -> pointer to pmemkv_config (can be obtained using pmemkv_config_get_object)
* `True`, `False` -> int64

Element stored in pmemkv_config, which were putted using a specific function can be obtained
only using corresponding pmemkv_config_get_ function (for example, elements putted using pmemkv_config_put_double
can only get obtained using pmemkv_config_get_double). Exception from this rule
are functions for uint64 and int64. If value putted by pmemkv_config_put_int64 is in uint64_t range
it can be obtained using pmemkv_config_get_uint64 and vice versa.

##### Errors #####

Each function, except for *pmemkv_config_new()*, *pmemkv_config_delete()* and
*pmemkv_close()* return status. Allowed return values are:

* PMEMKV_STATUS_OK - no error
* PMEMKV_STATUS_FAILED - unspecified error
* PMEMKV_STATUS_NOT_FOUND - element not found
* PMEMKV_STATUS_NOT_SUPPORTED - functions not supported in current engine
* PMEMKV_STATUS_INVALID_ARGUMENT - argument to function has wrong value
* PMEMKV_STATUS_CONFIG_PARSING_ERROR - parsing data to config failed
* PMEMKV_STATUS_CONFIG_TYPE_ERROR - element has different type than expected


# SEE ALSO #

**libpmemkv**(7) and **<http://pmem.io>**
