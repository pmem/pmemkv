#include "test_suits.h"

Basic basic_tests[] = {
	{
	.path = "/dev/shm",
	.size = (uint64_t)(1024 * 1024 * 1024),
	.force_create = 1,
	.engine = "vsmap",
	.key_length = 100,
        .value_length = 100,
	.test_data_size = 20,
	.name = "ShmVsmap100bKey100bValue",
	},
	{
	.path = "/dev/shm",
	.size = (uint64_t)(1024 * 1024 * 1024),
	.force_create = 1,
	.engine = "vcmap",
	.key_length = 100,
        .value_length = 100,
	.test_data_size = 20,
	.name = "ShmVcmap100bKey100bValue",
	},
	{
	.path = "/dev/shm",
	.size = (uint64_t)(1024 * 1024 * 1024),
	.force_create = 1,
	.engine = "blackhole",
	.key_length = 100,
        .value_length = 100,
	.test_data_size = 20,
	.name = "ShmBlackhole100bKey100bValue",
	}
};

