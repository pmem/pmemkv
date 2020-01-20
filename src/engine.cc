/*
 * Copyright 2017-2020, Intel Corporation
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

#include "engine.h"

#include "engines/blackhole.h"

#ifdef ENGINE_VSMAP
#include "engines/vsmap.h"
#endif

#ifdef ENGINE_VCMAP
#include "engines/vcmap.h"
#endif

#ifdef ENGINE_CMAP
#include "engines/cmap.h"
#endif

#ifdef ENGINE_CACHING
#include "engines-experimental/caching.h"
#endif

#ifdef ENGINE_STREE
#include "engines-experimental/stree.h"
#endif

#ifdef ENGINE_TREE3
#include "engines-experimental/tree3.h"
#endif

namespace pmem
{
namespace kv
{

engine_base::engine_base()
{
}

engine_base::~engine_base()
{
}

static constexpr const char *available_engines = "blackhole"
#ifdef ENGINE_CMAP
						 ", cmap"
#endif
#ifdef ENGINE_VSMAP
						 ", vsmap"
#endif
#ifdef ENGINE_VCMAP
						 ", vcmap"
#endif
#ifdef ENGINE_TREE3
						 ", tree3"
#endif
#ifdef ENGINE_STREE
						 ", stree"
#endif
#ifdef ENGINE_CACHING
						 ", caching"
#endif
	;

/* throws internal error (status) if config is null */
void engine_base::check_config_null(const std::string &engine_name,
				    std::unique_ptr<internal::config> &cfg)
{
	if (!cfg) {
		throw internal::invalid_argument("Config cannot be null for the '" +
						 engine_name + "' engine");
	}
}

std::unique_ptr<engine_base>
engine_base::create_engine(const std::string &engine,
			   std::unique_ptr<internal::config> cfg)
{
	if (engine == "blackhole")
		return std::unique_ptr<engine_base>(
			new pmem::kv::blackhole(std::move(cfg)));

#ifdef ENGINE_CMAP
	if (engine == "cmap") {
		engine_base::check_config_null(engine, cfg);
		return std::unique_ptr<engine_base>(new pmem::kv::cmap(std::move(cfg)));
	}
#endif

#ifdef ENGINE_VSMAP
	if (engine == "vsmap") {
		engine_base::check_config_null(engine, cfg);
		return std::unique_ptr<engine_base>(new pmem::kv::vsmap(std::move(cfg)));
	}
#endif

#ifdef ENGINE_VCMAP
	if (engine == "vcmap") {
		engine_base::check_config_null(engine, cfg);
		return std::unique_ptr<engine_base>(new pmem::kv::vcmap(std::move(cfg)));
	}
#endif

#ifdef ENGINE_TREE3
	if (engine == "tree3") {
		engine_base::check_config_null(engine, cfg);
		return std::unique_ptr<engine_base>(new pmem::kv::tree3(std::move(cfg)));
	}
#endif

#ifdef ENGINE_STREE
	if (engine == "stree") {
		engine_base::check_config_null(engine, cfg);
		return std::unique_ptr<engine_base>(new pmem::kv::stree(std::move(cfg)));
	}
#endif

#ifdef ENGINE_CACHING
	if (engine == "caching") {
		engine_base::check_config_null(engine, cfg);
		return std::unique_ptr<engine_base>(
			new pmem::kv::caching(std::move(cfg)));
	}
#endif

	throw internal::wrong_engine_name("Unknown engine name \"" + engine +
					  "\". Available engines: " + available_engines);
}

status engine_base::count_all(std::size_t &cnt)
{
	return status::NOT_SUPPORTED;
}

status engine_base::count_above(string_view key, std::size_t &cnt)
{
	return status::NOT_SUPPORTED;
}

status engine_base::count_equal_above(string_view key, std::size_t &cnt)
{
	return status::NOT_SUPPORTED;
}

status engine_base::count_equal_below(string_view key, std::size_t &cnt)
{
	return status::NOT_SUPPORTED;
}

status engine_base::count_below(string_view key, std::size_t &cnt)
{
	return status::NOT_SUPPORTED;
}
status engine_base::count_between(string_view key1, string_view key2, std::size_t &cnt)
{
	return status::NOT_SUPPORTED;
}

status engine_base::get_all(get_kv_callback *callback, void *arg)
{
	return status::NOT_SUPPORTED;
}

status engine_base::get_above(string_view key, get_kv_callback *callback, void *arg)
{
	return status::NOT_SUPPORTED;
}

status engine_base::get_equal_above(string_view key, get_kv_callback *callback, void *arg)
{
	return status::NOT_SUPPORTED;
}

status engine_base::get_equal_below(string_view key, get_kv_callback *callback, void *arg)
{
	return status::NOT_SUPPORTED;
}

status engine_base::get_below(string_view key, get_kv_callback *callback, void *arg)
{
	return status::NOT_SUPPORTED;
}

status engine_base::get_between(string_view key1, string_view key2,
				get_kv_callback *callback, void *arg)
{
	return status::NOT_SUPPORTED;
}

status engine_base::exists(string_view key)
{
	return status::NOT_SUPPORTED;
}

status engine_base::defrag(double start_percent, double amount_percent)
{
	return status::NOT_SUPPORTED;
}

} // namespace kv
} // namespace pmem
