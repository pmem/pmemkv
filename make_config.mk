CC=cc
CXX=g++
PLATFORM=OS_LINUX
PLATFORM_LDFLAGS= -lpthread -lrt
PLATFORM_CXXFLAGS= -std=c++11 -DOS_LINUX -fno-builtin-memcmp -march=native
USE_GTEST= 3rdparty/gtest/src/gtest-all.cc -I3rdparty/gtest/include -I3rdparty/gtest
USE_NVML= 3rdparty/nvml/lib/libpmemobj.a 3rdparty/nvml/lib/libpmem.a -I3rdparty/nvml/src/include