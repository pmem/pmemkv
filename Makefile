include make_config.mk

all: clean thirdparty example test

reset:
	rm -rf /dev/shm/pmemkv /tmp/pmemkv

clean: reset
	rm -rf libpmemkv.so pmemkv_example pmemkv_stress pmemkv_test

thirdparty:
	rm -rf 3rdparty && mkdir 3rdparty
	wget https://github.com/google/googletest/archive/release-1.7.0.tar.gz -O 3rdparty/gtest.tar.gz
	mkdir 3rdparty/gtest && tar -xzf 3rdparty/gtest.tar.gz -C 3rdparty/gtest --strip 1
	wget https://github.com/pmem/nvml/archive/1.2.tar.gz -O 3rdparty/nvml.tar.gz
	mkdir 3rdparty/nvml && tar -xzf 3rdparty/nvml.tar.gz -C 3rdparty/nvml --strip 1
	sh -c 'cd 3rdparty/nvml && make install prefix=`pwd`'

sharedlib:
	$(CXX) src/pmemkv.cc -o libpmemkv.so $(SO_FLAGS) $(USE_NVML) $(CXX_FLAGS)

install: sharedlib
	cp libpmemkv.so ${INSTALL_DIR}

uninstall:
	rm -rf ${INSTALL_DIR}/libpmemkv.so

example: reset sharedlib
	$(CXX) src/pmemkv_example.cc -o pmemkv_example $(USE_PMEMKV) $(CXX_FLAGS)
	LD_LIBRARY_PATH=.:${LD_LIBRARY_PATH} PMEM_IS_PMEM_FORCE=1 ./pmemkv_example

stress: reset sharedlib
	$(CXX) src/pmemkv_stress.cc -o pmemkv_stress -DNDEBUG $(USE_PMEMKV) $(CXX_FLAGS)
	LD_LIBRARY_PATH=.:${LD_LIBRARY_PATH} PMEM_IS_PMEM_FORCE=1 ./pmemkv_stress

test: reset sharedlib
	$(CXX) src/pmemkv_test.cc -o pmemkv_test $(USE_PMEMKV) $(USE_GTEST) $(CXX_FLAGS)
	LD_LIBRARY_PATH=.:${LD_LIBRARY_PATH} PMEM_IS_PMEM_FORCE=1 ./pmemkv_test
