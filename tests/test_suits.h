#ifndef TEST_SUITS_H
#define TEST_SUITS_H

struct Basic {
	const char *path;
	uint64_t size;
	uint64_t force_create;
	const char *engine;
	size_t key_length, value_length;
	size_t test_data_size;
	std::string name;
};


#endif //TEST_SUITS_H
