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

#include "cmap.h"
#include "../out.h"

#include <unistd.h>

#define DO_LOG 0
#define LOG(msg)                                                                         \
	do {                                                                             \
		if (DO_LOG)                                                              \
			std::cout << "[cmap] " << msg << "\n";                           \
	} while (0)

namespace pmem
{
namespace kv
{

cmap::cmap(const std::string &path, size_t size)
{
	if ((access(path.c_str(), F_OK) != 0) && (size > 0)) {
		LOG("Creating filesystem pool, path=" << path << ", size="
						      << std::to_string(size));
		pmpool = pool_t::create(path.c_str(), LAYOUT, size, S_IRWXU);
	} else {
		LOG("Opening pool, path=" << path);
		pmpool = pool_t::open(path.c_str(), LAYOUT);
	}
	LOG("Started ok");

	Recover();
}

cmap::~cmap()
{
	LOG("Stopping");
	pmpool.close();
	LOG("Stopped ok");
}

std::string cmap::name()
{
	return "cmap";
}

status cmap::count_all(std::size_t &cnt)
{
	LOG("count_all");
	cnt = container->size();

	return status::OK;
}

status cmap::get_all(get_kv_callback *callback, void *arg)
{
	LOG("get_all");
	for (auto it = container->begin(); it != container->end(); ++it) {
		auto ret = callback(it->first.c_str(), it->first.size(),
				    it->second.c_str(), it->second.size(), arg);

		if (ret != 0)
			return status::STOPPED_BY_CB;
	}

	return status::OK;
}

status cmap::exists(string_view key)
{
	LOG("exists for key=" << std::string(key.data(), key.size()));
	return container->count(key) == 1 ? status::OK : status::NOT_FOUND;
}

status cmap::get(string_view key, get_v_callback *callback, void *arg)
{
	LOG("get key=" << std::string(key.data(), key.size()));
	map_t::const_accessor result;
	bool found = container->find(result, key);
	if (!found) {
		LOG("  key not found");
		return status::NOT_FOUND;
	}

	callback(result->second.c_str(), result->second.size(), arg);
	return status::OK;
}

status cmap::put(string_view key, string_view value)
{
	LOG("put key=" << std::string(key.data(), key.size())
		       << ", value.size=" << std::to_string(value.size()));
	try {
		map_t::accessor acc;
		// XXX - do not create temporary string
		bool result = container->insert(
			acc, map_t::value_type(string_t(key), string_t(value)));
		if (!result) {
			pmem::obj::transaction::manual tx(pmpool);
			acc->second = value;
			pmem::obj::transaction::commit();
		}
	} catch (std::bad_alloc e) {
		ERR() << "Put failed due to exception, " << e.what();
		return status::FAILED;
	} catch (pmem::transaction_error e) {
		ERR() << "Put failed due to pmem::transaction_error, " << e.what();
		return status::FAILED;
	}

	return status::OK;
}

status cmap::remove(string_view key)
{
	LOG("remove key=" << std::string(key.data(), key.size()));
	try {
		bool erased = container->erase(key);
		return erased ? status::OK : status::NOT_FOUND;
	} catch (std::runtime_error e) {
		ERR() << "Remove failed due to exception, " << e.what();
		return status::FAILED;
	}
}

void cmap::Recover()
{
	auto root_data = pmpool.root();
	if (root_data->map_ptr) {
		container = root_data->map_ptr.get();
		container->initialize();
	} else {
		pmem::obj::transaction::manual tx(pmpool);
		root_data->map_ptr = pmem::obj::make_persistent<map_t>();
		pmem::obj::transaction::commit();
		container = root_data->map_ptr.get();
		container->initialize(true);
	}
}

} // namespace kv
} // namespace pmem
