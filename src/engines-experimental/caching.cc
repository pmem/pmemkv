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

#define ZERO 0

namespace pmem
{
namespace kv
{

static time_t convertTimeToEpoch(const char *theTime, const char *format = "%Y%m%d%H%M%S")
{
	std::tm tmTime;
	memset(&tmTime, 0, sizeof(tmTime));
	strptime(theTime, format, &tmTime);
	return mktime(&tmTime);
}

static std::string getTimeStamp(const time_t epochTime,
				const char *format = "%Y%m%d%H%M%S")
{
	char timestamp[64] = {0};
	strftime(timestamp, sizeof(timestamp), format, localtime(&epochTime));
	return timestamp;
}

static bool valueFieldConversion(std::string dateValue, int64_t ttl)
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

caching::caching(std::unique_ptr<internal::config> cfg)
{
	auto &config = *cfg;

	std::string subEngine;
	getString(config, "subengine", subEngine);

	getString(config, "remote_type", remoteType);
	getString(config, "remote_user", remoteUser);
	getString(config, "remote_pwd", remotePasswd);
	getString(config, "remote_url", remoteUrl);
	getString(config, "host", host);

	if (!config.get_int64("ttl", &ttl))
		ttl = 0;

	if (!config.get_int64("port", &port))
		throw internal::invalid_argument(
			"Config does not contain item with key: \"port\"");

	if (!config.get_int64("attempts", &attempts))
		throw internal::invalid_argument(
			"Config does not contain item with key: \"attempts\"");

	internal::config *subEngineConfig;
	if (!config.get_object("subengine_config", (void **)&subEngineConfig))
		throw internal::invalid_argument(
			"Config does not contain item with key: \"subengine_config\"");

	/* Remove item to pass ownership of it to subengine */
	config.remove("subengine_config");

	basePtr = engine_base::create_engine(
		subEngine, std::unique_ptr<internal::config>(subEngineConfig));

	LOG("Started ok");
}

caching::~caching()
{
	LOG("Stopped ok");
}

std::string caching::name()
{
	return "caching";
}

void caching::getString(internal::config &config, const char *key, std::string &str)
{
	const char *value;

	if (!config.get_string(key, &value))
		throw internal::invalid_argument(
			"Config does not contain item with key: \"" + std::string(key) +
			"\"");

	str = std::string(value);
}

status caching::count_all(std::size_t &cnt)
{
	LOG("count_all");
	std::size_t result = 0;
	get_all(
		[](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
			auto c = ((std::size_t *)arg);
			(*c)++;

			return 0;
		},
		&result);

	cnt = result;

	return status::OK;
}

struct GetAllCacheCallbackContext {
	void *arg;
	get_kv_callback *cBack;
	std::list<std::string> *expiredKeys;
	int64_t ttl;
};

status caching::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	std::list<std::string> removingKeys;
	GetAllCacheCallbackContext cxt = {arg, callback, &removingKeys, ttl};

	auto cb = [](const char *k, size_t kb, const char *v, size_t vb, void *arg) {
		const auto c = ((GetAllCacheCallbackContext *)arg);
		std::string localValue = v;
		std::string timeStamp = localValue.substr(0, 14);
		std::string value = localValue.substr(14);
		// TTL from config is ZERO or if the key is valid
		if (!c->ttl || valueFieldConversion(timeStamp, c->ttl)) {
			auto ret = c->cBack(k, kb, value.c_str(), value.length(), c->arg);
			if (ret != 0)
				return ret;
		} else {
			c->expiredKeys->push_back(k);
		}

		return 0;
	};

	if (basePtr) { // todo bail earlier if null
		auto s = basePtr->get_all(cb, &cxt);
		if (s != status::OK)
			return s;

		for (const auto &itr : removingKeys) {
			auto s = basePtr->remove(itr);
			if (s != status::OK)
				return s;
		}
	}

	return status::OK;
}

status caching::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	status s = status::NOT_FOUND;
	std::string value;
	if (getKey(std::string(key.data(), key.size()), value, true))
		s = status::OK;
	return s;
}

status caching::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));
	std::string value;
	if (getKey(std::string(key.data(), key.size()), value, false)) {
		callback(value.c_str(), value.size(), arg);
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
		timeValidFlag = valueFieldConversion(timeStamp, ttl);
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

status caching::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));
	const time_t curSysTime = time(0);
	const std::string curTime = getTimeStamp(curSysTime);
	const std::string ValueWithCurTime =
		curTime + std::string(value.data(), value.size());
	return basePtr->put(std::string(key.data(), key.size()), ValueWithCurTime);
}

status caching::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));
	return basePtr->remove(std::string(key.data(), key.size()));
}

} // namespace kv
} // namespace pmem
