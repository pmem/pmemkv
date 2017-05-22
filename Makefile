prefix=/usr/local

all: clean example test

reset:
	rm -rf /dev/shm/pmemkv /tmp/pmemkv

clean: reset
	rm -rf 3rdparty libpmemkv.so pmemkv_example pmemkv_stress pmemkv_test
	rm -rf ./bin googletest-*.zip

configure:
	mkdir -p ./bin
	cd ./bin && cmake .. -DCMAKE_BUILD_TYPE=Release

sharedlib:
	cd ./bin && make pmemkv

install: sharedlib
	cp ./bin/libpmemkv.so $(prefix)/lib
	cp ./src/pmemkv.h $(prefix)/include/libpmemkv.h

uninstall:
	rm -rf $(prefix)/lib/libpmemkv.so
	rm -rf $(prefix)/include/libpmemkv.h

example: configure reset
	cd ./bin && make pmemkv_example
	PMEM_IS_PMEM_FORCE=1 ./bin/pmemkv_example

stress: configure reset
	cd ./bin && make pmemkv_stress
	PMEM_IS_PMEM_FORCE=1 ./bin/pmemkv_stress

test: configure reset
	cd ./bin && make pmemkv_test
	PMEM_IS_PMEM_FORCE=1 ./bin/pmemkv_test
