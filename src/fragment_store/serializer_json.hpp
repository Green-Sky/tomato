#pragma once

#include <entt/core/type_info.hpp>
#include <entt/container/dense_map.hpp>
#include <entt/entity/handle.hpp>

#include <nlohmann/json_fwd.hpp>

// nlohmann
template<typename EntityType = entt::entity>
struct SerializerJsonCallbacks {
	using Registry = entt::basic_registry<EntityType>;
	using Handle = entt::basic_handle<Registry>;

	using serialize_fn = bool(*)(const Handle h, nlohmann::json& out);
	entt::dense_map<entt::id_type, serialize_fn> _serl;

	using deserialize_fn = bool(*)(Handle h, const nlohmann::json& in);
	entt::dense_map<entt::id_type, deserialize_fn> _deserl;

	template<typename T>
	static bool component_get_json(const Handle h, nlohmann::json& j) {
		if (h.template all_of<T>()) {
			if constexpr (!std::is_empty_v<T>) {
				j = h.template get<T>();
			}
			return true;
		}

		return false;
	}

	template<typename T>
	static bool component_emplace_or_replace_json(Handle h, const nlohmann::json& j) {
		if constexpr (std::is_empty_v<T>) {
			h.template emplace_or_replace<T>(); // assert empty json?
		} else {
			h.template emplace_or_replace<T>(static_cast<T>(j));
		}
		return true;
	}

	void registerSerializer(serialize_fn fn, const entt::type_info& type_info) {
		_serl[type_info.hash()] = fn;
	}

	template<typename CompType>
	void registerSerializer(
		serialize_fn fn = component_get_json<CompType>,
		const entt::type_info& type_info = entt::type_id<CompType>()
	) {
		registerSerializer(fn, type_info);
	}

	void registerDeSerializer(deserialize_fn fn, const entt::type_info& type_info) {
		_deserl[type_info.hash()] = fn;
	}

	template<typename CompType>
	void registerDeSerializer(
		deserialize_fn fn = component_emplace_or_replace_json<CompType>,
		const entt::type_info& type_info = entt::type_id<CompType>()
	) {
		registerDeSerializer(fn, type_info);
	}
};

