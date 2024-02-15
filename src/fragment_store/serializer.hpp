#pragma once

#include <entt/core/type_info.hpp>
#include <entt/container/dense_map.hpp>
#include <entt/entity/handle.hpp>

#include <nlohmann/json_fwd.hpp>

#include "./fragment_store_i.hpp"

struct SerializerCallbacks {
	// nlohmann
	// json/msgpack
	using serialize_json_fn = bool(*)(void* comp, nlohmann::json& out);
	entt::dense_map<entt::id_type, serialize_json_fn> _serl_json;

	using deserialize_json_fn = bool(*)(FragmentHandle fh, const nlohmann::json& in);
	entt::dense_map<entt::id_type, deserialize_json_fn> _deserl_json;

	template<typename T>
	static bool component_emplace_or_replace_json(FragmentHandle fh, const nlohmann::json& j) {
		if constexpr (std::is_empty_v<T>) {
			fh.emplace_or_replace<T>(); // assert empty json?
		} else {
			fh.emplace_or_replace<T>(static_cast<T>(j));
		}
		return true;
	}

	template<typename T>
	static bool component_get_json(const FragmentHandle fh, nlohmann::json& j) {
		if (fh.all_of<T>()) {
			if constexpr (!std::is_empty_v<T>) {
				j = fh.get<T>();
			}
			return true;
		}

		return false;
	}

	void registerSerializerJson(serialize_json_fn fn, const entt::type_info& type_info);

	template<typename CompType>
	void registerSerializerJson(serialize_json_fn fn, const entt::type_info& type_info = entt::type_id<CompType>()) { registerSerializerJson(fn, type_info); }

	void registerDeSerializerJson(deserialize_json_fn fn, const entt::type_info& type_info);

	template<typename CompType>
	void registerDeSerializerJson(
		deserialize_json_fn fn = component_emplace_or_replace_json<CompType>,
		const entt::type_info& type_info = entt::type_id<CompType>()
	) {
		registerDeSerializerJson(fn, type_info);
	}
};

