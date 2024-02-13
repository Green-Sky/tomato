#pragma once

#include <entt/core/type_info.hpp>
#include <entt/container/dense_map.hpp>

#include <nlohmann/json_fwd.hpp>

struct SerializerCallbacks {
	// nlohmann
	// json/msgpack
	using serialize_json_fn = bool(*)(void* comp, nlohmann::json& out);
	entt::dense_map<entt::id_type, serialize_json_fn> _serl_json;

	using deserialize_json_fn = bool(*)(void* comp, const nlohmann::json& in);
	entt::dense_map<entt::id_type, deserialize_json_fn> _deserl_json;

	void registerSerializerJson(serialize_json_fn fn, const entt::type_info& type_info);
	template<typename CompType>
	void registerSerializerJson(serialize_json_fn fn, const entt::type_info& type_info = entt::type_id<CompType>()) { registerSerializerJson(fn, type_info); }

	void registerDeSerializerJson(deserialize_json_fn fn, const entt::type_info& type_info);
	template<typename CompType>
	void registerDeSerializerJson(deserialize_json_fn fn, const entt::type_info& type_info = entt::type_id<CompType>()) { registerDeSerializerJson(fn, type_info); }
};

