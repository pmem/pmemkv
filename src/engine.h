#pragma once

#include <string>
#include <functional>

#include "status.h"

namespace pmem {
namespace kv {

using status = pmemkv_status;

typedef void(AllCallback)(void* context, int keybytes, const char* key);
typedef void(EachCallback)(void* context, int keybytes, const char* key, int valuebytes, const char* value);
typedef void(GetCallback)(void* context, int valuebytes, const char* value);

class EngineBase {
  public:
    EngineBase() {}

    virtual ~EngineBase() {}

    virtual std::string Engine() = 0;

    virtual void All(void* context, AllCallback* callback) = 0;

    virtual void AllAbove(void* context, const std::string& key, AllCallback* callback) = 0;

    virtual void AllBelow(void* context, const std::string& key, AllCallback* callback) = 0;

    virtual void AllBetween(void* context, const std::string& key1, const std::string& key2, AllCallback* callback) = 0;

    virtual int64_t Count() = 0;

    virtual int64_t CountAbove(const std::string& key) = 0;

    virtual int64_t CountBelow(const std::string& key) = 0;

    virtual int64_t CountBetween(const std::string& key1, const std::string& key2) = 0;

    virtual void Each(void* context, EachCallback* callback) = 0;

    virtual void EachAbove(void* context, const std::string& key, EachCallback* callback) = 0;

    virtual void EachBelow(void* context, const std::string& key, EachCallback* callback) = 0;

    virtual void EachBetween(void* context, const std::string& key1, const std::string& key2, EachCallback* callback) = 0;

    virtual status Exists(const std::string& key) = 0;

    virtual void Get(void* context, const std::string& key, GetCallback* callback) = 0;

    virtual status Put(const std::string& key, const std::string& value) = 0;

    virtual status Remove(const std::string& key) = 0;
};

} // namespace kv
} // namespace pmem