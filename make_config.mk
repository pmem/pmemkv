CXX=g++
CXX_FLAGS= -O2 -std=c++11 -DOS_LINUX -fno-builtin-memcmp -march=native -ldl -lpthread
SO_FLAGS= -shared -fPIC -Wl,-soname,libpmemkv.so
USE_GTEST= 3rdparty/gtest/src/gtest-all.cc -I3rdparty/gtest/include -I3rdparty/gtest
USE_NVML= 3rdparty/nvml/lib/libpmemobj.a 3rdparty/nvml/lib/libpmem.a -I3rdparty/nvml/src/include
USE_PMEMKV= ./libpmemkv.so -I3rdparty/nvml/src/include