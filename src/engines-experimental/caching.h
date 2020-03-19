// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2019, Intel Corporation */

#pragma once

#include "../engine.h"

namespace pmem
{
namespace kv
{

class db;

class caching : public engine_base {
public:
	caching(std::unique_ptr<internal::config> cfg);
	~caching();

	std::string name() final;

	status count_all(std::size_t &cnt) final;

	status get_all(get_kv_callback *callback, void *arg) final;

	status exists(string_view key) final;

	status get(string_view key, get_v_callback *callback, void *arg) final;

	status put(string_view key, string_view value) final;

	status remove(string_view key) final;

private:
	void getString(internal::config &cfg, const char *key, std::string &str);
	bool getFromRemoteRedis(const std::string &key, std::string &value);
	bool getFromRemoteMemcached(const std::string &key, std::string &value);
	bool getKey(const std::string &key, std::string &valueField, bool api_flag);

	std::unique_ptr<engine_base> basePtr;

	int64_t attempts;
	std::string host;
	int64_t port;
	std::string remoteType;
	std::string remoteUser;
	std::string remotePasswd;
	std::string remoteUrl;
	int64_t ttl;
};

} /* namespace kv */
} /* namespace pmem */
