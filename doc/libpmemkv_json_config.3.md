---
layout: manual
Content-Style: 'text/css'
title: _MP(PMEMKV_JSON_CONFIG, 3)
collection: libpmemkv
header: PMEMKV_JSON_CONFIG
secondary_title: pmemkv_json_config
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

[comment]: <> (libpmemkv_json_config.3 -- man page for libpmemkv_json_config configuration API)

[NAME](#name)<br />
[SYNOPSIS](#synopsis)<br />
[DESCRIPTION](#description)<br />
[EXAMPLE](#example)<br />
[SEE ALSO](#see-also)<br />


# NAME #

**pmemkv_json_config** - helper configuration API for libpmemkv

# SYNOPSIS #

```c
#include <libpmemkv_json_config.h>

int pmemkv_config_from_json(pmemkv_config *config, const char *jsonconfig);
const char *pmemkv_config_from_json_errormsg(void);
```

For general description of pmemkv and available engines see **libpmemkv**(7).
For description of pmemkv core API see **libpmemkv**(3).
For description of configuration API for libpmemkv see **libpmemkv_config**(3).

# DESCRIPTION #

pmemkv_json_config is a helper library that provides two functions:

`int pmemkv_config_from_json(pmemkv_config *config, const char *jsonconfig);`

:	Parses JSON string and puts all items found in JSON into `config`. Allowed types
	in JSON strings and their corresponding types in pmemkv_config are:
	+ **number** -- int64 or uint64
	+ **string** -- const char *
	+ **object** -- (another JSON string) -> pointer to pmemkv_config (can be obtained using pmemkv_config_get_object)
	+ **True**, **False** -- int64

`const char *pmemkv_config_from_json_errormsg(void);`

:	Returns a human readable string describing the last error.

The 'pmemkv_config_from_json' function depends on RapidJSON library
what is the direct cause of the creation of this small library.

The building of this library is enabled by default.
It can be disabled by setting the **BUILD_JSON_CONFIG** CMake variable to OFF:

```sh
cmake .. -DBUILD_JSON_CONFIG=OFF
```

## ERRORS ##

The *pmemkv_config_from_json()* function returns status. Possible return values are:

+ **PMEMKV_STATUS_OK** -- no error
+ **PMEMKV_STATUS_UNKNOWN_ERROR** -- unknown error
+ **PMEMKV_STATUS_CONFIG_PARSING_ERROR** -- parsing config data failed

# EXAMPLE #

An example can be found in **libpmemkv_config**(3).

# SEE ALSO #

**libpmemkv**(7), **libpmemkv**(3), **libpmemkv_config**(3) and **<https://pmem.io>**
