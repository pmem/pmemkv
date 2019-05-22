prefix=/usr/local

all: clean test

reset:
	rm -rf /dev/shm/pmemkv /tmp/pmemkv

clean: reset
	rm -rf 3rdparty libpmemkv.so pmemkv_bench pmemkv_example pmemkv_stress pmemkv_test
	rm -rf ./bin googletest-*.zip

configure:
	mkdir -p ./bin
	cd ./bin && cmake .. -DCMAKE_BUILD_TYPE=Release

build: configure
	cd ./bin && make pmemkv

test: configure reset
	cd ./bin && make pmemkv_test
	cd ./bin && ctest --output-on-failure
	$(MAKE) reset

install: configure
	cd ./bin && make install

uninstall: configure
	cd ./bin && make uninstall
