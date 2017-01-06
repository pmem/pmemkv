include make_config.mk

all: example test

example:
	$(CXX) $(CXXFLAGS) pmemkv.cc pmemkv_example.cc -o pmemkv_example -I../../include \
	-O2 -std=c++11 -ldl $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)
	rm -rf /dev/shm/pmemkv
	PMEM_IS_PMEM_FORCE=1 ./pmemkv_example

test:
	$(CXX) $(CXXFLAGS) pmemkv.cc third-party/gtest-1.7.0/fused-src/gtest/gtest-all.cc  \
	pmemkv_test.cc -o pmemkv_test -I../../include -Ithird-party/gtest-1.7.0/fused-src \
	-O2 -std=c++11 -ldl $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)
	rm -rf /dev/shm/pmemkv
	PMEM_IS_PMEM_FORCE=1 ./pmemkv_test

stress:
	$(CXX) $(CXXFLAGS) pmemkv.cc pmemkv_stress.cc -o pmemkv_stress -I../../include \
	-DNDEBUG -O2 -std=c++11 -ldl $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS)
	rm -rf /dev/shm/pmemkv
	PMEM_IS_PMEM_FORCE=1 ./pmemkv_stress

clean:
	rm -rf /dev/shm/pmemkv
	rm -rf pmemkv_example pmemkv_stress pmemkv_test
