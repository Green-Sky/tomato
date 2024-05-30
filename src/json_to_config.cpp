#include "./json_to_config.hpp"

#include <solanaceae/util/simple_config_model.hpp>

#include <nlohmann/json.hpp>

#include <iostream>

bool load_json_into_config(const nlohmann::ordered_json& config_json, SimpleConfigModel& conf) {
	if (!config_json.is_object()) {
		std::cerr << "TOMATO error: config file is not an json object!!!\n";
		return false;
	}
	for (const auto& [mod, cats] : config_json.items()) {
		for (const auto& [cat, cat_v] : cats.items()) {
			if (cat_v.is_object()) {
				if (cat_v.contains("default")) {
					const auto& value = cat_v["default"];
					if (value.is_string()) {
						conf.set(mod, cat, value.get_ref<const std::string&>());
					} else if (value.is_boolean()) {
						conf.set(mod, cat, value.get_ref<const bool&>());
					} else if (value.is_number_float()) {
						conf.set(mod, cat, value.get_ref<const double&>());
					} else if (value.is_number_integer()) {
						conf.set(mod, cat, value.get_ref<const int64_t&>());
					} else {
						std::cerr << "JSON error: wrong value type in " << mod << "::" << cat << " = " << value << "\n";
						return false;
					}
				}
				if (cat_v.contains("entries")) {
					for (const auto& [ent, ent_v] : cat_v["entries"].items()) {
						if (ent_v.is_string()) {
							conf.set(mod, cat, ent, ent_v.get_ref<const std::string&>());
						} else if (ent_v.is_boolean()) {
							conf.set(mod, cat, ent, ent_v.get_ref<const bool&>());
						} else if (ent_v.is_number_float()) {
							conf.set(mod, cat, ent, ent_v.get_ref<const double&>());
						} else if (ent_v.is_number_integer()) {
							conf.set(mod, cat, ent, ent_v.get_ref<const int64_t&>());
						} else {
							std::cerr << "JSON error: wrong value type in " << mod << "::" << cat << "::" << ent << " = " << ent_v << "\n";
							return false;
						}
					}
				}
			} else {
				if (cat_v.is_string()) {
					conf.set(mod, cat, cat_v.get_ref<const std::string&>());
				} else if (cat_v.is_boolean()) {
					conf.set(mod, cat, cat_v.get_ref<const bool&>());
				} else if (cat_v.is_number_float()) {
					conf.set(mod, cat, cat_v.get_ref<const double&>());
				} else if (cat_v.is_number_integer()) {
					conf.set(mod, cat, cat_v.get_ref<const int64_t&>());
				} else {
					std::cerr << "JSON error: wrong value type in " << mod << "::" << cat << " = " << cat_v << "\n";
					return false;
				}
			}
		}
	}

	return true;
}

