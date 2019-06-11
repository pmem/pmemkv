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

#include "caching.h"
#include <ctime>
#include <iostream>
#include <lib_acl.hpp>
#include <libmemcached/memcached.h>
#include <list>
#include <memory>
#include <unistd.h>

// XXX: needed to create subEngine, remove once configure structure is created
#include "../libpmemkv.hpp"

#define DO_LOG 0
#define LOG(msg)                                                                         \
	if (DO_LOG)                                                                      \
	std::cout << "[caching] " << msg << "\n"
#define ZERO 0

namespace pmem
{
namespace kv
{

caching::caching(void *context, pmemkv_config *config) : context(context)
{
	if (!readConfig(config))
		throw "caching Exception"; // todo propagate start exceptions properly

	if (!(basePtr = new db))
		throw "caching Exception"; // todo propagate start exceptions properly

	if (basePtr->open(subEngine, config) != status::OK)
		throw "caching Exception"; // todo propagate start exceptions properly

	LOG("Started ok");
}

caching::~caching()
{
	LOG("Stopping");
	if (basePtr)
		delete basePtr;
	LOG("Stopped ok");
}

std::string caching::name()
{
	return "caching";
}

void *caching::engine_context()
{
	return context;
}

bool caching::getString(pmemkv_config *config, const char *key, std::string &str)
{
	size_t length;

	if (pmemkv_config_get(config, key, NULL, 0, &length) == -1)
		return false;

	auto c_str = std::unique_ptr<char[]>(new char[length]);
	if (pmemkv_config_get(config, key, c_str.get(), length, NULL) != length)
		return false;

	str = std::string(c_str.get(), length);

	return true;
}

bool caching::readConfig(pmemkv_config *config)
{
	if (!getString(config, "subengine", subEngine))
		return false;

	if (!getString(config, "remote_type", remoteType))
		return false;

	if (!getString(config, "remote_user", remoteUser))
		return false;

	if (!getString(config, "remote_pwd", remotePasswd))
		return false;

	if (!getString(config, "remote_url", remoteUrl))
		return false;

	if (!getString(config, "host", host))
		return false;

	if (pmemkv_config_get(config, "ttl", &ttl, sizeof(int), NULL) != sizeof(int))
		ttl = 0;

	if (pmemkv_config_get(config, "port", &port, sizeof(unsigned long int), NULL) !=
	    sizeof(unsigned long int))
		return false;

	if (pmemkv_config_get(config, "attempts", &attempts, sizeof(int), NULL) !=
	    sizeof(int))
		return false;

	return true;
}

status caching::all(all_callback *callback, void *arg)
{
	LOG("All");

	std::size_t cnt;
	auto s = count(cnt);

	if (s == status::OK && cnt > 0 && basePtr)
		return basePtr->all(callback, arg);
	// todo refactor as single callback (Each --> All)

	return s;
}

status caching::count(std::size_t &cnt)
{
	LOG("Count");
	std::size_t result = 0;
	each(
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			auto c = ((std::size_t *)arg);
			(*c)++;
		},
		&result);

	cnt = result;

