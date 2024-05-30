#pragma once

#include <nlohmann/json_fwd.hpp>

// fwd
struct SimpleConfigModel;

bool load_json_into_config(const nlohmann::ordered_json& config_json, SimpleConfigModel& conf);

