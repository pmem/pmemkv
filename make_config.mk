CC=cc
CXX=g++
PLATFORM=OS_LINUX
PLATFORM_LDFLAGS= -lpthread -lrt -lgflags -lpmemobj -lpmem
PLATFORM_CXXFLAGS= -std=c++11 -DOS_LINUX -fno-builtin-memcmp -DGFLAGS=gflags -march=native
EXEC_LDFLAGS=
