// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2021, Intel Corporation */

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

	try {
		if (!config) {
			throw std::runtime_error("Config has to be specified");
		}
		if (!json) {
			throw std::runtime_error(
				"Configuration json has to be specified");
		}
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
						"Inserting string to the config failed with error: " +
						std::string(pmemkv_errormsg()));
			} else if (itr->value.IsInt64()) {
				auto value = itr->value.GetInt64();

				auto status = pmemkv_config_put_int64(
					config, itr->name.GetString(), value);
				if (status != PMEMKV_STATUS_OK)
					throw std::runtime_error(
						"Inserting int to the config failed with error: " +
						std::string(pmemkv_errormsg()));
			} else if (itr->value.IsTrue() || itr->value.IsFalse()) {
				auto value = itr->value.GetBool();

				auto status = pmemkv_config_put_int64(
					config, itr->name.GetString(), value);
				if (status != PMEMKV_STATUS_OK)
					throw std::runtime_error(
						"Inserting bool to the config failed with error: " +
						std::string(pmemkv_errormsg()));
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
						"Parsing subconfig failed with error: " +
						std::string(
							pmemkv_config_from_json_errormsg()));
				}

				status = pmemkv_config_put_object(
					config, itr->name.GetString(), sub_cfg,
					(void (*)(void *)) & pmemkv_config_delete);
				if (status != PMEMKV_STATUS_OK)
					throw std::runtime_error(
						"Inserting a new entry to the config failed with error: " +
						std::string(pmemkv_errormsg()));
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
