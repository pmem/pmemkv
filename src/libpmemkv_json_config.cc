/*
 * Copyright 2019, Intel Corporation
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

#include "libpmemkv_json_config.h"
#include "out.h"

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

extern "C" {

int pmemkv_config_from_json(pmemkv_config *config, const char *json)
{
	rapidjson::Document doc;
	rapidjson::Value::ConstMemberIterator itr;

	assert(config && json);

	try {
		if (doc.Parse(json).HasParseError()) {
			throw std::runtime_error("Config parsing failed");
		}
		for (itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
			if (itr->value.IsString()) {
				auto value = itr->value.GetString();

				auto status = pmemkv_config_put_string(
					config, itr->name.GetString(), value);
				if (status != PMEMKV_STATUS_OK)
					throw std::runtime_error(
						"Inserting string to the config failed");
			} else if (itr->value.IsInt64()) {
				auto value = itr->value.GetInt64();

				auto status = pmemkv_config_put_int64(
					config, itr->name.GetString(), value);
				if (status != PMEMKV_STATUS_OK)
					throw std::runtime_error(
						"Inserting int to the config failed");
			} else if (itr->value.IsTrue() || itr->value.IsFalse()) {
				auto value = itr->value.GetBool();

				auto status = pmemkv_config_put_int64(
					config, itr->name.GetString(), value);
				if (status != PMEMKV_STATUS_OK)
					throw std::runtime_error(
						"Inserting bool to the config failed");
			} else if (itr->value.IsObject()) {
				rapidjson::StringBuffer sb;
				rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
				itr->value.Accept(writer);

				auto sub_cfg = pmemkv_config_new();

				if (sub_cfg == nullptr) {
					ERR() << "Cannot allocate subconfig";
					return PMEMKV_STATUS_OUT_OF_MEMORY;
				}

				auto status =
					pmemkv_config_from_json(sub_cfg, sb.GetString());
				if (status != PMEMKV_STATUS_OK) {
					pmemkv_config_delete(sub_cfg);
					throw std::runtime_error(
						"Cannot parse subconfig");
				}

				status = pmemkv_config_put_object(
					config, itr->name.GetString(), sub_cfg,
					(void (*)(void *)) & pmemkv_config_delete);
				if (status != PMEMKV_STATUS_OK)
					throw std::runtime_error(
						"Inserting a new entry to the config failed");
			} else {
				static std::string kTypeNames[] = {
					"Null",	 "False",  "True",  "Object",
					"Array", "String", "Number"};

				throw std::runtime_error(
					"Unsupported data type in JSON string: " +
					kTypeNames[itr->value.GetType()]);
			}
		}
	} catch (const std::exception &exc) {
		ERR() << exc.what();
		return PMEMKV_STATUS_CONFIG_PARSING_ERROR;
	} catch (...) {
		ERR() << "Unspecified failure";
		return PMEMKV_STATUS_CONFIG_PARSING_ERROR;
	}

	return PMEMKV_STATUS_OK;
}

const char *pmemkv_config_from_json_errormsg(void)
{
	return out_get_errormsg();
}

} /* extern "C" */
