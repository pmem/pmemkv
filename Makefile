include make_config.mk

all: clean thirdparty example test

clean:
	rm -rf /dev/shm/pmemkv
	rm -rf pmemkv_example pmemkv_stress pmemkv_test

thirdparty:
	rm -rf 3rdparty && mkdir 3rdparty
	wget https://github.com/google/googletest/archive/release-1.7.0.tar.gz -O 3rdparty/gtest.tar.gz
	mkdir 3rdparty/gtest && tar -xzf 3rdparty/gtest.tar.gz -C 3rdparty/gtest --strip 1
	wget https://github.com/pmem/nvml/archive/1.2.tar.gz -O 3rdparty/nvml.tar.gz
	mkdir 3rdparty/nvml && tar -xzf 3rdparty/nvml.tar.gz -C 3rdparty/nvml --strip 1
	sh -c 'cd 3rdparty/nvml && make install prefix=`pwd`'

example:
	$(CXX) $(CXXFLAGS) pmemkv.cc pmemkv_example.cc -o pmemkv_example \
	$(USE_NVML) \
	-O2 -std=c++11 -ldl $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS)
	rm -rf /dev/shm/pmemkv
	PMEM_IS_PMEM_FORCE=1 ./pmemkv_example

stress:
	$(CXX) $(CXXFLAGS) pmemkv.cc pmemkv_stress.cc -o pmemkv_stress \
	$(USE_NVML) \
	-DNDEBUG -O2 -std=c++11 -ldl $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS)
	rm -rf /dev/shm/pmemkv
	PMEM_IS_PMEM_FORCE=1 ./pmemkv_stress

test:
	$(CXX) $(CXXFLAGS) pmemkv.cc pmemkv_test.cc -o pmemkv_test \
	$(USE_NVML) \
	$(USE_GTEST) \
	-O2 -std=c++11 -ldl $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS)
	rm -rf /dev/shm/pmemkv
	PMEM_IS_PMEM_FORCE=1 ./pmemkv_test