	return status::OK;
}

struct EachCacheCallbackContext {
	void *arg;
	each_callback *cBack;
	std::list<std::string> *expiredKeys;
};

status caching::each(each_callback *callback, void *arg)
{
	LOG("Each");
	std::list<std::string> removingKeys;
	EachCacheCallbackContext cxt = {arg, callback, &removingKeys};

	auto cb = [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
		const auto c = ((EachCacheCallbackContext *)arg);
		std::string localValue = v;
		std::string timeStamp = localValue.substr(0, 14);
		std::string value = localValue.substr(14);
		// TTL from config is ZERO or if the key is valid
		if (!ttl || valueFieldConversion(timeStamp)) {
			(*c->cBack)(k, kb, value.c_str(), value.length(), c->arg);
		} else {
			c->expiredKeys->push_back(k);
		}
	};

	if (basePtr) { // todo bail earlier if null
		auto s = basePtr->each(cb, &cxt);
		if (s != status::OK)
			return s;

		for (const auto &itr : removingKeys) {
			auto s = basePtr->remove(itr);
			if (s != status::OK)
				return status::FAILED;
		}
	}
}

status caching::exists(const std::string &key)
{
	LOG("Exists for key=" << key);
	status s = status::NOT_FOUND;
	std::string value;
	if (getKey(key, value, true))
		s = status::OK;
	return s;
	// todo fold into single return statement
}

status caching::get(const std::string &key, get_callback *callback, void *arg)
{
	LOG("Get key=" << key);
	std::string value;
	if (getKey(key, value, false)) {
		(*callback)(value.c_str(), value.size(), arg);
		return status::OK;
	} else
		return status::NOT_FOUND;
}

bool caching::getKey(const std::string &key, std::string &valueField, bool api_flag)
{
	auto cb = [](const char *v, size_t vb, void *arg) {
		const auto c = ((std::string *)arg);
		c->append(v, vb);
	};
	std::string value;
	basePtr->get(key, cb, &value);
	bool timeValidFlag;
	if (!value.empty()) {
		std::string timeStamp = value.substr(0, 14);
		valueField = value.substr(14);
		timeValidFlag = valueFieldConversion(timeStamp);
	}
	// No value for a key on local cache or if TTL not equal to zero and TTL is
	// expired
	if (value.empty() || (ttl && !timeValidFlag)) {
		// api_flag is true  when request is from Exists API and no need to
		// connect to remote service api_flag is false when request is from Get
		// API and connect to remote service
		if (api_flag ||
		    (remoteType == "Redis" && !getFromRemoteRedis(key, valueField)) ||
		    (remoteType == "Memcached" &&
		     !getFromRemoteMemcached(key, valueField)) ||
		    (remoteType != "Redis" && remoteType != "Memcached"))
			return false;
	}
	put(key, valueField);
	return true;
}

bool caching::getFromRemoteMemcached(const std::string &key, std::string &value)
{
	LOG("getFromRemoteMemcached");
	bool retValue = false;
	memcached_server_st *servers = NULL;
	memcached_st *memc;
	memcached_return rc;

	memc = memcached_create(NULL); // todo reuse connection
	servers = memcached_server_list_append(servers, const_cast<char *>(host.c_str()),
					       port, &rc);
	rc = memcached_server_push(memc, servers);

	// Multiple tries to connect to remote memcached
	for (int i = 0; i < attempts; ++i) {
		memcached_stat(memc, NULL, &rc);
		if (rc == MEMCACHED_SUCCESS) {
			size_t return_value_length;
			uint32_t flags;
			char *memcacheRet =
				memcached_get(memc, key.c_str(), key.length(),
					      &return_value_length, &flags, &rc);
			if (memcacheRet) {
				value = memcacheRet;
				retValue = true;
			}
			break;
		}
		sleep(1); // todo expose as configurable attempt delay
	}
	return retValue;
}

bool caching::getFromRemoteRedis(const std::string &key, std::string &value)
{
	LOG("getFromRemoteRedis");
	bool retValue = false;
	std::string hostport = host + ":" + std::to_string(port);
	acl::string addr(hostport.c_str()), passwd;
	acl::acl_cpp_init(); // todo reuse connection
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
		sleep(1); // todo expose as configurable attempt delay
	}
	return retValue;
}

status caching::put(const std::string &key, const std::string &value)
{
	LOG("Put key=" << key << ", value.size=" << std::to_string(value.size()));
	const time_t curSysTime = time(0);
	const std::string curTime = getTimeStamp(curSysTime);
	const std::string ValueWithCurTime = curTime + value;
	return basePtr->put(key, ValueWithCurTime);
}

status caching::remove(const std::string &key)
{
	LOG("Remove key=" << key);
	return basePtr->remove(key);
}

time_t convertTimeToEpoch(const char *theTime, const char *format)
{
	std::tm tmTime;
	memset(&tmTime, 0, sizeof(tmTime));
	strptime(theTime, format, &tmTime);
	return mktime(&tmTime);
}

std::string getTimeStamp(const time_t epochTime, const char *format)
{
	char timestamp[64] = {0};
	strftime(timestamp, sizeof(timestamp), format, localtime(&epochTime));
	return timestamp;
}

bool valueFieldConversion(std::string dateValue)
{
	bool timeValidFlag = false;
	if (ttl > ZERO) {
		const time_t now = convertTimeToEpoch(dateValue.c_str());
		struct tm then_tm = *localtime(&now);
		char ttlTimeStr[80];

		then_tm.tm_sec += ttl;
		mktime(&then_tm);
		std::strftime(ttlTimeStr, 80, "%Y%m%d%H%M%S", &then_tm);

		const time_t curTime = time(0);
		std::string curTimeStr = getTimeStamp(curTime);

		if (ttlTimeStr >= curTimeStr)
			timeValidFlag = true;
	}
	return timeValidFlag;
}

} // namespace kv
} // namespace pmem
