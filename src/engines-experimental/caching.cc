/*
 * Copyright 2017-2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef EXPERIMENTAL

#include <ctime>
#include <iostream>
#include <lib_acl.hpp>
#include <libmemcached/memcached.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/ostreamwrapper.h>
#include <unistd.h>
#include "caching.h"

#define DO_LOG 0
#define LOG(msg) if (DO_LOG) std::cout << "[caching] " << msg << "\n"
#define ZERO 0

namespace pmemkv {
namespace caching {

CachingEngine::CachingEngine(const string& config) {
    if (!readConfig(config) || !(basePtr = KVEngine::Start(subEngine, subEngineConfig)))
        throw "CachingEngine Exception"; // todo propagate start exceptions properly
    LOG("Started ok");
}

CachingEngine::~CachingEngine() {
    LOG("Stopping");
    if (basePtr) KVEngine::Stop(basePtr);
    LOG("Stopped ok");
}

bool CachingEngine::readConfig(const string& config) {
    rapidjson::Document d;
    if (d.Parse(config.c_str()).HasParseError()) return false; // todo throw specific exceptions
    if (!d.HasMember("subengine") || !d["subengine"].IsString()) return false;
    if (!d.HasMember("subengine_config") || !d["subengine_config"].IsObject()) return false;
    if (!d["subengine_config"].HasMember("path") || !d["subengine_config"]["path"].IsString()) return false;
    if (!d.HasMember("host") || !d["host"].IsString()) return false;
    if (!d.HasMember("port") || !d["port"].IsInt()) return false;
    if (!d.HasMember("remote_type") || !d["remote_type"].IsString()) return false;
    if (!d.HasMember("remote_user") || !d["remote_user"].IsString()) return false;
    if (!d.HasMember("remote_pwd") || !d["remote_pwd"].IsString()) return false;
    if (!d.HasMember("remote_url") || !d["remote_url"].IsString()) return false;
    if (!d.HasMember("attempts") || !d["attempts"].IsInt()) return false;
    if (d.HasMember("ttl") && !d["ttl"].IsUint()) return false;

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    d["subengine_config"].Accept(writer);
    subEngineConfig = sb.GetString();
    subEngine = d["subengine"].GetString();
    remoteType = d["remote_type"].GetString();
    remoteUser = d["remote_user"].GetString();
    remotePasswd = d["remote_pwd"].GetString();
    remoteUrl = d["remote_url"].GetString();
    ttl = d.HasMember("ttl") ? d["ttl"].GetInt() : 0;
    host = d["host"].GetString();
    port = d["port"].GetInt();
    attempts = d["attempts"].GetInt();
    return true;
}

void CachingEngine::All(void* context, KVAllCallback* callback) {
    LOG("All");
    int result = 0;
    Each(&result, [](void* context, int kb, const char* k, int vb, const char* v) {
        auto c = ((int*) context);
        (*c)++;
    });
    if (result > 0 && basePtr) {
        basePtr->All(context, callback);
    }
    // todo refactor as single callback (Each --> All)
}

int64_t CachingEngine::Count() {
    LOG("Count");
    int result = 0;
    Each(&result, [](void* context, int kb, const char* k, int vb, const char* v) {
        auto c = ((int*) context);
        (*c)++;
    });
    return result;
}

struct EachCacheCallbackContext {
    void* context;
    KVEachCallback* cBack;
    std::list<string>* expiredKeys;
};

void CachingEngine::Each(void* context, KVEachCallback* callback) {
    LOG("Each");
    std::list<string> removingKeys;
    EachCacheCallbackContext cxt = {context, callback, &removingKeys};

    auto cb = [](void* context, int32_t kb, const char* k, int32_t vb, const char* v) {
        const auto c = ((EachCacheCallbackContext*) context);
        string localValue = v;
        string timeStamp = localValue.substr(0, 14);
        string value = localValue.substr(14);
        // TTL from config is ZERO or if the key is valid
        if (!ttl || valueFieldConversion(timeStamp)) {
            (*c->cBack)(c->context, kb, k, value.length(), value.c_str());
        } else {
            c->expiredKeys->push_back(k);
        }
    };

    if (basePtr) {  // todo bail earlier if null
        basePtr->Each(&cxt, cb);
        for (const auto& itr : removingKeys)
            basePtr->Remove(itr);
    }
}

KVStatus CachingEngine::Exists(const string& key) {
    LOG("Exists for key=" << key);
    KVStatus status = NOT_FOUND;
    string value;
    if (getKey(key, value, true))
        status = OK;
    return status;
    // todo fold into single return statement
}

void CachingEngine::Get(void* context, const string& key, KVGetCallback* callback) {
    LOG("Get key=" << key);
    string value;
    if (getKey(key, value, false)) (*callback)(context, (int32_t) value.size(), value.c_str());
}

bool CachingEngine::getKey(const string& key, string& valueField, bool api_flag) {
    auto cb = [](void* context, int32_t vb, const char* v) {
        const auto c = ((string*) context);
        c->append(v, vb);
    };
    string value;
    basePtr->Get(&value, key, cb);
    bool timeValidFlag;
    if (!value.empty()) {
        string timeStamp = value.substr(0, 14);
        valueField = value.substr(14);
        timeValidFlag = valueFieldConversion(timeStamp);
    }
    // No value for a key on local cache or if TTL not equal to zero and TTL is expired
    if (value.empty() || (ttl && !timeValidFlag)) {
        // api_flag is true  when request is from Exists API and no need to connect to remote service
        // api_flag is false when request is from Get API and connect to remote service
        if (api_flag || (remoteType == "Redis" && !getFromRemoteRedis(key, valueField)) ||
            (remoteType == "Memcached" && !getFromRemoteMemcached(key, valueField)) ||
            (remoteType != "Redis" && remoteType != "Memcached"))
            return false;
    }
    Put(key, valueField);
    return true;
}

bool CachingEngine::getFromRemoteMemcached(const string& key, string& value) {
    LOG("getFromRemoteMemcached");
    bool retValue = false;
    memcached_server_st* servers = NULL;
    memcached_st* memc;
    memcached_return rc;

    memc = memcached_create(NULL);  // todo reuse connection
    servers = memcached_server_list_append(servers, const_cast<char*>(host.c_str()), port, &rc);
    rc = memcached_server_push(memc, servers);

    // Multiple tries to connect to remote memcached
    for (int i = 0; i < attempts; ++i) {
        memcached_stat(memc, NULL, &rc);
        if (rc == MEMCACHED_SUCCESS) {
            size_t return_value_length;
            uint32_t flags;
            char* memcacheRet = memcached_get(memc, key.c_str(), key.length(), &return_value_length, &flags, &rc);
            if (memcacheRet) {
                value = memcacheRet;
                retValue = true;
            }
            break;
        }
        sleep(1);  // todo expose as configurable attempt delay
    }
    return retValue;
}

bool CachingEngine::getFromRemoteRedis(const string& key, string& value) {
    LOG("getFromRemoteRedis");
    bool retValue = false;
    string hostport = host + ":" + to_string(port);
    acl::string addr(hostport.c_str()), passwd;
    acl::acl_cpp_init();  // todo reuse connection
    acl::redis_client client(addr.c_str(), 0, 0);

    // Multiple tries to connect to remote redis
    for (int i = 0; i < attempts; ++i) {
        if (client.get_stream() && client.get_stream()->opened()) {
            client.set_password(passwd);
            acl::redis cmd(&client);
            acl::string key1, _value;
            key1.format("%s", key.c_str());
            cmd.get(key1, _value);
            value = _value.c_str();
            if (value.length() > 0)
                retValue = true;
            break;
        }
        sleep(1);  // todo expose as configurable attempt delay
    }
    return retValue;
}

KVStatus CachingEngine::Put(const string& key, const string& value) {
    LOG("Put key=" << key << ", value.size=" << to_string(value.size()));
    const time_t curSysTime = time(0);
    const string curTime = getTimeStamp(curSysTime);
    const string ValueWithCurTime = curTime + value;
    return basePtr->Put(key, ValueWithCurTime);
}

KVStatus CachingEngine::Remove(const string& key) {
    LOG("Remove key=" << key);
    return basePtr->Remove(key);
}

time_t convertTimeToEpoch(const char* theTime, const char* format) {
    std::tm tmTime;
    memset(&tmTime, 0, sizeof(tmTime));
    strptime(theTime, format, &tmTime);
    return mktime(&tmTime);
}

string getTimeStamp(const time_t epochTime, const char* format) {
    char timestamp[64] = {0};
    strftime(timestamp, sizeof(timestamp), format, localtime(&epochTime));
    return timestamp;
}

bool valueFieldConversion(string dateValue) {
    bool timeValidFlag = false;
    if (ttl > ZERO) {
        const time_t now = convertTimeToEpoch(dateValue.c_str());
        struct tm then_tm = *localtime(&now);
        char ttlTimeStr[80];

        then_tm.tm_sec += ttl;
        mktime(&then_tm);
        std::strftime(ttlTimeStr, 80, "%Y%m%d%H%M%S", &then_tm);

        const time_t curTime = time(0);
        string curTimeStr = getTimeStamp(curTime);

        if (ttlTimeStr >= curTimeStr)
            timeValidFlag = true;
    }
    return timeValidFlag;
}

} // namespace caching
} // namespace pmemkv

#endif