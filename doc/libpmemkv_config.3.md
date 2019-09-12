---
layout: manual
Content-Style: 'text/css'
title: _MP(PMEMKV_CONFIG, 3)
collection: libpmemkv
header: PMEMKV_CONFIG
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

[comment]: <> (libpmemkv_config.3 -- man page for libpmemkv configuration API)

[NAME](#name)<br />
[SYNOPSIS](#synopsis)<br />
[DESCRIPTION](#description)<br />
[EXAMPLE](#example)<br />
[SEE ALSO](#see-also)<br />


# NAME #

**pmemkv_config** - Configuration API for libpmemkv

# SYNOPSIS #

```c
#include <libpmemkv.h>

pmemkv_config *pmemkv_config_new(void);
void pmemkv_config_delete(pmemkv_config *config);
int pmemkv_config_put_data(pmemkv_config *config, const char *key, const void *value,
			size_t value_size);
int pmemkv_config_put_object(pmemkv_config *config, const char *key, void *value,
			void (*deleter)(void *));
int pmemkv_config_put_uint64(pmemkv_config *config, const char *key, uint64_t value);
int pmemkv_config_put_int64(pmemkv_config *config, const char *key, int64_t value);
int pmemkv_config_put_string(pmemkv_config *config, const char *key, const char *value);
int pmemkv_config_get_data(pmemkv_config *config, const char *key, const void **value,
			size_t *value_size);
int pmemkv_config_get_object(pmemkv_config *config, const char *key, void **value);
int pmemkv_config_get_uint64(pmemkv_config *config, const char *key, uint64_t *value);
int pmemkv_config_get_int64(pmemkv_config *config, const char *key, int64_t *value);
int pmemkv_config_get_string(pmemkv_config *config, const char *key, const char **value);
```

For general pmemkv description and engines descriptions see **libpmemkv**(7).
For description of pmemkv core API see **libpmemkv**(3).

# DESCRIPTION #

pmemkv database is configured using pmemkv_config structure. It stores mappings
of keys (null-terminated strings) to values. A value can be:

+ **uint64_t**
+ **int64_t**
+ **c-style string**
+ **binary data**
+ **pointer to an object** (with accompanying deleter function)

List of options which are required by pmemkv database is specific to an engine.
Every engine has documented all supported config parameters (please see **libpmemkv**(7) for details).

`pmemkv_config *pmemkv_config_new(void);`
:	Creates an instance of configuration for pmemkv database.

	On failure, NULL is returned.

`void pmemkv_config_delete(pmemkv_config *config);`
:	Deletes pmemkv_config. Should be called ONLY for configs which were not
	passed to pmemkv_open (as this function moves ownership of the config to
	the database).

`int pmemkv_config_put_uint64(pmemkv_config *config, const char *key, uint64_t value);`

:	Puts uint64_t value `value` to pmemkv_config at key `key`.

`int pmemkv_config_put_int64(pmemkv_config *config, const char *key, int64_t value);`

:	Puts int64_t value `value` to pmemkv_config at key `key`.

`int pmemkv_config_put_string(pmemkv_config *config, const char *key, const char *value);`

:	Puts null-terminated string to pmemkv_config. The string is copied to the config.

`int pmemkv_config_put_data(pmemkv_config *config, const char *key, const void *value, size_t value_size);`

:	Puts copy of binary data pointed by `value` to pmemkv_config. `value_size`
	specifies size of the data.

`int pmemkv_config_put_object(pmemkv_config *config, const char *key, void *value, void (*deleter)(void *));`

:	Puts `value` to pmemkv_config. `value` can point to arbitrary object.
	`deleter` parameter specifies function which will be called for `value`
	when the config is destroyed (using pmemkv_config_delete).

`int pmemkv_config_get_uint64(pmemkv_config *config, const char *key, uint64_t *value);`

:	Gets value of a config item with key `key`. Value is copied to variable pointed by
	`value`.

`int pmemkv_config_get_int64(pmemkv_config *config, const char *key, int64_t *value);`

:	Gets value of a config item with key `key`. Value is copied to variable pointed by
	`value`.

`int pmemkv_config_get_string(pmemkv_config *config, const char *key, const char **value);`

:	Gets pointer to a null-terminated string. The string is not copied. After successful call
	`value` points to string stored in pmemkv_config.

`int pmemkv_config_get_data(pmemkv_config *config, const char *key, const void **value, size_t *value_size);`

:	Gets pointer to binary data. Data is not copied. After successful call
	`*value` points to data stored in pmemkv_config and `value_size` holds size of the data.

`int pmemkv_config_get_object(pmemkv_config *config, const char *key, const void **value);`

:	Gets pointer to an object. After successful call, `*value` points to the object.

Config items stored in pmemkv_config, which were put using a specific function can be obtained
only using corresponding pmemkv_config_get_ function (for example, config items put using pmemkv_config_put_object
can only be obtained using pmemkv_config_get_object). Exception from this rule
are functions for uint64 and int64. If value put by pmemkv_config_put_int64 is in uint64_t range
it can be obtained using pmemkv_config_get_uint64 and vice versa.

## ERRORS ##

Each function, except for *pmemkv_config_new()* and *pmemkv_config_delete()*, returns status.
Possible return values are:

+ **PMEMKV_STATUS_OK** -- no error
+ **PMEMKV_STATUS_FAILED** -- unspecified error
+ **PMEMKV_STATUS_NOT_FOUND** -- record (or config item) not found
+ **PMEMKV_STATUS_CONFIG_PARSING_ERROR** -- parsing data to config failed
+ **PMEMKV_STATUS_CONFIG_TYPE_ERROR** -- config item has different type than expected

# EXAMPLE #

```c
#include <libpmemkv.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* deleter for int pointer */
void free_int_ptr(void *ptr) {
	free(ptr);
}

int main(){
	pmemkv_config *config = pmemkv_config_new();
	assert(config != NULL);

	/* Put int64_t value */
	int status = pmemkv_config_put_int64(config, "size", 1073741824);
	assert(status == PMEMKV_STATUS_OK);

	char buffer[] = "ABC";

	/* Put binary data stored in buffer */
	status = pmemkv_config_put_data(config, "binary", buffer, 3);
	assert(status == PMEMKV_STATUS_OK);

	const void *data;
	size_t data_size;

	/* Get pointer to binary data stored in config */
	status = pmemkv_config_get_data(config, "binary", &data, &data_size);
	assert(status == PMEMKV_STATUS_OK);
	assert(data_size == 3);
	assert(((const char*)data)[0] == 'A');

	int *int_ptr = malloc(sizeof(int));
	*int_ptr = 10;

	/* Put pointer to dynamically allocated object, free_int_ptr is called on pmemkv_config_delete */
	status = pmemkv_config_put_object(config, "int_ptr", int_ptr, &free_int_ptr);
	assert(status == PMEMKV_STATUS_OK);

	int *get_int_ptr;

	/* Get pointer to object stored in config */
	status = pmemkv_config_get_object(config, "int_ptr", &get_int_ptr);
	assert(status == PMEMKV_STATUS_OK);
	assert(*get_int_ptr == 10);

	pmemkv_config_delete(config);

	pmemkv_config *config_from_json = pmemkv_config_new();
	assert(config_from_json != NULL);

	/* Parse JSON and put all items found into config_from_json */
	status = pmemkv_config_from_json(config_from_json,
		"{\"path\":\"/dev/shm\",\
		 \"size\":1073741824,\
		 \"subconfig\":{\
			\"size\":1073741824\
			}\
		}");
	assert(status == PMEMKV_STATUS_OK);

	const char *path;
	status = pmemkv_config_get_string(config_from_json, "path", &path);
	assert(status == PMEMKV_STATUS_OK);
	assert(strcmp(path, "/dev/shm") == 0);

	pmemkv_config *subconfig;

	/* Get pointer to nested configuration "subconfig" */
	status = pmemkv_config_get_object(config_from_json, "subconfig", &subconfig);
	assert(status == PMEMKV_STATUS_OK);

	size_t sub_size;
	status = pmemkv_config_get_uint64(subconfig, "size", &sub_size);
	assert(status == PMEMKV_STATUS_OK);
	assert(sub_size == 1073741824);

	pmemkv_config_delete(config_from_json);
}
```

# SEE ALSO #

**libpmemkv**(7), **libpmemkv**(3) and **<http://pmem.io>**
